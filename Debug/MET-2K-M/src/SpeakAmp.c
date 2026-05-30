/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-20 08:53:40
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-10-23 14:13:32
 * @FilePath: /project_3/src/SpeakAmp.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "SpeakAmp.h"
#include <stdio.h>
#define APEAK_AMP_GPIO 32
static int AmpStatus = -1;
/**
 * @description: 初始化功放控制脚
 * @return {*}
 */
void SpeakAmpGpioInit(void)
{
    if (GpioOpen(APEAK_AMP_GPIO, GPIO_DIR_LOW, true) == false)
    {
        return;
    }
    AmpStatus = 0;
}

/**
 * @description: 获取功放控制脚状态
 * @return {*}
 */
GPIO_LEVEL SpeakAmpStatus(void)
{
    GPIO_LEVEL Level;
    if (GpioLevelGet(APEAK_AMP_GPIO, &Level) == false)
    {
        return GPIO_LEVEL_UNKNOWN;
    }
    AmpStatus = Level;
    return Level;
}

/**
 * @description: 使能功放
 * @return {*}
 */
void SpeakAmpEnable(void)
{
    if (AmpStatus)
        return;
    AmpStatus = !AmpStatus;
    // printf("[%s]!!!\n", __func__);
    GpioLevelSet(APEAK_AMP_GPIO, GPIO_LEVEL_HIGH);
}

/**
 * @description: 失能功放
 * @return {*}
 */
void SpeakAmpDisable(void)
{
    if (!AmpStatus)
        return;
    AmpStatus = !AmpStatus;
    // printf("[%s]!!!\n", __func__);
    GpioLevelSet(APEAK_AMP_GPIO, GPIO_LEVEL_LOW);
}
