#include "stm32f10x.h" 

void Buzzer_Init(void)
{
	//使能GPIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	//初始化GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//默认关闭蜂鸣器，给高电平
	GPIO_SetBits(GPIOA, GPIO_Pin_3);
	
}

/*==========Buzzer_ON函数===============
 *功能：有源蜂鸣器响
 *参数：无
 *返回值：无
 *======================================
*/
void Buzzer_ON(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_3);
}

/*==========Buzzer_OFF函数===============
 *功能：有源蜂鸣器不响
 *参数：无
 *返回值：无
 *===================================
*/
void Buzzer_OFF(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_3);
}
