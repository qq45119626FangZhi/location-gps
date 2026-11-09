#define DEBUG_ENABE

#include "App_Main.h"
#include "App_LowPower.h"
#include "App_Work.h"

/**
 * @brief 项目工程自定义程序入口
 *
 */
void App_Main(void)
{
    Com_Debug_println("App_Main");

    while (1) {
        // 退出低功耗
        App_LowPower_Exit();
        // 初始化工作模式
        App_Work_Init();
        // 执行任务
        App_Work_Execute();
        // 进入低功耗
        App_LowPower_Enter();

        HAL_Delay(10000);
    }
}
