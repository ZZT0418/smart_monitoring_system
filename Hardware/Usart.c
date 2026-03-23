#include "stm32f10x.h"
#include <stdio.h>

void Usart1_Init(uint32_t baudrate)
{
	//使能GPIO、Usart时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
	//初始化并配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	
	 //配置PA9（TX引脚）：复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;                  // 指定配置PA9
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;            // 复用推挽输出（关键！串口TX必须用AF_PP）
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;          // 引脚输出速度50MHz（串口常用）
    GPIO_Init(GPIOA, &GPIO_InitStructure);                     // 应用配置到GPIOA
	
	  //配置PA10（RX引脚）：浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;                 // 指定配置PA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;      // 浮空输入（串口RX常用模式）
    GPIO_Init(GPIOA, &GPIO_InitStructure); 
	
	   //配置USART1通信参数
	  USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baudrate;             // 波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;// 数据位：8位（串口最常用）
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     // 停止位：1位（默认）
    USART_InitStructure.USART_Parity = USART_Parity_No;        // 校验位：无
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控（不用CTS/RTS）
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 同时启用发送和接收模式
    USART_Init(USART1, &USART_InitStructure);                  // 应用配置到USART1
    

    //启用USART1外设（核心：配置完必须ENABLE才会工作）
    USART_Cmd(USART1, ENABLE);
}

// 发送字符串到USART1
void Usart1_SendString(char *str)
{
	while(*str != '\0')
	{
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待发送缓冲区空
		USART_SendData(USART1, *str); // 发送字符到串口
		str++;
	}
}



// 重定向fputc函数，使printf能通过USART1输出（需要勾选编译器的微库选项）
int fputc(int ch, FILE *f) {
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待发送缓冲区空
    USART_SendData(USART1, (uint8_t)ch); // 发送字符到串口
    return ch;
}



