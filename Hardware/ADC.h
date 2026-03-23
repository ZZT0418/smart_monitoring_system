#ifndef __ADC_H
#define __ADC_H

void My_ADC_Init(void);
uint16_t ADC_GetValue(uint8_t ADC_Channel);
uint16_t ADC_GetFilterValue(uint8_t ADC_Channel, uint8_t sample_num);
float Calculate_Temperature(uint16_t temp_adc);
float Calculate_Light_Lux(uint16_t light_adc);

#endif
