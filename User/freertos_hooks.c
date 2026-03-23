/* freertos_hooks.c
 * FreeRTOS 回调钩子实现（中文注释）
 * 适用于 STM32F103C8T6 工程，当在 FreeRTOSConfig.h 中启用
 * configUSE_MALLOC_FAILED_HOOK 和 configCHECK_FOR_STACK_OVERFLOW 时
 * 链接器会寻找以下实现：
 *   void vApplicationMallocFailedHook(void);
 *   void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
 *
 * 说明：
 * - 这些钩子函数在发生严重错误（内存分配失败或任务栈溢出）时被调用。
 * - 推荐在开发/调试阶段停在这里（无限循环）以方便使用调试器定位问题，
 *   或者拼接串口/LED 输出帮助定位；在发布固件时可改为重启或安全降级策略。
 * - 注意：钩子函数运行在异常上下文（可能中断已被禁用），因此尽量避免在钩子内调用非确定性/可阻塞的 API。
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

/* vApplicationMallocFailedHook
 * 当动态内存分配（pvPortMalloc）失败时被调用。
 * 推荐行为：立即禁用中断并停机（便于调试）；也可以记录信息后重启。
 */
void vApplicationMallocFailedHook(void)
{
    /* 禁用中断，防止系统继续运行造成不可预知的后果 */
    taskDISABLE_INTERRUPTS();

    /*
     * 在这里添加你希望的调试/重启逻辑：
     * - 使用串口打印错误信息（注意线程安全和重入问题）
     * - 点亮/闪烁错误 LED
     * - 将错误信息写入非易失存储
     * - 或者调用 NVIC_SystemReset() 进行复位
     * 下面选择停在此处，便于用调试器查看调用栈和内存。 
     */

    for( ;; )
    {
        /* 永久循环，等待调试器 */
    }
}


/* vApplicationStackOverflowHook
 * 当 FreeRTOS 检测到任务栈溢出（依据 configCHECK_FOR_STACK_OVERFLOW）时被调用。
 * 参数：
 *  - xTask：发生栈溢出的任务句柄（在某些移植层可能为 NULL）
 *  - pcTaskName：任务名（如果启用了任务名）
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    /* 禁用中断，停止系统以便调试 */
    taskDISABLE_INTERRUPTS();

    /*
     * 可在此处添加调试输出（谨慎使用，因为堆栈可能已损坏）或 LED 指示。
     * 推荐做法：在开发阶段停机，以调试器检查任务调用栈、栈使用情况并增大对应任务栈。
     */

    for( ;; )
    {
        /* 永久循环，等待调试器 */
    }
}
