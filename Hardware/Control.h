#ifndef __CONTROL_H
#define __CONTROL_H

void PWM_Init(void);
void Control_Init(void);
void Motor_Forward(uint8_t speed);
void Motor_Stop(void);
void Motor_Reverse(uint8_t speed);



#endif
