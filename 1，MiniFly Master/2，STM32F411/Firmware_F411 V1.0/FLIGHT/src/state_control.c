#include "string.h"
#include "stabilizer.h"
#include "sensfusion6.h"
#include "state_control.h"
#include "attitude_pid.h"
#include "position_pid.h"
#include "config_param.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 四轴姿态控制代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

static float actualThrust;
static attitude_t attitudeDesired;
static attitude_t rateDesired;


void stateControlInit(void)
{
	attitudeControlInit();	/*初始化姿态PID*/	
	positionControlInit();	/*初始化位置PID*/
}

bool stateControlTest(void)
{
	bool pass = true;
	pass &= attitudeControlTest();
	return pass;
} 

void stateControl(control_t *control, sensorData_t *sensors, state_t *state, setpoint_t *setpoint, const u32 tick)
{
	static u16 cnt = 0;
	
	if (RATE_DO_EXECUTE(POS_UPDATE_RATE, tick))		/* 250Hz */ 
	{
		altholdPID(&actualThrust, state, setpoint);		/* 定高PID(速度模式)*/
	}

	if (RATE_DO_EXECUTE(ATTITUDE_UPDATE_RATE, tick))	/* 500Hz */  
	{
		if(setpoint->isAltHold == false) 
		{
			actualThrust = setpoint->thrust;
		}
		if(control->flipDir == CENTER)
		{
			attitudeDesired.yaw -= setpoint->attitude.yaw/ATTITUDE_UPDATE_RATE;	/*期望YAW 速率模式*/
			while(attitudeDesired.yaw > 180.0f) 
				attitudeDesired.yaw -= 360.0f;
			while(attitudeDesired.yaw < -180.0f) 
				attitudeDesired.yaw += 360.0f;
		}
		attitudeDesired.roll = setpoint->attitude.roll;		/*期望ROLL 角度(自稳)模式*/
		attitudeDesired.pitch = setpoint->attitude.pitch;	/*期望PITCH 角度(自稳)模式*/
		
		attitudeAnglePID(&state->attitude, &attitudeDesired, &rateDesired);	/* 角度(外环)PID */
		
		if(control->flipDir != CENTER)	/*空翻的时候只使用内环PID*/
		{
			rateDesired.pitch = setpoint->attitude.pitch;
			rateDesired.roll = setpoint->attitude.roll;
		}
		attitudeRatePID(&sensors->gyro, &rateDesired, control);				/* 角速度(内环)PID */

	#ifdef ENABLE_PID_TUNING	/* 使能PID调节 yaw值不更新 */
		control->yaw = 0;	
	#endif
	}

	control->thrust = (u16)actualThrust;	

	if (control->thrust == 0)
	{			
		control->thrust = 0;
		control->roll = 0;
		control->pitch = 0;
		control->yaw = 0;
		
		attitudeResetAllPID();	/*复位姿态PID*/	
		positionResetAllPID();	/*复位位置PID*/
		attitudeDesired.yaw = state->attitude.yaw;	/*复位计算的期望yaw值*/
		
		float velZ = state->velocity.z;
		if(velZ<0.001f && velZ>-0.001f && cnt++>1000)	/*降落完成后保存参数*/
		{
			cnt = 0;
			configParamGiveSemaphore();
		}
	}

}


