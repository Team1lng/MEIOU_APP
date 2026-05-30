/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-23 09:50:54
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-20 13:49:36
 * @FilePath: /Doorbell/common/HardwareControl/EpollGpioEvent.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _EPOLL_GPIO_EVENT_H_
#define _EPOLL_GPIO_EVENT_H_

#define EpollOperater(Event)                                                        \
    __attribute__((weak)) const int Event##EpollEventInit(struct EpollEvent *Epoll) \
    {                                                                               \
        Epoll->Fd = -1;                                                             \
        Epoll->EpollEventHandle = NULL;                                             \
        printf("%s\n", __func__);                                                   \
        return -1;                                                                  \
    }

#define EPOLL_EVENT_LIST(EVENT) \
    EVENT(HouseSwitch)          \
    EVENT(InfraredDetect)       /*\
    EVENT(FingerDetect)*/

typedef struct EpollEvent
{
    int Fd;
    int TriggerLevel;    // 触发电平
    int ChatterTimeMs;   //消抖时间（毫秒）
    const int (*EpollEventInit)(struct EpollEvent *Epoll);
    int (*EpollEventHandle)(int Level);
} EpollEvent;

#define DEFINE_EVENT(Event) Event,
typedef enum
{
    EPOLL_EVENT_LIST(DEFINE_EVENT)
        Epoll_Event_Max
} EPOLL_GPIO_EVENT;

/**
 * @description: 引脚事件通知初始化
 * @return {*}
 */
int EpollGpioEventInit(void);
#endif