#include "nrf.h"
#include "systick.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 滴答定时器驱动代码
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

static uint32_t tick = 0;



void TIMER2_IRQHandler()
{
	tick++;
	NRF_TIMER2->EVENTS_COMPARE[0] = 0;	//清除比较寄存器事件标志
}

/*滴答定时器初始化1ms中断一次*/
void systickInit()
{
	NRF_TIMER2->TASKS_CLEAR = 1;	//清除定时器

	NRF_TIMER2->PRESCALER = 4;		//2^4=16分频,16M/16=1Mhz
	NRF_TIMER2->CC[0] = 1000;		//设置捕获比较值	
	NRF_TIMER2->SHORTS = 1UL<<TIMER_SHORTS_COMPARE0_CLEAR_Pos;		//清除比较存器0计数值
	NRF_TIMER2->INTENSET = (1UL << TIMER_INTENSET_COMPARE0_Pos);	//设置比较存器0比较中断
	NVIC_SetPriority(TIMER2_IRQn, 2);
	NVIC_EnableIRQ(TIMER2_IRQn);	

	NRF_TIMER2->TASKS_START = 1;
}

uint32_t systickGetTick()
{
	return tick;
}

void msDelay(int len)
{
	uint32_t start = systickGetTick();
	while (systickGetTick() < start+len);
}

