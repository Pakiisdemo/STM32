#include <stdbool.h>
#include "nrf.h"
#include "config_param.h"

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

/*保存配置参数地址为FLASH最后一页*/
#define CONFIG_PARAM_ADDR  (NRF_FICR->CODEPAGESIZE * (NRF_FICR->CODESIZE-1))

configParam_t configParam;/*配置参数全局变量*/

static bool isConfigParamOK;
static configParam_t configParamDefault=
{
	.version = VERSION,
	.radio.channel = RADIO_CHANNEL,
	.radio.dataRate = RADIO_DATARATE,
	.radio.addressHigh = ((uint64_t)RADIO_ADDRESS >> 32),
	.radio.addressLow = (RADIO_ADDRESS & 0xFFFFFFFFULL),
};

void flash_write(uint32_t pageAddress, uint32_t* data, uint32_t writeNum)
{
	uint32_t* writeAddress = (uint32_t*)pageAddress;
	
	/*擦除指定pageAddress扇区*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);/*使能擦除FLASH*/
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy);/*等待NVMC空闲*/
	NRF_NVMC->ERASEPAGE = (uint32_t)pageAddress;// Erase page:
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy);/*等待NVMC空闲*/
	
	/*写FLASH*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);/*使能写FLASH*/
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy);/*等待NVMC空闲*/
	for(int i=0; i<writeNum; i++)
	{
		*writeAddress = data[i];
		writeAddress += 4;
	}
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy);/*等待NVMC空闲*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);/*配置为只读*/
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy);/*等待NVMC空闲*/
}

void flash_read(uint32_t pageAddress, uint32_t* buffer, uint32_t readNum)
{
	uint32_t* readAddress = (uint32_t*)pageAddress;
	for(int i=0; i<readNum; i++)
	{
		buffer[i] = *readAddress;
		readAddress += 4;
	}
}

/* 计算校验值 */
static uint8_t calculate_cksum(configParam_t* data)
{ 
	uint8_t cksum=0;	
	uint8_t* c = (uint8_t*)data;
	
	for (int i=0; i<sizeof(configParam_t); i++)
		cksum += *(c++);
	cksum -= data->cksum;
	return cksum;
}

/*配置参数初始化*/
void configParamInit(void)
{
	uint32_t size = sizeof(configParam);
	uint32_t readNum = size/4 + (size%4 ? 1:0);
	
	/* 读取配置参数 */
	flash_read(CONFIG_PARAM_ADDR, (uint32_t*)&configParam, readNum);
	
	if(configParam.version == VERSION)/*版本正确*/
	{
		uint8_t cksum = calculate_cksum(&configParam);
		if(cksum == configParam.cksum)/*校验正确*/
			isConfigParamOK = true;
		 else
			isConfigParamOK = false;
	}
	else/*版本更新*/
	{
		isConfigParamOK = false;
	}
	
	/* 配置参数错误，写入默认参数 */
	if(isConfigParamOK == false)	
	{
		configParam = configParamDefault;
		writeConfigParamToFlash();
		isConfigParamOK=true;
	}
}

/*写配置参数进Flash*/
void writeConfigParamToFlash(void)
{
	uint32_t size = sizeof(configParam);
	uint32_t writeNum = size/4 + (size%4 ? 1:0);
	
	uint8_t cksum = calculate_cksum(&configParam);
	configParam.cksum = cksum;
	flash_write(CONFIG_PARAM_ADDR, (uint32_t*)&configParam, writeNum);
}

/*复位配置参数为默认参数*/
void resetConigParam(void)
{
	configParam = configParamDefault;
	writeConfigParamToFlash();	
}

