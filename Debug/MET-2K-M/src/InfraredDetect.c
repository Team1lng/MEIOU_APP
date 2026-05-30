/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-23 20:37:33
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-12 09:00:34
 * @FilePath: /82225-EPC/src/InfraredDetect.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "GeneralInterface.h"
#include "EpollGpioEvent.h"
#include "LightControl.h"
#include "GpioControl.h"
#include "VideoInput.h"
#include "Timer.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define IR_FEED_GPIO 70//65
#define IRCUT_INA_GPIO 65//66
#define IRCUT_INB_GPIO 66//67
#define IR_LIGHT_GPIO 58//81

/**
 * @description: 获取是否处于夜间模式
 * @return {*}
 */
int DarkModeStatus(void)
{
    GPIO_LEVEL level;
    GpioLevelGet(IR_FEED_GPIO, &level);
    return level;
}

static void IrCutColse(void *u)
{
    GpioLevelSet(IRCUT_INA_GPIO, GPIO_LEVEL_LOW);
    GpioLevelSet(IRCUT_INB_GPIO, GPIO_LEVEL_LOW);
}

static void InfraredDebounce(void *u)
{
    GPIO_LEVEL GpioLevel;
    static GPIO_LEVEL StableLevel = GPIO_LEVEL_UNKNOWN;
    GpioLevelGet(IR_FEED_GPIO, &GpioLevel);
    GpioEdge(IR_FEED_GPIO, GpioLevel == GPIO_LEVEL_LOW ? RISING_EDGE : FALLING_EDGE);

    if (StableLevel != GpioLevel)
    {
        StableLevel = GpioLevel;
        VideoSwitchMode(StableLevel);
        InfraredLightControl(StableLevel);
        /* 夜视 */
        if (StableLevel)
        {
            printf("====================================================>>>>>夜视!!\n\n\n");
            GpioLevelSet(IRCUT_INA_GPIO, GPIO_LEVEL_LOW);
            GpioLevelSet(IRCUT_INB_GPIO, GPIO_LEVEL_HIGH);
        }
        /* 白天 */
        else
        {
            printf("====================================================>>>>>白天!!\n\n\n");
            GpioLevelSet(IRCUT_INA_GPIO, GPIO_LEVEL_HIGH);
            GpioLevelSet(IRCUT_INB_GPIO, GPIO_LEVEL_LOW);
        }

        SetTimer(100, IrCurCloseTimer, IrCutColse, NULL);
    }
    
}

static int InfraredDetectHandle(int Level)
{
    if(TimerEnablestatus(IrFeedTimer))
    {
        RefreshTimer(1000,IrFeedTimer);
    }
    else
    {
        SetTimer(1000, IrFeedTimer, InfraredDebounce, NULL);
    }
    return 0;
}

/**
 * @description: 夜视检测初始化
 * @return {*}
 */
static int InfraredDetectInit(void)
{
    if (GpioOpen(IRCUT_INA_GPIO, GPIO_DIR_LOW, false) == false)
    {
        return -1;
    }

    if (GpioOpen(IRCUT_INB_GPIO, GPIO_DIR_LOW, false) == false)
    {
        return -1;
    }

    return 0;
}

int InfraredDetectEpollEventInit(struct EpollEvent *Event)
{
    InfraredDetectInit();

    GPIO_LEVEL Level;
    if (GpioOpen(IR_FEED_GPIO, GPIO_DIR_IN, true) == false)
    {
        return -1;
    }

    GpioLevelGet(IR_FEED_GPIO, &Level);
    GpioEdge(IR_FEED_GPIO, Level == GPIO_LEVEL_LOW ? RISING_EDGE : FALLING_EDGE);
    char Path[64] = {0};
    memset(Path, 0, sizeof(Path));
    sprintf(Path, "/sys/class/gpio/gpio%d/value", IR_FEED_GPIO);
    Event->Fd = open(Path, O_RDONLY);
    Event->TriggerLevel = -1;
    Event->EpollEventHandle = InfraredDetectHandle;
    InfraredDetectHandle(DarkModeStatus());
    return 0;
}
