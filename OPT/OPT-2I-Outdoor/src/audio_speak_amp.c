#include "audio_speak_amp.h"
#include "gpio_base.h"
#include <stdio.h>

#define AUDIO_SPEAK_GPIO_PIN 82
/***** 使能GPIO-高电平 *****/
#define AUDIO_SPEAK_ENABLE_LEVEL GPIO_LEVEL_HIGH
/***** 失能GPIO-低电平 *****/
#define AUDIO_SPEAK_DISABLE_LEVEL GPIO_LEVEL_LOW

static bool is_audio_speak_enable = false;


/***
**  date:2022/04/19
**  author:刘炼
**  初始化音频输出功能使能IO
***/
bool audio_speak_init(void)
{
    printf("%s ====================>>>\n\r",__func__);
     /***** 初始化开锁GPIO 为输出模式*****/
    gpio_direction_set(AUDIO_SPEAK_GPIO_PIN,GPIO_DIR_OUT);

    /***** 将其置为关闭开锁状态 *****/
    audio_speak_disable();
    return true;
}


/***
**  date:2022/04/19
**  author:刘炼
**  使能音频输出功能使能IO
***/
bool audio_speak_enable(void)
{
    // if(is_audio_speak_enable == true)
    // {
    //     return false;
    // }

    // printf("%s ====================>>>\n\r",__func__);
    gpio_level_set(AUDIO_SPEAK_GPIO_PIN,AUDIO_SPEAK_ENABLE_LEVEL);
    GPIO_LEVEL  value;
    if(gpio_level_read(AUDIO_SPEAK_GPIO_PIN,&value) == true)
    {
        // printf("gpio_level_read 82 ===================>>>%d\n\r",value);
    }
    is_audio_speak_enable = true;
    return true;
}


/***
**  date:2022/04/19
**  author:刘炼
**  失能音频输出功能使能IO
***/
bool audio_speak_disable(void)
{
    printf("%s ====================>>>\n\r",__func__);
    // if(is_audio_speak_enable == false)
    // {
    //     return false;
    // }

    gpio_level_set(AUDIO_SPEAK_GPIO_PIN,AUDIO_SPEAK_DISABLE_LEVEL);
    is_audio_speak_enable = false;
    return true;
}