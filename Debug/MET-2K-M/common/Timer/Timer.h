/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-23 15:36:18
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-28 14:05:34
 * @FilePath: /82225-EPC/common/Timer/Timer.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _TIMER_H_
#define _TIMER_H_
#include <time.h>
#include <stdatomic.h>

enum
{
    LockTimer,
    GateTimer,
    AmpTimer,
    IrFeedTimer,
    IrCurCloseTimer,
    MonitorTimer,
    CommunicateTimer,
    MDTimer,
    KeypadTimer,
    AdminOutTimer,
    AddCardTimer,
    DelCardTimer,
    CodeCardUnlockTimer,
    ModifyCodeCardTimer,
    SecirityErrorTimer,
    SecirityTriggerTimer,
    LightFlashesTimer,
    CallBusyTimer,
    AddFingerTimer,
    FingerGetImageTimer,
    SVPTimer,
    FingerPowerTimer,
    FINGER_POWER_CONTROL_TIMER,
    TotalTimer,
} TimerSigno;

typedef void (*TimeridHandle)(void *sv);
typedef struct
{
    atomic_int TimerEn;
    timer_t Timerid;
    TimeridHandle Handle;
    void *Data;
} Timer;

/**
 * @description: 设置定时器
 * @param {int} ms  时长
 * @param {int} signo   自定义信号类型
 * @param {TimeridHandle} handle    回调
 * @return {*}0-失败，1-成功
 */
int SetTimer(int ms, int signo, TimeridHandle handle, void *data);

/**
 * @description: 刷新定时器
 * @param {int} ms  时长
 * @param {int} signo   自定义信号类型
 * @param {TimeridHandle} handle    回调
 * @return {*}0-失败，1-成功
 */
int RefreshTimer(int ms, int signo);

/**
 * @description: 定时器使能状态
 * @param {int} signo   自定义定时器类型
 * @return {*}0-失败，1-成功
 */
int TimerEnablestatus(int signo);

/**
 * @description: 定时器获取
 * @param {int} signo   自定义定时器类型
 * @return {*}NULL-失败，!NULL-成功
 */
Timer *TimerGet(int signo);

/**
 * @description: 定时器销毁
 * @param {int} signo 自定义定时器类型
 * @return {*}
 */
void TimerDestroy(int signo);

#endif