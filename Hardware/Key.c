#include "Key.h"
#include "Led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//信号量句柄
extern xSemaphoreHandle xKeySemaphore;

// /*=================按键初始化函数=================*/
// //通过轮询方式读取按键的值，判断按键是否触发（另一种方式为中断触发）
// void KEY_Init(void)
// {
// 	//开启GPIOA时钟
// 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
// 	//声明结构体变量
// 	GPIO_InitTypeDef GPIO_InitStructure;
// 	//配置GPIO,给变量成员赋值
// 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
// 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
// 	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
// }



// /*==================获取按键状态值===================*/
// uint8_t Key_GetStatus(void)
// {
// 	static uint8_t key_flag = 0;  // 静态变量，记录按键是否已处理
// 	uint8_t  status = 0;
	
//     // 1. 检测按键按下（消抖）
//     if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 0 && key_flag == 0)
//     {
//         // 替换裸机Delay_ms为FreeRTOS非阻塞延时（20ms消抖）
//         vTaskDelay(20);   
        
//         // 再次确认按键按下（消抖验证）
//         if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 0)
//         {
//             key_flag = 1;  // 标记按键已按下，避免重复触发
//             status = 1;    // 返回按键按下状态
//         }
//     }
//     // 2. 检测按键松开（重置标记，非阻塞）
//     else if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 1 && key_flag == 1)
//     {
//         vTaskDelay(20);    
//         if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 1)
//         {
//             key_flag = 0;  // 重置标记，允许下次检测
//         }
//     }
	
//     // 返回状态：1=按键按下（仅触发一次），0=无按键/按键未松开
//     return status;
// }

/*==================按键中断初始化===================*/
void KEY_Init(void)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	
	//声明结构体变量
	GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

	//配置GPIO,给变量成员赋值
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
    //配置中断
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);
    EXTI_InitStructure.EXTI_Line = EXTI_Line11;                 
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;             //中断模式
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;         //下降沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                       //使能中断线
    EXTI_Init(&EXTI_InitStructure);
    //配置中断优先级
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

/*===================== PA11 中断服务函数 =====================*/
void EXTI15_10_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line11) != RESET)
    {
        for(uint32_t i=0; i<10000; i++);

        if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)
        {
            // 给任务发信号
            if(xKeySemaphore != NULL)
            {
                xSemaphoreGiveFromISR(xKeySemaphore, NULL);
            }
        }
    }

        EXTI_ClearITPendingBit(EXTI_Line11);
}

