/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-27 17:34:47
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-08-19 08:37:02
 * @FilePath: /project_3/src/RC522Card.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "DrvSimulatingI2c.h"
#include "GeneralInterface.h"
#include "UserNetManage.h"
#include "DrvSwipeCard.h"
#include "VoiceRingPlay.h"
#include "RC522Card.h"
#include "UserConfig.h"
#include "UserCard.h"
#include <sys/ioctl.h>
#include "Unlock.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "Timer.h"
#include <stdio.h>
#include <fcntl.h>

#define RC522_DRIVE_PATH "/usr/modules/RC522.ko"
#define SIMULATED_I2C_DRIVE_PATH "/usr/modules/sim_idrv.ko"
#define RC522_DEV_PATH "/dev/RC522"

/**
 * @description: RC522复位
 * @return {*}
 */
static void Rc522Reset(int DrvFd)
{
#define DEVICE_RESET_CMD 0xABCD
    // 发送设备复位命令
    if (ioctl(DrvFd, DEVICE_RESET_CMD) < 0)
    {
        perror("ioctl");
        return;
    }
}

static int FilterDuplicareCard(const char *Data)
{
    static char CardCache[CARD_CACHE_BUFFER_SIZE] = {0};

    static struct timespec time;

    if (memcmp(CardCache, Data, RC522_CARD_LEN) == 0)
    {
        int DiffMs = DiffClockTimeMs(&time);
        GetClockTimeMs(&time);
        return DiffMs < 1000 ? -1 : 0;
    }
    GetClockTimeMs(&time);
    memcpy(CardCache, Data, CARD_CACHE_BUFFER_SIZE);
    return 0;
}
/**********************************************************弱函数重定义************************************************************8*/
int Rc522CardModuleInit(int *DrvFd, int *DrvLen)
{
    printf("[=====================\t%s\t=====================]\n", __func__);
    if (access(RC522_DRIVE_PATH, F_OK) != 0)
    {
        printf("%s no exist\n", RC522_DRIVE_PATH);
        return -1;
    }

    system("insmod " SIMULATED_I2C_DRIVE_PATH);

    usleep(1000 * 10);

    system("insmod " RC522_DRIVE_PATH);

    *DrvFd = open(RC522_DEV_PATH, O_RDWR);
    if (*DrvFd < 0)
    {
        return -1;
    }
    printf("%s Open Succeed!!!\n", RC522_DEV_PATH);
    *DrvLen = RC522_CARD_LEN;
    return 0;
}

int Rc522CardModuleHandle(int *DrvFd, char *Data)
{
    static int CardIndex = 0;
    if (FilterDuplicareCard(Data) == -1)
    {
        return -1;
    }

    if (TimerEnablestatus(SecirityTriggerTimer))
    {
        return -1;
    }

    printf("CARD : [%02x %02x %02x %02x %02x]\n", Data[0], Data[1], Data[2], Data[3], Data[4]);
    CardIndex = UserCardSearch(Data);
    if (CardIndex != -1)
    {
        printf("Card %d Exist,Code:%s\n", CardIndex, UserCardGet()->Deck[CardIndex].Code);
    }

    if (TimerEnablestatus(AddCardTimer))
    {
        int ret = 0;
        int IfNetManage = (TimerGet(AddCardTimer)->Data) ? 0 : 1;
        signed char Index = IfNetManage == 0 ? *((char *)(TimerGet(AddCardTimer)->Data)) : -1;
        if (CardIndex == -1)
        {
            if ((ret = UserCardAdd(Index, Data, 1)))
            {
                printf("Add Card [%d] Succeed!!!\n", Index);
                UserCardSave();
                RefreshTimer(30 * 1000, AddCardTimer);

                goto AddFinish;
                return 1;
            }
        }
        printf("Add Card [%d] Fail!!!\n", Index);

    AddFinish:
        VoiceRingPlay(ret ? Bi2 : Bi4, VoiceDefVol);
        if (IfNetManage)
        {
            NetManageShortPack(1, ManageAddCard, ret ? 1 : 9, 0);
        }
        return 0;
    }
    else if (TimerEnablestatus(DelCardTimer))
    {
        if (CardIndex != 0)
        {
            printf("Del Card Index:%d\n", CardIndex);
            if (UserCardSetPerm(CardIndex, 0))
            {
                UserCardSave();
                VoiceRingPlay(Bi3, VoiceDefVol);
                printf("Del Card [%d] Succeed!!!\n", CardIndex);
                return 1;
            }
        }
        VoiceRingPlay(Bi4, VoiceDefVol);
        printf("Del Card [%d] Fail!!!\n", CardIndex);
        return 0;
    }
    else if (TimerEnablestatus(ModifyCodeCardTimer))
    {
        TimerGet(ModifyCodeCardTimer)->Data = &CardIndex;
        printf("[ModifyCodeCardTimer] CardIndex:%d\n", CardIndex);
        RefreshTimer(30 * 1000, ModifyCodeCardTimer);
        VoiceRingPlay(Bi1, VoiceDefVol);
        return 1;
    }

    if (CardIndex != -1)
    {
        if (UserConfigGet()->LockWay == CardAndCodeWay)
        {
            if (TimerEnablestatus(CodeCardUnlockTimer))
            {
                TimerDestroy(CodeCardUnlockTimer);
                goto error;
            }
            else
            {
                SetTimer(30 * 1000, CodeCardUnlockTimer, NULL, &CardIndex);
                VoiceRingPlay(Bi1, VoiceDefVol);
                return 1;
            }
        }
        else
        {
            VoiceRingPlay(Bi1, VoiceDefVol);

            if (UserCardGet()->Deck[CardIndex].Perm & LOCK_TYPE)
                Unlock(UserConfigGet()->UnlockTime, LOCK_TYPE);
            if (UserCardGet()->Deck[CardIndex].Perm & GATE_TYPE)
                Unlock(UserConfigGet()->UngateTime, GATE_TYPE);

            printf("Verify Card Succeed!!! Permissions[%s]\n", UserCardGet()->Deck[CardIndex].Perm == LOCK_TYPE ? "Lock" : UserCardGet()->Deck[CardIndex].Perm == GATE_TYPE ? "Gate"
                                                                                                                                                                            : "Total");
            return 1;
        }
    }
error:
    VoiceRingPlay(Bi4, VoiceDefVol);
    printf("Verify Card  Fail!!!\n");
    return 0;
}

int Rc522CardModuleError(int *DrvFd, int len)
{
    if (len < 0)
        Rc522Reset(*DrvFd);
    return 0;
}
/***********************************************************************************************************************/