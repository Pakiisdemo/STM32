#ifndef __PINOUT_H
#define __PINOUT_H

#define LED_PIN 			8
#define BUTTON_PIN 			17	
#define PM_VCCEN_PIN 		29	/* LDO使能控制脚 */
#define PM_CHG_STATE_PIN 	1	
#define PM_VBAT_SINK_PIN 	5	
#define USB_CONNECTED_PIN	7	/*判断USB是否接入引脚*/

#define RADIO_PAEN_PIN 		19
#define RADIO_PATX_DIS_PIN 	20

/* 与STM32连接引脚 */
#define STM_BOOT0_PIN 		9	
#define STM_NRST_PIN 		3	
#define UART_RX_PIN 		0
#define UART_TX_PIN 		30
#define UART_RTS_PIN 		2

#define AIN_VBAT 			ADC_CONFIG_PSEL_AnalogInput5	/*电压采集引脚*/
#define AIN_VBAT_DIVIDER 	ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling	/*模拟输入1/3分压*/
#define ADC_SCALER 			(2.0/1.0)	/*电阻分压*/
#define ADC_DIVIDER 		(3.0/1.0)	/*电压缩放比例*/

#endif //__PINOUT_H

