#ifndef __RADIOLINK_H
#define __RADIOLINK_H
#include <stdint.h>
#include <stdbool.h>
#include "atkp.h"
#include "esb.h"

/*上行指令*/
#define  U_RADIO_RSSI		0x01
#define  U_RADIO_CONFIG		0x02	

/*下行指令*/
#define  D_RADIO_HEARTBEAT	0xFF
#define  D_RADIO_GET_CONFIG	0x01
#define  D_RADIO_SET_CONFIG	0x02

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 无线连接通信代码
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

void radiolinkInit(void);
bool radiolinkSendATKPPacket(const atkp_t* send);
bool radiolinkReceiveATKPPacket(atkp_t* rxPacket);
uint8_t getRadioRssi(void);
void radiolinkSetAddress(uint64_t address);
void radiolinkSetChannel(uint8_t channel);
void radiolinkSetDatarate(EsbDatarate dataRate);


#endif /*__RADIOLINK_H*/
