#ifndef __ATTITUDE_PID_H
#define __ATTITUDE_PID_H
#include <stdbool.h>
#include "commander.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 姿态PID控制代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

#define ATTITUDE_UPDATE_RATE 	RATE_500_HZ
#define ATTITUDE_UPDATE_DT 		(1.0f / ATTITUDE_UPDATE_RATE)

void attitudeControlInit(void);
bool attitudeControlTest(void);

void attitudeRatePID(Axis3f *actualRate,attitude_t *desiredRate,control_t *output);	/* 角速度环PID */
void attitudeAnglePID(attitude_t *actualAngle,attitude_t *desiredAngle,attitude_t *outDesiredRate);	/* 角度环PID */
void attitudeResetAllPID(void);		/*复位PID*/
void attitudePIDwriteToConfigParam(void);

#endif /* __ATTITUDE_PID_H */
