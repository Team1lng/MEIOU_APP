/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-22 20:56:52
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-03 11:01:22
 * @FilePath: /82225-EPC/src/AdcDetectEvent.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "NetworkCommon.h"
#include "UserNetManage.h"
#include "NetMsgComm.h"
#include "VoiceRingPlay.h"
#include "LightControl.h"
#include "GpioControl.h"
#include "AdcControl.h"
#include "UserConfig.h"
#include "Unlock.h"
#include "Timer.h"
#include <math.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define KeyInterval 100
#define DEFINE_ADC_TAG(_TAG, ADC, FUNC) _TAG,
#define DEFINE_ADC_VOLTAGE(_TAG, ADC, FUNC) {#_TAG, ADC, FUNC},

#define TAG_DISABLE(format, ...)
#define ADC_TAG_LIST(TAG)           \
    TAG(Call1, 1990, CallKeyHandle) \
    TAG(Call2, 55, CallKeyHandle)    \
    TAG(Call3, 725, CallKeyHandle)  \
    TAG(Call4, 2490, CallKeyHandle)  \
    TAG(Exit1, 970, ExitBtnHandle)  \
    TAG(Exit2, 1290, ExitBtnHandle)
//Call1 1715
//Call2 0
//Call3 560
//Call4 2220
typedef enum
{
    ADC_TAG_LIST(DEFINE_ADC_TAG)
        AdcTagMax
} AdcTagList;

typedef struct
{
    char *Str;
    int Voltage;
    void (*Func)(void *Arg);
} AdcVoltage;

/**
 * @description: 呼叫按键灯光控制
 * @param {int} Floor 户型
 * @return {*}
 */
void CallKeyLightCtrl(int Floor)
{
    KeyLightControl(abs((Floor)-Call4));
}

static void CallBusyHandle(void *u)
{
    VoiceRingPlay(CallBusy, VoiceDefVol);
    KeyLightControl(-1);
}

static void CallKeyHandle(void *Arg)
{
    if (IsNetManageEntryState())
        return;

#define OPEN1_SW_GPIO 25   //41
    static GPIO_LEVEL level = GPIO_LEVEL_UNKNOWN;
    if (level == GPIO_LEVEL_UNKNOWN)
    {
        GpioOpen(OPEN1_SW_GPIO, GPIO_DIR_IN, false);
    }

    if (!GpioLevelGet(OPEN1_SW_GPIO, &level))
    {
        level = GPIO_LEVEL_LOW;
    }

    int Key = *(int *)Arg;
    NetworkMsgData Data;
    Data.Device = DEVICE_ALL;
    Data.Cmd = DoorbellEvent;
    Data.Arg1 = 0;
    Data.Arg2 = Key + 1 - level;
    NetworkMsgSend(Data);

    KeyLightControl(abs((Key)-Call4));
    VoiceRingPlay(Bi1, VoiceDefVol);

    if (!TimerEnablestatus(CallBusyTimer) && !TimerEnablestatus(MonitorTimer) && !TimerEnablestatus(CommunicateTimer))
    {
        SetTimer(3000, CallBusyTimer, CallBusyHandle, NULL);
    }

    printf("[%s][%d]Key:%d,Offset:%d       valun %d\n", __func__, __LINE__, Key, level, abs((Key)-Call4));
}

static void ExitBtnHandle(void *Arg)
{
    int Exit = *(int *)Arg;
    printf("[%s][%d]Exit:%d\n", __func__, __LINE__, Exit);
    Unlock(Exit == Exit1 ? UserConfigGet()->UnlockTime : UserConfigGet()->UngateTime, Exit == Exit1 ? LOCK_TYPE : GATE_TYPE);
}

static int QsortOrder(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

static void DoorbellCallDetect(int Voltage)
{
#define VoltageDefault 3299
#define VoltageCacheMax 7

    static AdcVoltage AdcVoltageGroup[] = {ADC_TAG_LIST(DEFINE_ADC_VOLTAGE)};
    static int AdcVoltageCache[VoltageCacheMax];
    static int CacheIndex = 0;
    static int VaildIndex = -1;

    /* 值未达到有效变动范围 */
    if (Voltage > (VoltageDefault - KeyInterval)) /* 去掉误差 */
    {
        VaildIndex = -1;
        CacheIndex = 0;
        return;
    }

    /* 滤波 */
    if (CacheIndex >= VoltageCacheMax)
    {
#if 0
			/* 取中位数做最终值，若没有中位数，则取前一位数据 */
			int Index = (VoltageCacheMax % 2) ? (VoltageCacheMax >> 1) + 1 : (VoltageCacheMax >> 1);
			*Voltage = AdcVoltageCache[Index];
#else
        /* 升序排序 */
        qsort(AdcVoltageCache, sizeof(AdcVoltageCache) / sizeof(AdcVoltageCache[0]), sizeof(AdcVoltageCache[0]), QsortOrder);
        /* 去掉分别去掉两个最大最小值，取平均值做最终值 */
        int Num = 0, Sum = 0, i = 2;
        for (i = 2; i < VoltageCacheMax - 2; i++, Num++)
        {
            Sum += AdcVoltageCache[i];
            // printf("[%d]:%d\n", i, AdcVoltageCache[i]);
        }
        // printf("\n\n");
        Voltage = Sum / Num;
#endif
        CacheIndex = 0;
    }
    else
    {
        AdcVoltageCache[CacheIndex++] = Voltage;
        return;
    }

    /* 事件驱动 */
    for (int i = 0; i < sizeof(AdcVoltageGroup) / sizeof(AdcVoltage); i++)
    {
        if (abs(Voltage - AdcVoltageGroup[i].Voltage) < KeyInterval)
        {
            if (VaildIndex != i)
            {
                VaildIndex = i;
                if (AdcVoltageGroup[i].Func)
                {
                    AdcVoltageGroup[i].Func(&i);
                }
            }
            return;
        }
    }
    VaildIndex = -1;
}

int AdcDetectHandle(int Voltage)
{
    DoorbellCallDetect(Voltage);
    return 0;
}