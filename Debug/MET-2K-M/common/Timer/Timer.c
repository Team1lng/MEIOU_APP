#include "Timer.h"
#include "string.h"
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

static Timer TimerList[256] = {[0 ... 255] = {.Data = NULL,.Handle = NULL,.TimerEn = ATOMIC_VAR_INIT(0)}};
static void TimerHandler(int sig, siginfo_t *si, void *uc)
{
    Timer *timer = si->si_value.sival_ptr;
    // printf("TimerHandler:timer:%p\n", timer);
    if(atomic_exchange(&timer->TimerEn,0))
    {
        timer_delete(timer->Timerid);
        if (timer->Handle)
        {
            timer->Handle(timer->Data);
        }
    }
}

/**
 * @description: 设置定时器
 * @param {int} ms  时长
 * @param {int} signo   自定义信号类型
 * @param {TimeridHandle} handle    回调
 * @return {*}-1-失败，0-已创建，1-成功
 */
int SetTimer(int ms, int signo, TimeridHandle handle, void *data)
{
    if (signo > sizeof(TimerList) / sizeof(Timer) || ms == 0)
        return -1;
    if (atomic_exchange(&TimerList[signo].TimerEn,1))
        return 0;

    TimerList[signo].Handle = handle;
    TimerList[signo].Data = data;

    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = TimerHandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &TimerList[signo];
    // printf("SetTimer sival_ptr:%p,signo:%d\n", sev.sigev_value.sival_ptr, signo);
    timer_create(CLOCK_REALTIME, &sev, &TimerList[signo].Timerid);
    its.it_value.tv_sec = ms / 1000;
    its.it_value.tv_nsec = (ms % 1000) * 1000 * 1000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(TimerList[signo].Timerid, 0, &its, NULL);
    return 1;
}

/**
 * @description: 刷新定时器
 * @param {int} ms  时长
 * @param {int} signo   自定义信号类型
 * @param {TimeridHandle} handle    回调
 * @return {*}0-失败，1-成功
 */
int RefreshTimer(int ms, int signo)
{
    if (signo > sizeof(TimerList) / sizeof(Timer))
        return 0;
    if (!atomic_load(&TimerList[signo].TimerEn))
        return 0;
    struct itimerspec its;

    its.it_value.tv_sec = ms / 1000;
    its.it_value.tv_nsec = (ms % 1000) * 1000 * 1000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(TimerList[signo].Timerid, 0, &its, NULL);
    return 1;
}

/**
 * @description: 定时器使能状态
 * @param {int} signo   自定义定时器类型
 * @return {*}0-失败，1-成功
 */
int TimerEnablestatus(int signo)
{
    return (atomic_load(&TimerList[signo].TimerEn));
}

/**
 * @description: 定时器获取
 * @param {int} signo   自定义定时器类型
 * @return {*}NULL-失败，!NULL-成功
 */
Timer *TimerGet(int signo)
{
    return &TimerList[signo];
}

/**
 * @description: 定时器销毁
 * @param {int} signo 自定义定时器类型
 * @return {*}
 */
void TimerDestroy(int signo)
{
    if (!atomic_exchange(&TimerList[signo].TimerEn,0))
        return;
    TimerList[signo].Handle = NULL;
    timer_delete(TimerList[signo].Timerid);
}
