/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-06-06 15:06:15
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-10-23 14:13:09
 * @FilePath: /Doorbell/src/LightControl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "PeripheralControl.h"
#include "GpioControl.h"
#include "Timer.h"

static int CardLightControl(int en)
{
    return 0;
    static int EnState = -1;
    if (en != EnState)
    {
        EnState = en;
        char Data = en ? (_74hc595dDevStatus() | (1 << 4)) : (_74hc595dDevStatus() & ~(1 << 4));
        _74hc595dDevWrite(Data);
        return EnState;
    }
    return EnState;
}

static void CardLightFlashesHandle(void *us)
{
    return;
    static int Light = 1;
    if (TimerEnablestatus(AddCardTimer))
    {
        CardLightControl((Light = !Light));
        SetTimer(200, LightFlashesTimer, CardLightFlashesHandle, NULL);
    }
    else
    {
        CardLightControl(0);
    }
}

/**
 * @description: 刷卡灯光闪烁
 * @return {*}
 */
void CardLightFlashes(void)
{
    return;
    if (!TimerEnablestatus(LightFlashesTimer))
    {
        CardLightControl(1);
        SetTimer(200, LightFlashesTimer, CardLightFlashesHandle, NULL);
    }
}

/**
 * @description: 开锁灯光控制
 * @param {int} en 使能标志位
 * @return {*}
 */
int LockLightControl(int en)
{
    return 0; /* 硬件自启 */
    static int EnState = -1;
    if (en != EnState)
    {
        EnState = en;
        // GpioLevelSet(LOCK_LED, EnState);
        return EnState;
    }
    return EnState;
}

/**
 * @description: 红外灯光控制
 * @param {int} en 使能标志位
 * @return {*}
 */
int InfraredLightControl(int en)
{
    static int EnState = -1;
    if (en != EnState)
    {
        EnState = en;
        char Data = en ? (_74hc595dDevStatus() | (1 << 5)) : (_74hc595dDevStatus() & ~(1 << 5));
        _74hc595dDevWrite(Data);
        return EnState;
    }
    return EnState;
}

#define TALK_LED 67//6
/**
 * @description: 通话灯光控制
 * @param {int} en 使能标志位
 * @return {*}
 */
int TalkLightControl(int en)
{
    static int EnState = -1;
    if (en != EnState)
    {
        EnState = en;
        GpioLevelSet(TALK_LED, EnState);
        return EnState;
    }
    return EnState;
}

/**
 * @description: 按键灯光控制
 * @param {int} index 按键索引 [-1 - 全关] [0~3 - 开启]
 * @return {*}
 */
void KeyLightControl(int index)
{
    // printf("==1=========%02x    %d\n",_74hc595dDevStatus(), index);
    char Clear = 0xf0 & _74hc595dDevStatus();
    char Data = index < 0 ? Clear : (Clear | (1 << index));
    _74hc595dDevWrite(Data);
    // printf("==data=========%02x\n",Data);
    return;
}

/**
 * @description: 灯光引脚初始化
 * @return {*}
 */
void LightGpioInit(void)
{
    if (GpioOpen(TALK_LED, GPIO_DIR_LOW, false))
    {
    }
}