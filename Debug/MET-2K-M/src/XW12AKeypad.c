/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-28 13:44:27
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-08-19 08:37:34
 * @FilePath: /project_3/src/XW12AKeypad.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifdef KEYPAD_ENABLE

#include "DrvNumericKeypad.h"
#include "GeneralInterface.h"
#include "NumericKeypad.h"
#include "XW12AKeypad.h"
#include "VoiceRingPlay.h"
#include "UserConfig.h"
#include "Timer.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define XW12A_DRIVE_PATH "/usr/modules/XW12A.ko"
#define SIMULATED_I2C_DRIVE_PATH "/usr/modules/sim_idrv.ko"
#define XW12A_DEV_PATH "/dev/XW12A"
#define SIMULAT_I2C_DEV_PATH "/dev/SIMULAT_I2C"

static Keyboard Keypad;
static char KeyValue[] = {
    [0x0A] = 0,
    [0x01] = 1,
    [0x02] = 2,
    [0x03] = 3,
    [0x04] = 4,
    [0x05] = 5,
    [0x06] = 6,
    [0x10] = 7,
    [0x08] = 8,
    [0x07] = 9,
    [0x09] = KEYSTAR,
    [0x0B] = KEYPOUND};

static int KeypadBuffClear(void)
{
    Keypad.Cursor = 0;
    return 0;
}

static int KeypadDataVerify(char Data)
{
    if (Keypad.Cursor && DiffClockTimeMs(&Keypad.Time) > 30000)
    {
        Keypad.Cursor = 0;
    }
    GetClockTimeMs(&Keypad.Time);
    if (Keypad.Cursor >= sizeof(Keypad.Buff))
    {
        Keypad.Cursor = 0;
        return -1;
    }
    if (Data == KEYPOUND)
    {
        Keypad.Buff[Keypad.Cursor++] = '#';
    }
    else if (Data == KEYSTAR)
    {
        if (Keypad.Cursor)
        {
            Keypad.Cursor--;
            return 0;
        }
        else
        {
            Keypad.Buff[Keypad.Cursor++] = '*';
        }
    }
    else
    {
        Keypad.Buff[Keypad.Cursor++] = Data + 48;
        return 0;
    }
    return 1;
}
/**********************************************************弱函数重定义************************************************************8*/
int XW12AModuleInit(int *DrvFd, int *DrvLen)
{
    printf("[=====================\t%s\t=====================]\n", __func__);
    if (access(XW12A_DRIVE_PATH, F_OK) != 0)
    {
        printf("%s no exist\n", XW12A_DRIVE_PATH);
        return -1;
    }

    if (access(SIMULAT_I2C_DEV_PATH, F_OK) != 0)
    {
        system("insmod " SIMULATED_I2C_DRIVE_PATH);
    }
    if (access(XW12A_DEV_PATH, F_OK) != 0)
    {
        system("insmod " XW12A_DRIVE_PATH);
    }

    *DrvFd = open(XW12A_DEV_PATH, O_RDWR);
    if (*DrvFd < 0)
    {
        return -1;
    }

    NumericKeypadInit();
    printf("%s Open Succeed!!!\n", XW12A_DEV_PATH);
    *DrvLen = XW12A_KEYPAD_LEN;

    memset(&Keypad, 0, sizeof(Keypad));
    return 0;
}

int XW12AModuleHandle(char *Data)
{
    // printf("[%s][%d]\n\n", __func__, __LINE__);
    if (Data[0] >= sizeof(KeyValue))
        return -1;

    // printf("[%s][%d]\n\n", __func__, __LINE__);
    if (TimerEnablestatus(SecirityTriggerTimer))
        return -1;
    // printf("[%s][%d]\n\n", __func__, __LINE__);
    if (VoiceDecodeStatus())
        return -1;

    // printf("[%s][%d]\n\n", __func__, __LINE__);
    int ret = KeypadDataVerify(KeyValue[(int)Data[0]]);
    if (ret < 0)
    {
        printf("Verify Fail!!!!\n");
        VoiceRingPlay(Bi4, VoiceDefVol);
        return 0;
    }
    else
    {
        VoiceRingPlay(Dio, VoiceDefVol);
        if (!ret)
        {
            printf("[");
            for (int i = 0; i < KEYPAD_BUFFER_SIZE; i++)
            {
                if (i < Keypad.Cursor)
                    printf("%c", Keypad.Buff[i]);
                else
                    printf(" ");
            }
            printf("]\r");
            fflush(stdout);
        }
        else
        {
            KeypadProess(&Keypad);
            printf("\r\n");
            KeypadBuffClear();
        }
    }
    return 0;
}
/***********************************************************************************************************************/
#endif