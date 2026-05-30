/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-20 08:53:44
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-01-23 11:38:12
 * @FilePath: /project_3/src/SpeakAmp.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _SPEAK_AMP_H_
#define _SPEAK_AMP_H_
#include "GpioControl.h"

/**
 * @description: 初始化功放控制脚
 * @return {*}
 */
void SpeakAmpGpioInit(void);

/**
 * @description: 获取功放控制脚状态
 * @return {*}
 */
GPIO_LEVEL SpeakAmpStatus(void);

/**
 * @description: 使能功放
 * @return {*}
 */
void SpeakAmpEnable(void);

/**
 * @description: 失能功放
 * @return {*}
 */
void SpeakAmpDisable(void);
#endif