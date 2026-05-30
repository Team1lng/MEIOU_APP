/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-07-09 11:08:02
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-26 08:41:18
 * @FilePath: /Doorbell/common/HardwareControl/Peripheral.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "PeripheralControl.h"

#define WIEGAND_DRIVE_PAYH "/usr/modules/wiegand.ko"
#define WIEGAND_DEVICE_PAYH "/dev/wiegan_control"

/**
 * @description: 韦根数据写入
 * @param {char} *Data 数据
 * @param {int} Size    数据长度
 * @return {*}
 */
int WiegandDevWrite(char *Data, int Size)
{
    static int Fd = -1;

    if (access(WIEGAND_DEVICE_PAYH, F_OK) != 0)
    {
        printf("%s not exit\n", WIEGAND_DEVICE_PAYH);
        return 0;
    }

    if (Fd == -1)
    {
        if ((Fd = open(WIEGAND_DEVICE_PAYH, O_WRONLY)) < 0)
        {
            printf("open %s failed \n", WIEGAND_DEVICE_PAYH);
            return 0;
        }
    }

    if (write(Fd, Data, Size) != Size)
    {
        printf("write %s failed \n", WIEGAND_DEVICE_PAYH);
        return 0;
    }
    return 1;
}

#define _74HC595D_DRIVE_PAYH "/usr/modules/74HC595D.ko"
#define _74HC595D_DEVICE_PAYH "/dev/_74hc595d_control"
static char _74HC595_Status = 0;

/**
 * @description: 获取74hc595d当前状态
 * @return {*}
 */
char _74hc595dDevStatus(void)
{
    return _74HC595_Status;
}

/**
 * @description:74hc595d设备写入
 * @param {char} Data   写入数据
 * @return {*}
 */
int _74hc595dDevWrite(char Data)
{
    static int Fd = -1;
    if (access(_74HC595D_DEVICE_PAYH, F_OK) != 0)
    {
        printf("%s not exit\n", _74HC595D_DEVICE_PAYH);
        return 0;
    }

    if (Fd == -1)
    {
        if ((Fd = open(_74HC595D_DEVICE_PAYH, O_WRONLY)) < 0)
        {
            printf("open %s failed \n", _74HC595D_DEVICE_PAYH);
            return 0;
        }
    }

    // printf("[%s][0x%x]\n", __func__, _74HC595_Status);

    if (write(Fd, &Data, sizeof(Data)) != sizeof(Data))
    {
        printf("write %s failed \n", _74HC595D_DEVICE_PAYH);
        return 0;
    }
    _74HC595_Status = Data;
    return 1;
}

/**
 * @description: 外设控制器初始化
 * @return {*}
 */
int PeripheralControlInit(void)
{
    if (access(WIEGAND_DEVICE_PAYH, F_OK) != 0)
    {
        if (system("insmod " WIEGAND_DRIVE_PAYH))
        {
            printf("insmod %s Fail...\n", WIEGAND_DRIVE_PAYH);
        }
    }

    if (access(_74HC595D_DEVICE_PAYH, F_OK) != 0)
    {
        if (system("insmod " _74HC595D_DRIVE_PAYH))
        {
            printf("insmod %s Fail...\n", _74HC595D_DRIVE_PAYH);
        }
    }

    _74hc595dDevWrite(0);
    return 1;
}