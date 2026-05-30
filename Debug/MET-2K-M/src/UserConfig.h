/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-26 14:42:21
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-15 09:14:33
 * @FilePath: /82225-EPC/src/UserConfig.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define USER_DATA_PATH "/etc/config/UserConfig.cfg"
#define ADMIN_CODE_LEN 6
#define UNLOCK_CODE_LEN 6

typedef enum
{
    CardWay,
    CardOrCodeWay,
    CardAndCodeWay
} UnlockWay;

typedef enum
{
    English,
    Chinese,
    Germany,
    Hebrew,
    Polish,
    Portugal,
    Spain,
    French,
    Japanese,
    Italy,
    LanguageTotal,
} Language;

typedef struct
{
    int UnlockTime;
    int UngateTime;
    Language Language;
    UnlockWay LockWay;
} UserConfig;

/**
 * @description: 保存用户配置
 * @return {*}
 */
int UserConfigSave(void);

/**
 * @description: 获取用户配置
 * @return {*}
 */
UserConfig *UserConfigGet(void);

/**
 * @description: 用户配置初始化
 * @return {*}
 */
int UserConfigInit(void);

/**
 * @description: 恢复用户默认配置
 * @return {*}
 */
int UserConfigReset(void);
#endif