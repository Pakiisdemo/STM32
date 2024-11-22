#include "uartlink.h"
#include "uart.h"

/********************************************************************************	 
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * 串口连接通信代码
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/2
 * 版本：V1.0
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

/*串口连接接收ATKPPacket*/
bool uartlinkReceiveATKPPacket(atkp_t *p)
{
	static uartLinkRxSate_e rxState = waitForStartByte1;
	static uint8_t dataIndex=0;
	static uint8_t cksum=0;
	static uint8_t length=0;
	uint8_t c;

	p->dataLen = 0;

	if (rxState == state_done)
	{
		rxState = waitForStartByte1;
		dataIndex = 0;
	}

	while (uartIsDataReceived() && (rxState != state_done))
	{
		c = uartGetc();
		switch(rxState)
		{
			case waitForStartByte1:
				rxState = (c == UP_BYTE1) ? waitForStartByte2 : waitForStartByte1;
				cksum = c;
				break;
			case waitForStartByte2:
				rxState = (c == UP_BYTE2) ? waitForMsgID : waitForStartByte1;
				cksum += c;
				break;
			case waitForMsgID:
				p->msgID = c;
				rxState = waitForDataLength;
				cksum += c;
				break;
			case waitForDataLength:
				length = c;
				cksum += c;
				dataIndex = 0;
				if (length > 0 && length <= ATKP_MAX_DATA_SIZE)
					rxState = waitForData;
				else if (length > ATKP_MAX_DATA_SIZE)
					rxState = waitForStartByte1;
				else
					rxState = waitForChksum1;
				break;
			case waitForData:
				if(dataIndex < ATKP_MAX_DATA_SIZE)
				{
					p->data[dataIndex] = c;
					cksum += c;
				}
				dataIndex++;
				if (dataIndex >= length)
				{
					rxState = waitForChksum1;
				}
				break;
			case waitForChksum1:
				if (cksum == c)	/*所有校验正确*/
				{
					p->dataLen = length;
					rxState = state_done;
				} 
				else	/*校验错误*/
				{
					rxState = waitForStartByte1;	
					dataIndex = 0;
				}
				break;
			case state_done:
				break;
		}
	}			
	return (rxState == state_done);
}

/*串口连接接收ATKPPacket*/
bool uartlinkSendATKPPacket(atkp_t *packet)
{
	uint8_t cksum=0;
	int i;

	uartPutc(DOWN_BYTE1);
	cksum = DOWN_BYTE1;
	uartPutc(DOWN_BYTE2);
	cksum += DOWN_BYTE2;
	uartPutc((unsigned char)packet->msgID);
	cksum += packet->msgID;
	uartPutc((unsigned char)packet->dataLen);
	cksum += packet->dataLen;

	for (i=0; i < packet->dataLen; i++)
	{
		uartPutc(packet->data[i]);
		cksum += packet->data[i];
	}
	uartPutc(cksum);
	return true;
}
