/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-06 16:44:41
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-16 11:34:38
 * @FilePath: /project_3/NetworkCommon.h.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/select.h>
#include "NetworkRaw.h"

#define MAC_HEAD_LENGHT 60
#define MAC_MTU_LENGHT 1500

/**
 * @description:    设置混杂模式
 * @param {char} *netifr 网卡接口名
 * @return {*}
 */
int PromiscuousModeConfig(char *netifr)
{
    int Fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (Fd < 0)
    {
        perror("socket");
        return -1;
    }

    struct ifreq Ifr;
    // Enable promiscuous mode
    strncpy(Ifr.ifr_name, netifr, IFNAMSIZ);
    if (ioctl(Fd, SIOCGIFFLAGS, &Ifr) == -1)
    {
        perror("ioctl SIOCGIFFLAGS");
        close(Fd);
        return -1;
    }
    Ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(Fd, SIOCSIFFLAGS, &Ifr) == -1)
    {
        perror("ioctl SIOCSIFFLAGS");
        close(Fd);
        return -1;
    }
    close(Fd);
    return 0;
}

/**
 * @description:    获取网卡MAC地址
 * @param {char} *netifr 网卡接口名
 * @param {char} *MAC   MAC地址缓存
 * @return {*}
 */
int NetIfrMacGet(char *netifr, char *MAC)
{
    struct ifreq Req;
    int Fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (Fd < 0)
    {
        perror("socket");
        return -1;
    }

    strcpy(Req.ifr_name, netifr);
    ioctl(Fd, SIOCGIFHWADDR, &Req);
    close(Fd);

    memcpy(MAC, Req.ifr_hwaddr.sa_data, 6);
    return 0;
}

/**
 * @description:    绑定网卡接口
 * @param {int} fd    套接字
 * @param {char} *netifr 网卡接口名
 * @return {*}
 */
int RawNetIfrBind(int fd, char *netifr, int protocol)
{
    struct sockaddr_ll BindAddress;
    memset(&BindAddress, 0, sizeof(BindAddress));
    BindAddress.sll_family = AF_PACKET;
    BindAddress.sll_protocol = htons(protocol);
    BindAddress.sll_ifindex = if_nametoindex(netifr);

    if (bind(fd, (struct sockaddr *)&BindAddress, sizeof(BindAddress)) == -1)
    {
        perror("Binding to interface failed");
        return -1;
    }
    return 0;
}

/**
 * @description:    原始套接字网络接口地址配置
 * @param {int} fd    套接字
 * @param {char} *netifr 网卡接口名
 * @param {sockaddr_ll} *sll    原始套接字地址结构
 * @return {*}
 */
int RawNetIfrAddrConfig(int fd, char *netifr, struct sockaddr_ll *sll)
{
    struct ifreq Ethreq; // 网络接口地址

    strncpy(Ethreq.ifr_name, netifr, IFNAMSIZ); // 指定网卡名称
    if (-1 == ioctl(fd, SIOCGIFINDEX, &Ethreq)) // 获取网络接口
    {
        perror("ioctl");
        return -1;
    }

    /*将网络接口赋值给原始套接字地址结构*/
    bzero(sll, sizeof(struct sockaddr_ll));
    sll->sll_ifindex = Ethreq.ifr_ifindex;
    return 0;
}

/**
 * @description:    封装链路层数据头
 * @param {unsigned char} *buf  数据报缓存地址
 * @param {int} protocol    协议类型
 * @return {*}
 */
int RawPacketHead(unsigned char *buf, char *netifr, int protocol)
{
    static char *MacHead = NULL;
    if (MacHead == NULL)
    {
        MacHead = malloc(MAC_HEAD_LENGHT);
        memset(MacHead, 0, MAC_HEAD_LENGHT);
#if 1
        // 设置目的网卡地址
        MacHead[0] = 0x01;
        MacHead[1] = 0x01;
        MacHead[2] = 0x01;
        MacHead[3] = 0x01;
        MacHead[4] = 0x01;
        MacHead[5] = 0x01;
#endif
        if (NetIfrMacGet(netifr, &MacHead[6]) == -1)
        {
            free(MacHead);
            MacHead = NULL;
            return -1;
        }
    }
    MacHead[12] = protocol / 256;
    MacHead[13] = protocol % 256;
    memcpy(buf, MacHead, MAC_HEAD_LENGHT);
    return 0;
}

/**
 * @description: 链路层原始套接字发送
 * @param {int} fd  套接字
 * @param {sockaddr_ll} localsll    原始套接字地址结构
 * @param {void} *buf   发送缓存
 * @param {int} size    发送缓存大小
 * @param {char} *netifr 网卡接口名
 * @param {int} protocol    协议类型
 * @return {*}
 */
int RawPacketSend(int fd, struct sockaddr_ll *localsll, unsigned char *buf, int size, char *netifr, int protocol)
{
    int SendIndex = 0;
    int ValidLen = 0;
    unsigned char Package[1514] = {0};
    while (size)
    {
        memset(Package, 0, sizeof(Package));
        RawPacketHead(Package, netifr, protocol);
        if (size > MAC_MTU_LENGHT)
        {
            ValidLen = MAC_MTU_LENGHT;
            size -= ValidLen;
            memcpy(&Package[MAC_HEAD_LENGHT], &buf[SendIndex], ValidLen);
            SendIndex += ValidLen;
        }
        else
        {
            ValidLen = size;
            size -= ValidLen;
            memcpy(&Package[MAC_HEAD_LENGHT], &buf[SendIndex], ValidLen);
        }

        if (sendto(fd, Package, ValidLen + MAC_HEAD_LENGHT, 0, (struct sockaddr *)localsll, sizeof(*localsll)) < 0)
        {
            perror("sendto fail ");
            return -1;
        }
    }

    return 0;
}

/**
 * @description: 链路层原始套接字接收
 * @param {int} fd  套接字
 * @param {unsigned char} *recvbufer    接收缓存
 * @param {int} size    缓存大小
 * @param {unsigned int} timeout_ms 接收超时
 * @return {*}
 */
int RawPacketReceive(int fd, unsigned char *recvbufer, int size, unsigned int timeout_ms)
{
    fd_set Recvfd; /* 存储文件描述符集合 */
    struct timeval Timeout;
    Timeout.tv_sec = timeout_ms / 1000;
    Timeout.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&Recvfd);    /* 清空文件描述符集合 */
    FD_SET(fd, &Recvfd); /* 将原始套接字添加到文件描述符集合中 */

    static int PrevAllocSize = 0;
    static unsigned char *Buffer;
    if (PrevAllocSize < size)
    {
        if (Buffer != NULL)
            free(Buffer);

        Buffer = malloc(size);
        if (Buffer == NULL)
            return -1;

        PrevAllocSize = size;
    }

    if (select(fd + 1, &Recvfd, NULL, NULL, &Timeout) > 0)
    {
        /* 检查原始套接字是否准备好读取 ,这段代码可以屏蔽，因为recvfd中就只有一个原始套接字fd*/
        if (FD_ISSET(fd, &Recvfd))
        {
            if ((size = recvfrom(fd, Buffer, size, 0, NULL, NULL)) <= 0)
            {
                return -1;
            }
            else if (size > MAC_HEAD_LENGHT)
            {
                memcpy(recvbufer, &Buffer[MAC_HEAD_LENGHT], size - MAC_HEAD_LENGHT);
                return size - MAC_HEAD_LENGHT;
            }
        }
    }
    return -1;
}