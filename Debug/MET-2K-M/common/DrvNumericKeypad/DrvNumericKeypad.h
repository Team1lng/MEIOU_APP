/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-28 11:45:34
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-11 09:53:20
 * @FilePath: /project_3/common/DrvNumericKeypad/DrvNumericKeypad.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _DRV_NUMERIC_KEYPAD_H_
#define _DRV_NUMERIC_KEYPAD_H_

#define KEY_MODULE_LIST(MODULE) MODULE(SC92F836)

#define KEYPAD_CACHE_BUFFER_SIZE 8

#define DrvKeypadOperater(Module)                                   \
    __attribute__((weak)) int Module##ModuleInit(int *fd, int *len) \
    {                                                               \
        printf("%s\n", __func__);                                   \
        return -1;                                                  \
    }                                                               \
    __attribute__((weak)) int Module##ModuleHandle(char *data)      \
    {                                                               \
        printf("%s\n", __func__);                                   \
        return -1;                                                  \
    }

typedef struct
{
    /* 模块句柄 */
    int ModuleFd;
    /* 按键有效数据大小 */
    int KeyDataLen;
    /* 模块初始化函数，重定义 */
    int (*ModuleInit)(int *fd, int *len);
    /* 模块数据处理函数，重定义 */
    int (*ModuleHandle)(char *data);
} DrvKeypadAttr;

/**
 * @description: 键盘驱动初始化
 * @return {*}
 */
int DrvNumericKeypadInit(void);
#endif