#include "stm32f10x.h"

// 添加一个函数来检查定时器是否启用
static uint8_t Is_TIM3_Enabled(void)
{
    // 检查TIM3的CR1寄存器中的CEN位（第0位）
    // 如果CEN=1，表示定时器已启用
    return (TIM3->CR1 & TIM_CR1_CEN) ? 1 : 0;
}

void PWM_Init(void)
{
    //1.使能GPIOA的时钟并初始化PA6引脚为复用推挽输出，用于定时器3的通道1输出PWM信号控制电机转速
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 使能GPIOA的时钟
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; // 选择PA6引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 输出速度50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 初始化GPIOA的PA6引脚

    //2.使能定时器3的时钟并初始化
    //TIM3 的基础时钟 = 72MHz（APB1 总线时钟 ×2）
    //PWM 频率 = 72MHz / (PSC+1) / (ARR+1) = 72MHz / 7200 / 100 = 100Hz= 10ms周期
    //占空比 = CCR / (ARR+1) = CCR / 100，范围0-100%，对应CCR1捕获/比较寄存器的值0-100
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); // 使能定时器3的时钟
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;//表示定时器时钟不分频
    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;//表示定时器向上计数模式
    TIM_TimeBaseStructure.TIM_Period=99;// ARR,设置自动重装载寄存器周期为99，即计数到100时产生一个更新事件
    TIM_TimeBaseStructure.TIM_Prescaler=7199;  //PSC 设置预分频器为7199，即定时器时钟频率为72MHz/7200=10kHz，每计数一次为0.1ms 
    TIM_TimeBaseStructure.TIM_RepetitionCounter=0;// 设置重复计数器为0，通用定时器不使用重复计数器
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); // 初始化定时器3的时间基准
    //3.配置定时器3的输出比较模式为PWM模式1，初始占空比为0，并使能输出
    TIM_OCInitTypeDef TIM_OCInitStructure;// 定义定时器输出比较结构体
    TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1; // 设置为PWM模式1
    TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High; // 输出极性为高电平有效
    TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStructure.TIM_Pulse=0; // 初始占空比为0
    TIM_OC1Init(TIM3, &TIM_OCInitStructure); // 初始化定时器3的通道1
    //使能预装载（保证参数修改生效）
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable); // 使能CCR1预装载（修改CCR1后下次计数生效）
    TIM_ARRPreloadConfig(TIM3, ENABLE);               // 使能ARR预装载（修改ARR后下次计数生效）
    TIM_Cmd(TIM3, ENABLE); // 使能定时器3

}

void Control_Init(void)
{
    // 初始化PWM信号生成
    PWM_Init(); 
    // 使能GPIOB的时钟并初始化PB13和PB14引脚为推挽输出，用于控制电机状态
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); 
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14; // PB13->AIN1和PB14->AIN2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 输出速度50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure); // 初始化GPIOB的PB13和PB14引脚

    // 设置PB13和PB14引脚初始状态为低电平，电机停止
    GPIO_ResetBits(GPIOB, GPIO_Pin_13|GPIO_Pin_14);

}

//===================电机反转函数===================//
//功能：根据传入的速度值启动电机(反转)
//参数：speed - 电机速度值，范围0-100，表示占空比百分比
//返回值：无
void Motor_Reverse(uint8_t speed)
{
    if(speed > 100) speed = 100; // 限制速度值在0-100范围内
    uint16_t pulse = (speed * 100) / 100; // 将速度值转换为占空比对应的脉冲值，定时器周期为99，所以占空比为pulse/100
    TIM_SetCompare1(TIM3, pulse); // 设置定时器3的通道1的比较寄存器值，更新占空比
    // 如果定时器已关闭，重新启用
    if (!Is_TIM3_Enabled()) {
        TIM_Cmd(TIM3, ENABLE);
    }
    // 设置PB13和PB14引脚状态，根据需要控制电机正转或反转，这里假设正转为PB13高电平，PB14低电平
    GPIO_ResetBits(GPIOB, GPIO_Pin_13); // PB13低电平
    GPIO_SetBits(GPIOB, GPIO_Pin_14); // PB14高电平
}

//===================电机停止函数===================//
void Motor_Stop(void)
{
    TIM_SetCompare1(TIM3, 0); // 将占空比设置为0，停止PWM输出
    // 设置PB13和PB14引脚状态，停止电机，这里假设停止为PB13和PB14都为低电平
    GPIO_ResetBits(GPIOB, GPIO_Pin_13); // PB13低电平
    GPIO_ResetBits(GPIOB, GPIO_Pin_14); // PB14低电平

    //新增：低功耗，电机停止时关闭定时器
    TIM_Cmd(TIM3, DISABLE);
}

//===================电机正转函数===================//
//功能：根据传入的速度值启动电机(正转)
//参数：speed - 电机速度值，范围0-100，表示占空比
//返回值：无
void Motor_Forward(uint8_t speed)
{
    if(speed > 100) speed = 100; // 限制速度值在0-100范围内
    uint16_t pulse = (speed * 100) / 100; // 将速度值转换为占空比对应的脉冲值，定时器周期为99，所以占空比为pulse/100
    // 如果定时器已关闭，重新启用
    if (!Is_TIM3_Enabled()) {
        TIM_Cmd(TIM3, ENABLE);
    }
    TIM_SetCompare1(TIM3, pulse); // 设置定时器3的通道1的比较寄存器值，更新占空比
    // 设置PB13和PB14引脚状态，正转电机，这里假设正转为PB13高电平，PB14低电平
    GPIO_SetBits(GPIOB, GPIO_Pin_13); // PB13高电平
    GPIO_ResetBits(GPIOB, GPIO_Pin_14); // PB14低电平
}
