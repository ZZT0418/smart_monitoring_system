/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/


/* ----------------- 基于 STM32F103C8T6 (Cortex-M3) 的推荐配置 -----------------
	本文件对 FreeRTOS 的宏进行了注释（中文），便于在 STM32F103 上开发时理解和调整。
	主要关注点：中断优先级位宽 (__NVIC_PRIO_BITS)、可用优先级数量、堆大小、
	常用同步对象支持（互斥量/计数信号量）、软件定时器、以及调试钩子。
*/

/* Cortex-M NVIC 优先级位宽：STM32F1 系列通常为 4（优先级 0..15）
	如果工程中 CMSIS/startup 已定义该宏，这里不会覆盖。 */
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS    4
#endif

/* 调度与时钟 */
#define configUSE_PREEMPTION                    1   /* 使用抢占式调度 */
#define configUSE_IDLE_HOOK                     0   /* 是否使用空闲钩子函数 */
#define configUSE_TICK_HOOK                     0   /* 是否使用滴答钩子函数 */
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 72000000 ) /* 主频 72MHz */
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) /* 系统节拍 1kHz（1ms） */

/* 优先级与栈/堆 */
/* 增加优先级数，默认设置为 8（优先级范围 0..7），便于后续扩展 */
#define configMAX_PRIORITIES                    ( 8 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 ) /* 最小栈，通常为 idle 任务 */
/*增加总堆大小，考虑到多个任务和软件定时器的内存需求，设置为 17KB（根据实际需求调整）*/
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 17 * 1024 ) )

#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/* 包含的 API 函数选择：按需开启 */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1

/* 常用同步机制/定时器支持：建议默认开启 */
#define configUSE_MUTEXES                       1   /* 支持互斥量（用于外设保护如 UART/OLED） */
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1   /* 支持计数信号量 */
#define configUSE_TIMERS                        1   /* 支持软件定时器 */

/* 软件定时器任务配置（当 configUSE_TIMERS = 1 时生效） */
#define configTIMER_TASK_PRIORITY               ( 2 )
#define configTIMER_QUEUE_LENGTH                5
#define configTIMER_TASK_STACK_DEPTH            ( 256 )

/* 内存分配与溢出检测：建议开启以便开发时定位问题，这里关闭，感觉没用到*/
#define configUSE_MALLOC_FAILED_HOOK            0   /* 1：malloc 失败回调 */
#define configCHECK_FOR_STACK_OVERFLOW          0   /* 2：启用堆栈溢出检测（模式 2） */

/* 断言：便于在异常条件下停机调试 */
#ifndef configASSERT
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }
#endif

/* 中断优先级配置（按位宽计算，避免硬编码）
	说明：
	- configLIBRARY_KERNEL_INTERRUPT_PRIORITY：库使用的最低优先级值（数值越大优先级越低）
	- configKERNEL_INTERRUPT_PRIORITY：内核使用的 NVIC 优先级（左移到位宽）
	- configMAX_SYSCALL_INTERRUPT_PRIORITY：能调用 FreeRTOS API 的最高中断优先级阈值（数值越小优先级越高）

	对于 STM32F1（__NVIC_PRIO_BITS = 4），下面通过左移将库优先级转换为 NVIC 中的值。
*/
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY    15
#define configKERNEL_INTERRUPT_PRIORITY            ( ( configLIBRARY_KERNEL_INTERRUPT_PRIORITY ) << (8 - __NVIC_PRIO_BITS) )

/* 设定一个合理的可调用 RTOS API 的中断优先级阈值（这里示例使用库优先级 5）
	如果你的某些中断（如按键/ADC）会调用 FromISR API，确保它们的库优先级数值 >= 5。
	如需更严格实时性，可把该值调小（注意与调用 API 的中断配合）。 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY       ( ( 5 ) << (8 - __NVIC_PRIO_BITS) )

/* 映射移植层的中断处理函数名（保持原有映射） */
#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler
#define xPortSysTickHandler SysTick_Handler



#endif /* FREERTOS_CONFIG_H */

