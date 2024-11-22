#include <string.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "led.h"
#include "button.h"
#include "pm.h"
#include "pinout.h"
#include "systick.h"
#include "radiolink.h"
#include "uartlink.h"
#include "config_param.h"
#include "uart.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * main.c	
 * 包括系统初始化和通信处理
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

static void mainloop(void);
static void handleRadioCmd(atkp_t * packet);

int main()
{
	/*等待外部晶振起振*/
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
	
	LED_INIT();
	LED_OFF();
	systickInit();		
	buttonInit(buttonIdle);

	/*按键长按2S松开后启动STM32进入bootloader*/
	if(BUTTON_READ() == BUTTON_PRESSED)
	{
		msDelay(50);
		uint32_t ledTime= systickGetTick();		
		uint32_t tickCnt = systickGetTick();
		while(BUTTON_READ() == BUTTON_PRESSED)
		{
			if(systickGetTick() - tickCnt > 2000)
			{
				nrf_gpio_cfg_output(UART_RTS_PIN);
				nrf_gpio_pin_set(UART_RTS_PIN);	
				if(systickGetTick() - ledTime > 200)
				{	
					ledTime = systickGetTick();
					LED_TOGGLE();
				}					
			}
		}
	}
	/*外设和通信初始化*/
	LED_ON();
	pmInit();
	uartInit();
	configParamInit();
	radiolinkInit();
	/*通信处理*/
	mainloop();
	while(1);
}

static void mainloop(void)
{
	static atkp_t radiolinkRxPacket;
	static atkp_t uartlinkRxPacket;
	static int vbatSendTime;
	static int radioRSSISendTime;
	
	while(1)
	{
		if(radiolinkReceiveATKPPacket(&radiolinkRxPacket) == true)
		{
			if(radiolinkRxPacket.msgID == DOWN_RADIO)//无线设置
			{
				handleRadioCmd(&radiolinkRxPacket);
			}
			else//转发给STM32
			{	
				uartlinkSendATKPPacket(&radiolinkRxPacket);
			}
		}

		if(uartlinkReceiveATKPPacket(&uartlinkRxPacket) == true)
		{
			if(uartlinkRxPacket.msgID == DOWN_RADIO)//无线设置
			{

			}
			else//转发给遥控器
			{
				radiolinkSendATKPPacket(&uartlinkRxPacket);
			}
		}

		if (systickGetTick() > 1500)/*延时1500ms等STM32启动*/
		{
			/*100ms发送电池电量给STM32*/
			if (systickGetTick() >= vbatSendTime + 100)
			{
				float bat;
				uint8_t flags = 0;
				atkp_t txPacket;

				vbatSendTime = systickGetTick();
				txPacket.msgID = DOWN_POWER;
				txPacket.dataLen = 5;
				flags |= (pmIsCharging() == true)?0x01:0;
				flags |= (pmUSBPower() == true)?0x02:0;
				txPacket.data[0] = flags;
				bat = pmGetVBAT();
				memcpy(txPacket.data+1, &bat, sizeof(float));
				uartlinkSendATKPPacket(&txPacket);
			}
			/*50ms发送一次RSSI给遥控器*/
			if(systickGetTick() >= radioRSSISendTime + 50) 
			{
				atkp_t txPacket;

				radioRSSISendTime = systickGetTick();
				txPacket.msgID = UP_RADIO;
				txPacket.dataLen = 2;
				txPacket.data[0] = U_RADIO_RSSI;
				txPacket.data[1] = getRadioRssi();
				radiolinkSendATKPPacket(&txPacket);
			}
		}

		/*按键事件处理*/
		buttonEvent_e be = buttonGetState();
		if(be == buttonShortPress)
		{
			pmPowerOff();/*关机*/
		}
		else if(be == buttonLongPress)
		{
			nrf_gpio_pin_clear(PM_VCCEN_PIN);
			int timeOut = systickGetTick();
			while(BUTTON_READ()==BUTTON_PRESSED &&
				systickGetTick() < timeOut+3000)/*按键释放或超时3S则退出*/
			{
				msDelay(400);
				LED_TOGGLE();
			}
			resetConigParam();/*恢复默认参数*/
			__disable_irq();
			NVIC_SystemReset();/*软件复位*/
		}
		
		/*按键处理和电源管理*/
		buttonProcess();
		pmProcess();
	}
}

/*产生随机数*/
static uint8_t generateRandomNum(void)
{
	NRF_RNG->TASKS_START = 1;
	NRF_RNG->EVENTS_VALRDY = 0;
	while(NRF_RNG->EVENTS_VALRDY == 0);
	NRF_RNG->TASKS_STOP = 1;
	return NRF_RNG->VALUE;
}

/*无线配置处理*/
static void handleRadioCmd(atkp_t *packet)
{
	if(packet->data[0] == D_RADIO_HEARTBEAT) 
	{
		uartlinkSendATKPPacket(packet);
	}
	else if(packet->data[0] == D_RADIO_GET_CONFIG)
	{
		atkp_t txPacket;
		radioConfig_t radio;
		
		radio.addressLow = NRF_FICR->DEVICEID[0];/*芯片UID低四字节*/
		radio.addressHigh = NRF_FICR->DEVICEID[1];/*芯片UID高四字节*/
		radio.dataRate = RADIO_DATARATE;
		uint8_t ch = generateRandomNum();
		ch = ch & 0x7F;
		if(ch < 2) ch = 2;
		else if(ch > 125) ch = 125;
		radio.channel = ch;/*通道：2-125*/
		
		txPacket.msgID = UP_RADIO;
		txPacket.dataLen = sizeof(radio) + 1;
		txPacket.data[0] = U_RADIO_CONFIG;
		memcpy(txPacket.data+1, &radio, sizeof(radio));
		radiolinkSendATKPPacket(&txPacket);
	}
	else if(packet->data[0] == D_RADIO_SET_CONFIG)
	{
		radioConfig_t radio;
		memcpy(&radio, packet->data+1, sizeof(radio));
		
		radiolinkSetChannel(radio.channel);
		radiolinkSetDatarate(radio.dataRate);
		uint64_t addr = (uint64_t)radio.addressHigh<<32 | radio.addressLow;
		radiolinkSetAddress(addr);
		configParam.radio = radio;
		writeConfigParamToFlash();
	}
}


