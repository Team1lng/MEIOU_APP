/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-28 22:41:28
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-29 15:17:09
 * @FilePath: /project_3/src/GpioKeyPad.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "DrvNumericKeypad.h"
#include "GpioKeyPad.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define GPIO_KEYPAD_DRIVE_PATH "/usr/modules/GpioKeypad.ko"
#define GPIO_KEYPAD_DEV_PATH "/dev/GpioKeypad"

/**********************************************************弱函数重定义************************************************************8*/
int GpioKeypadModuleInit(int *DrvFd, int *DrvLen)
{
    printf("%s  Succeed!!!\n", __func__);
    if (access(GPIO_KEYPAD_DRIVE_PATH, F_OK) != 0)
    {
        printf("%s no exist\n", GPIO_KEYPAD_DRIVE_PATH);
        return -1;
    }

    system("insmod " GPIO_KEYPAD_DRIVE_PATH);

    *DrvFd = open(GPIO_KEYPAD_DEV_PATH, O_RDWR);
    if (*DrvFd < 0)
    {
        return -1;
    }
    printf("%s Open Succeed!!!\n", GPIO_KEYPAD_DEV_PATH);
    *DrvLen = GPIO_KEYPAD_LEN;
    return 0;
}

int GpioKeypadModuleHandle(char *Data)
{
    printf("0x%02x\n", Data[0]);
    return 0;
}
/***********************************************************************************************************************/
