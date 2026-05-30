/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-30 16:58:52
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-01-30 19:46:28
 * @FilePath: /82225-EPC/common/Network/TcpSocket.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _TCP_SOCKET_H_
#define _TCP_SOCKET_H_
#include "stdint.h"

int TcpSocketInit(const char *Ip, int Port);

int TcpAccept(int Fd);

int TcpConnect(const char *Ip, int Port);

int TcpNonblockingRecv(int ConnSockfd, void *RxBuf, int BufLen, int TimevalSec, int TimevalUsec);

int TcpBlockingRecv(int ConnSockfd, void *RxBuf, uint16_t BufLen);

int TcpSend(int ConnSockfd, uint8_t *TxBuf, uint16_t BufLen);

void TcpClose(int Sockfd);
#endif