#ifndef __CONFIG_PARAM_H
#define __CONFIG_PARAM_H
#include "sys.h"
#include <stdbool.h>

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 配置参数驱动代码	
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
	float kp;
	float ki;
	float kd;
} pidInit_t;

typedef struct
{
	pidInit_t roll;
	pidInit_t pitch;	
	pidInit_t yaw;	
} pidParam_t;

typedef struct
{
	pidInit_t vx;
	pidInit_t vy;
	pidInit_t vz;	
} pidParamPos_t;

typedef struct	
{
	u8 version;				/*软件版本号*/
	pidParam_t pidAngle;	/*角度PID*/	
	pidParam_t pidRate;		/*角速度PID*/	
	pidParamPos_t pidPos;	/*位置PID*/
	u16 thrustBase;			/*油门基础值*/
	u8 cksum;				/*校验*/
} configParam_t;


extern configParam_t configParam;

void configParamInit(void);	/*参数配置初始化*/
void configParamTask(void* param);	/*参数配置任务*/
bool configParamTest(void);

void configParamGiveSemaphore(void);
void resetConfigParamPID(void);

#endif /*__CONFIG_PARAM_H */

