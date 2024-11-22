#ifndef __STATE_ESTIMATOR_H
#define __STATE_ESTIMATOR_H
#include "stabilizer_types.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 姿态估测代码	
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

typedef struct
{
	float estimatedZ; // The current Z estimate, has same offset as asl
	float velocityZ; // Vertical speed (world frame) integrated from vertical acceleration (m/s)
	float estAlpha;
	float velocityFactor;
	float vAccDeadband; // Vertical acceleration deadband
	float velZAlpha;   // Blending factor to avoid vertical speed to accumulate error
	float velocityX;	/* x 方向速度(世界坐标系) 由x方向加速度积分得出 单位(m/s)*/
	float velocityY;	/* y 方向速度(世界坐标系) 由y方向加速度积分得出 单位(m/s)*/
} selfState_t;

void stateEstimator(state_t *state, const sensorData_t *sensorData, const u32 tick);

#endif /* __STATE_ESTIMATOR_H */
