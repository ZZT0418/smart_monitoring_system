#ifndef __FREERTOS_DEMO_H
#define __FREERTOS_DEMO_H

void FreeRTOS_Start(void);
void start_task(void *pvParameters);
void adc_task(void *pvParameters);
void alarm_task(void *pvParameters);
void display_task(void *pvParameters);
void usart_task(void *pvParameters);


#endif
