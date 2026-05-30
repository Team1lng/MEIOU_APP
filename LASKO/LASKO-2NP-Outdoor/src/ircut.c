#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "ak_common.h"
#include "ak_vi.h"
#include "ak_mem.h"
#include "ak_log.h"
#include "ak_vpss.h"
#include "ak_drv.h"
#include "ak_thread.h"
#include "ak_ps_ir.h"
#include "ircut.h"
#include "gpio_base.h"



#if (IRCUT_MODE == 0)

extern bool is_network_video_send_package_open(void);
extern unsigned long long os_get_ms(void);

#define IRCUT_RUN_TIME 100 //驱动电机时长 ms
#define IRCUT_DELAY_TIME 4000 //控制后延时时长 ms

void ircut_gpio_init(void)
{
    gpio_direction_set(IR_FEED_PIN, GPIO_DIR_IN);

    gpio_direction_set(IRCUT_INA_PIN, GPIO_DIR_OUT);
    gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_LOW);
    
    gpio_direction_set(IRCUT_INB_PIN, GPIO_DIR_OUT);
    gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);

    gpio_direction_set(IR_LED_CTRL_PIN, GPIO_DIR_OUT);
    gpio_level_set(IR_LED_CTRL_PIN, GPIO_LEVEL_LOW);
}

static void ir_led_ctrl(bool en)
{
    if(access(IR_LED_PATH,F_OK)  == 0)
    {
        int fd = open(IR_LED_PATH, O_WRONLY);
        if(fd == -1)
        {
            printf("ir led dev open error:%s\n", IR_LED_PATH);
            return;
        }
        if((en ? write(fd, "255", 3) : write(fd, "0", 1)) == -1)
        {
            printf("ir led dev write error\n");
        }
        close(fd);
    }
    else
    {
        printf("%s================>>%d\n",__func__,en);
        gpio_level_set(IR_LED_CTRL_PIN, en ? GPIO_LEVEL_HIGH:GPIO_LEVEL_LOW);
    }
}
#if 0
void ircut_ctrl_handler(void)
{
    static unsigned long long pre_time;
    static GPIO_LEVEL pre_level = GPIO_LEVEL_HIGH;
    static bool ircut_is_adjusted = false;

    if(is_network_video_send_package_open() == false)
    {
        if(pre_level == GPIO_LEVEL_HIGH)
        {//恢复白天状态
            pre_level = GPIO_LEVEL_LOW;
            ircut_is_adjusted = false;
            ir_led_ctrl(false);
            ak_vi_switch_mode(VIDEO_DEV0, VI_MODE_DAY_OUTDOOR);
            gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_HIGH);
            gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
            ak_sleep_ms(IRCUT_RUN_TIME);
            gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_LOW);
            gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
        }
        return;
    }

            // printf("%s============================>>> \n",__func__);
    GPIO_LEVEL cur_level;
    gpio_level_read(IR_FEED_PIN, &cur_level);
    
    if(cur_level != pre_level && ircut_is_adjusted == false)
    {
        if(cur_level == GPIO_LEVEL_HIGH)
        {
            //正转进入夜视
            printf("============================>>>电平：高 -> 正转 夜视 \n");
            ak_vi_switch_mode(VIDEO_DEV0, VI_MODE_NIGHTTIME);
            gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_LOW);
            gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_HIGH);
            ir_led_ctrl(true);
            pre_level = GPIO_LEVEL_HIGH;
        }
        else 
        {
            //反转进入白天
            printf("============================>>>电平：低 -> 反转 白天 \n");
            ak_vi_switch_mode(VIDEO_DEV0, VI_MODE_DAY_OUTDOOR);
            gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_HIGH);
            gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
            ir_led_ctrl(false);
            pre_level = GPIO_LEVEL_LOW;
        }

        ak_sleep_ms(IRCUT_RUN_TIME);
        printf("============================>>>停止 \n");
        gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_LOW);
        gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
        pre_time = os_get_ms();
        ircut_is_adjusted = true;
    }

    // if(ircut_is_adjusted == true && (os_get_ms() - pre_time) >= IRCUT_DELAY_TIME) 
    //     ircut_is_adjusted = false;
}
#else
void ircut_ctrl_handler(void)
{
    // static unsigned long long pre_time;
    static GPIO_LEVEL pre_level = GPIO_LEVEL_HIGH;
    static bool ircut_is_adjusted = false;

    if(is_network_video_send_package_open() == false)
    {
        ircut_is_adjusted = false;
        if(pre_level == GPIO_LEVEL_HIGH)
        {//恢复白天状态
            printf("%s============================>>>无视频，转白天 \n",__func__);
            pre_level = GPIO_LEVEL_LOW;
            ir_led_ctrl(false);
            ak_vi_switch_mode(VIDEO_DEV0, VI_MODE_DAY_OUTDOOR);
            gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_HIGH);
            gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
            ak_sleep_ms(IRCUT_RUN_TIME);
            gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_LOW);
            gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
        }
        return;
    }


    if(ircut_is_adjusted == false)
    {
        GPIO_LEVEL cur_level;
        gpio_level_read(IR_FEED_PIN, &cur_level);
        if(cur_level != pre_level)
        {
            if(cur_level == GPIO_LEVEL_HIGH)
            {
                //正转进入夜视
                printf("%s============================>>>电平：高 -> 正转 夜视 \n",__func__);
                ak_vi_switch_mode(VIDEO_DEV0, VI_MODE_NIGHTTIME);
                gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_LOW);
                gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_HIGH);
                ir_led_ctrl(true);
                pre_level = GPIO_LEVEL_HIGH;
            }
            else 
            {
                //反转进入白天
                printf("%s============================>>>电平：低 -> 反转 白天 \n",__func__);
                ak_vi_switch_mode(VIDEO_DEV0, VI_MODE_DAY_OUTDOOR);
                gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_HIGH);
                gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
                ir_led_ctrl(false);
                pre_level = GPIO_LEVEL_LOW;
            }

            ak_sleep_ms(IRCUT_RUN_TIME);
            printf("============================>>>停止 \n");
            gpio_level_set(IRCUT_INA_PIN, GPIO_LEVEL_LOW);
            gpio_level_set(IRCUT_INB_PIN, GPIO_LEVEL_LOW);
            // pre_time = os_get_ms();
        }
        // ircut_is_adjusted = true;
    }

    // if(ircut_is_adjusted == true && (os_get_ms() - pre_time) >= IRCUT_DELAY_TIME) 
    //     ircut_is_adjusted = false;
}
#endif
#else

#define PS_IR_PATH "/etc/config/ps_ir_3918.conf"//保存ircut引脚和工作电平 以及硬件光敏的使用参数

int ps_mode = AUTO_PHOTOSENSITIVE;
int day_night_mode = SET_AUTO_MODE;

/**
 * set_auto_day_night_param: set_auto_day_night_param
 * return: 0 success, -1 failed
 */
static int ps_set_auto_day_night_param(void)
{
    int i = 0;
    struct ak_auto_day_night_threshold threshold;
    
	threshold.day_to_night_lum = 1000;//6400;//减小能提高日切夜灵敏度
	threshold.night_to_day_lum = 16000;//2048;//增大能提高夜切日灵敏度(10000)
	threshold.lock_time = 1800000;
	
	for (i = 0; i < NIGHT_ARRAY_NUM; i++) {
		threshold.night_cnt[i] = 500000;//减小能提高夜切日灵敏度（最小为0，最大为sensor像素个数）(1000000)
	}

	for (i = 0; i < DAY_ARRAY_NUM; i++) {
		threshold.day_cnt[i] = 1800000;//增大能提高日切夜灵敏度（最小为0，最大为sensor像素个数2073600）
	}
    // threshold.day2night_sleep_time = 500;
    // threshold.night2day_sleep_time = 2000;


    // threshold.day_to_night_lum = 16120;
    // threshold.night_to_day_lum = 9216;

    // threshold.lock_time = 1800000;

    // threshold.night_cnt[0] = 700000;
    // threshold.night_cnt[1] = 700000;
    // threshold.night_cnt[2] = 700000;
    // threshold.night_cnt[3] = 700000;
    // threshold.night_cnt[4] = 700000;

    // threshold.day_cnt[0] = 290000;
    // threshold.day_cnt[1] = 340000;
    // threshold.day_cnt[2] = 290000;
    // threshold.day_cnt[3] = 290000;
    // threshold.day_cnt[4] = 290000;
    // threshold.day_cnt[5] = 290000;
    // threshold.day_cnt[6] = 290000;
    // threshold.day_cnt[7] = 290000;
    // threshold.day_cnt[8] = 290000;
    // threshold.day_cnt[9] = 2073600;

    threshold.day2night_sleep_time = 500;
    threshold.night2day_sleep_time = 1000;

	return ak_vpss_set_auto_day_night_param(VIDEO_DEV0, &threshold);
}

/**
 * 需要放在视频设备打开之后
 */
int ircut_start(void)
{
    if (ps_mode > AUTO_PHOTOSENSITIVE)
    {
        ak_print_error_ex(MODULE_ID_VPSS, "ps_mode:%d error!\n", ps_mode);
        return -1;
    }

    if (day_night_mode > SET_AUTO_MODE)
    {
        ak_print_error_ex(MODULE_ID_VPSS, "day_night_mode:%d error!\n", day_night_mode);
        return -1;
    }

    if (AUTO_PHOTOSENSITIVE == ps_mode)
    {
        ps_set_auto_day_night_param();
    }

    if (ak_ps_start(VIDEO_DEV0, ps_mode, day_night_mode, PS_IR_PATH))
        return -1;

	return 0;
}

void ircut_stop(void)
{
    ak_ps_stop();
}
#endif