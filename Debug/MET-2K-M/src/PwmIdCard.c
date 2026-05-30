/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-25 15:31:54
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-28 14:31:52
 * @FilePath: /project_3/src/PwmIdCard.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "GeneralInterface.h"
#include "UserNetManage.h"
#include "DrvSwipeCard.h"
#include "VoiceRingPlay.h"
#include "PwmControl.h"
#include "PwmIdCard.h"
#include "UserConfig.h"
#include "UserCard.h"
#include "Unlock.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "Timer.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdint.h>
#include "FingerAndCardLock.h"

#define RFID_DEV_PATH "/dev/rfid_control"
#define RFID_MODULE_KO "/usr/modules/rfid.ko"

#define CARD_LOCK_DURATION_MS 30000    // 禁用时长：30秒
#define CARD_ERROR_MAX_CNT 5           // 连续错误次数上限

struct
{
    PwmIdAction Action;
} PwmIdCardT;

static int FilterDuplicareCard(const char *Data)
{
    static char CardCache[CARD_CACHE_BUFFER_SIZE] = {0};
    static struct timespec time;

    if (memcmp(CardCache, Data, PWM_ID_CARD_LEN) == 0)
    {
        int DiffMs = DiffClockTimeMs(&time);
        GetClockTimeMs(&time);
        return DiffMs < 1000 ? -1 : 0;
    }
    GetClockTimeMs(&time);
    memcpy(CardCache, Data, CARD_CACHE_BUFFER_SIZE);
    return 0;
}

static void PwmCardEventHandle(void)
{
}

// ========== 标记禁用刷卡 ==========
static void CardForbidUse(void)
{
    if (atomic_load(&LockFlag) == 1)
    {
        printf("************* 刷卡已禁用，无需重复标记 *************\n");
        return;
    }

    // 标记禁用 + 记录开始时间
    atomic_store(&LockFlag, 1);
    struct timespec now_time;
    GetClockTimeMs(&now_time);
    uint64_t now_ms = (uint64_t)now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000;
    atomic_store(&LockTimes, now_ms);

    printf("************* 连续刷卡错误5次,禁用刷卡30秒 *************\n");
    VoiceRingPlay(Bi4, VoiceDefVol);
}

#if 1
/**********************************************************弱函数重定义************************************************************8*/
int PwmIdCardModuleInit(int *DrvFd, int *DrvLen)
{
    if (AkDrvPwmOpen(3) != 0)
    {
        printf("AkDrvPwmOpen fail\n");
        return -1;
    }
    if (AkDrvPwmSet(3, 4000, 8000) != 0)
    {
        AkDrvPwmClose(3);
        printf("AkDrvPwmSet fail\n");
        return -1;
    }

    if (access(RFID_MODULE_KO, F_OK) != 0)
    {
        AkDrvPwmClose(3);
        printf("%s Not Found!!!\n", RFID_MODULE_KO);
        return -1;
    }

    system("insmod " RFID_MODULE_KO);

    if ((*DrvFd = open(RFID_DEV_PATH, O_RDONLY)) < 0)
    {
        return -1;
    }
    *DrvLen = PWM_ID_CARD_LEN;
    return *DrvFd;
}

int PwmIdCardModuleHandle(int *DrvFd, char *Data)
{
    // ========== 新增：第一步检查是否禁用刷卡，禁用则直接返回 ==========
    if (IsVerifyLocked() == 1)
    {
        printf("************* 刷卡禁用期，拒绝处理刷卡请求 *************\n");
        return -1;
    }

    // 原有去重逻辑
    if (FilterDuplicareCard(Data) == -1)
    {
        return -1;
    }

    printf("CARD : [%02x %02x %02x %02x]\n", Data[0], Data[1], Data[2], Data[3]);
    int CardIndex = UserCardSearch(Data);
    if (CardIndex != -1)
    {
        printf("Card %d Exist,Code:%s\n", CardIndex, UserCardGet()->Deck[CardIndex].Code);
    }

    // 新增：标记是否是验证失败（非添加卡模式+卡不存在）

    if (TimerEnablestatus(AddCardTimer))
    {
        // 添卡模式：不计数错误
        int ret = -1;
        int IfNetManage = (TimerGet(AddCardTimer)->Data) ? 0 : 1;
        if (CardIndex == -1)
        {
            if ((ret = UserCardAdd(-1, Data, LOCK_PERM | GATE_PERM)) != -1)
            {
                printf("Add Card [%d] Succeed!!!\n", ret);
                UserCardSave();
                RefreshTimer(30 * 1000, AddCardTimer);

                goto AddFinish;
            }
        }
        printf("Add Card Fail!!! Ret:%d\n", ret);

    AddFinish:
        VoiceRingPlay(ret != -1 ? Bi2 : Bi4, VoiceDefVol);
        if (IfNetManage)
        {
            NetManageShortPack(1, ManageAddCard, ret != -1 ? 1 : 9, 0);
        }
        return 0;
    }
    else if (CardIndex != -1)
    {
        // 验证成功：重置错误计数
        ResetVerifyLockState();
        
        VoiceRingPlay(Bi1, VoiceDefVol);
        if (UserCardGet()->Deck[CardIndex].Perm & LOCK_TYPE)
            Unlock(UserConfigGet()->UnlockTime, LOCK_TYPE);
        if (UserCardGet()->Deck[CardIndex].Perm & GATE_TYPE)
            Unlock(UserConfigGet()->UngateTime, GATE_TYPE);

        printf("Verify Card Succeed!!! Permissions[%s]\n", UserCardGet()->Deck[CardIndex].Perm == LOCK_TYPE ? "Lock" : UserCardGet()->Deck[CardIndex].Perm == GATE_TYPE ? "Gate"
                                                                                                                                                                        : "Total");
        return 1;
    }
    else
    {
        // ========== 新增：刷卡验证失败，累加错误次数 ==========
        int cur_error_cnt = atomic_fetch_add(&ErrorCnt , 1) + 1;
        //printf("************* 刷卡验证失败，当前连续错误次数：%d/%d *************\n", cur_error_cnt, CARD_ERROR_MAX_CNT);

        // 错误次数达到上限，触发禁用
        if (cur_error_cnt >= CARD_ERROR_MAX_CNT)
        {
            TriggerVerifyLock();       // 标记禁用30秒
            atomic_store(&ErrorCnt, 0); // 重置错误计数
        }
    }

    // 原有失败提示
    VoiceRingPlay(Bi4, VoiceDefVol);
    printf("Verify Card  Fail!!!\n");
    printf("1#####################################\n");
    return 0;
}
/***********************************************************************************************************************/

#endif








