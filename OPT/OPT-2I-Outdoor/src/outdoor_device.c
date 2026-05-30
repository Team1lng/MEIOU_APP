#include <stdio.h>
#include "gpio_base.h"

#define OUTDOOR_READ_GPIO_PIN 80

/***** 开机设备读取后的初始值 *****/
static int outdoor_device_value = -1;

/***
** date: 2022/04/18
** author：刘炼
** 读取户外机设备
** 没插入跳线帽读取为高电平，插入后为电电平
** 默认处理：0:outdoor1 1:outdoor2
** 返回-1:读取错误
***/
int outdoor_device_read(void)
{
    /***** 设置GPIO的IO方向 *****/
    gpio_direction_set(OUTDOOR_READ_GPIO_PIN,GPIO_DIR_IN);

    /***** 读取当前IO的电平 ,防止读取错误，连续读取三次*****/
    GPIO_LEVEL level1,level2;
    if(gpio_level_read(OUTDOOR_READ_GPIO_PIN, &level1) == false)
    {
        printf("read gpio%d failed \n",OUTDOOR_READ_GPIO_PIN);
        return -1;
    }

    for(int i = 0 ;i < 3; i++)
    {
        if(gpio_level_read(OUTDOOR_READ_GPIO_PIN, &level2) == false)
        {
            printf("read gpio%d failed \n",OUTDOOR_READ_GPIO_PIN);
            return -1;
        }

        if(level2 != level1)
        {
            level1 = level2;
            i = 0;
        }
    }
    outdoor_device_value = (level2 == GPIO_LEVEL_HIGH)?0:1;
    return outdoor_device_value;
}



/***
** date: 2022/04/18
** author：刘炼
** 读取户外机设备
** 返回true：设备发生改变
***/
bool outdoor_device_change(void)
{
    GPIO_LEVEL level;
    if(gpio_level_read(OUTDOOR_READ_GPIO_PIN, &level) == false)
    {
        printf("read gpio%d failed \n",OUTDOOR_READ_GPIO_PIN);
        return false;
    }

    int value  = (level == GPIO_LEVEL_HIGH)?0:1;
    return (value != outdoor_device_value)?true:false;
}