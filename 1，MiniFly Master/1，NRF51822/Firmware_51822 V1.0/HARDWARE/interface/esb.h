#ifndef __ESB_H
#define __ESB_H
#include <stdbool.h>
#include <stdint.h>

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 无线驱动代码
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

#pragma anon_unions

/*无线数据包格式*/
typedef __packed struct esbPacket_s 
{
	__packed struct 
	{
		uint8_t size;
		__packed union
		{
			uint8_t s1;
			__packed struct 
			{
				uint8_t noack :1;
				uint8_t pid :2;
			};
		};
		uint8_t data[32];
	};
	uint8_t rssi; /*0-100（单位db）*/
	unsigned int crc;
} EsbPacket;

typedef enum esbDatarate_e { 
	esbDatarate250K=0, 
	esbDatarate1M=1, 
	esbDatarate2M=2 
} EsbDatarate;


void esbInit(void);
void esbDeinit(void);
void esbInterruptHandler(void);
bool esbIsRxPacket(void);
EsbPacket * esbGetRxPacket(void);
void esbReleaseRxPacket(void);
bool esbCanTxPacket(void);
EsbPacket * esbGetTxPacket(void);
void esbSendTxPacket(void);
void esbSetDatarate(EsbDatarate datarate);
void esbSetChannel(unsigned int channel);
void esbSetTxPower(int power);
void esbSetAddress(uint64_t address);

#endif //__ESB_H
