#include "config.h"
#include "stdbool.h"
#include "delay.h"
#include "ak8963.h"
#include "iic1.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * AK8963驱动代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

static u8 buffer[6];

static bool ak8963EvaluateSelfTest(s16 min, s16 max, s16 value, char* string);

/*AK8963初始化*/
bool ak8963Init(void)
{
	u8 id=ak8963GetDeviceID();
	printf("AK8963 ID IS: %X\n",id);
	if (id==0x48)	
	{
		printf("AK8963 I2C connection [OK].\r\n");
		ak8963SetMode(AK8963_MODE_16BIT|AK8963_MODE_CONT2);	/*16bit 100Hz*/
		if(ak8963SelfTest())	/*AK8963自检*/
			return true;
		else
			return false;
	}else
	{
		printf("AK8963 I2C connection [FAIL].\r\n");
		return false;
	}
}

/*AK8963自检*/
bool ak8963SelfTest(void)
{
	bool testStatus = true;
	s16 mx, my, mz;  // positive magnetometer measurements
	u8 confSave;
	u8 timeout = 20;
	
	confSave=iicDevReadByte(AK8963_ADDR,AK8963_RA_CNTL);

	ak8963SetMode(AK8963_MODE_POWERDOWN);
	ak8963SetSelfTest(true);
	ak8963SetMode(AK8963_MODE_16BIT | AK8963_MODE_SELFTEST);
	ak8963GetOverflowStatus();// Clear ST1 by reading ST2
	
	while (!ak8963GetDataReady() && timeout--)
	{
		delay_xms(1);
	}
	ak8963GetHeading(&mx, &my, &mz);
	ak8963SetMode(AK8963_MODE_POWERDOWN);

	if (ak8963EvaluateSelfTest(AK8963_ST_X_MIN, AK8963_ST_X_MAX, mx, "X") &&
		ak8963EvaluateSelfTest(AK8963_ST_Y_MIN, AK8963_ST_Y_MAX, my, "Y") &&
		ak8963EvaluateSelfTest(AK8963_ST_Z_MIN, AK8963_ST_Z_MAX, mz, "Z"))
	{
		printf("AK8963 Self test [OK].\n");
	}else
	{
		printf("AK8963 Self test [FAIL].\n");
		testStatus = false;
	}
	
	ak8963SetMode(confSave);// Power up with saved config

	return testStatus;
}


static bool ak8963EvaluateSelfTest(s16 min, s16 max, s16 value, char* string)
{
	if (value < min || value > max)
	{
		printf("Self test %s [FAIL]. low: %d, high: %d, measured: %d\n", string, min, max, value);
		return false;
	}
	return true;
}

u8 ak8963GetDeviceID()
{
	return iicDevReadByte(AK8963_ADDR,AK8963_RA_WIA);
}

// INFO 寄存器
u8 ak8963GetInfo()
{
	return iicDevReadByte(AK8963_ADDR,AK8963_RA_INFO);
}

// ST1 寄存器
u8 ak8963GetDataReady()
{
	return iicDevReadByte(AK8963_ADDR,AK8963_RA_ST1);
}

// H* 寄存器
void ak8963GetHeading(s16 *x, s16 *y, s16 *z)
{
	iicDevRead(AK8963_ADDR,AK8963_RA_HXL,6,buffer);
	*x = (((s16) buffer[1]) << 8) | buffer[0];
	*y = (((s16) buffer[3]) << 8) | buffer[2];
	*z = (((s16) buffer[5]) << 8) | buffer[4];
}
s16 ak8963GetHeadingX()
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_CNTL,AK8963_MODE_SINGLE);
	iicDevRead(AK8963_ADDR,AK8963_RA_HXL,2,buffer);
	return (((s16) buffer[1]) << 8) | buffer[0];
}
s16 ak8963GetHeadingY()
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_CNTL,AK8963_MODE_SINGLE);
	iicDevRead(AK8963_ADDR,AK8963_RA_HYL,2,buffer);
	return (((s16) buffer[1]) << 8) | buffer[0];
}
s16 ak8963GetHeadingZ()
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_CNTL,AK8963_MODE_SINGLE);
	iicDevRead(AK8963_ADDR,AK8963_RA_HZL,2,buffer);
	return (((s16) buffer[1]) << 8) | buffer[0];
}

// ST2 寄存器
bool ak8963GetOverflowStatus()
{
	u8 temp=iicDevReadByte(AK8963_ADDR,AK8963_RA_ST2);
	return true;//(temp&(1<<AK8963_ST2_HOFL_BIT));
}
bool ak8963GetDataError()
{
	u8 temp=iicDevReadByte(AK8963_ADDR,AK8963_RA_ST2);
	return true;//(temp&(1<<AK8963_ST2_DERR_BIT));
}

// CNTL 寄存器
u8 ak8963GetMode()
{
	return iicDevReadByte(AK8963_ADDR,AK8963_RA_CNTL);
}
void ak8963SetMode(u8 mode)
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_CNTL,mode);
}
void ak8963Reset()
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_CNTL,AK8963_MODE_POWERDOWN);
}

// ASTC 寄存器
void ak8963SetSelfTest(bool enabled)
{
	u8 temp=iicDevReadByte(AK8963_ADDR,AK8963_RA_ASTC);
	if(enabled)
		temp|=1<<AK8963_ASTC_SELF_BIT;
	else
		temp&=~(1<<AK8963_ASTC_SELF_BIT);
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_ASTC,temp);
}

// I2CDIS
void ak8963DisableI2C()
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_I2CDIS,AK8963_I2CDIS);
}

// ASA* 寄存器
void ak8963GetAdjustment(s8 *x, s8 *y, s8 *z)
{
	iicDevRead(AK8963_ADDR,AK8963_RA_ASAX,3,buffer);
	*x = buffer[0];
	*y = buffer[1];
	*z = buffer[2];
}
void ak8963SetAdjustment(s8 x, s8 y, s8 z)
{
	buffer[0] = x;
	buffer[1] = y;
	buffer[2] = z;
	iicDevWrite(AK8963_ADDR,AK8963_RA_ASAX,3,buffer);
}
u8 ak8963GetAdjustmentX()
{
	return iicDevReadByte(AK8963_ADDR,AK8963_RA_ASAX);
}
void ak8963SetAdjustmentX(u8 x)
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_ASAX,x);
}
u8 ak8963GetAdjustmentY()
{
	return iicDevReadByte(AK8963_ADDR,AK8963_RA_ASAY);
}
void ak8963SetAdjustmentY(u8 y)
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_ASAY,y);
}
u8 ak8963GetAdjustmentZ()
{
	return iicDevReadByte(AK8963_ADDR,AK8963_RA_ASAZ);
}
void ak8963SetAdjustmentZ(u8 z)
{
	iicDevWriteByte(AK8963_ADDR,AK8963_RA_ASAZ,z);
}
