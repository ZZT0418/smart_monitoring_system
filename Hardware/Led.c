#include "stm32f10x.h"                  // Device header


//LED初始化函数
void LED_Init(void)
{
	//开启GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	//声明结构体变量
	GPIO_InitTypeDef GPIO_InitStructure;
	//配置GPIO,给变量成员赋值
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//配置GPIO,给变量成员赋值
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	
	//默认给高电平关闭LED
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	
}

void LED1_ON(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}

void LED1_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
}


/*=============LED1灯状态反转函数==============*/
void LED1_Turn(void)
{
	//读取引脚的输出状态，若状态为低电平0亮，则给高电平熄灭，反之点亮。
	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12) == 0)
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_12);
	}
	else
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	}
}

void LED2_ON(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}

void LED2_OFF(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/*=============LED2灯状态反转函数==============*/
void LED2_Turn(void)
{
	//同理LED1
	if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) == 0)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
	}
	else
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	}
}





