/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-26 14:42:21
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-15 09:14:10
 * @FilePath: /82225-EPC/src/UserConfig.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UserConfig.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define DEFAULT_UNLOCK_TIME 2

static UserConfig UserConf;
static UserConfig UserConfDefault = {
    .UngateTime = DEFAULT_UNLOCK_TIME,
    .UnlockTime = DEFAULT_UNLOCK_TIME,
    .Language = English,
    .LockWay = CardOrCodeWay,
};

/**
 * @description: 保存用户配置
 * @return {*}
 */
int UserConfigSave(void)
{
    int fd = open(USER_DATA_PATH, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        printf("write open %s fail \n", USER_DATA_PATH);
        return 0;
    }

    write(fd, &UserConf, sizeof(UserConfig));

    close(fd);
    system("fsync -d " USER_DATA_PATH);
    return 1;
}

/**
 * @description: 获取用户配置
 * @return {*}
 */
UserConfig *UserConfigGet(void)
{
    return &UserConf;
}

/**
 * @description: 用户配置初始化
 * @return {*}
 */
int UserConfigInit(void)
{
    int fd = open(USER_DATA_PATH, O_RDONLY);
    if (fd < 0)
    {
        UserConf = UserConfDefault;
        UserConfigSave();
        return 0;
    }

    read(fd, &UserConf, sizeof(UserConfig));

    close(fd);

    return 1;
}

/**
 * @description: 恢复用户默认配置
 * @return {*}
 */
int UserConfigReset(void)
{
    printf("[%s]\n", __func__);
    UserConf = UserConfDefault;
    UserConfigSave();
    return 1;
}
