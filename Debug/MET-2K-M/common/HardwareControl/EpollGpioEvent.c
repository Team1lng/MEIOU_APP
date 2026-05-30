/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-23 09:50:45
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-20 13:49:18
 * @FilePath: /82225-EPC/common/HardwareControl/EpollGpioEvent.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "EpollGpioEvent.h"
#include <sys/types.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/prctl.h>
static int PollFd = -1;

#define DefineFunc(Epoll) [Epoll].Fd = -1,            \
                          [Epoll].TriggerLevel = 0,   \
                          [Epoll].ChatterTimeMs = 10, \
                          [Epoll].EpollEventInit = Epoll##EpollEventInit,

EPOLL_EVENT_LIST(EpollOperater)

static EpollEvent EpollEventList[Epoll_Event_Max] = {EPOLL_EVENT_LIST(DefineFunc)};

static int AddFdToEpoll(int addFd)
{
    assert(addFd != -1);
    struct epoll_event event;
    event.events = EPOLLET;
    event.data.fd = addFd;
    return epoll_ctl(PollFd, EPOLL_CTL_ADD, addFd, &event);
}

static int EpollEventRegister(void)
{
    EpollEvent *Event = NULL;
    for (int EventNum = sizeof(EpollEventList) / sizeof(EpollEvent) - 1; EventNum >= 0; EventNum--)
    {
        Event = &EpollEventList[EventNum];
        if (Event->EpollEventInit(Event) > -1)
            AddFdToEpoll(Event->Fd);
    }

    return 0;
}

static void *EpollThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int ret = -1;
    char buff[10];
    int value = -1;
    struct epoll_event events;
    while (1)
    {
        ret = epoll_wait(PollFd, &events, Epoll_Event_Max, 500);
        if (ret < 0)
        {
            // printf("epoll_wait %s err %s\n", __func__, strerror(errno));
        }
        else if (ret == 0)
        {
        }
        else
        {
            for (int EventNum = sizeof(EpollEventList) / sizeof(EpollEvent) - 1; EventNum >= 0; EventNum--)
            {
                if (events.data.fd == EpollEventList[EventNum].Fd)
                {
                    lseek(events.data.fd, 0, SEEK_SET);
                    memset(buff, 0, sizeof(buff));
                    usleep(EpollEventList[EventNum].ChatterTimeMs * 1000); /* 消抖 */
                    ret = read(events.data.fd, buff, sizeof(buff));
                    if (ret <= 0)
                    {
                        printf("read error, %d, %s\n", errno, strerror(errno));
                        continue;
                    }
                    value = atoi(buff);

                    if (EpollEventList[EventNum].TriggerLevel == value || EpollEventList[EventNum].TriggerLevel == -1)
                    {
                        if (EpollEventList[EventNum].EpollEventHandle)
                            EpollEventList[EventNum].EpollEventHandle(value);
                    }
                    break;
                }
            }
        }
    }
    return NULL;
}

/**
 * @description: 引脚事件通知初始化
 * @return {*}
 */
int EpollGpioEventInit(void)
{
    if (Epoll_Event_Max == 0)
    {
        return 0;
    }
    assert((PollFd = epoll_create(Epoll_Event_Max)) > 0);
    EpollEventRegister();

    pthread_t Thread;
    pthread_create(&Thread, NULL, EpollThread, NULL);
    pthread_detach(Thread);
    return 0;
}