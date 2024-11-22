#include <math.h>
#include "pid.h"
#include "commander.h"
#include "config_param.h"
#include "position_pid.h"
#include "remoter_ctrl.h"
/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 位置PID控制代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

#define THRUST_SCALE	(100.0f)
#define START_HIRHT		(0.0f)
#define THRUSTBASE_HIGH	38000
#define THRUSTBASE_LOW	28000

typedef struct  
{
	PidObject pid;
	float setpoint;
	bool preMode;
}pidAxis_t;

typedef struct  
{
	pidAxis_t pidVX;
	pidAxis_t pidVY;
	pidAxis_t pidVZ;
	u16 thrustBase; 
}posPid_t;

static posPid_t posPid;

/*基础油门值限制*/
static float limitThrustBase(float input)
{
	if(input > THRUSTBASE_HIGH)
		return THRUSTBASE_HIGH;
	else if(input < THRUSTBASE_LOW)
		return THRUSTBASE_LOW;
	else 
		return input;
}

void positionControlInit(void)
{
	pidInit(&posPid.pidVX.pid, 0, configParam.pidPos.vx, POS_UPDATE_DT);	/*vx PID初始化*/
	pidInit(&posPid.pidVY.pid, 0, configParam.pidPos.vy, POS_UPDATE_DT);	/*vy PID初始化*/
	pidInit(&posPid.pidVZ.pid, 0, configParam.pidPos.vz, POS_UPDATE_DT);	/*vz PID初始化*/
	
	positionResetAllPID();
	posPid.thrustBase = limitThrustBase(configParam.thrustBase);
}

static float runPidZ(pidAxis_t *axis, float input, const setpoint_t *setpoint, float dt) 
{
	float out = 0.f;
	if (axis->preMode == false && setpoint->isAltHold == true)
	{
		positionResetAllPID();
		axis->setpoint = input + START_HIRHT;
		posPid.thrustBase = limitThrustBase(configParam.thrustBase);
	}
	axis->preMode = setpoint->isAltHold;

	if(setpoint->isAltHold == true)
	{
		axis->setpoint += setpoint->velocity.z * dt;
		out = pidUpdate(&axis->pid, axis->setpoint - input);
	}
	return out;
}

static void detecWeight(float thrust, float newThrust, float velocity)
{
	
	static u16 cnt = 0;
	static float sum = 0.0;
	static float detaThrust;
	
	if(fabs(velocity) < 0.02f && thrust > 25000.f)
	{
		sum += newThrust;
		if(++cnt >= 500)	/* 500*0.004=2S */
		{
			cnt = 0;
			u16 temp = limitThrustBase(posPid.thrustBase + sum/500.0f);
			if(fabs(configParam.thrustBase-temp) > 1000)
			{
				configParam.thrustBase = temp;
				detaThrust = (configParam.thrustBase - posPid.thrustBase)/5000.0f;
			}	
		}
	}
	else 
	{
		cnt = 0;	
		sum = 0.0;
	}
	
	if(fabs(configParam.thrustBase - posPid.thrustBase)>50.0f)
	{
		cnt = 0;	
		sum = 0.0;
		posPid.thrustBase += detaThrust;
	}	
}

void altholdPID(float* thrust, const state_t *state, const setpoint_t *setpoint)
{
	float newThrust = 0.0;	
	
	newThrust = THRUST_SCALE * runPidZ(&posPid.pidVZ, state->position.z, setpoint, POS_UPDATE_DT);
	
	if(getCommanderKeyFlight())	/* 定高飞模式检测重量*/
		detecWeight(*thrust, newThrust, state->velocity.z);
	
	*thrust = newThrust + posPid.thrustBase;
	
	if (*thrust > 60000) 
	{
		*thrust = 60000;
	}	
}

void positionResetAllPID(void)
{
  pidReset(&posPid.pidVX.pid);
  pidReset(&posPid.pidVY.pid);
  pidReset(&posPid.pidVZ.pid);
}

void getPositionPIDZ(float* kp, float* ki, float* kd)
{
	*kp = posPid.pidVZ.pid.kp;
	*ki = posPid.pidVZ.pid.ki;
	*kd = posPid.pidVZ.pid.kd ;
}

void setPositionPIDZ(float kp, float ki, float kd)
{
	posPid.pidVZ.pid.kp = kp;
	posPid.pidVZ.pid.ki = ki;
	posPid.pidVZ.pid.kd = kd;
	
	configParam.pidPos.vz.kp  = kp;
	configParam.pidPos.vz.ki  = ki;
	configParam.pidPos.vz.kd  = kd;
}
