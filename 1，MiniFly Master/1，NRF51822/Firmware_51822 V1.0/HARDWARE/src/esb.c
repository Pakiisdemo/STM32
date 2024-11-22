#include <string.h>
#include "esb.h"
#include "nrf.h"
#include "atkp.h"
#include "radiolink.h"
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

#define RXQ_LEN 16
#define TXQ_LEN 16

/*默认配置*/
static uint8_t channel = 2;	//2400+channel(MHz)
static uint8_t datarate = esbDatarate250K;
static uint8_t txpower = RADIO_TXPOWER_TXPOWER_Pos4dBm;
static uint64_t address = 0x123456789AULL;

static bool isInit = true;
static enum {doTx, doRx} rs;	//Radio state
static EsbPacket ackPacket;		// Empty ack packet

/*
接收到一个数据包rxq_head+1
处理了一个数据包rxq_tail+1
*/
static EsbPacket rxPackets[TXQ_LEN];
static int rxq_head = 0;
static int rxq_tail = 0;
/*
装载了一个数据包txq_head+1
发送了一个数据包txq_tail+1
*/
static EsbPacket txPackets[TXQ_LEN];
static int txq_head = 0;
static int txq_tail = 0;


/*bit交换函数,output[7:0] = input[0:7]*/
static uint32_t swap_bits(uint32_t inp)
{
	uint32_t i;
	uint32_t retval = 0;
	inp = (inp & 0x000000FFUL);
	for(i = 0; i < 8; i++)
	{
		retval |= ((inp >> i) & 0x01) << (7 - i);
	}
	return retval;
}

/*uint32型小端转大端 + bit交换*/
static uint32_t bytewise_bitswap(uint32_t inp)
{
	return (swap_bits(inp >> 0 ) << 24)
		 | (swap_bits(inp >> 8 ) << 16)
		 | (swap_bits(inp >> 16) << 8)
		 | (swap_bits(inp >> 24) << 0);
}

/*判断是否为重发*/
static bool isRetry(EsbPacket *pk)
{
	static int prevPid;
	static int prevCrc;
	bool retry = false;
	
	if ((prevPid == pk->pid) && (prevCrc == pk->crc)) 
	{
		retry = true;
	}
	prevPid = pk->pid;
	prevCrc = pk->crc;

	return retry;
}

/*发送*/
static void setupTx(bool retry)
{
	static EsbPacket * lastSentPacket;

	if (retry) 
	{
		NRF_RADIO->PACKETPTR = (uint32_t)lastSentPacket;
	} 
	else 
	{
		if (lastSentPacket != &ackPacket) 
		{
			//No retry, TX payload has been sent!
			if (txq_head != txq_tail) 		
				txq_tail = ((txq_tail+1)%TXQ_LEN);			
		}
		
		if (txq_tail != txq_head)
		{
			// Send next TX packet
			NRF_RADIO->PACKETPTR = (uint32_t)&txPackets[txq_tail];		
			lastSentPacket = &txPackets[txq_tail];
		} 
		else 
		{
			// Send empty ACK
			ackPacket.size = 4;
			ackPacket.data[0] = UP_RADIO;
			ackPacket.data[1] = 0x02;
			ackPacket.data[2] = U_RADIO_RSSI;
			ackPacket.data[3] = NRF_RADIO->RSSISAMPLE;

			NRF_RADIO->PACKETPTR = (uint32_t)&ackPacket;
			lastSentPacket = &ackPacket;
		}
	}
	//After being disabled the radio will automatically send the ACK
	NRF_RADIO->SHORTS &= ~RADIO_SHORTS_DISABLED_RXEN_Msk;
	NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_TXEN_Msk;
	rs = doTx;
	NRF_RADIO->TASKS_DISABLE = 1UL;
}

/*接收*/
static void setupRx(void)
{
	NRF_RADIO->PACKETPTR = (uint32_t)&rxPackets[rxq_head];
	NRF_RADIO->SHORTS &= ~RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_RXEN_Msk;
	rs = doRx;
	NRF_RADIO->TASKS_DISABLE = 1UL;
}

/*无线中断处理*/
void RADIO_IRQHandler()
{
	EsbPacket *pk;

	if(NRF_RADIO->EVENTS_END) 
	{
		NRF_RADIO->EVENTS_END = 0UL;
		switch (rs)
		{
			case doRx:
				/*错误的CRC校验则丢弃*/
				if (!NRF_RADIO->CRCSTATUS) 
				{
					NRF_RADIO->TASKS_START = 1UL;
					return;
				}
				/*接收队列没空余则丢弃*/
				if (((rxq_head+1)%RXQ_LEN) == rxq_tail) 
				{
					NRF_RADIO->TASKS_START = 1UL;
					return;
				}
				pk = &rxPackets[rxq_head];
				pk->rssi = (uint8_t) NRF_RADIO->RSSISAMPLE;
				pk->crc = NRF_RADIO->RXCRC;
				
				/*需要应答*/
				if(pk->noack)
				{
					if(isRetry(pk))
					{
						setupTx(true);/*如果是重发则返回ACK*/
						return;
					}
					setupTx(false);
				}
				/*正确接收*/
				rxq_head = ((rxq_head+1)%RXQ_LEN);
				break;
				
			case doTx:
				/*启动重新接收*/
				setupRx();
				break;
		}
	}
}


// S1 is used for compatibility with NRF24L0+. These three bits are used
// to store the PID and NO_ACK.
#define PACKET0_S1_SIZE                  (3UL)
// S0 is not used
#define PACKET0_S0_SIZE                  (0UL)
// The size of the packet length field is 6 bits
#define PACKET0_PAYLOAD_SIZE             (6UL)
// The size of the base address field is 4 bytes
#define PACKET1_BASE_ADDRESS_LENGTH      (4UL)
// Don't use any extra added length besides the length field when sending
#define PACKET1_STATIC_LENGTH            (0UL)
// Max payload allowed in a packet
#define PACKET1_PAYLOAD_SIZE             (32UL)

/*无线配置初始化*/
void esbInit()
{
	NRF_RADIO->POWER = 1;
	NVIC_EnableIRQ(RADIO_IRQn);
	NRF_RADIO->TXPOWER = (txpower << RADIO_TXPOWER_TXPOWER_Pos);

	switch (datarate) 
	{
		case esbDatarate250K:
			NRF_RADIO->MODE = (RADIO_MODE_MODE_Nrf_250Kbit << RADIO_MODE_MODE_Pos);
			break;
		case esbDatarate1M:
			NRF_RADIO->MODE = (RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos);
			break;
		case esbDatarate2M:
			NRF_RADIO->MODE = (RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos);
			break;
	}

	NRF_RADIO->FREQUENCY = channel;

	// Radio address config
	// Using logical address 0 so only BASE0 and PREFIX0 & 0xFF are used
	NRF_RADIO->PREFIX0 = 0xC4C3C200UL | (swap_bits(address) & 0xFF);  // Prefix byte of addresses 3 to 0
	NRF_RADIO->PREFIX1 = 0xC5C6C7C8UL;  // Prefix byte of addresses 7 to 4
	NRF_RADIO->BASE0   = bytewise_bitswap((uint32_t)(address>>8));  // Base address for prefix 0
	NRF_RADIO->BASE1   = 0x00C2C2C2UL;  // Base address for prefix 1-7
	NRF_RADIO->TXADDRESS = 0x00UL;      // Set device address 0 to use when transmitting
	NRF_RADIO->RXADDRESSES = 0x01UL;    // Enable device address 0 to use which receiving

	// Packet configuration
	NRF_RADIO->PCNF0 =	(PACKET0_S1_SIZE << RADIO_PCNF0_S1LEN_Pos) |
						(PACKET0_S0_SIZE << RADIO_PCNF0_S0LEN_Pos) |
						(PACKET0_PAYLOAD_SIZE << RADIO_PCNF0_LFLEN_Pos);

	// Packet configuration
	NRF_RADIO->PCNF1 =	(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos)    |
						(RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos)           |
						(PACKET1_BASE_ADDRESS_LENGTH << RADIO_PCNF1_BALEN_Pos)       |
						(PACKET1_STATIC_LENGTH << RADIO_PCNF1_STATLEN_Pos)           |
						(PACKET1_PAYLOAD_SIZE << RADIO_PCNF1_MAXLEN_Pos);

	// CRC Config
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits
	NRF_RADIO->CRCINIT = 0xFFFFUL;      // Initial value
	NRF_RADIO->CRCPOLY = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1

	// Enable interrupt for end event
	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;

	// Set all shorts so that RSSI is measured and only END is required interrupt
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk;
	NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
	NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_RSSISTOP_Enabled;

	// Set RX buffer and start RX
	rs = doRx;
	NRF_RADIO->PACKETPTR = (uint32_t)&rxPackets[rxq_head];
	NRF_RADIO->TASKS_RXEN = 1U;

	isInit = true;
}

/*无线复位*/
void esbReset()
{
	if (!isInit) return;
	__disable_irq();
	NRF_RADIO->TASKS_DISABLE = 1;
	NRF_RADIO->POWER = 0;

	NVIC_GetPendingIRQ(RADIO_IRQn);
	__enable_irq();
	esbInit();
}

/*关闭无线*/
void esbDeinit()
{
	NVIC_DisableIRQ(RADIO_IRQn);
	NRF_RADIO->INTENCLR = RADIO_INTENSET_END_Msk;
	NRF_RADIO->SHORTS = 0;
	NRF_RADIO->TASKS_DISABLE = 1;
	NRF_RADIO->POWER = 0;
}

bool esbIsRxPacket()
{
	return (rxq_head != rxq_tail);
}

EsbPacket * esbGetRxPacket()
{
	EsbPacket *pk = NULL;

	if (esbIsRxPacket()) 
	{
		pk = &rxPackets[rxq_tail];
	}

	return pk;
}

void esbReleaseRxPacket()
{
	rxq_tail = (rxq_tail+1)%RXQ_LEN;
}

bool esbCanTxPacket()
{
	return ((txq_head+1)%TXQ_LEN)!=txq_tail;
}

EsbPacket * esbGetTxPacket()
{
	EsbPacket *pk = NULL;

	if (esbCanTxPacket()) 
	{
		pk = &txPackets[txq_head];
	}

	return pk;
}

void esbSendTxPacket()
{
	txq_head = (txq_head+1)%TXQ_LEN;
}

void esbSetDatarate(EsbDatarate dr)
{
	datarate = dr;
	esbReset();
}

void esbSetChannel(unsigned int ch)
{
	if (channel < 126) 
	{
		channel = ch;
	}
	esbReset();
}

void esbSetTxPower(int power)
{
	txpower = power;
	esbReset();
}

void esbSetAddress(uint64_t addr)
{
	address = addr;
	esbReset();
}
