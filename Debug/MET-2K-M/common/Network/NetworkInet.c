/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-16 08:30:50
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-19 13:47:35
 * @FilePath: /project_3/common/Network/NetworkInet.c
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
#include "NetworkInet.h"

/**
 * @description:    网络套接字绑定网卡接口
 * @param {int} fd    网络套接字
 * @param {char} *netifr 网卡接口名
 * @return {*}
 */
int InetNetInterfaceBind(int fd, char *netifr)
{
    struct ifreq Interface;
    memset(&Interface, 0x00, sizeof(Interface));
    snprintf(Interface.ifr_name, IFNAMSIZ, "%s", netifr);
    if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&Interface, sizeof(Interface)) < 0)
    {
        printf("%s:SO_BINDTODEVICE failed!!\n", netifr);
        return -1;
    }

    return 0;
}

/**
 * @description:    网络套接字配置
 * @param {int} fd    网络套接字
 * @param {int} optname  行为、属性
 * @param {int} optvol 选项值
 * @return {*}
 */
int InetSockConfig(int fd, int optname, int optvol)
{
    int on = optvol;
    if (setsockopt(fd, SOL_SOCKET, optname, &on, sizeof(on)) < 0)
    {
        printf("InetSockConfig failed\n");
        return -1;
    }
    return 0;
}
