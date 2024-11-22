#include <string.h>
#include "eeprom.h"
#include "stmflash.h"

/*FreeRTOS相关头文件*/
#include "FreeRTOS.h"
#include "task.h"

static uint8_t devAddr;
static bool isInit;


bool eepromTest(void)
{
	bool status;

	status = eepromTestConnection();
	if (status)
	{
		printf("I2C connection [OK].\n");
	}
	else
	{
		printf("I2C connection [FAIL].\n");
	}

	return status;
}


bool eepromTestConnection(void)
{
	return true;/* lyc*/
}

bool eepromReadBuffer(uint8_t* buffer, uint32_t readAddr, uint32_t len)
{
	bool status;

	STMFLASH_Read(readAddr, (uint32_t*)buffer, len);
	return true;/* lyc*/
}

bool eepromWriteBuffer(uint8_t* buffer, uint32_t writeAddr, uint32_t len)
{
	bool status = true;
	uint16_t index;
	
	STMFLASH_Write(writeAddr, (uint32_t*)buffer, len/4+(len%4 ? 1:0));
	vTaskDelay(1);
	return true;/* lyc*/
}

bool eepromWritePage(uint8_t* buffer, uint16_t writeAddr)
{	
	return false;	//TODO: implement
}
