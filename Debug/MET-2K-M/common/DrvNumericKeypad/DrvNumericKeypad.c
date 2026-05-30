/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-28 11:45:28
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-05 11:10:02
 * @FilePath: /project_3/common/DrvNumericKeypad/DrvNumericKeypad.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "DrvNumericKeypad.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/prctl.h>

#define DefineFunc(_Module) .ModuleFd = -1,                    \
                            .KeyDataLen = -1,                  \
                            .ModuleInit = _Module##ModuleInit, \
                            .ModuleHandle = _Module##ModuleHandle,

/* 定义刷卡模块相关弱函数 */
KEY_MODULE_LIST(DrvKeypadOperater)

static DrvKeypadAttr Keypad = {KEY_MODULE_LIST(DefineFunc)};

static void *DrvNumericKeypadThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    char RecvData[KEYPAD_CACHE_BUFFER_SIZE] = {0};
    while (1)
    {
        memset(RecvData, 0, sizeof(RecvData));
        if (read(Keypad.ModuleFd, RecvData, Keypad.KeyDataLen) > 0)
        {
            Keypad.ModuleHandle(RecvData);
        }
        usleep(1000 * 10);
    }
    return NULL;
}

/**
 * @description: 键盘驱动初始化
 * @return {*}
 */
int DrvNumericKeypadInit(void)
{
    if (Keypad.ModuleInit(&Keypad.ModuleFd, &Keypad.KeyDataLen) == -1)
    {
        return -1;
    }
    pthread_t Thread;
    pthread_create(&Thread, NULL, DrvNumericKeypadThread, NULL);
    pthread_detach(Thread);
    return 0;
}