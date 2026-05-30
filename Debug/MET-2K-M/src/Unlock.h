/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-23 11:39:04
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-28 11:29:15
 * @FilePath: /82225-EPC/src/Unlock.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _UNLOCK_H_
#define _UNLOCK_H_

typedef enum
{
    NONE_TYPE,
    LOCK_TYPE,
    GATE_TYPE,
} LockType;

/**
 * @description: 开锁
 * @param {int} time    开锁时间
 * @param {LockType} type   开锁类型
 * @return {*}0-失败，1-成功
 */
int UnlockL(int time, LockType type, int VoiceEn);
#define Unlock(time, type) UnlockL(time, type, 1)
/**
 * @description: 锁引脚初始化
 * @return {*}
 */
void LockGpioInit(void);
#endif