#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"


/*=================按键初始化函数=================*/
//通过轮询方式读取按键的值，判断按键是否触发（另一种方式为中断触发）
void KEY_Init(void)
{
	//开启GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//声明结构体变量
	GPIO_InitTypeDef GPIO_InitStructure;
	//配置GPIO,给变量成员赋值
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
}



/*==================获取按键状态值===================*/
uint8_t Key_GetStatus(void)
{
	static uint8_t key_flag = 0;  // 静态变量，记录按键是否已处理
	uint8_t  status = 0;
	
    // 1. 检测按键按下（消抖）
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0 && key_flag == 0)
    {
        // 替换裸机Delay_ms为FreeRTOS非阻塞延时（20ms消抖）
        vTaskDelay(20);   
        
        // 再次确认按键按下（消抖验证）
        if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
        {
            key_flag = 1;  // 标记按键已按下，避免重复触发
            status = 1;    // 返回按键按下状态
        }
    }
    // 2. 检测按键松开（重置标记，非阻塞）
    else if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 1 && key_flag == 1)
    {
        vTaskDelay(20);    
        if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 1)
        {
            key_flag = 0;  // 重置标记，允许下次检测
        }
    }
	
    // 返回状态：1=按键按下（仅触发一次），0=无按键/按键未松开
    return status;
}


