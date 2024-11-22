#ifndef __STABILIZER_TYPES_H
#define __STABILIZER_TYPES_H
#include "sys.h"
#include <stdbool.h>
#include "sensors_types.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 结构体类型定义	
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
	u32 timestamp;	/*时间戳*/

	float roll;
	float pitch;
	float yaw;
} attitude_t;

struct  vec3_s 
{
	u32 timestamp;	/*时间戳*/

	float x;
	float y;
	float z;
};

typedef struct vec3_s point_t;
typedef struct vec3_s velocity_t;
typedef struct vec3_s acc_t;


typedef struct
{
	float pressure;
	float temperature;
	float asl;
} baro_t;

typedef struct
{
	Axis3f acc;
	Axis3f gyro;
	Axis3f mag;
	baro_t baro;
	point_t position;
} sensorData_t;

typedef struct
{
	attitude_t attitude;
	point_t position;
	velocity_t velocity;
	acc_t acc;
} state_t;

enum dir_e
{
	CENTER=0,
	FORWARD,
	BACK,
	LEFT,
	RIGHT,
};

typedef struct
{
	s16 roll;
	s16 pitch;
	s16 yaw;
	float thrust;
	enum dir_e flipDir;		/*翻滚方向*/
} control_t;

typedef enum
{
	modeDisable = 0,
	modeAbs,
	modeVelocity
} mode_t;

typedef struct
{
	attitude_t attitude;
	velocity_t velocity;
	float thrust;	
	bool isAltHold;
} setpoint_t;


#define RATE_5_HZ		5
#define RATE_10_HZ		10
#define RATE_25_HZ		25
#define RATE_50_HZ		50
#define RATE_100_HZ		100
#define RATE_200_HZ 	200
#define RATE_250_HZ 	250
#define RATE_500_HZ 	500
#define RATE_1000_HZ 	1000

#define MAIN_LOOP_RATE 	RATE_1000_HZ
#define MAIN_LOOP_DT	(u32)(1000/MAIN_LOOP_RATE)	/*单位ms*/

#define RATE_DO_EXECUTE(RATE_HZ, TICK) ((TICK % (MAIN_LOOP_RATE / RATE_HZ)) == 0)

#endif


