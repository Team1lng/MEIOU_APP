/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-31 10:41:01
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-08-19 08:35:50
 * @FilePath: /82225-EPC/src/NetManageCom.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UserNetManage.h"
#include "UserFingerprint.h"
#include "VoiceRingPlay.h"
#include "LightControl.h"
#include "UserCard.h"
#include "Timer.h"
#include "stdio.h"

#define EventList(EVENT)       \
    EVENT(ManageAddFinger)     \
    EVENT(ManageDelFinger)     \
    EVENT(ManageVerifyFinger)  \
    EVENT(ManageGetFinger)     \
    EVENT(ManageSetFingerPerm) \
    EVENT(ManageExitFinger)    \
    EVENT(ManageAddCard)       \
    EVENT(ManageDelCard)       \
    EVENT(ManageVerifyCard)    \
    EVENT(ManageGetCard)       \
    EVENT(ManageSetCardPerm)   \
    EVENT(ManageExitCard)      \
    EVENT(ManageAccessDenied)

#define DefineEventFunc(_EVENT) [_EVENT].Proc = _EVENT##Func, [_EVENT].Str = #_EVENT,

static void ManageAddFingerFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);

    SetTimer(30 * 1000, AddFingerTimer, NULL, NULL);
}

static void ManageDelFingerFunc(NetManagePacket Packet)
{
    printf("[%s][%d]arg1:%d,arg2:%d\n", __func__, __LINE__, Packet.Data.Arg[0], Packet.Data.Arg[1]);
    DelFingerEvnetRegister(Packet.Data.Arg[0], Packet.Data.Arg[1]);
    VoiceRingPlay(Bi2, VoiceDefVol);
}

static void ManageVerifyFingerFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
}

static void ManageGetFingerFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
    unsigned char *DeckPack;
    int DeckPackLen = UserFingerPermGet(&DeckPack) + sizeof(GetFingerInfo()->TotalNum);
    NetManageLongPack(1, ManageGetFinger, DeckPack, DeckPackLen);
    // printf("Total Num [%d]\n", DeckPack[0]);
    // DeckPack++;
    // for (int i = 0; i < 3; i++)
    // {
    //     printf("\n[%d] Prem:%d \nData:\n", i, DeckPack[i * 6]);
    //     for (int j = 0; (j < 6); j++)
    //     {
    //         printf("%02x", DeckPack[i * 6 + j]);
    //     }
    // }
}

static void ManageSetFingerPermFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
    FingerSetPerm(Packet.Data.Arg[0], Packet.Data.Arg[1]);
}

static void ManageExitFingerFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
    TimerDestroy(AddFingerTimer);
    NetManageShortPack(1, ManageExitFinger, 0, 0);
    VoiceRingPlay(Bi4, VoiceDefVol);
}

/* ******************************************************************************************************************************** */

static void ManageAddCardFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);

    SetTimer(30 * 1000, AddCardTimer, NULL, NULL);

    CardLightFlashes();
}

static void ManageDelCardFunc(NetManagePacket Packet)
{
    printf("[%s][%d]arg1:%d,arg2:%d\n", __func__, __LINE__, Packet.Data.Arg[0], Packet.Data.Arg[1]);
    if (Packet.Data.Arg[0] == DECK_SIZE_MAX)
    {
        UserDeckFormat();
    }
    else if (Packet.Data.Arg[0] < DECK_SIZE_MAX && Packet.Data.Arg[0] >= 0)
    {
        UserCardSetPerm(Packet.Data.Arg[0], 0);
    }
    VoiceRingPlay(Bi2, VoiceDefVol);
}

static void ManageVerifyCardFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
}

static void ManageGetCardFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
    unsigned char *DeckPack;
    int DeckPackLen = UserDeckPermGet(&DeckPack);
    printf("DeckPackLen[%d]\n", DeckPackLen);
    NetManageLongPack(1, ManageGetCard, DeckPack, DeckPackLen);
}

static void ManageSetCardPermFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
    UserCardSetPerm(Packet.Data.Arg[0], Packet.Data.Arg[1]);
}

static void ManageExitCardFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
    TimerDestroy(AddCardTimer);
    RefreshTimer(1, LightFlashesTimer);
    NetManageShortPack(1, ManageExitCard, 0, 0);
    VoiceRingPlay(Bi4, VoiceDefVol);
}

static void ManageAccessDeniedFunc(NetManagePacket Packet)
{
    printf("[%s][%d]\n", __func__, __LINE__);
    NetManageShortPack(1, ManageAccessDenied, Packet.Data.Arg[0], 1);
}

NetManageHandle NetManageGroup[ManageTotal] = {EventList(DefineEventFunc)};