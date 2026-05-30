/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-25 14:30:17
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-05 11:10:06
 * @FilePath: /project_3/src/SwipeCardModule.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "DrvSwipeCard.h"
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/prctl.h>

#define DefineFunc(_Module) [_Module].ModuleFd = -1,                        \
                            [_Module].RecvCacheLen = -1,                    \
                            [_Module].CardDataLen = -1,                     \
                            [_Module].ModuleInit = _Module##ModuleInit,     \
                            [_Module].ModuleHandle = _Module##ModuleHandle, \
                            [_Module].ModuleError = _Module##ModuleError,   \
                            [_Module].ModuleClear = _Module##ModuleClear,

/* 定义刷卡模块相关弱函数 */
CARD_MODULE_LIST(DrvCardOperater)

/* 定义刷卡模块相关属性、操作 */
static DrvCardAttr CardModule[CardModuleMax] = {CARD_MODULE_LIST(DefineFunc)};
static DrvCardAttr *CardPtr = NULL;

static int RecvBuffSize(int fd)
{
    int Bytes = 0;
    ioctl(fd, FIONREAD, &Bytes);
    return Bytes;
}

static void *DrvSwipeCardThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    char RecvData[CARD_CACHE_BUFFER_SIZE] = {0};
    int RecvLen = 0;
    while (1)
    {
        memset(RecvData, 0, sizeof(RecvData));
        for (int i = 0; i < CardModuleMax; i++)
        {
            if (CardModule[i].ModuleFd < 0)
                continue;

            CardPtr = &CardModule[i];
            int RecvSize = RecvBuffSize(CardPtr->ModuleFd);

            /* 残留不符合规则数据，清除 */
            if (RecvSize > 0 && RecvSize == CardPtr->RecvCacheLen)
            {
                CardPtr->ModuleClear(CardPtr->ModuleFd);
                RecvSize = -1;
            }

            CardPtr->RecvCacheLen = RecvSize;
            if (CardPtr->RecvCacheLen > CardPtr->CardDataLen && CardPtr->RecvCacheLen % CardPtr->CardDataLen)
            {
                CardPtr->ModuleClear(CardPtr->ModuleFd);
            }

            if (!(CardPtr->RecvCacheLen % CardPtr->CardDataLen) && (RecvLen = read(CardPtr->ModuleFd, RecvData, CardPtr->CardDataLen)) == CardPtr->CardDataLen)
            {
                CardPtr->ModuleHandle(&CardModule[i].ModuleFd, RecvData);
            }
            else if (RecvLen != CardPtr->CardDataLen)
            {
                CardPtr->ModuleError(&CardModule[i].ModuleFd, RecvLen);
            }
        }
        usleep(1000 * 200);
    }
    return NULL;
}

/**
 * @description: 刷卡驱动初始化
 * @return {*}
 */
int DrvSwipeCardInit(void)
{
    for (int i = 0; i < CardModuleMax; i++)
    {
        CardPtr = &CardModule[i];
        CardPtr->ModuleInit(&CardPtr->ModuleFd, &CardPtr->CardDataLen);
    }

    pthread_t Thread;
    pthread_create(&Thread, NULL, DrvSwipeCardThread, NULL);
    pthread_detach(Thread);
    return 0;
}