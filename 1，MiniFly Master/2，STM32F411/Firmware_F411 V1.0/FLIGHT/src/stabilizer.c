#include "system.h"
#include "stabilizer.h"
#include "sensors.h"
#include "commander.h"
#include "anomal_detec.h"
#include "state_control.h"
#include "state_estimator.h"
#include "power_control.h"
#include "flip.h"

/*FreeRTOS相关头文件*/
#include "FreeRTOS.h"
#include "task.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 四轴自稳控制代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

static bool isInit;
static bool isCalibPass;
static setpoint_t 	setpoint;	/*设置目标状态*/
static sensorData_t sensorData;	/*传感器数据*/
static state_t 		state;		/*四轴姿态*/
static control_t 	control;	/*四轴控制参数*/

void stabilizerTask(void* param);

void stabilizerInit(void)
{
	if(isInit) return;

	sensorsInit();			/*传感器初始化*/
	stateControlInit();		/*姿态PID初始化*/
	powerControlInit();		/*电机初始化*/

	isInit = true;
}

bool stabilizerTest(void)
{
	bool pass = true;

	pass &= sensorsTest();
	pass &= stateControlTest();
	pass &= powerControlTest();

	return pass;
}


void stabilizerTask(void* param)
{
	u32 tick = 0;
	u32 lastWakeTime = getSysTickCnt();
	
	ledseqRun(SYS_LED, seq_alive);
	
	while(!sensorsAreCalibrated())
	{
		vTaskDelayUntil(&lastWakeTime, MAIN_LOOP_DT);
	}
	isCalibPass = true;
	while(1) 
	{
		vTaskDelayUntil(&lastWakeTime, MAIN_LOOP_DT);		/*1ms周期延时*/

		sensorsAcquire(&sensorData, tick);					/*获取6轴和气压数据*/
		stateEstimator(&state, &sensorData, tick);			/*四元数 欧拉角计算*/

		commanderGetSetpoint(&setpoint, &state);			/*目标数据和飞行模式设定*/	
		flyerFlipCheck(&setpoint, &control, &state, tick);	/*翻滚检测*/
		anomalDetec(&sensorData, &state, &control);			/*异常检测*/
		stateControl(&control, &sensorData, &state, &setpoint, tick);/*PID控制*/		
		powerControl(&control);								/*控制电机输出*/

		tick++;
	}
}

void getAttitudeData(attitude_t* get)
{
	*get = state.attitude;
}

float getBaroData(void)
{
	return sensorData.baro.asl;
}

float getPositionZ(void)
{
	return state.position.z;
}

bool getIsCalibPass(void)
{
	return isCalibPass;
}
