#include "stm32f10x.h"                  // Device header
#include "Led.h"
#include "Buzzer.h"
#include "ADC.h"
#include "OLED.h"
#include "Usart.h"
#include "Control.h"
#include "FreeRTOS_demo.h"

int main(void)
{
	LED_Init();
	Buzzer_Init();
	My_ADC_Init();
	OLED_Init();
	Usart1_Init(115200);
	Control_Init();
	FreeRTOS_Start();
	
	
}
