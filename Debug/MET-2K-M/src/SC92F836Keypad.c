/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-28 13:44:27
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-11 10:33:18
 * @FilePath: /project_3/src/SC92F836Keypad.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "DrvNumericKeypad.h"
#include "SC92F836Keypad.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define SC92F836_DRIVE_PATH "/usr/modules/SC92F836.ko"
#define SIMULATED_I2C_DRIVE_PATH "/usr/modules/sim_idrv.ko"
#define SC92F836_DEV_PATH "/dev/sc92f836"

#define TOUCH_KEY_0_VALUE 0xFFBF
#define TOUCH_KEY_1_VALUE 0x7FFF
#define TOUCH_KEY_2_VALUE 0xBFFF
#define TOUCH_KEY_3_VALUE 0xDFFF
#define TOUCH_KEY_4_VALUE 0xEFFF
#define TOUCH_KEY_5_VALUE 0xF7FF
#define TOUCH_KEY_6_VALUE 0xFBFF
#define TOUCH_KEY_7_VALUE 0xFFFF
#define TOUCH_KEY_8_VALUE 0xFEFF
#define TOUCH_KEY_9_VALUE 0xFDFF
#define TOUCH_KEY_STAR_VALUE 0xFF7F
#define TOUCH_KEY_POUND_VALUE 0xFFDF

#define CONTROL_INTERRUPT_CMD 0xABCD
/**********************************************************弱函数重定义*************************************************************/
int SC92F836ModuleInit(int *DrvFd, int *DrvLen)
{
    printf("%s  Succeed!!!\n", __func__);
    if (access(SC92F836_DRIVE_PATH, F_OK) != 0)
    {
        printf("%s no exist\n", SC92F836_DRIVE_PATH);
        return -1;
    }

    system("insmod " SIMULATED_I2C_DRIVE_PATH);

    usleep(1000 * 10);

    system("insmod " SC92F836_DRIVE_PATH);

    *DrvFd = open(SC92F836_DEV_PATH, O_RDWR);
    if (*DrvFd < 0)
    {
        return -1;
    }

    // 设置中断配置
    if (ioctl(*DrvFd, CONTROL_INTERRUPT_CMD, 1) < 0)
    {
        perror("ioctl");
    }

    printf("%s Open Succeed!!!\n", SC92F836_DEV_PATH);
    *DrvLen = SC92F836_KEYPAD_LEN;
    return 0;
}

int SC92F836ModuleHandle(char *Data)
{
    printf("0x%02x\n", Data[0]);
    return 0;
}
/***********************************************************************************************************************/
