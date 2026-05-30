#include <stdint.h>
#include <stdatomic.h>
#include "Timer.h"
#include "GeneralInterface.h"
#include "FingerAndCardLock.h"
#include "stdio.h"
#include "Fingerprint.h"
#include "VoiceRingPlay.h"

atomic_int ErrorCnt = ATOMIC_VAR_INIT(0);
atomic_int LockFlag = ATOMIC_VAR_INIT(0);
atomic_ullong LockStartMs = ATOMIC_VAR_INIT(0);
atomic_int LockTimes = ATOMIC_VAR_INIT(0);

int IsVerifyLocked(void)
{
    // 未锁死，直接返回正常
    if (atomic_load(&LockFlag) != 1)
    {
        return 0;
    }

    // 已锁死，计算剩余时长
    struct timespec now_time;
    GetClockTimeMs(&now_time);
    uint64_t now_ms = (uint64_t)now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000;
    uint64_t lock_start_ms = atomic_load(&LockStartMs);
    uint64_t elapsed_ms = now_ms - lock_start_ms;
    // 计算当前锁死时长：基础时长 * 锁死次数
    uint64_t lock_duration_ms = LOCK_BASE_DURATION_MS * atomic_load(&LockTimes);

    // 超时则清除锁死标志
    if (elapsed_ms >= lock_duration_ms)
    {
        atomic_store(&LockFlag, 0);
        atomic_store(&LockStartMs, 0);
        printf("************* 验证锁死期结束(%lld秒),恢复指纹/刷卡功能 *************\n", lock_duration_ms / 1000);
        return 0;
    }

    // 未超时，返回锁死状态
    printf("************* 指纹/刷卡仍锁死，剩余：%lld秒 *************\n", (lock_duration_ms - elapsed_ms) / 1000);
    return 1;
}

void TriggerVerifyLock(void)
{
    if (atomic_load(&LockFlag) == 1)
    {
        printf("************* 指纹/刷卡已锁死，无需重复标记 *************\n");
        return;
    }

    // 锁死次数+1（第一次1，第二次2，以此类推）
    atomic_fetch_add(&LockTimes, 1);
    // 标记锁死 + 记录开始时间
    atomic_store(&LockFlag, 1);
    struct timespec now_time;
    GetClockTimeMs(&now_time);
    uint64_t now_ms = (uint64_t)now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000;
    atomic_store(&LockStartMs, now_ms);
    // 计算当前锁死时长
    uint64_t lock_duration_ms = LOCK_BASE_DURATION_MS * atomic_load(&LockTimes);
    printf("************* 连续验证错误%d次,触发指纹/刷卡锁死%lld秒 *************\n", LOCK_ERROR_MAX_CNT, lock_duration_ms / 1000);
    
    // 锁死时的提示（灯光+声音，可根据需求调整）
    FingerLightControl(1, YELLOW, 4); // 指纹灯光黄闪4次
    VoiceRingPlay(Bi4, VoiceDefVol);  // 播放错误提示音
}

void ResetVerifyLockState(void)
{
    // 重置错误计数、锁死次数、锁死标志、锁死开始时间
    atomic_store(&ErrorCnt, 0);
    atomic_store(&LockTimes, 0);
    atomic_store(&LockFlag, 0);
    atomic_store(&LockStartMs, 0);
    printf("************* 验证成功，重置锁死状态 *************\n");
}