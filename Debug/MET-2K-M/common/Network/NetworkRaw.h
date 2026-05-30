/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-06 17:13:04
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-16 11:28:33
 * @FilePath: /project_3/NetworkRaw.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _NETWORK_H_
#define _NETWORK_H_
#include <linux/if_packet.h>

/**
 * @description:    设置混杂模式
 * @param {char} *netifr 网卡接口名
 * @return {*}
 */
int PromiscuousModeConfig(char *netifr);

/**
 * @description:    获取网卡MAC地址
 * @param {char} *netifr 网卡接口名
 * @param {char} *MAC   MAC地址缓存
 * @return {*}
 */
int NetIfrMacGet(char *netifr, char *MAC);

/**
 * @description:    原始套接字绑定网卡接口
 * @param {int} fd    原始套接字
 * @param {char} *netifr 网卡接口名
 * @param {int} protocol    协议类型
 * @return {*}
 */
int RawNetIfrBind(int fd, char *netifr, int protocol);

/**
 * @description:    原始套接字网络接口地址配置
 * @param {int} fd    原始套接字
 * @param {char} *netifr 网卡接口名
 * @param {sockaddr_ll} *sll    原始套接字地址结构
 * @return {*}
 */
int RawNetIfrAddrConfig(int fd, char *netifr, struct sockaddr_ll *sll);

/**
 * @description:    封装链路层数据头
 * @param {unsigned char} *buf  数据报缓存地址
 * @param {char} *netifr 网卡接口名
 * @param {int} protocol    协议类型
 * @return {*}
 */
int RawPacketHead(unsigned char *buf, char *netifr, int protocol);

/**
 * @description: 链路层原始套接字发送
 * @param {int} fd  原始套接字
 * @param {sockaddr_ll} localsll    原始套接字地址结构
 * @param {void} *buf   发送缓存
 * @param {int} size    发送缓存大小
 * @param {char} *netifr 网卡接口名
 * @param {int} protocol    协议类型
 * @return {*}
 */
int RawPacketSend(int fd, struct sockaddr_ll *localsll, unsigned char *buf, int size, char *netifr, int protocol);

/**
 * @description: 链路层原始套接字接收
 * @param {int} fd  原始套接字
 * @param {unsigned char} *recvbufer    接收缓存
 * @param {int} size    缓存大小
 * @param {unsigned int} timeout_ms 接收超时
 * @return {*}
 */
int RawPacketReceive(int fd, unsigned char *recvbufer, int size, unsigned int timeout_ms);
#endif
