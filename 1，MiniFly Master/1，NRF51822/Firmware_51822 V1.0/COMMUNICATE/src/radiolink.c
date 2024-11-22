#include <string.h>
#include "radiolink.h"
#include "esb.h"
#include "config_param.h"

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

static uint8_t rssi;

/*无线初始化*/
void radiolinkInit(void)
{
	uint64_t addr = (uint64_t)configParam.radio.addressHigh<<32 | configParam.radio.addressLow;
	esbInit();
	esbSetDatarate((EsbDatarate)configParam.radio.dataRate);
	esbSetChannel(configParam.radio.channel);
	esbSetAddress(addr);
}

/*无线发送ATKPPacket*/
bool radiolinkSendATKPPacket(const atkp_t* send)
{
	if(esbCanTxPacket() && (send->dataLen <= ATKP_MAX_DATA_SIZE))
	{
		EsbPacket* packet = esbGetTxPacket();
		if (packet) 
		{
			memcpy(packet->data, send, send->dataLen+2);/*2即msgID、dataLen两字节*/
			packet->size = send->dataLen+2;
			esbSendTxPacket();
			return true;
		}
	}
	return false;
}

/*无线接收ATKPPacket*/
bool radiolinkReceiveATKPPacket(atkp_t* rxPacket)
{	
	if(esbIsRxPacket())
	{
		EsbPacket* packet = esbGetRxPacket();
		if(packet)
		{
			rssi = packet->rssi;/*将RSSI先保存下来待会儿返回给遥控器*/
			memcpy(rxPacket, packet->data, packet->size);
			esbReleaseRxPacket();
			return true;
		}
	}
	return false;
}

/*获取无线信号*/
uint8_t getRadioRssi(void)
{
	return rssi;
}

/*设置无线地址*/
void radiolinkSetAddress(uint64_t address)
{
	esbSetAddress(address);
}

/*设置无线通道*/
void radiolinkSetChannel(uint8_t channel)
{
	esbSetChannel(channel);
}

/*设置无线速率*/
void radiolinkSetDatarate(EsbDatarate dataRate)
{
	esbSetDatarate(dataRate);
}
