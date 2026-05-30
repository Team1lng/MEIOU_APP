#include "gpio_base.h"
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>

/***
**  开锁相关的有四个IO口,其中的两个用于输出开锁，另外的两个用于检测是否需要开锁
**  开锁1:DOOR_OPEN(GPIO33),开锁2:GATE_OPEN(GPIO32)
**  开锁电平为高电平，平时为低电平
**  检测开锁1:OPEN1_SW(GPIO41),检测开锁2:OPEN2_SW(GPIO31)
**  检测开锁电平为高电平，检测需要开锁时为低电平
**
**  备注：开锁来源有两个途径:1,通过室内机发送指令开锁，2，检测上述两个IO口开锁
****/
#define UNLOCK_ON_LEVEL GPIO_LEVEL_HIGH
#define UNLOCK_OFF_LEVEL GPIO_LEVEL_LOW

#define UNLOCK_DET_ON_LEVEL GPIO_LEVEL_LOW
#define UNLOCK_DET_OFF_LEVEL GPIO_LEVEL_HIGH


/***** 开锁IO控制输出 *****/
#define DOOR_UNLOCK_GPIO_PIN 33
#define GATE_UNLOCK_GPIO_PIN 32

/***** 检测开锁IO 输入检测 *****/
#define DOOR_UNLOCK_DET_GPIO_PIN 41
#define GATE_UNLOCK_DET_GPIO_PIN 31

/***** 开锁时间 *****/
#define UNLOCK_OPEN_TIME 5000
#define UNGATE_OPEN_TIME 5000
static bool door_unlock_enable = false;
static bool gate_unlock_enable = false;
static unsigned long long door_unlock_timesmp = 0;
static unsigned long long gate_unlock_timesmp = 0;

/***
** door 开锁处理
***/
static void device_door_unlock_open(void)
{
    /***** 读取当前时间戳，作为倒计时的起始时间 *****/
    struct timeval tv;
    gettimeofday(&tv,NULL);
    door_unlock_timesmp = tv.tv_sec*1000 + tv.tv_usec/1000;
    gpio_level_set(DOOR_UNLOCK_GPIO_PIN,UNLOCK_ON_LEVEL);

    door_unlock_enable = true;
}
static void device_door_unlock_close(void)
{
    gpio_level_set(DOOR_UNLOCK_GPIO_PIN,UNLOCK_OFF_LEVEL);
    door_unlock_enable = false;
}




/***
** gate 开锁处理
***/
static void device_gate_unlock_open(void)
{
    /***** 读取当前时间戳，作为倒计时的起始时间 *****/
    struct timeval tv;
    gettimeofday(&tv,NULL);
    gate_unlock_timesmp = tv.tv_sec*1000 + tv.tv_usec/1000;
    gpio_level_set(GATE_UNLOCK_GPIO_PIN,UNLOCK_ON_LEVEL);

    gate_unlock_enable = true;
}
static void device_gate_unlock_close(void)
{
    gpio_level_set(GATE_UNLOCK_GPIO_PIN,UNLOCK_OFF_LEVEL);
     gate_unlock_enable = false;
}



/***
** 上电初始化开锁相关的IO
** 将开锁的电平置为关闭状态
***/
bool device_unlock_init(void)
{
    /***** 初始化开锁GPIO 为输出模式*****/
    gpio_direction_set(DOOR_UNLOCK_GPIO_PIN,GPIO_DIR_OUT);
    gpio_direction_set(GATE_UNLOCK_GPIO_PIN,GPIO_DIR_OUT);

    /***** 将其置为关闭开锁状态 *****/
    device_door_unlock_close();
    device_gate_unlock_close();




    /***** 初始化检测开锁IO 为输入检测状态 *****/
    gpio_direction_set(DOOR_UNLOCK_DET_GPIO_PIN,GPIO_DIR_IN);
    gpio_direction_set(GATE_UNLOCK_DET_GPIO_PIN,GPIO_DIR_IN);


    /***** 读取其上电的初始电平 *****/
    GPIO_LEVEL level;
    gpio_level_read(DOOR_UNLOCK_DET_GPIO_PIN,&level);
    /***** 如果开机检测到需要开锁,将检测开锁初始值写入参数中 *****/
    if(level == UNLOCK_DET_ON_LEVEL)
    { 
       device_door_unlock_open();
    }


    gpio_level_read(GATE_UNLOCK_DET_GPIO_PIN,&level);
    if(level ==UNLOCK_DET_ON_LEVEL )
    {
        device_gate_unlock_open();
    }
    return true;
}




/***
**  开锁检测处理
**  开锁标志位开启时，启动计时，倒计时间到后自动将开锁置为处理
**  此函数用于检测IO开锁电平，并且计时开锁时间，并且时间到后关闭
***/

static void device_door_unlock_process(void)
{   
    /***** 已经开锁状态了，计算开锁耗时 ****/
    if(door_unlock_enable == true)
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        unsigned long long timesmp =  tv.tv_sec*1000 + tv.tv_usec/1000;
        if(abs(timesmp - door_unlock_timesmp) > UNLOCK_OPEN_TIME)
        {
            device_door_unlock_close();
        }
    }
    else
    {
        /***** 检测开锁IO电平 *****/
        GPIO_LEVEL level;
        gpio_level_read(DOOR_UNLOCK_DET_GPIO_PIN,&level);
        if(level == UNLOCK_DET_ON_LEVEL)
        { 
            device_door_unlock_open();
        }
        //printf("read gpio%d level:%d \n",DOOR_UNLOCK_DET_GPIO_PIN,level);
    }
}
static void device_gate_unlock_process(void)
{
    /***** 已经开锁状态了，计算开锁耗时 ****/
    if(gate_unlock_enable == true)
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        unsigned long long timesmp =  tv.tv_sec*1000 + tv.tv_usec/1000;
        if(abs(timesmp - gate_unlock_timesmp) > UNLOCK_OPEN_TIME)
        {
            device_gate_unlock_close();
        }
    }
    else
    {
        /***** 检测开锁IO电平 *****/
        GPIO_LEVEL level;
        gpio_level_read(GATE_UNLOCK_DET_GPIO_PIN,&level);
        if(level == UNLOCK_DET_ON_LEVEL)
        { 
            device_gate_unlock_open();
        }
        // printf("read gpio%d level:%d \n",GATE_UNLOCK_DET_GPIO_PIN,level);
    }
}



/***
** date:2022/04/18
** author:刘炼
** 开锁检测以及处理，包括开锁时间到自动关闭锁处理
***/
void device_unlock_process(void)
{
   device_door_unlock_process();
   device_gate_unlock_process();
}



/***
**  接收到室内机发送指令开锁
**  unlock_dev:1：door unlock,2:gate unlock
***/
bool device_unlock_cmd_process(int unlock_dev)
{
    if(unlock_dev == 1)
    {
        if(door_unlock_enable == true)
        {
            return false;
        }
        device_door_unlock_open();
    }
    else if(unlock_dev == 2)
    {
        if(gate_unlock_enable == true)
        {
            return false;
        }
        device_gate_unlock_open();
    }

    return true;
}