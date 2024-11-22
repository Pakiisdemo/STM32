
最新资料下载地址:
	http://www.openedv.com/thread-105197-1-1.html
	
硬件资源：
	1,MCU:STM32F103C8T6 (FLAH:64K, RAM:20K, 系统运行时钟频率:72MHz) 
	2,按键(KEY_L:PB11)
	3,运行指示灯(BLUE_LED1:PB3)

实验现象：
	遥控器按下KEY_L按键开机直至运行指示灯BLUE_LED1闪烁，说明程序进入了Bootloader
	
注意事项:
	代码下载和调试前，请将下载器开关拨到STM32档。
	bootloader起始地址（BOOTLOADER_START_ADDR） : 0x8000000
	固件起始地址（FIRMWARE_START_ADDR） : 0x8002400


					正点原子@ALIENTEK
					2017-6-30
					广州市星翼电子科技有限公司
					电话：020-38271790
					传真：020-36773971
					购买：http://shop62103354.taobao.com
					http://shop62057469.taobao.com
					公司网站：www.alientek.com
					技术论坛：www.openedv.com





































