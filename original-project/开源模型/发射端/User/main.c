#include "stm32f10x.h"                  // Device header"
#include "OLED.h"
#include "Delay.h"
#include "inv_mpu.h"
#include "Servo.h"
#include "Key.h"
#include "NRF24L01.h"

uint8_t KeyNum;

uint8_t SendFlag;								
uint8_t SendSuccessCount, SendFailedCount;		

uint8_t ReceiveFlag;							
uint8_t ReceiveSuccessCount, ReceiveFailedCount;
uint8_t Pitch1,Roll1,Yaw1;
float Pitch,Roll,Yaw;
float Angle;
int main(void)
{
	Key_Init();
	NRF24L01_Init();
	delay_init();
	OLED_Init();
	Servo_Init();
	MPU6050_DMP_Init();
	u8 ac;
	KeyNum = 0;
	

	
	NRF24L01_TxPacket[0] = 0x00;
	NRF24L01_TxPacket[1] = 0x01;
	NRF24L01_TxPacket[2] = 0x02;
	NRF24L01_TxPacket[3] = 0x03;
	
	while(1)
	{
		ac=MPU6050_DMP_Get_Data(&Pitch,&Roll,&Yaw);
		
		if(Pitch < -90)
			Pitch = -90;
		else if(Pitch > 90)
			Pitch = 90;
		
		if(Roll < -90)
			Roll = -90;
		else if(Roll > 90)
			Roll = 90;
		
		if(Yaw < -90)
			Yaw = -90;
		else if(Yaw > 90)
			Yaw = 90;
		
		Pitch1 = (uint8_t)(Pitch+90);
		Roll1 = (uint8_t)(Roll+90);
		Yaw1 = (uint8_t)(Yaw+90);
		
		
		
		
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)==1)
		{
			KeyNum = 1;
		}
		else
		{
			KeyNum = 0;
		}
		NRF24L01_TxPacket[0] = KeyNum; 
		NRF24L01_TxPacket[1] = Roll1;
		NRF24L01_TxPacket[2] = Yaw1;
		NRF24L01_TxPacket[3] ++;
			
			
			SendFlag = NRF24L01_Send();
			
			if (SendFlag == 1)			
			{
				SendSuccessCount ++;	
			}
			else						
			{
				SendFailedCount ++;		
			}	

	}
}
