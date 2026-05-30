/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-08-08 15:59:35
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-15 15:47:37
 * @FilePath: /outdoor_pro/common/gpio_base.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#ifndef _GPIO_BASE_H_
#define _GPIO_BASE_H_
#include <stdbool.h>

typedef enum
{
    GPIO_DIR_IN = 0,
    GPIO_DIR_OUT = 1,
    GPIO_DIR_HIGH,
    GPIO_DIR_LOW,
    GPIO_DIR_RETAIN
} GPIO_DIR;

typedef enum
{
    GPIO_LEVEL_LOW = 0x01,
    GPIO_LEVEL_HIGH = 0x02,
    GPIO_LEVEL_UNKOWN = 0x03
} GPIO_LEVEL;

/*
*** 导出GPIO
*/
bool gpio_export(const int pin);

/*
*** 设置GPIO输入/输出
*/
bool gpio_direction_set(const int pin, GPIO_DIR dir);

/*
*** 设置IO口电平
*** 在设置IO电平之前,需要调用 gpio_direction_set
*/
bool gpio_level_set(const int pin, GPIO_LEVEL level);

/*
*** 读取IO电平
*** 在读取IO电平之前,需要调用 gpio_direction_set
*/
bool gpio_level_read(const int pin, GPIO_LEVEL *level);

/*
*** 设置gpio 内置上拉使能
*/
bool gpio_pull_enable(const int pin, bool enable);

bool gpio_set(const int pin, GPIO_LEVEL level);

bool gpio_read(const int pin, GPIO_LEVEL *level);

#endif
