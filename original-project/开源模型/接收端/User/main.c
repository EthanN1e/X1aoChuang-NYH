#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Key.h"
#include "NRF24L01.h"
#include "Servo.h"
#include "LED.h"

uint8_t KeyNum;

uint8_t SendFlag;								//发送标志位
uint8_t SendSuccessCount, SendFailedCount;		//发送成功计次，发送失败计次

uint8_t ReceiveFlag;							//接收标志位
uint8_t ReceiveSuccessCount, ReceiveFailedCount;//接收成功计次，接收失败计次

int main(void)
{
	/*初始化*/
	OLED_Init();
	Key_Init();
	NRF24L01_Init();
	Servo_Init();
	LED_Init();
	/*OLED显示静态字符串*/
	OLED_ShowString(1, 1, "T:000-000-0");		//格式为：T:发送成功计次-发送失败计次-发送标志位
	OLED_ShowString(3, 1, "R:000-000-0");		//格式为：R:接收成功计次-接收失败计次-接收标志位
	
	/*初始化测试数据，此处值为任意设定，便于观察实验现象*/
	NRF24L01_TxPacket[0] = 0x00;
	NRF24L01_TxPacket[1] = 0x01;
	NRF24L01_TxPacket[2] = 0x02;
	NRF24L01_TxPacket[3] = 0x03;
	Servo_SetAngle1(90);
	Servo_SetAngle2(90);
	
	while (1)
	{
		
		
		ReceiveFlag = NRF24L01_Receive();
		
		if (ReceiveFlag)				//接收标志位不为0，表示收到了一个数据包
		{
			if (ReceiveFlag == 1)		//接收标志位为1，表示接收成功
			{
				ReceiveSuccessCount ++;	//接收成功计次变量自增
			}
			else	//接收标志位不为0也不为1，即2/3，表示此次接收产生了错误，错误接收的数据不应该使用
			{
				ReceiveFailedCount ++;	//接收失败计次变量自增
			}
			
			
			
			if(NRF24L01_RxPacket[0]==1)
			{
				GPIO_SetBits(GPIOA, GPIO_Pin_9);
				GPIO_SetBits(GPIOA, GPIO_Pin_11);
				
			}
			else
			{
				GPIO_ResetBits(GPIOA, GPIO_Pin_9);
				GPIO_ResetBits(GPIOA, GPIO_Pin_11);
			}
			Servo_SetAngle1(NRF24L01_RxPacket[1]);
			Servo_SetAngle2(NRF24L01_RxPacket[2]);
			
		}
			
	}
}
