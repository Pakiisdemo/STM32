#include <stdbool.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "pinout.h"
#include "pm.h"
#include "led.h"
#include "systick.h"
#include "uart.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 电源管理驱动代码
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

#define TICK_BETWEEN_ADC_MEAS 	5

static float vBat;

/*开启ADC转换*/
static void pmStartAdc(void)
{	
	NRF_ADC->CONFIG = AIN_VBAT << ADC_CONFIG_PSEL_Pos |
	ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos |
	ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos |
	AIN_VBAT_DIVIDER << ADC_CONFIG_INPSEL_Pos;

	NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;
	NRF_ADC->TASKS_START = 0x01;
}

/*开启电源*/
void pmPowerOn(void) 
{
	nrf_gpio_cfg_output(UART_TX_PIN);	//使能串口TX
	nrf_gpio_pin_set(UART_TX_PIN);
	
	nrf_gpio_cfg_output(RADIO_PAEN_PIN);// 开启无线功能
	nrf_gpio_pin_set(RADIO_PAEN_PIN);
	
	nrf_gpio_cfg_output(PM_VBAT_SINK_PIN);// 设置ADC
	nrf_gpio_pin_clear(PM_VBAT_SINK_PIN);

	pmStartAdc();	//开启ADC转换
}

/*关闭电源*/
void pmPowerOff(void) 
{	
	nrf_gpio_cfg_input(UART_TX_PIN, NRF_GPIO_PIN_PULLDOWN);
	uartDeinit();
	
	nrf_gpio_cfg_input(STM_NRST_PIN, NRF_GPIO_PIN_PULLDOWN);
	nrf_gpio_pin_clear(STM_NRST_PIN);
	
	nrf_gpio_pin_clear(PM_VCCEN_PIN);	
	
	LED_OFF();	
	nrf_gpio_pin_clear(RADIO_PAEN_PIN);//关闭PA
	nrf_gpio_cfg_input(PM_VBAT_SINK_PIN, NRF_GPIO_PIN_NOPULL);
	NRF_POWER->SYSTEMOFF = 1UL;
	while(1);
}

void pmInit()
{	
	/* STM32 电源配置 */
	nrf_gpio_cfg_output(PM_VCCEN_PIN);
	nrf_gpio_pin_set(PM_VCCEN_PIN);		//使能stm32电源
	msDelay(100);
	
	/* STM32 复位 */
	nrf_gpio_cfg_output(STM_NRST_PIN);
	nrf_gpio_pin_clear(STM_NRST_PIN);
	msDelay(100);	
	nrf_gpio_pin_set(STM_NRST_PIN);
	msDelay(100);	

	nrf_gpio_cfg_input(USB_CONNECTED_PIN, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(PM_CHG_STATE_PIN, NRF_GPIO_PIN_PULLUP);
	
	pmPowerOn();
}

bool pmUSBPower(void) 
{
	return nrf_gpio_pin_read(USB_CONNECTED_PIN) != 0;	/*USB接入 该引脚被拉高*/
}

bool pmIsCharging(void) 
{
	return nrf_gpio_pin_read(PM_CHG_STATE_PIN) == 0;	/*充电状态下 该引脚被拉低*/
}

float pmGetVBAT(void) 
{
	return vBat;
}


void pmProcess() 
{
	static int lastAdcTick = 0;

	/* VBAT采集在100Hz比较好*/
	if(systickGetTick() - lastAdcTick > TICK_BETWEEN_ADC_MEAS && !NRF_ADC->BUSY)
	{
		uint16_t rawValue = NRF_ADC->RESULT;
		lastAdcTick = systickGetTick();

		vBat = (float) (rawValue / 1023.0) * 1.2 * ADC_SCALER * ADC_DIVIDER;
		pmStartAdc();//开启ADC转换		
	}
}

