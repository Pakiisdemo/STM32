#include <math.h>
#include "flip.h"
#include "config_param.h"
#include "commander.h"

/*FreeRTOS相关头文件*/
#include "FreeRTOS.h"
#include "task.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 四轴空翻控制代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

#define FLIP_RATE			RATE_500_HZ
#define MID_VAL				(180.f * FLIP_RATE)
#define MAX_FLIP_RATE		1380	/* <2000 */
#define DELTA_VAL			(30000.f/MAX_FLIP_RATE)
#define FLIP_TIMEOUT		800
#define REVER_SPEEDUP_TIME	160

static enum
{
	FLIP_IDLE = 0,
	FLIP_SET,
	FLIP_SPEED_UP,
	FLIP_SLOW_DOWN,
	FLIP_PERIOD,
	FLIP_FINISHED,
	REVER_SPEED_UP,
	FLIP_ERROR,
}flipState = FLIP_IDLE;

enum dir_e flipDir;
static u16 step = 0;
static float vec_z = 0.45f;
static float out = 0.f;

/********************************************************
* Flyer 翻滚检测 
*********************************************************/
void flyerFlipCheck(setpoint_t* setpoint, control_t* control, state_t* state, const u32 tick)
{
	static u16 flipThrust = 0;
	static u16 tempThrust = 0;
	static u16 reverTime = 0;
	static u16 flipTimeout = 0;
	static float inter = 0.f;
	static float pitchTemp = 0.0;
	static float rollTemp = 0.0;
	static float yawTemp = 0.0;
		
	if(RATE_DO_EXECUTE(FLIP_RATE, tick)==false)
	{
		return;
	}

	switch(flipState)
	{
		case FLIP_IDLE:	/*翻滚空闲状态*/
		{
			if(flipDir!=CENTER)
			{
				if(control->thrust>25000 && state->velocity.z > -0.3f)
					flipState = FLIP_SET;
				else
					flipDir = CENTER;
			}			
			break;
		}
		case FLIP_SET:	/*翻滚设置*/
		{
			out = 0.f;
			step = 0;
			inter = 0.f;
			
			flipTimeout = 0;
			control->flipDir = flipDir;
			flipThrust = configParam.thrustBase - 2000;
			tempThrust = flipThrust; 
			
			rollTemp = state->attitude.roll;
			pitchTemp = state->attitude.pitch;									
			yawTemp = state->attitude.yaw;
			
			flipState = FLIP_SPEED_UP;
			break;
		}
		case FLIP_SPEED_UP:	/*加速上升过程*/
		{
			if(state->velocity.z < vec_z)
			{
				setpoint->isAltHold = false;
				if(tempThrust < flipThrust+15000)
					tempThrust += 500;
				setpoint->thrust = tempThrust;
				
				if(flipTimeout++ > FLIP_TIMEOUT)	/*超时处理*/
				{
					flipTimeout = 0;
					flipState = FLIP_ERROR;
				}														
			}else	
			{	
				flipTimeout = 0;					
				flipState = FLIP_SLOW_DOWN;
			}		
			break;
		}
		case FLIP_SLOW_DOWN:	/*减速过程*/
		{
			if(state->velocity.z>0.1f && tempThrust>flipThrust)
			{
				tempThrust -= 3000;
				setpoint->isAltHold = false;
				setpoint->thrust = tempThrust;
			}else
			{
				flipState = FLIP_PERIOD;
			}
		}
		case FLIP_PERIOD:	/*翻滚过程*/
		{
			if(flipTimeout++ > FLIP_TIMEOUT)	/*超时处理*/
			{
				flipTimeout = 0;
				flipState = FLIP_ERROR;
			}
			
			setpoint->isAltHold = false;
			setpoint->thrust = flipThrust - 3*out;
			
			{
				inter += out;
				if(inter < MID_VAL)
				{
					if(out < MAX_FLIP_RATE)
						out += DELTA_VAL;
					else
						step++;					
				}else
				{
					if(step > 5)
					{						
						step--;			
					}else
					{
						if(out>DELTA_VAL && inter < 2*MID_VAL)						
							out -= DELTA_VAL;
						else							
							flipState = FLIP_FINISHED;						
					}
				}
			}
			switch(control->flipDir)	
			{
				case FORWARD:	/* pitch+ */
					setpoint->attitude.pitch = out;	
					setpoint->attitude.roll = state->attitude.roll = rollTemp;
					setpoint->attitude.yaw = state->attitude.yaw = yawTemp;
					break;				
				case BACK:		/* pitch- */
					setpoint->attitude.pitch = -out;
					setpoint->attitude.roll = state->attitude.roll = rollTemp;
					setpoint->attitude.yaw = state->attitude.yaw = yawTemp;
					break;
				case LEFT:		/* roll- */
					setpoint->attitude.roll = -out;	
					setpoint->attitude.pitch = state->attitude.pitch = pitchTemp;
					setpoint->attitude.yaw = state->attitude.yaw = yawTemp;
					break;
				case RIGHT:		/* roll+ */					
					setpoint->attitude.roll = out;
					setpoint->attitude.pitch = state->attitude.pitch = pitchTemp;
					setpoint->attitude.yaw = state->attitude.yaw = yawTemp;
					break;
				default :break;
			}
			break;
		}
		case FLIP_FINISHED:	/*翻滚完成*/
		{
			setpoint->isAltHold = false;
			setpoint->thrust = tempThrust;
			tempThrust = flipThrust;
			
			reverTime = 0;
			flipTimeout = 0;
			flipDir = CENTER;	
			control->flipDir = flipDir;

			flipState = REVER_SPEED_UP;
			break;
		}
		case REVER_SPEED_UP:	/*翻滚完成后 反向加速*/
		{
			if(reverTime++<REVER_SPEEDUP_TIME)	
			{
				if(tempThrust < flipThrust+15000)
					tempThrust += 1000;
				setpoint->isAltHold = false;
				setpoint->thrust = tempThrust;
			}else
			{				
				flipTimeout = 0;
				flipState = FLIP_IDLE;							
			}
			break;
		}
		case FLIP_ERROR:
		{
			reverTime = 0;
			flipDir = CENTER;	
			control->flipDir = CENTER;
			
			setpoint->isAltHold = false;
			setpoint->thrust = 0;
			if(flipTimeout++ > 5)
			{
				flipTimeout = 0;
				flipState = FLIP_IDLE;
			}
			break;
		}
		default : break;
	}			
}


//设置翻滚方向
void setFlipDir(u8 dir)
{
	flipDir = (enum dir_e)dir;	
}

