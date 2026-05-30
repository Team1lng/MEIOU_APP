/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-25 14:31:42
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-15 15:03:31
 * @FilePath: /project_3/src/SwipeCardModule.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _DRV_SWIPE_CARD_H_
#define _DRV_SWIPE_CARD_H_

#define CARD_CACHE_BUFFER_SIZE 32

#define DrvCardOperater(Module)                                         \
    __attribute__((weak)) int Module##ModuleInit(int *fd, int *len)     \
    {                                                                   \
        printf("%s\n", __func__);                                       \
        return -1;                                                      \
    }                                                                   \
    __attribute__((weak)) int Module##ModuleHandle(int *fd, char *data) \
    {                                                                   \
        printf("%s\n", __func__);                                       \
        return -1;                                                      \
    }                                                                   \
    __attribute__((weak)) int Module##ModuleError(int *fd, int recvlen) \
    {                                                                   \
        return -1;                                                      \
    }                                                                   \
    __attribute__((weak)) int Module##ModuleClear(int fd)               \
    {                                                                   \
        printf("%s\n", __func__);                                       \
        return -1;                                                      \
    }

/*
 *  MODULE(Rc522Card)
 *  MODULE(UartIcCard)
 *  MODULE(PwmIdCard)
 */
#define CARD_MODULE_LIST(MODULE) MODULE(PwmIdCard)

#define DEFINE_MOD(_Module) _Module,
typedef enum
{
    CARD_MODULE_LIST(DEFINE_MOD)
        CardModuleMax
} DRV_CARD_MODULE;

typedef struct
{
    /* 模块句柄 */
    int ModuleFd;
    /* 模块接收缓存大小 */
    int RecvCacheLen;
    /* 卡片有效数据大小 */
    int CardDataLen;
    /* 模块初始化函数，重定义 */
    int (*ModuleInit)(int *fd, int *len);
    /* 模块数据处理函数，重定义 */
    int (*ModuleHandle)(int *fd, char *data);
    /* 模块错误处理函数，重定义 */
    int (*ModuleError)(int *fd, int recvlen);
    /* 模块缓存清除函数，重定义 */
    int (*ModuleClear)(int fd);
} DrvCardAttr;

/**
 * @description: 刷卡驱动初始化
 * @return {*}
 */
int DrvSwipeCardInit(void);
#endif