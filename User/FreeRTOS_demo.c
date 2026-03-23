#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "Led.h"
#include "Key.h"
#include "Usart.h"
#include "Buzzer.h"
#include "OLED.h"
#include "ADC.h"
#include "Redsensor.h"
#include "Control.h"
#include <stdio.h>
#include <string.h>

int alarm_flag = 0; // 0-无报警，1-环境报警，2-障碍物报警，3-混合报警
int motor_flag = 0;

// 队列和互斥量句柄声明
QueueHandle_t  xQueue_AdcData_Display = NULL;   // ADC数据队列 — 显示任务使用
QueueHandle_t  xQueue_AdcData_Alarm = NULL;     // ADC数据队列 — 报警任务使用
QueueHandle_t  xQueue_AdcData_Usart = NULL;    // ADC数据队列 — USART任务使用

// 定义ADC采集数据结构体，接收端一次接收就是一个完整的数据对，避免了数据错乱和需要两次接收的问题。
typedef struct
{
  uint16_t adc_light;        // 光照采集值
  uint16_t adc_temperature;  // 温度采集值
} ADCData_t;
ADCData_t adc_data;           // 全局变量，用于存储当前采集的ADC数据

// 启动任务的配置
#define START_TASK_STACK 128            // 任务栈的大小，默认最小128
#define START_TASK_PRIORITY 1           // 任务优先级：数值越大，优先级越高
TaskHandle_t start_task_handle;         // 任务句柄
void start_task(void *pvParameters);    // 开始任务的函数

// 报警任务配置
#define ALARM_TASK_STACK 512
#define ALARM_PRIORITY 5
TaskHandle_t alarm_task_handle;
void alarm_task(void *pvParameters);

// ADC数据采集任务任务配置
#define ADC_TASK_STACK 1024           // 任务栈的大小1024，防止数据采集过程中发生栈溢出
#define ADC_TASK_PRIORITY 4
TaskHandle_t adc_task_handle;
void adc_task(void *pvParameters);


// 显示任务配置
#define DISPLAY_TASK_STACK 1024
#define DISPLAY_TASK_PRIORITY 3
TaskHandle_t display_task_handle;
void display_task(void *pvParameters);

// 串口数据接收任务配置
#define USART_TASK_STACK 512
#define USART_TASK_PRIORITY 4
TaskHandle_t usart_task_handle;
void usart_task(void *pvParameters);


//====================== 启动FreeRTOS函数 ===================//
void FreeRTOS_Start(void)
{
	// 1. 创建一个启动任务
	xTaskCreate( (TaskFunction_t) start_task,
               (char *) "start_task",
               (configSTACK_DEPTH_TYPE) START_TASK_STACK,
               (void * ) NULL,
               (UBaseType_t) START_TASK_PRIORITY,
               (TaskHandle_t * )&start_task_handle );
	// 2. 启动调度器
	vTaskStartScheduler();

}

//=================== 启动任务函数 ====================
/* 功能：用来创建其它任务
 * 参数：void *pvParameters
 * 返回值：无
 */
void start_task(void *pvParameters)
{
	// 进入临界区：临界区内代码强制执行完,中途不会被打断
	taskENTER_CRITICAL();

    // 创建队列
    xQueue_AdcData_Display = xQueueCreate(10, sizeof(ADCData_t)); // ADC数据队列（显示）
    xQueue_AdcData_Alarm = xQueueCreate(10, sizeof(ADCData_t));   // ADC数据队列（报警）
    xQueue_AdcData_Usart = xQueueCreate(10, sizeof(ADCData_t));  // ADC数据队列（USART）

	// 创建其它的5个任务
		xTaskCreate( (TaskFunction_t) alarm_task,
               (char *) "alarm_task",
               (configSTACK_DEPTH_TYPE) ALARM_TASK_STACK,
               (void * ) NULL,
               (UBaseType_t) ALARM_PRIORITY,
               (TaskHandle_t * )&alarm_task_handle );  // 任务句柄的地址

		xTaskCreate( (TaskFunction_t) adc_task,
               (char *) "adc_task",
               (configSTACK_DEPTH_TYPE) ADC_TASK_STACK,
               (void * ) NULL,
               (UBaseType_t) ADC_TASK_PRIORITY,
               (TaskHandle_t * )&adc_task_handle );
		xTaskCreate( (TaskFunction_t) display_task,
               (char *) "display_task",
               (configSTACK_DEPTH_TYPE) DISPLAY_TASK_STACK,
               (void * ) NULL,
               (UBaseType_t) DISPLAY_TASK_PRIORITY,
               (TaskHandle_t * )&display_task_handle );
        xTaskCreate( (TaskFunction_t) usart_task,
               (char *) "usart_task",
               (configSTACK_DEPTH_TYPE) USART_TASK_STACK,
               (void * ) NULL,
               (UBaseType_t) USART_TASK_PRIORITY,
               (TaskHandle_t * )&usart_task_handle );

    // 删除启动任务自己
		vTaskDelete(NULL);
		// 结束临界区
		taskEXIT_CRITICAL();
}


//============================ ADC数据采集任务函数 ================================
// 功能：周期性100ms地从 ADC 读取数据，并将采集到的数据通过队列发送给控制任务和显示任务。
// 参数：void *pvParameters
// 返回值：无
void adc_task(void *pvParameters)
{
	while(1)
	{
        //定义一个发送字符串的缓冲区
        char send_buf[64];

        // 多次采集并取平均值，进行滤波，核心解决单次采集跳变问题，提升数据稳定性
        adc_data.adc_light = ADC_GetFilterValue(ADC_Channel_0, 15);            // 从 ADC 获取光照采集值
        adc_data.adc_temperature = ADC_GetFilterValue(ADC_Channel_1, 10);      // 从 ADC 获取温度采集值
  
        // 转换为物理量
        float lux = Calculate_Light_Lux(adc_data.adc_light);
        float temperature = Calculate_Temperature(adc_data.adc_temperature);
        //将格式化的内容写入字符数组（缓冲区），而非直接输出。
        sprintf(send_buf, "TEMP=%.1f,LUX=%.1f,ALARM=%d,MOTOR=%d\r\n", temperature,lux,alarm_flag,motor_flag);       //将数据按照 TEMP=XX.X,LUX=XX.X\r\n 的固定格式拼接成字符串，然后通过串口 1 发送
        Usart1_SendString(send_buf);

        // 将采集的数据分别发送到显示和报警队列（非阻塞）
        xQueueSend(xQueue_AdcData_Display, &adc_data, 0);
        xQueueSend(xQueue_AdcData_Alarm, &adc_data, 0);
        xQueueSend(xQueue_AdcData_Usart, &adc_data, 0);
        vTaskDelay(pdMS_TO_TICKS(100));                               // 延时100ms，周期性采集
	}

 }

//============================ 报警任务函数 ==================================
/* 功能：1. 接收ADC数据，根据采集的光照与温度数据进行判断，若光照过强或温度过高则点亮LED与蜂鸣器报警;
       2. 接收红外传感器数据，若检测到有障碍物则触发LED与蜂鸣器报警。*/
// 参数：void *pvParameters
// 返回值：无
void alarm_task(void *pvParameters)
{
    ADCData_t local_data;  // 使用局部变量接收队列数据
    while(1)
    {
        if (xQueueReceive(xQueue_AdcData_Alarm, &local_data, portMAX_DELAY) == pdPASS)
        {
            // 计算物理量
            float lux = Calculate_Light_Lux(local_data.adc_light);
            float temperature = Calculate_Temperature(local_data.adc_temperature);
            int red_value = Redsensor(); // 0表示有障碍物

            // 清零标志位（每次重新计算）
            alarm_flag = 0;
            motor_flag = 0;

            // 判断报警类型
            if (lux > 200 || temperature > 35)
            {
							for(int count=0;count<=100;count++)
							{
                alarm_flag = 1; // 环境报警
                motor_flag = 1;
							}
            }
            if (red_value == 0)
            {
                if (alarm_flag == 1)      // 已有环境报警
                    alarm_flag = 3;        // 混合报警
                else
                    alarm_flag = 2;        // 仅障碍物报警
                // 障碍物报警时不启动电机（motor_flag 保持 0）
            }

            // 根据最终报警类型执行动作
            switch (alarm_flag)
            {
                case 0:
                    LED1_OFF();
                    Buzzer_OFF();
                    Motor_Stop();
                    break;
                case 1:
                    printf("环境因素触发报警\r\n");
                    LED1_ON();
                    Buzzer_ON();
                    Motor_Forward(50);
                    break;
                case 2:
                    printf("障碍物触发报警！\r\n");
                    LED1_ON();
                    Buzzer_ON();
                    Motor_Stop();  // 确保电机停止（避免环境报警遗留的影响）
                    break;
                case 3:
                    printf("环境因素与障碍物同时触发报警！\r\n");
                    LED1_ON();
                    Buzzer_ON();
                    Motor_Forward(50);
                    break;
                default:
                    break;
            }
        }
    }
}



//======================= 显示任务函数 ==========================
// 功能：接收ADC数据，并将光照强度和温度数据显示在OLED屏幕上
// 参数：void *pvParameters
// 返回值：无
void display_task(void *pvParameters)
{
	while(1)
	{
    // 从显示队列接收ADC数据，阻塞等待直到有数据可用
    if(xQueueReceive(xQueue_AdcData_Display, &adc_data, portMAX_DELAY) == pdPASS)
    {
        
        // 将光照强度和温度数据显示在OLED屏幕上
        float lux = Calculate_Light_Lux(adc_data.adc_light);                   // 将ADC光照值转换为实际光照强度
        float temperature = Calculate_Temperature(adc_data.adc_temperature);   // 将ADC温度值转换为实际温度值
        uint16_t lux_int = (uint16_t)lux;
        uint16_t lux_dec = (uint16_t)((lux - lux_int) * 10);
        uint8_t temp_int = (uint8_t)temperature;
        uint8_t temp_dec = (uint8_t)((temperature - temp_int) * 10);

            // OLED_Clear(); // 清屏并更新显示，会出现屏幕闪烁，故去掉清屏操作，直接覆盖显示内容
            OLED_ShowCN(1, 1, 0, 1); // 显示汉字"光"
            OLED_ShowCN(1, 2, 1, 1); // 显示汉字"照"
            OLED_ShowCN(1, 3, 2, 1); // 显示汉字"强"
            OLED_ShowCN(1, 4, 3, 1); // 显示汉字"度"
            OLED_ShowChar(1, 9, ':');
            OLED_ShowNum(2, 1, lux_int, 3);
            OLED_ShowChar(2, 4, '.');
            OLED_ShowNum(2, 5, lux_dec, 1);
            OLED_ShowString(2, 7, "Lux");
            OLED_ShowCN(3, 1, 4, 1); // 显示汉字"温"
            OLED_ShowCN(3, 2, 3, 1); // 显示汉字"度"
            OLED_ShowChar(3, 5, ':');
            OLED_ShowNum(4, 1, temp_int, 2);
            OLED_ShowChar(4, 3, '.');
            OLED_ShowNum(4, 4, temp_dec, 1);
            OLED_ShowCN(4, 4, 5, 1); // 显示汉字"摄"
            OLED_ShowCN(4, 5, 6, 1); // 显示汉字"氏"
            OLED_ShowCN(4, 6, 3, 1); // 显示汉字"度"
            // 释放OLED显示互斥量，允许其他任务访问OLED显示资源
            vTaskDelay(pdMS_TO_TICKS(100)); // 每100ms更新一次显示
    }
	}

}


//======================= 串口数据接收任务函数 ======================
// 功能：通过串口接收ESP32S3发送的指令
// 参数：void *pvParameters
// 返回值：无
void usart_task(void *pvParameters)
{
	      uint8_t rx_buf[64];     //用来接收一个字节数据；缓冲区是64 个 8 位无符号整数组成的缓冲区，最多存63个有效字符（留1位给结束符）
        uint16_t index = 0;    // 缓冲区写入索引，记录当前已接收字符数
        uint8_t ch;            // 临时存储单次接收的字节
	while(1)
	{

        // 从串口接收数据，阻塞等待直到有数据可用
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET)
        {
            ch = USART_ReceiveData(USART1); // 从USART1接收一个字节数据
            if (ch == '\n' && index > 0)   // 如果接收的字节是换行符，并且缓冲区有数据
            {
                // 收到换行，认为一帧结束
                rx_buf[index] = '\0';
                index = 0;

                // 解析命令
                if (strncmp((char*)rx_buf, "CMD:", 4) == 0)
                {
                    char *cmd = (char*)rx_buf + 4;  // 获取真正的命令部分，跳过"CMD:"
                    // 示例：CMD:ALARM,2
                    char command[16];               // 存储解析出的命令
                    int param;                 // 存储解析出的参数
                    if (sscanf(cmd, "%[^,],%d", command, &param) == 2)  // 解析命令和参数，返回2表示成功
                    {
                        // 根据命令执行操作
                        if (strcmp(command, "ALARM") == 0)
                        {
														switch(param)
														{
															case 0:
																				LED1_OFF();
																				Buzzer_OFF();
																		break;
															
															case 1:
																				LED1_ON();
																				Buzzer_ON();
																		break;
															case 2:
                                        LED1_ON();
																				Buzzer_ON();
																		break;
															case 3:
																				LED1_ON();
																				Buzzer_ON();
																		break;
															default:
																break;
																
															
														}
                        }
                        else if (strcmp(command, "MOTOR") == 0)
                        {
                            if (param==1)
                            {
                                Motor_Forward(50);
                            }
                            else if (param==0)
                            {
                                Motor_Stop();
                            }
                        }
                    }
                }
            }
            else if (ch == '\r')
            {
                // 忽略回车，等待换行
            }
            else if (index < sizeof(rx_buf) - 1)
            {
                rx_buf[index++] = ch;
            }
            else
            {
                // 溢出，丢弃
                index = 0;
            }
        }
        // 简单延时，避免任务空转
        vTaskDelay(pdMS_TO_TICKS(100));

	}
}
