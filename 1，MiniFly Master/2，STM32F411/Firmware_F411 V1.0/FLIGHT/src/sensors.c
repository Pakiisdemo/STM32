#include <math.h>
#include "stdio.h"
#include "delay.h"
#include "config.h"
#include "config_param.h"
#include "ledseq.h"
#include "iic1.h"
#include "mpu.h"
#include "mpu6500.h"
#include "sensors.h"
#include "ak8963.h"
#include "bmp280.h"

/*FreeRTOS相关头文件*/
#include "FreeRTOS.h"
#include "task.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 传感器控制代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

#define SENSORS_BIAS_SAMPLES		512		/*计算方差的采样样本个数*/
#define GYRO_VARIANCE_BASE			5000	/* 陀螺仪零偏方差阈值 */
#define GYRO_MIN_BIAS_TIMEOUT_MS   (1000)

typedef struct
{
	Axis3f     bias;
	bool       isBiasValueFound;
	bool       isBufferFilled;
	Axis3i16*  bufHead;
	Axis3i16   buffer[SENSORS_BIAS_SAMPLES];
}BiasObj;

BiasObj	gyroBias;

static bool isInit;
static Axis3i16	gyroRaw;
static Axis3i16	accRaw;
static Axis3i16 magRaw;
static Axis3i16	accLPF;
static Axis3i16	mag;
static float sensorsAccLpfAttFactor;
static bool isMPUPresent=false;
static bool isMagPresent=false;
static bool isBaroPresent=false;

static void sensorsBiasInit(BiasObj* bias);
static void sensorsCalculateVarianceAndMean(BiasObj* bias, Axis3f* varOut, Axis3f* meanOut);
static bool sensorsFindBiasValue(BiasObj* bias);
static void sensorsAddBiasValue(BiasObj* bias, Axis3i16* dval);
static void sensorsAccIIRLPFilter(Axis3i16 *in,Axis3i16 *out);
static void sensor6Read(Axis3f* gyro_out, Axis3f* acc_out);
static void sensor9Read(Axis3f* gyro_out, Axis3f* acc_out, Axis3f* mag_out);

/* 9轴传感器初始化 */
void sensor9Init(void)
{
	iicDevInit();	/*初始化I2C1*/
	isMPUPresent = mpu6500Init();	/*MPU6500初始化*/

#ifdef SENSORS_ENABLE_MAG_AK8963
	isMagPresent=ak8963Init();	/*AK8963初始化*/
#endif

	sensorsBiasInit(&gyroBias);
	
	sensorsAccLpfAttFactor = SENSORS_ACC_IIR_FACTOR;
}

static void sensorsBiasInit(BiasObj* bias)
{
	bias->isBufferFilled = false;
	bias->bufHead = bias->buffer;
}

bool sensorsTest(void)
{
	bool testStatus = true;

	if (!isInit)
	{
		printf("Uninitialized\n");
		testStatus = false;
	}

	testStatus&=isBaroPresent;
	
	return testStatus;
}

static bool sensorsIsCalibrated(void)
{
	bool status;

	status = gyroBias.isBiasValueFound;
	
	return status;
}
void sensor6Read(Axis3f* gyroOut, Axis3f* accOut)
{
	mpu6500GetRawData(&accRaw.y, &accRaw.x, &accRaw.z, &gyroRaw.y, &gyroRaw.x, &gyroRaw.z);

	if (!gyroBias.isBiasValueFound)
	{
		sensorsAddBiasValue(&gyroBias, &gyroRaw);
		sensorsFindBiasValue(&gyroBias);
		if (gyroBias.isBiasValueFound)
		{
			ledseqRun(SYS_LED, seq_calibrated);
		}
	}

	sensorsAccIIRLPFilter(&accRaw,&accLPF);

	gyroOut->x =-(gyroRaw.x - gyroBias.bias.x) * MPU6500_DEG_PER_LSB_2000;
	gyroOut->y = (gyroRaw.y - gyroBias.bias.y) * MPU6500_DEG_PER_LSB_2000;
	gyroOut->z = (gyroRaw.z - gyroBias.bias.z) * MPU6500_DEG_PER_LSB_2000;

	accOut->x =-(accLPF.x) * MPU6500_G_PER_LSB_8;
	accOut->y = (accLPF.y) * MPU6500_G_PER_LSB_8;
	accOut->z = (accLPF.z) * MPU6500_G_PER_LSB_8;	
	
}

void sensor9Read(Axis3f* gyroOut, Axis3f* accOut, Axis3f* magOut)
{
	sensor6Read(gyroOut, accOut);
	if (isMagPresent)
	{
		ak8963GetHeading(&mag.x, &mag.y, &mag.z);	
		ak8963GetOverflowStatus();
		magOut->x = (float)mag.x / MAG_GAUSS_PER_LSB;
		magOut->y = (float)mag.y / MAG_GAUSS_PER_LSB;
		magOut->z = (float)mag.z / MAG_GAUSS_PER_LSB;
	}else
	{
		magOut->x = 0.0;
		magOut->y = 0.0;
		magOut->z = 0.0;
	}
}


/*计算方差和平均值*/
static void sensorsCalculateVarianceAndMean(BiasObj* bias, Axis3f* varOut, Axis3f* meanOut)
{
	u32 i;
	int64_t sum[3] = {0};
	int64_t sumsq[3] = {0};

	for (i = 0; i < SENSORS_BIAS_SAMPLES; i++)
	{
		sum[0] += bias->buffer[i].x;
		sum[1] += bias->buffer[i].y;
		sum[2] += bias->buffer[i].z;
		sumsq[0] += bias->buffer[i].x * bias->buffer[i].x;
		sumsq[1] += bias->buffer[i].y * bias->buffer[i].y;
		sumsq[2] += bias->buffer[i].z * bias->buffer[i].z;
	}

	varOut->x = (sumsq[0] - ((int64_t)sum[0] * sum[0]) / SENSORS_BIAS_SAMPLES);
	varOut->y = (sumsq[1] - ((int64_t)sum[1] * sum[1]) / SENSORS_BIAS_SAMPLES);
	varOut->z = (sumsq[2] - ((int64_t)sum[2] * sum[2]) / SENSORS_BIAS_SAMPLES);

	meanOut->x = (float)sum[0] / SENSORS_BIAS_SAMPLES;
	meanOut->y = (float)sum[1] / SENSORS_BIAS_SAMPLES;
	meanOut->z = (float)sum[2] / SENSORS_BIAS_SAMPLES;
}

static void sensorsAddBiasValue(BiasObj* bias, Axis3i16* dval)
{
	bias->bufHead->x = dval->x;
	bias->bufHead->y = dval->y;
	bias->bufHead->z = dval->z;
	bias->bufHead++;

	if (bias->bufHead >= &bias->buffer[SENSORS_BIAS_SAMPLES])
	{
		bias->bufHead = bias->buffer;
		bias->isBufferFilled = true;
	}
}

static bool sensorsFindBiasValue(BiasObj* bias)
{
	bool foundbias = false;

	if (bias->isBufferFilled)
	{
		Axis3f variance;
		Axis3f mean;

		sensorsCalculateVarianceAndMean(bias, &variance, &mean);

		if (variance.x < GYRO_VARIANCE_BASE && variance.y < GYRO_VARIANCE_BASE && variance.z < GYRO_VARIANCE_BASE)
		{
			bias->bias.x = mean.x;
			bias->bias.y = mean.y;
			bias->bias.z = mean.z;
			foundbias = true;
			bias->isBiasValueFound= true;
		}else
			bias->isBufferFilled=false;
	}
	return foundbias;
}
/* 加速计IIR低通滤波 */
static void sensorsAccIIRLPFilter(Axis3i16 *in,Axis3i16 *out)
{
	out->x = out->x + sensorsAccLpfAttFactor*(in->x - out->x); 
	out->y = out->y + sensorsAccLpfAttFactor*(in->y - out->y); 
	out->z = out->z + sensorsAccLpfAttFactor*(in->z - out->z); 
}

/* 传感器初始化 */
void sensorsInit(void)
{
	if(isInit) return;
	
	sensor9Init();		/* 9轴传感器初始化 */
	
#ifdef SENSORS_ENABLE_PRESSURE_BMP280	
	isBaroPresent=bmp280Init();	/*BMP280初始化*/
#endif
	
	isInit = true;
}
void sensorsAcquire(sensorData_t *sensors, const u32 tick)	/*获取传感器数据*/
{	
	if (RATE_DO_EXECUTE(SENSOR9_UPDATE_RATE, tick)) /** 500Hz 2ms update **/
	{
		sensor9Read(&sensors->gyro, &sensors->acc, &sensors->mag);
	}

	if (RATE_DO_EXECUTE(BARO_UPDATE_RATE, tick) && isBaroPresent==true) /** 50Hz 40ms update **/
	{	
	#ifdef SENSORS_ENABLE_PRESSURE_BMP280			
		bmp280GetData(&sensors->baro.pressure, &sensors->baro.temperature, &sensors->baro.asl);
	#endif
	}
}

bool sensorsAreCalibrated()	/*传感器数据校准*/
{
	Axis3f dummyData;
	sensor6Read(&dummyData, &dummyData);
	return sensorsIsCalibrated();
}

void getSensorRawData(Axis3i16* acc, Axis3i16* gyro, Axis3i16* mag)
{
	*acc = accRaw;
	*gyro = gyroRaw;
	*mag = magRaw;
}

bool getIsMPU9250Present(void)
{
	bool value = isMPUPresent;
#ifdef SENSORS_ENABLE_MAG_AK8963
	value &= isMagPresent;
#endif
	return value;
}

bool getIsBaroPresent(void)
{
	return isBaroPresent;
}



