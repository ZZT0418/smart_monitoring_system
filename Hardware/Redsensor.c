#include "stm32f10x.h"

void Redsensor_Init(void)
{
	//使能GPIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	//初始化GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;    //上拉输入
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

}
/*==============Redsensor函数====================
 *功能：获取反射式红外传感器引脚电平值
 *参数：无 
 *返回值：uint8_t value
*/
uint8_t Redsensor(void)
{
	uint8_t value;
	value = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2);
	return value;
}
