/*
 * @Description: 
 * @Version: 1.0
 * @Autor: wxj
 * @Date: 2022-07-22 11:11:01
 * @LastEditors: wxj
 * @LastEditTime: 2022-07-25 10:36:53
 */
#ifndef _UART_CTRL_H_
#define _UART_CTRL_H_
#include <stdbool.h>
#include <stdio.h>
/***
**   日期:2022-05-30 17:41:52
**   作者: leo.liu
**   函数作用：打开串口
**   参数说明:
***/
int uart_open(char *dev, int speed, int data_bits, int stop_bits, int parity);
/***
**   日期:2022-05-30 17:42:38
**   作者: leo.liu
**   函数作用：发送串口数据
**   参数说明:
***/
int uart_write(int fd, char *data, int size);
/***
**   日期:2022-05-30 17:46:47
**   作者: leo.liu
**   函数作用：读取串口数据
**   参数说明:
***/
int uart_read(int fd, char *data, int size);
/***
**   日期:2022-05-30 17:46:55
**   作者: leo.liu
**   函数作用：关闭串口
**   参数说明:
***/
bool uart_close(int fd);
/***
**   日期:2022-09-21 17:46:55
**   作者: leo.wu
**   函数作用：清空串口清空输入输出缓存
**   参数说明:
***/
bool uart_clear(int fd);

/***
**   日期:2023-01-09 09:42:55
**   作者: leo.wu
**   函数作用：读取串口缓存
**   参数说明:
***/
int uart_buffer_size(int fd);
#endif