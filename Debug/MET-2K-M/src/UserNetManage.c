/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-30 19:25:44
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-05 11:10:47
 * @FilePath: /82225-EPC/src/UserNetManage.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UserNetManage.h"
#include "TcpSocket.h"
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/prctl.h>
#include "Timer.h"
#include "Fingerprint.h"


#define NET_MAMAGE_PORT 4321
static int NetFd = -1;
static atomic_int NetManageState = ATOMIC_VAR_INIT(0);
extern NetManageHandle NetManageGroup[ManageTotal];

typedef enum
{
    ValidPack, /* 有效包 */
    ShortPack,
    LongPack,
} PackType;

int NetManageShortPack(char SendDev, unsigned char Cmd, unsigned char arg1, unsigned char arg2)
{
    if (NetFd <= 0)
    {
        return 0;
    }

    unsigned char Code[SHORT_PACK_LEN];
    Code[0] = SHORT_PACK_START;
    Code[1] = 1;
    Code[2] = SendDev;
    Code[3] = Cmd;
    Code[4] = arg1;
    Code[5] = arg2;
    Code[6] = (Code[1] + Code[2] + Code[3] + Code[4] + Code[5]) & 0xFF;
    Code[7] = PACK_END;
    int send_len = TcpSend(NetFd, Code, SHORT_PACK_LEN);
    if (send_len <= 0)
    {
        printf("TcpSend Cmd 0x%x error!\n", Cmd);
        return 0;
    }

    printf("TcpSend Cmd 0x%x succeed!\n", Cmd);
    return 1;
}

int NetManageLongPack(char SendDev, unsigned char Cmd, unsigned char *Data, unsigned char Arg)
{
    if (NetFd <= 0)
    {
        return 0;
    }

    unsigned char Code[LONG_PACK_LEN] = {0};
    memcpy(&Code[6], Data, LONG_PACK_LEN - 7);
    Code[0] = LONG_PACK_START;
    Code[1] = 1;
    Code[2] = SendDev;
    Code[3] = Cmd;
    Code[4] = Arg;
    Code[LONG_PACK_LEN - 1] = PACK_END;
    int send_len = TcpSend(NetFd, Code, LONG_PACK_LEN);
    if (send_len <= 0)
    {
        printf("TcpSend Cmd 0x%x error!\n", Cmd);
        return 0;
    }
    printf("TcpSend Cmd 0x%x succeed!\n", Cmd);
    return 1;
}

int NetManageCheckVaild(unsigned char *Buf, int Ret)
{
    if (Ret < 0)
        return ValidPack;

    /*判断设备是否正确*/
    if ((Buf[2] != 1) && (Buf[2] != 0xFF))
    {
        return ValidPack;
    }

    if ((Buf[Ret - 1] == PACK_END))
    {
        if ((Buf[0] == SHORT_PACK_START))
        {
            return ShortPack;
        }

        if (Buf[0] == LONG_PACK_START)
        {
            return LongPack;
        }
    }
    return ValidPack;
}

static void UserNetManageHandle(PackType Type, unsigned char *buf, int len)
{
    // printf("[%s]SendDev[%d] Cmd[%d] Len[%d]!!!!!\n", __func__, buf[1], buf[3], len);
    NetManagePacket Packet;
    Packet.Dev = buf[1];
    Packet.Cmd = buf[3];
    Packet.DataLen = len;
    if (Packet.DataLen > SHORT_PACK_LEN)
    {
        Packet.Data.DP = &buf[4];
    }
    else
    {
        Packet.Data.Arg[0] = buf[4];
        Packet.Data.Arg[1] = buf[5];
    }
    if (NetManageGroup[buf[3]].Proc)
        NetManageGroup[buf[3]].Proc(Packet);

    if (NetManageGroup[buf[3]].Str)
        printf("[%s]\n", NetManageGroup[buf[3]].Str);
}

static void *UserNetManageThread(void *Arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int ServerFd = *((int *)Arg);
    unsigned char Buffer[1288] = {0};
    while (1)
    {
        NetFd = TcpAccept(ServerFd);
        if (NetFd <= 0)
        {
            usleep(1000);
            continue;
        }

        printf("[Net Manage Client[%d] Tcp Accept Succeed!!!!!!]\n", NetFd);
        atomic_store(&NetManageState, 1);
        while (1)
        {
            memset(Buffer, 0, sizeof(Buffer));
            int RecvLen = TcpNonblockingRecv(NetFd, Buffer, sizeof(Buffer), 0, 5000);
            if (RecvLen == 0)
            {
                break;
            }
            PackType Type = ValidPack;
            if ((Type = NetManageCheckVaild(Buffer, RecvLen)))
            {
                UserNetManageHandle(Type, Buffer, RecvLen);
            }
            usleep(1000);
        }
        TcpClose(NetFd);
        atomic_store(&NetManageState, 0);
        printf("[Client[%d]Tcp Connect Broken Link!!!]\n", NetFd);
        usleep(1000);
    }
    return NULL;
}

/**
 * @description: 判断是否进入管理界面
 * @return {*}
 */
int IsNetManageEntryState(void)
{
    return atomic_load(&NetManageState);
}

/**
 * @description: 初始化网络用户资料管理
 * @return {*}
 */
int UserNetManageInit(void)
{
    static int ServerFd = -1;
    if ((ServerFd = TcpSocketInit(NULL, NET_MAMAGE_PORT)) == -1)
        return 0;

    printf("[Manage Server[%d] Init Succeed !!!!]\n", ServerFd);
    pthread_t thread;
    pthread_create(&thread, NULL, UserNetManageThread, &ServerFd);
    pthread_detach(thread);
    return 1;
}