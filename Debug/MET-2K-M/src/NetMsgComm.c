/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-23 21:37:12
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-16 14:12:41
 * @FilePath: /82225-EPC/src/NetMsgComm.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "NetMsgComm.h"
#include "NetworkRaw.h"
#include "CircularList.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <ak_common.h>
#include "AudioInput.h"
#include "VideoInput.h"
/**********************************************************网络通信实现************************************************************8*/
#define NETWORK_INTERFACE_NAME "eth0"
#define ETH_P_CMD 0xFFFF

static int NetSendFd = -1;
static int NetRecvFd = -1;
static pthread_mutex_t NetSendMutex = PTHREAD_MUTEX_INITIALIZER;
static NetworkDevice LocalDeviceId = DEVICE_OUTDOOR_1;
struct sockaddr_ll LocalSll; // 原始套接字地址结构
extern NetworkEventHandle HandleFuncGroup[TotalEvent];

static void NetworkCodePack(NetworkMsgData *src, unsigned char *dst)
{
    NetworkMsgData *Data = (NetworkMsgData *)src;

    dst[0] = NET_COMMON_CMD_START;
    dst[1] = LocalDeviceId;
    dst[2] = Data->Device;
    dst[3] = Data->Cmd;
    dst[4] = Data->Arg1;
    dst[5] = Data->Arg2;
    dst[6] = (dst[1] + dst[2] + dst[3] + dst[4] + dst[5]) & 0xFF;
    dst[7] = NET_COMMON_CMD_END;
}

static void RawPacketHandle(unsigned char *buf, int len)
{
    if (buf[0] == NET_COMMON_CMD_START && buf[len - 1] == NET_COMMON_CMD_END)
    {
        if (buf[1] < DEVICE_OUTDOOR_1 && buf[3] < TotalEvent && HandleFuncGroup[buf[3]].proc != NULL)
        {
            //printf("[%s]SendDev[%d] Cmd[%d] Len[%d]!!!!!\n", __func__, buf[1], buf[3], len);
            NetworkMsgPacket Packet;
            Packet.SendDev = buf[1];
            Packet.ReceiveDev = buf[2];
            Packet.Cmd = buf[3];
            Packet.DataLen = len;
            if (Packet.DataLen > 8)
            {
                Packet.Data.DP = &buf[4];
            }
            else
            {
                Packet.Data.Arg[0] = buf[4];
                Packet.Data.Arg[1] = buf[5];
            }
            if (Packet.ReceiveDev == DEVICE_ALL || Packet.ReceiveDev == LocalDeviceId)
                HandleFuncGroup[buf[3]].proc(Packet);
        }
    }
}

static void RawPacketPars(unsigned char *buf, int len)
{
    char src_mac[18] = "";
    char dst_mac[18] = "";
    sprintf(dst_mac, "%02X:%02X:%02X:%02X:%02X:%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    sprintf(src_mac, "%02X:%02X:%02X:%02X:%02X:%02X", buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
    // 判断是否为IP数据包
    if (buf[12] == 0x08 && buf[13] == 0x00)
    {
        printf("______________IP数据报_______________\n");
        printf("MAC:%s >> %s\n", src_mac, dst_mac);
        struct iphdr *ip_header = (struct iphdr *)(buf + 14); // IP header starts at byte 14

        // Extracting IP source and destination addresses
        struct in_addr src_addr, dst_addr;
        src_addr.s_addr = ip_header->saddr;
        dst_addr.s_addr = ip_header->daddr;

        printf("Source IP: %s\n", inet_ntoa(src_addr));
        printf("Destination IP: %s\n", inet_ntoa(dst_addr));
    } // 判断是否为ARP数据包
    else if (buf[12] == 0x08 && buf[13] == 0x06)
    {
        printf("______________ARP数据报_______________\n");
        printf("MAC:%s >> %s\n", src_mac, dst_mac);
    } // 判断是否为RARP数据包
    else if (buf[12] == 0x80 && buf[13] == 0x35)
    {
        printf("______________RARP数据报_______________\n");
        printf("MAC:%s>>%s\n", src_mac, dst_mac);
    }
    else
    {
        printf("______________自定义数据报:%x%x_______________\n", buf[12], buf[13]);
        printf("MAC:%s>>%s\n", src_mac, dst_mac);
    }
}

static int NetMsgReceiveSockCreate(void)
{
    if (NetRecvFd != -1)
    {
        return -1;
    }

    NetRecvFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_CMD));
    if (NetRecvFd < 0)
    {
        perror("socket create failed");
        return -1;
    }

    /*
     *   设置混杂模式才能接受目的地非本地MAC的数据报
     */
    if (PromiscuousModeConfig(NETWORK_INTERFACE_NAME) == -1)
    {
        return -1;
    }

    /*
     *   绑定网卡，尽量避免创建过多原始套接字，且原始套接字要尽量绑定网卡。因为收到每个报文除了会将其分发给绑定在该网卡上的原始套接字外，
     *   还会分发给没有绑定网卡的原始套接字。如果原始套接字较多，一个报文就会在软中断上下文中分发多次，造成处理时间过长
     */
    if (RawNetIfrBind(NetRecvFd, NETWORK_INTERFACE_NAME, (int)ETH_P_CMD) == -1)
    {
        return -1;
    }

    return 0;
}

static int NetworkCmdReceive(unsigned char *recvbufer, int size)
{
    return RawPacketReceive(NetRecvFd, recvbufer, size, 5000);
}

static int EthInterfaceState(const char *Interface, int enable)
{
    struct ifreq Ifr;
    int Sockfd;

    // 创建一个套接字，用于与网络接口进行交互
    Sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (Sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // 准备 ifreq 结构体
    strncpy(Ifr.ifr_name, Interface, IFNAMSIZ);

    // 获取当前接口的标志
    if (ioctl(Sockfd, SIOCGIFFLAGS, &Ifr) == -1) {
        perror("ioctl(SIOCGIFFLAGS) failed");
        close(Sockfd);
        return -1;
    }

    // 根据 enable 参数设置接口状态
    if (enable) {
        Ifr.ifr_flags |= IFF_UP;  // 设置 IFF_UP 标志，启用接口
    } else {
        Ifr.ifr_flags &= ~IFF_UP; // 清除 IFF_UP 标志，禁用接口
    }

    // 设置接口的标志
    if (ioctl(Sockfd, SIOCSIFFLAGS, &Ifr) == -1) {
        perror("ioctl(SIOCSIFFLAGS) failed");
        close(Sockfd);
        return -1;
    }

    if (enable) {
        printf("Network interface %s has been enabled\n", Interface);
    } else {
        printf("Network interface %s has been disabled\n", Interface);
    }

    close(Sockfd);
    return 0;
}

int network_stream_count = 0;
static void *NetMsgReceiveThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    unsigned char RecvBuffer[1024 + 73] = "";
    int ReceiveLen;
    int EthReset = 1;
    struct ak_timeval tv_cur;
    struct ak_timeval heatpack_t ;
    /* 创建(PF_PACKET)链路层(SOCK_RAW)原始套接字, (ETH_P_ALL)接收本机收到的所有二层报文*/
    if (NetMsgReceiveSockCreate() == -1)
    {
        goto exit;
    }

    while (1)
    {
        memset(RecvBuffer, 0, sizeof(RecvBuffer));
        // 获取链路层的数据帧
        if ((ReceiveLen = NetworkCmdReceive(RecvBuffer, sizeof(RecvBuffer))) > 0)
        {
            EthReset = 0;
            if (0)
                RawPacketPars(RecvBuffer, sizeof(RecvBuffer));
                RawPacketHandle(RecvBuffer, ReceiveLen);

        }
        else if(ReceiveLen <= 0 && EthReset == 0)
        {
            EthReset = 1;
            printf("Raw Socket Receive Anomaly!!!\n");
            EthInterfaceState(NETWORK_INTERFACE_NAME,0);
            usleep(1000);
            EthInterfaceState(NETWORK_INTERFACE_NAME,1);
            continue;;
        }

        ak_get_ostime(&tv_cur);
        if(ak_diff_ms_time(&tv_cur, &heatpack_t) > 10000 )
        {
            heatpack_t = tv_cur;
            network_stream_count = (is_network_video_send_package_open() == 1) ? network_stream_count + 1 : 0;
            if(network_stream_count > 3)
            {
                network_video_send_package_stop();
                if(is_network_audio_send_package_open() == 1)
                {
                    network_audio_send_package_stop();
                }
                network_stream_count = 0;
            }
        }
        usleep(1000);
    }

exit:
    if (NetRecvFd != -1)
    {
        close(NetRecvFd);
        NetRecvFd = -1;
    }
    printf("============ [%s] [exit]============\n", __func__);
    return NULL;
}

static void NetMsgReceiveTaskCreate(void)
{
    pthread_t thread;
    pthread_create(&thread, NULL, NetMsgReceiveThread, NULL);
    pthread_detach(thread);
}

static int NetMsgSendSockCreate(void)
{
    if (NetSendFd != -1)
    {
        return -1;
    }

    NetSendFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_CMD));
    if (NetSendFd < 0)
    {
        perror("socket create failed");
        return -1;
    }

    return RawNetIfrAddrConfig(NetSendFd, NETWORK_INTERFACE_NAME, &LocalSll);
}

/**
 * @description: 网络消息发送
 * @param {NetworkMsgData} Data 数据报结构
 * @return {*}
 */
void NetworkMsgSend(NetworkMsgData Data)
{
    unsigned char buf[8] = {0};
    NetworkCodePack(&Data, buf);
    pthread_mutex_lock(&NetSendMutex);
    RawPacketSend(NetSendFd, &LocalSll, buf, sizeof(buf), NETWORK_INTERFACE_NAME, ETH_P_CMD);
    pthread_mutex_unlock(&NetSendMutex);
}

/**
 * @description: 本地网络设备ID设置
 * @param {NetworkDevice} Id  设备ID
 * @return {*}
 */
void NetLocalDeviceIDSet(NetworkDevice Id)
{
    LocalDeviceId = Id;
}

/**
 * @description: 本地网络设备ID获取
 * @return {*}
 */
NetworkDevice NetLocalDeviceIDGet(void)
{
    return LocalDeviceId;
}

/**
 * @description: 网络信息通信初始化
 * @return {*}
 */
void NetMsgCommInit(void)
{
    char Buffer[128] = {0};
    sprintf(Buffer, "ifconfig eth0 %s netmask 255.255.255.0", LocalDeviceId == DEVICE_OUTDOOR_1 ? "192.168.37.7" : "192.168.37.8");
    system(Buffer);
    NetMsgReceiveTaskCreate();
    NetMsgSendSockCreate();
}