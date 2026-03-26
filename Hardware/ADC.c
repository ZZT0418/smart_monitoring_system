#include "stm32f10x.h"
#include <math.h>                       // 必须引入，否则log()函数报错

void My_ADC_Init(void)
{
	//使能GPIO时钟、ADC时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	
	//初始化并配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;                 //模拟输入模式
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//初始化并配置ADC
	
	//1.两通道 扫描模式，连续转换
    ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_NbrOfChannel= 2;                //2个采集通道
	ADC_InitStructure.ADC_ScanConvMode= ENABLE;           //开启扫描模式
	ADC_InitStructure.ADC_ContinuousConvMode= ENABLE;     //开启连续转换
	ADC_InitStructure.ADC_DataAlign=ADC_DataAlign_Right;  //数据右对齐，12 位采集值存放在 16 位寄存器的低 12 位
	ADC_InitStructure.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;  //无外部触发：转换由软件触发（而非定时器 / 外部引脚触发）
	ADC_InitStructure.ADC_Mode=ADC_Mode_Independent;      //独立模式，不用与ADC2等同步，是常用的模式
	ADC_Init(ADC1, &ADC_InitStructure);
	//2.设置通道顺序和采样时间
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5); // PA0
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_55Cycles5); // PA1
    //3.使能ADC
	ADC_Cmd(ADC1,ENABLE);
	//校准：消除 ADC 内部电路的偏移误差，提升采集精度，每次上电初始化都必须执行
	ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/*==============ADC_GetValue函数==========================
 *功能：读取所用的ADC1通道数据（连续扫描模式下，需按顺序读取）
 *参数：uint8_t ADC_Channel
 *返回值：ADC_GetConversionValue();
 *========================================================
*/
uint16_t ADC_GetValue(uint8_t ADC_Channel)
{
    uint16_t adc_value = 0;
    
    // 1. 等待上一次转换完成（确保数据有效）
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    
    // 2. 清除转换完成标志，避免误判
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    
    // 3. 连续扫描模式下，按通道顺序读取数据
    if (ADC_Channel == ADC_Channel_0)
    {
        // 通道0（光照）是扫描的第1个通道，直接读取当前转换值
        adc_value = ADC_GetConversionValue(ADC1);
    }
    else if (ADC_Channel == ADC_Channel_1)
    {
        // 通道1（温度）是扫描的第2个通道，需等待通道0转换完成后再读
        // 额外等待一次转换（确保扫描到通道1）
        while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
        adc_value = ADC_GetConversionValue(ADC1);
    }
		return adc_value;
}


// 热敏电阻参数
#define R_FIX     10000.0f   // 分压固定电阻（10kΩ）
#define R0        10000.0f   // NTC 25℃时的电阻值（10kΩ）
#define B_VALUE   3950.0f    // NTC的B值（3950K）
#define T0        298.15f    // 25℃对应的开尔文温度
/**
 * @brief  将ADC采集值转换为温度（℃）
 * @param  temp_adc: ADC采集到的热敏电阻通道数值（0~4095）
 * @retval 换算后的温度值（℃）
 */
float Calculate_Temperature(uint16_t temp_adc)
{
    float V_adc, R_ntc, T_k, T_c;
    
    // 步骤1：ADC值转换为实际电压（0~3.3V）
    V_adc = (float)temp_adc * 3.3f / 4095.0f;
    
    // 步骤2：电压值转换为NTC热敏电阻阻值（防除0异常）
    if(V_adc >= 3.3f - 0.001f) { // 留微小余量，避免浮点精度问题导致除以0
        R_ntc = 0.0f;
    } else {
        R_ntc = R_FIX * V_adc / (3.3f - V_adc);
    }
    
    // 步骤3：电阻值转换为温度（开尔文→摄氏度，防异常值）
    if(R_ntc <= 0.0f || R_ntc > 1000000.0f) { // 限制电阻范围，避免对数出错
        T_c = 0.0f; // 异常时返回0℃，也可根据需求改为-99.9等标识值
    } else {
        // B值公式核心计算
        T_k = 1.0f / ( (1.0f/T0) + (1.0f/B_VALUE) * log(R_ntc/R0) );
        T_c = T_k - 273.15f; // 转换为摄氏度
    }
    
    return T_c;
}

#define R_FIX_LIGHT 10000.0f // 光照分压固定电阻（10kΩ）

/**
 * @brief  将光照ADC值转换为实际光照强度（lx，勒克斯）
 * @param  light_adc: ADC采集的光照通道数值（0~4095）
 * @retval 光照强度（lx）
 */
float Calculate_Light_Lux(uint16_t light_adc)
{
    float V_adc, R_ldr, lux;
    
    // 步骤1：ADC值转电压
    V_adc = (float)light_adc * 3.3f / 4095.0f;
    
    // 步骤2：电压转光敏电阻阻值（防除0）
    if(V_adc >= 3.3f - 0.001f) {
        R_ldr = 0.0f;
    } else {
        R_ldr = R_FIX_LIGHT * V_adc / (3.3f - V_adc);
    }
    
    // 步骤3：阻值转光照强度（替换为你的传感器公式！）
    // 示例公式（需根据厂家规格书调整，这里是通用拟合公式）：
    if(R_ldr <= 0.0f || R_ldr > 1000000.0f) {
        lux = 0.0f; // 异常值
    } else {
        // 拟合公式：lux = K * (1/R_ldr) （K为校准系数，需实测调整）
        float K = 1000000.0f; // 校准系数，比如实测1000lx时R_ldr=10kΩ，则K=1000*10000=10^7
        lux = K / R_ldr;
        
        // 可选：限制光照范围（比如0~20000lx）
        if(lux > 20000.0f) lux = 20000.0f;
        if(lux < 0.0f)      lux = 0.0f;
    }
    
    return lux;
}

// // ===================== 新增：ADC滤波函数（核心解决跳变） =====================
// /**
//  * @brief  读取滤波后的ADC值（平均值滤波，稳定光照/温度值）
//  * @param  ADC_Channel: 通道号（ADC_Channel_0=光照，ADC_Channel_1=温度）
//  * @param  sample_num: 采样次数（推荐10次，兼顾稳定和响应速度）
//  * @retval 滤波后的ADC平均值
//  */
// uint16_t ADC_GetFilterValue(uint8_t ADC_Channel, uint8_t sample_num)
// {
//     uint32_t sum = 0;
//     // 采集多次ADC值，累加求和
//     for(uint8_t i=0; i<sample_num; i++)
//     {
//         sum += ADC_GetValue(ADC_Channel);
//         // 短延时，避免采样过快导致数据重复
//         for(uint16_t j=0; j<200; j++);
//     }
//     // 返回平均值（核心：用多次平均抵消单次跳变）
//     return sum / sample_num;
// }

// ===================== 一阶滞后滤波（一阶指数滤波）ADC读取函数 =====================
/**
 * @brief  读取一阶滤波后的ADC值（稳定光照/温度，无数组、更平滑）
 * @param  ADC_Channel: 通道号（ADC_Channel_0=光照，ADC_Channel_1=温度）
 * @param  sample_num: 采样次数（多次采样求平均后再滤波，更稳定）
 * @retval 一阶滤波后的ADC值
 */
//一阶指数滤波 = 让新数据慢慢靠近旧数据，让曲线变顺滑，消除跳变。
uint16_t ADC_GetFilterValue(uint8_t ADC_Channel, uint8_t sample_num)
{
    static uint16_t last_adc[2] = {0};  // 保存上一次滤波值：[0]光照 [1]温度
    static uint8_t first_run[2] = {1, 1}; // 标记是否第一次运行
    uint32_t sum = 0;
    float a;                             // 滤波系数
    uint16_t new_val, filter_val;

    // 1. 多次采样 → 求原始平均值
    for(uint8_t i=0; i<sample_num; i++)
    {
        sum += ADC_GetValue(ADC_Channel);
        for(uint16_t j=0; j<200; j++);   // 简单延时
    }
    new_val = sum / sample_num;          // 原始稳定采样值

    // ===================== 第一次不滤波：静态变量初始为 0，导致第一次输出跳变巨大 =====================
    if(first_run[ADC_Channel] == 1)
    {
        first_run[ADC_Channel] = 0;
        last_adc[ADC_Channel] = new_val;
        return new_val;
    }

    // 2. 根据通道选择滤波系数（温度更平滑，光照稍灵敏）
    if(ADC_Channel == 0)    // 光照：系数稍大，响应快
        a = 0.1f;
    else                    // 温度：系数小，超平滑
        a = 0.05f;

    // 3. 一阶滞后滤波核心公式：新滤波值 = 旧值 × (1 - 系数) + 本次采集值 × 系数
    filter_val = (uint16_t)(last_adc[ADC_Channel] * (1 - a) + new_val * a);

    // 4. 更新上一次值
    last_adc[ADC_Channel] = filter_val;

    return filter_val;
}
















