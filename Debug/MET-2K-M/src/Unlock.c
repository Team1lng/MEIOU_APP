/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-09-14 16:17:55
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-10-23 14:11:29
 * @FilePath: /2CD-ME2K-M/src/Unlock.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "PeripheralControl.h"
#include "GeneralInterface.h"
#include "EpollGpioEvent.h"
#include "VoiceRingPlay.h"
#include "LightControl.h"
#include "GpioControl.h"
#include "UserConfig.h"
#include "Unlock.h"
#include "Timer.h"
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define LOCK_GPIO 68//7
#define GATE_GPIO 31//8
static char LockGpio[] = {0, LOCK_GPIO, GATE_GPIO};
static void GateColse(void *u)
{
    printf("[%s]\n", __func__);
    GpioLevelSet(GATE_GPIO, GPIO_LEVEL_HIGH);
    if (!TimerEnablestatus(LockTimer))
    {
        LockLightControl(0);
    }
}

static void LockColse(void *u)
{
    printf("[%s]\n", __func__);
    GpioLevelSet(LOCK_GPIO, GPIO_LEVEL_HIGH);
    if (!TimerEnablestatus(GateTimer))
    {
        LockLightControl(0);
    }
}

/**
 * @description: 开锁
 * @param {int} time    开锁时间
 * @param {LockType} type   开锁类型
 * @param {int} VoiceEn   开锁类型
 * @return {*}0-失败，1-成功
 */
int UnlockL(int time, LockType type, int VoiceEn)
{
    assert(type <= GATE_TYPE);
    if (SetTimer(time * 1000, type == LOCK_TYPE ? LockTimer : GateTimer, type == LOCK_TYPE ? LockColse : GateColse, NULL))
    {
        printf("[%s] %ds\n", type == LOCK_TYPE ? "Unlock" : "Ungate", time);

        if (!TimerEnablestatus(type == LOCK_TYPE ? GateTimer : LockTimer))
        {
            int Voice = VoiceEn ? UnlockEng + UserConfigGet()->Language : Bi1;
            VoiceRingPlay(Voice, VoiceDefVol);
        }

        LockLightControl(1);

        GpioLevelSet(LockGpio[type], GPIO_LEVEL_LOW);

        char Data[] = {0x7e, 0xa5, 0x97};
        WiegandDevWrite(Data, sizeof(Data));

        return 1;
    }
    return 0;
}

/**
 * @description: 锁引脚初始化
 * @return {*}
 */
void LockGpioInit(void)
{
    for (int i = LOCK_TYPE; i < sizeof(LockGpio); i++)
    {
        if (GpioOpen(LockGpio[i], GPIO_DIR_HIGH, false))
        {
        }
    }
}