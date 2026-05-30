// 全局验证锁死配置
#define LOCK_ERROR_MAX_CNT 5           // 指纹/刷卡累计错误上限
#define LOCK_BASE_DURATION_MS 30000    // 基础锁死时长（30秒）

extern atomic_int ErrorCnt;
extern atomic_int LockFlag;
extern atomic_ullong LockStartMs;
extern atomic_int LockTimes;

// 全局锁死控制函数声明
int IsVerifyLocked(void);                  // 检查是否被锁死
void TriggerVerifyLock(void);              // 触发锁死（错误累计5次时调用）
void ResetVerifyLockState(void);           // 重置锁死状态（验证成功时调用）
/**
 * @brief 检查是否因验证错误被锁死（指纹/刷卡共用）
 * @return 1=锁死，0=正常
 */
int IsVerifyLocked(void);

/**
 * @brief 标记触发验证锁死（指纹/刷卡共用）
 */
void TriggerVerifyLock(void);

/**
 * @brief 重置验证锁死状态（验证成功时调用）
 */
void ResetVerifyLockState(void);