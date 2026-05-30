/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-26 09:41:09
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-08-19 08:37:22
 * @FilePath: /project_3/src/UartIcCard.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UserNetManage.h"
#include "VoiceRingPlay.h"
#include "LightControl.h"
#include "UartControl.h"
#include "UartIcCard.h"
#include "UserConfig.h"
#include "UserCard.h"
#include "Unlock.h"
#include "Timer.h"
#include <string.h>

int UartIcCardCheck(const char *Data)
{
    if ((Data[0] ^ Data[1] ^ Data[2] ^ Data[3]) == Data[4])
    {
        return 0;
    }
    return -1;
}

/**********************************************************弱函数重定义************************************************************8*/
int UartIcCardModuleInit(int *DrvFd, int *DrvLen)
{
    *DrvFd = UartOpen("ttySAK1", 115200, 8, 1, 'n');
    if (*DrvFd < 0)
    {
        printf("open ttySAK1 faild \n");
        return -1;
    }
    *DrvLen = UART_IC_CARD_LEN;
    return 0;
}

int UartIcCardModuleHandle(int *DrvFd, char *Data)
{
    static int CardIndex = 0;
    if (UartIcCardCheck(Data) == -1)
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

    CardLightFlashes();

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
        if (CardIndex != -1)
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

int UartIcCardModuleClear(int Fd)
{
    UartClear(Fd);
    return 0;
}
/***********************************************************************************************************************/