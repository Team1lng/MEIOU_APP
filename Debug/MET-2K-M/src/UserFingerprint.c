#include "UserFingerprint.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define FINGER_CONFIG_PATH "/etc/config/UserFinger.cfg"

static FingerInfo UserFinger;

/**
 * @description: 保存指纹信息
 * @return {*}
 */
int FingerInfoSave(void)
{
    int fd = open(FINGER_CONFIG_PATH, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        printf("write open %s fail \n", FINGER_CONFIG_PATH);
        return -1;
    }

    write(fd, &UserFinger, sizeof(FingerInfo));

    close(fd);
    system("fsync -d " FINGER_CONFIG_PATH);
    return 0;
}

/**
 * @description: 指纹数据初始化
 * @param {int} Index 指纹存储索引
 * @return {*}
 */
int FingerDataInit(int Index)
{
    if (Index >= sizeof(UserFinger.Finger) / sizeof(Fingerprintf))
        return -1;

    UserFinger.Finger[Index].Data[0] = (uint8_t)((Index + 1) >> 8) & 0xFF;
    UserFinger.Finger[Index].Data[1] = (uint8_t)(Index + 1) & 0xFF;
    UserFinger.Finger[Index].Data[2] = 0x00;
    UserFinger.Finger[Index].Data[3] = (uint8_t) ~(((Index + 1) >> 8) & 0xFF);
    UserFinger.Finger[Index].Data[4] = (uint8_t) ~((Index + 1) & 0xFF);
    return 0;
}

/**
 * @description: 指纹数据复位
 * @param {int} Index 指纹存储索引
 * @return {*}
 */
int FingerDataReset(int Index)
{
    if (Index >= sizeof(UserFinger.Finger) / sizeof(Fingerprintf))
        return -1;

    memset(&UserFinger.Finger[Index].Data, 0, sizeof(UserFinger.Finger[Index].Data));
    return 0;
}

/**
 * @description:  指纹信息格式化
 * @return {*}
 */
int FingerInfoFormat(void)
{
    UserFinger.TotalNum = UserFinger.NextEnptyIndex = 0;
    for (int i = 0; i < sizeof(UserFinger.Finger) / sizeof(Fingerprintf); i++)
    {
        memset(&UserFinger.Finger[i], 0, sizeof(Fingerprintf));
    }
    FingerInfoSave();
    return 1;
}

/**
 * @description: 指纹信息初始化
 * @return {*}
 */
int FingerInfoInit(void)
{
    if (access(FINGER_CONFIG_PATH, F_OK) != 0)
    {
        FingerInfoFormat();
    }
    int fd = open(FINGER_CONFIG_PATH, O_RDONLY);
    if (fd < 0)
    {
        FingerInfoSave();
        return 0;
    }

    read(fd, &UserFinger, sizeof(FingerInfo));
    close(fd);

    return 0;
}

/**
 * @description: 设置权限，指纹权限为0时删除指纹
 * @param {int} index   指纹存储索引
 * @param {int} Perm   指纹权限
 * @return {*}0-失败，1-成功
 */
int FingerSetPerm(int index, char Perm)
{
    if (index >= sizeof(UserFinger.Finger) / sizeof(Fingerprintf))
        return -1;

    if (Perm < 0 || Perm > 3)
        return -1;

    if (!(UserFinger.Finger[index].Perm = Perm))
    {
        UserFinger.TotalNum--;
    }

    FingerInfoSave();
    return 0;
}

/**
 * @description: 获取指纹权限
 * @param {int} index 指纹存储索引
 * @return {*} 权限
 */
int FingerPermGet(int index)
{
    return UserFinger.Finger[index].Perm;
}

/**
 * @description: 获取指纹信息
 * @return {*}
 */
FingerInfo *GetFingerInfo(void)
{
    return &UserFinger;
}

/**
 * @description: 获取指纹权限及数据
 * @param {uint8_t} *Deck  指纹缓存
 * @return {*} 指纹数据大小
 */
int UserFingerPermGet(uint8_t **Deck)
{
    *Deck = (uint8_t *)&(UserFinger.TotalNum);
    // printf("DeckData[%p]\n", DeckData);
    return sizeof(UserFinger.Finger);
}