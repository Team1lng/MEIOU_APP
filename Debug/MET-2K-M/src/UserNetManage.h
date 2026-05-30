/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-30 19:25:50
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-08-19 08:29:42
 * @FilePath: /Doorbell/src/UserNetManage.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _USER_NET_MANAGE_H_
#define _USER_NET_MANAGE_H_

#define SHORT_PACK_LEN 8
#define LONG_PACK_LEN 1288

#define SHORT_PACK_START 0xAA
#define LONG_PACK_START 0xBB
#define PACK_END 0xCC

typedef enum
{
    ManageAck = 0x1,
    ManageAddFinger,
    ManageDelFinger,
    ManageVerifyFinger,
    ManageGetFinger,
    ManageSetFingerPerm,
    ManageExitFinger,

    ManageAddCard = 0x10,
    ManageDelCard,
    ManageVerifyCard,
    ManageGetCard,
    ManageSetCardPerm,
    ManageExitCard,

    ManageAccessDenied = 0x90,
    ManageTotal,
} NetManageEvent;

typedef struct
{
    unsigned char Dev;
    unsigned char Cmd;
    unsigned int DataLen;
    union
    {
        unsigned char Arg[2];
        unsigned char *DP;
    } Data;
} NetManagePacket;

typedef struct
{
    const char *Str;
    void (*Proc)(NetManagePacket Packet);
} NetManageHandle;

int NetManageShortPack(char SendDev, unsigned char Cmd, unsigned char arg1, unsigned char arg2);

int NetManageLongPack(char SendDev, unsigned char Cmd, unsigned char *Data, unsigned char Arg);

/**
 * @description: 判断是否进入管理界面
 * @return {*}
 */
int IsNetManageEntryState(void);

/**
 * @description: 初始化网络用户资料管理
 * @return {*}
 */
int UserNetManageInit(void);
#endif