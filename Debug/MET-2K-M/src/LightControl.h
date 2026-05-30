/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-06-06 09:43:29
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-10 09:04:12
 * @FilePath: /Non_visual_indoor/src/LightControl.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H
/**
 * @description: 刷卡灯光闪烁
 * @return {*}
 */
void CardLightFlashes(void);

/**
 * @description: 开锁灯光控制
 * @param {int} en 使能标志位
 * @return {*}
 */
int LockLightControl(int en);

/**
 * @description: 红外灯光控制
 * @param {int} en 使能标志位
 * @return {*}
 */
int InfraredLightControl(int en);

/**
 * @description: 通话灯光控制
 * @param {int} en 使能标志位
 * @return {*}
 */
int TalkLightControl(int en);

/**
 * @description: 按键灯光控制
 * @param {int} index 按键索引 [-1 - 全关] [0~3 - 开启]
 * @return {*}
 */
void KeyLightControl(int index);

/**
 * @description: 灯光引脚初始化
 * @return {*}
 */
void LightGpioInit(void);
#endif