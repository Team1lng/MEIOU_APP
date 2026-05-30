#include <stdio.h>
#include <stdbool.h>
#include "gpio_base.h"
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>



/***
**  LED（GPIO81） 平时是关闭状态(低电平)，按下CALL机按钮后，将其置为打开状态（高电平）
**  在打开状态后开启定时器，开始倒计时（5s），时间到后置为关闭状态（低电平）
***/
#define LED_GPIO_PIN 81//未使用

#define LED_ON_LEVEL GPIO_LEVEL_HIGH
#define LED_OFF_LEVEL GPIO_LEVEL_LOW


/***** 开锁时间 *****/
#define DEVICE_LED_OPEN_TIME 5000

static bool device_led_enable = false;
static bool is_device_led_on = false;
static unsigned long long device_led_timesmp = 0;


/***
** LED 开启处理
***/
static void device_led_open(void)
{
    /***** 读取当前时间戳，作为倒计时的起始时间 *****/
    struct timeval tv;
    gettimeofday(&tv,NULL);
    device_led_timesmp = tv.tv_sec*1000 + tv.tv_usec/1000;
    gpio_level_set(LED_GPIO_PIN,LED_ON_LEVEL);

    is_device_led_on = true;
}
static void device_led_close(void)
{
    gpio_level_set(LED_GPIO_PIN,LED_OFF_LEVEL);
    is_device_led_on = false;
}




/***
** 初始化LED gpio工作模式
***/
bool device_led_init(void)
{
    /***** 初始化开锁GPIO 为输出模式*****/
    gpio_direction_set(LED_GPIO_PIN,GPIO_DIR_OUT);

    /***** 将其置为关闭开锁状态 *****/
    device_led_close();
    return true;
}



/***
**  LED检测处理
**  LED标志位开启时，启动计时，倒计时间到后自动将LED关闭
***/
void device_led_process(void)
{   
    if(device_led_enable == true)
    {
        if(is_device_led_on == false)
        {
            device_led_open();
        }
        else
        {
            struct timeval tv;
            gettimeofday(&tv,NULL);
            unsigned long long timesmp =  tv.tv_sec*1000 + tv.tv_usec/1000;
            if(abs(timesmp - device_led_timesmp) > DEVICE_LED_OPEN_TIME)
            {
                device_led_close();
                device_led_enable = false;
            }
        }
    }
}




/***
** 外部使能led状态。
***/
bool device_led_open_enable(void)
{
    if(device_led_enable == true)
    {
        return false;
    }

    device_led_enable = true;
    return true;
}
