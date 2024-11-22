#include <math.h>
#include "commander.h"
#include "atkp.h"
#include "config_param.h"
#include "radiolink.h"
#include "remoter_ctrl.h"
/*FreeRTOS相关头文件*/
#include "FreeRTOS.h"
#include "task.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 获取遥控数据驱动代码
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/


#define MIN_THRUST  5000
#define MAX_THRUST  60000

static ctrlValCache_t remoteCache;	/* 遥控缓存数据 */
static ctrlValCache_t wifiCache;	/* wifi缓存数据 */
static ctrlValCache_t* nowCache = &remoteCache;/*默认为遥控*/

static YawModeType yawMode = XMODE;	/* 默认为X飞行模式 */
static commanderBits_t commander;

static void commanderLevelRPY(void)
{
	nowCache->tarVal[nowCache->activeSide].roll = 0;
	nowCache->tarVal[nowCache->activeSide].pitch = 0;
	nowCache->tarVal[nowCache->activeSide].yaw = 0;
}

static void commanderDropToGround(void)
{
	commanderLevelRPY();
	nowCache->tarVal[nowCache->activeSide].thrust = 0;
}

/********************************************************
 *ctrlDataCacheUpdate()	更新控制数据缓存
 *遥控数据 优先级高于wifi控制数据
*********************************************************/
static void ctrlDataCacheUpdate(void)	
{
	u32 tickNow = getSysTickCnt();

	if ((tickNow - remoteCache.timestamp) < COMMANDER_WDT_TIMEOUT_STABILIZE) 
	{
		nowCache = &remoteCache;	/* 遥控缓存数据 */
	}else if ((tickNow - wifiCache.timestamp) < COMMANDER_WDT_TIMEOUT_STABILIZE) 
	{
		nowCache = &wifiCache;		/* wifi缓存数据 */
	}else if ((tickNow - remoteCache.timestamp) < COMMANDER_WDT_TIMEOUT_SHUTDOWN) 
	{
		nowCache = &remoteCache;	/* 遥控缓存数据 */
		commanderLevelRPY();
	}else if ((tickNow - wifiCache.timestamp) < COMMANDER_WDT_TIMEOUT_SHUTDOWN) 
	{
		nowCache = &wifiCache;		/* wifi缓存数据 */
		commanderLevelRPY();
	} else 
	{
		nowCache = &remoteCache;
		commanderDropToGround();
	}
}

/************************************************************************
* 四轴carefree(无头模式)，参考世界坐标系，当四轴围绕YAW旋转后，
* 四轴前方任然保持开始的方向，这个模式对新手非常实用
************************************************************************/
static void rotateYawCarefree(setpoint_t *setpoint, const state_t *state)
{
	float yawRad = state->attitude.yaw * DEG2RAD;
	float cosy = cosf(yawRad);
	float siny = sinf(yawRad);
	float originalRoll = setpoint->attitude.roll;
	float originalPitch = setpoint->attitude.pitch;

	setpoint->attitude.roll = originalRoll * cosy + originalPitch * siny;
	setpoint->attitude.pitch = originalPitch * cosy - originalRoll * siny;
}

/*飞控数据缓存*/
void flightCtrldataCache(ctrlSrc_e ctrlSrc, ctrlVal_t pk)
{
	switch(ctrlSrc)
	{
		case ATK_REMOTER:
			remoteCache.tarVal[!remoteCache.activeSide] = pk;
			remoteCache.activeSide = !remoteCache.activeSide;
			remoteCache.timestamp = getSysTickCnt();
			break;
		
		case WIFI:
			wifiCache.tarVal[!remoteCache.activeSide] = pk;
			wifiCache.activeSide = !remoteCache.activeSide;
			wifiCache.timestamp = getSysTickCnt();
			break;
	}
}

static u8 count=0;
static float autoLandSpeed=-0.25;
/********************************************************
* flyerAutoLand()
* 四轴自动降落
*********************************************************/
void flyerAutoLand(setpoint_t *setpoint,const state_t *state)
{
	float velocityZ=state->acc.z;	/*垂直方向速度*/
	static u16 cnt1=0,cnt2=0;
	static float lastVel=0.0;
	static bool flag=false;
	static bool highLow=true;
		
	setpoint->thrust = 0;
	setpoint->isAltHold = true;		
	setpoint->velocity.z = autoLandSpeed;
	
	if((flag == true)&&(cnt2++ > 1500))
	{
		cnt1=0;
		cnt2=0;
		count=0;
		flag=false;
		highLow=true;			
	}
	if((velocityZ > 0.08f)&&(highLow == true))
	{
		setpoint->velocity.z = 0.2f * velocityZ;
		if((velocityZ < lastVel)&&(cnt1++ > 8))
		{
			cnt1 = 0;
			count++;
			cnt2 = 0;
			flag = true;
			highLow = false;
		}
		lastVel = velocityZ;
	}
	if((velocityZ < -0.04)&&(highLow == false))
	{
		setpoint->velocity.z = 0.2f * velocityZ;
		if((velocityZ > lastVel)&&(cnt1++ > 8))
		{
			cnt1 = 0;
			count++;
			cnt2  = 0;
			flag = true;
			highLow = true;
		}
		lastVel = velocityZ;
	}
	if(count > 4)
	{
		cnt1 = 0;
		cnt2 = 0;
		count = 0;
		highLow = true;
		
		commander.keyLand = false;
		commander.keyFlight = false;
	}	
}


void commanderGetSetpoint(setpoint_t *setpoint, const state_t *state)
{ 
	ctrlDataCacheUpdate();	/*更新控制数据缓存*/
	uint16_t rawThrust = nowCache->tarVal[nowCache->activeSide].thrust;
	if (rawThrust < MIN_THRUST)
		rawThrust = 0;	
	else 		
		rawThrust = (rawThrust>=MAX_THRUST) ? MAX_THRUST:rawThrust;
	
	if(commander.altHoldMode)/*定高模式*/
	{
		if(commander.keyLand)/*一键降落*/
		{
			flyerAutoLand(setpoint,state);
		}
		else if(commander.keyFlight)/*一键起飞*/ 
		{
			setpoint->thrust = 0;
			setpoint->isAltHold = true;
			setpoint->velocity.z = (rawThrust - 32768.f) / 32768.f;
		}
		else/*着陆状态*/
		{
			setpoint->isAltHold = false;
			setpoint->thrust = 0;
			setpoint->velocity.z = 0;
		}
	}
	else /*手动飞模式*/
	{
		setpoint->isAltHold = false;
		setpoint->thrust = rawThrust;
	}
	
	if(commander.emerStop)/*紧急停机*/
	{
		setpoint->isAltHold = false;
		setpoint->thrust = 0;
		setpoint->velocity.z = 0;
	}
	
	if(commander.flightMode)/*飞行模式*/
		yawMode = CAREFREE;
	else
		yawMode = XMODE;
 	
	setpoint->velocity.x = 0;
	setpoint->velocity.y = 0;
	setpoint->attitude.roll = nowCache->tarVal[nowCache->activeSide].roll;
	setpoint->attitude.pitch = nowCache->tarVal[nowCache->activeSide].pitch;
	setpoint->attitude.yaw  = nowCache->tarVal[nowCache->activeSide].yaw;
	if(yawMode == CAREFREE)
	{
		rotateYawCarefree(setpoint, state);
	}
	setpoint->attitude.roll += nowCache->tarVal[nowCache->activeSide].trimRoll;
	setpoint->attitude.pitch += nowCache->tarVal[nowCache->activeSide].trimPitch;
}

void getCommanderTrim(float* pitch, float* roll)
{
	*pitch = nowCache->tarVal[nowCache->activeSide].trimPitch;
	*roll = nowCache->tarVal[nowCache->activeSide].trimRoll;
}

void setCommanderAltholdMode(bool set)
{
	commander.altHoldMode = set;
}
bool getCommanderAltholdMode(void)
{
	return commander.altHoldMode;
}

void setCommanderKeyFlight(bool set)
{
	commander.keyFlight = set;
}
bool getCommanderKeyFlight(void)
{
	return commander.keyFlight;
}

void setCommanderKeyland(bool set)
{
	commander.keyLand = set;
}
bool getCommanderKeyland(void)
{
	return commander.keyLand;
}

void setCommanderFlightmode(bool set)
{
	commander.flightMode = set;
}

void setCommanderEmerStop(bool set)
{
	commander.emerStop = set;
}


