/*******************************************************************
 * @Descripttion   : 
 * @version        : 1.0.0
 * @Author         : wxj
 * @Date           : 2022-09-12 08:27
 * @LastEditTime   : 2022-10-06 15:51
*******************************************************************/
#ifndef _IRCUT_H_
#define _IRCUT_H_

#include <stdbool.h>

//0:硬件光敏(小門口機) 1:软件光敏(大門口機)
#define IRCUT_MODE 0


#if (IRCUT_MODE == 0)

//硬件光敏使用
#define IR_FEED_PIN 65
//电机驱动使能脚，硬件光敏使用，软件光敏使用配置文件设置ircut的a、b引脚，这两个宏不使用
#define IRCUT_INA_PIN 66
#define IRCUT_INB_PIN 67
//由leds子模块管理
#define IR_LED_CTRL_PIN 81
//控制红外补光灯的leds子模块路径
#define IR_LED_PATH "/sys/class/leds/irled/brightness"

void ircut_gpio_init(void);
void ircut_ctrl_handler(void);

#else

int ircut_start(void);
void ircut_stop(void);

#endif

#endif // _IRCUT_H_