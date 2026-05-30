#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_CONNECT_NUM 1



/******** 
 * ****- FUNCTION:初始化一个TCP服务器socket
*******- 参数：`Ip` 表示绑定的IP地址（如果为NULL则绑定任意地址），`Port` 表示绑定的端口。
*******- 返回值：成功返回socket文件描述符，失败返回-1。
 ********/
int TcpSocketInit(const char *Ip, int Port)
{
    int Optval = 1;
    int Fd = socket(AF_INET, SOCK_STREAM, 0); /* 创建TCP socket（`SOCK_STREAM`）。 */
    if (Fd < 0)
    {
        perror("socket");
        return -1;
    }

    /* 解除端口占用 */
    if (setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &Optval, sizeof(Optval)) < 0)  /*  设置socket选项`SO_REUSEADDR`，允许地址重用（避免端口占用问题） */
    {
        perror("setsockopt\n");
        return -1;
    }
    /* 将socket设置为非阻塞模式（使用`fcntl`设置`O_NONBLOCK`标志） */
    int Flags = fcntl(Fd, F_GETFL, 0);

    fcntl(Fd, F_SETFL, Flags | O_NONBLOCK);

    struct sockaddr_in ServerAddr;
    bzero(&ServerAddr, sizeof(struct sockaddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    if (NULL == Ip)
    {
        ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /*  初始化服务器地址结构，包括地址族、端口和IP地址 */
    }
    else
    {
        ServerAddr.sin_addr.s_addr = inet_addr(Ip);
    }

    if (bind(Fd, (struct sockaddr *)&ServerAddr, sizeof(struct sockaddr)) < 0) /* 绑定socket到指定地址和端口 */
    {
        perror("bind");
        close(Fd);
        return -1;
    }

    if (listen(Fd, MAX_CONNECT_NUM) < 0)  /* 开始监听，最大连接数由宏`MAX_CONNECT_NUM`定义（这里为1，即只能有一个等待连接）。 */
    {
        perror("listen");
        close(Fd);
        return -1;
    }

    return Fd;  /* 注意：这个函数返回的socket是监听socket，并且是非阻塞的。 */
}

/******** 
 * ****- FUNCTION:从监听socket接受一个客户端连接
*******- 参数：`Fd` 是监听socket的文件描述符。
*******- 返回值：成功返回新的连接socket描述符，超时返回0，失败返回-1。
 ********/
int TcpAccept(int Fd)
{
    fd_set Rdset;

    FD_ZERO(&Rdset);

    FD_SET(Fd, &Rdset);

    struct timeval Timeout;
    Timeout.tv_sec = 0;
    Timeout.tv_usec = 50000;

    int Ret = select(Fd + 1, &Rdset, NULL, NULL, &Timeout);  /* 使用`select`监听监听socket，设置超时时间为50毫秒（0秒50000微秒） */

    if (Ret > 0)  /* 如果`select`返回大于0（表示有连接到来），则调用`accept`接受连接。 */
    {
        FD_CLR(Fd, &Rdset);

        struct sockaddr_in ClientAddr = {0};
        socklen_t addrlen = sizeof(struct sockaddr);
        int NewFd = accept(Fd, (struct sockaddr *)&ClientAddr, &addrlen);
        if (NewFd < 0)
        {
            perror("accept");
            close(Fd);
            return -1;
        }
        printf("TcpAccept Client(Ip = %s, Port = %d)\n", inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));  /*  打印客户端的IP地址和端口。 */

        return NewFd;  /* 返回新的连接socket */
    }
    return 0;
}

/******** 
 * ****- FUNCTION:作为客户端连接到一个TCP服务器
*******- 参数：`Ip` 是服务器IP地址，`Port` 是服务器端口
*******- 返回值：成功返回连接socket描述符，失败返回-1。
 ********/
int TcpConnect(const char *Ip, int Port)
{
    int Fd = socket(AF_INET, SOCK_STREAM, 0);/* 创建TCP socket。 */
    if (Fd < 0)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in ServerAddr;   /* 初始化服务器地址结构。 */
    bzero(&ServerAddr, sizeof(struct sockaddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    ServerAddr.sin_addr.s_addr = inet_addr(Ip);

    if (connect(Fd, (struct sockaddr *)&ServerAddr, sizeof(struct sockaddr)) < 0)  /* 调用`connect`连接服务器。 */
    {
        perror("connect");
        close(Fd);
        return -1;
    }

    return Fd;  /* 返回连接socket。 */
    //注意：这个socket是阻塞的（因为创建时没有设置非阻塞），所以`connect`会阻塞直到连接成功或失败。
}

/******** 
 * ****- FUNCTION:非阻塞接收数据
*******- 参数：`ConnSockfd`已建立的TCP连接套接字描述符（连接socket），`RxBuf`（接收缓冲区），`BufLen`（缓冲区长度），`TimevalSec`（超时秒数），`TimevalUsec`（超时微秒数）。
*******- 返回值：成功返回接收的字节数，超时或出错返回-1。
 ********/
int TcpNonblockingRecv(int ConnSockfd, void *RxBuf, int BufLen, int TimevalSec, int TimevalUsec)
{
    fd_set Readset;
    struct timeval Timeout = {0, 0}; /* 设置`select`的超时时间。 */
    int Maxfd = 0;
    int Fp0 = 0;
    int RecvBytes = 0;
    int Ret = 0;

    Timeout.tv_sec = TimevalSec;
    Timeout.tv_usec = TimevalUsec;
    FD_ZERO(&Readset); // 清空文件描述符集合
    FD_SET(ConnSockfd, &Readset);// 将连接套接字加入集合

    Maxfd = ConnSockfd > Fp0 ? (ConnSockfd + 1) : (Fp0 + 1);

    Ret = select(Maxfd, &Readset, NULL, NULL, &Timeout);  /* 使用`select`监听连接socket，等待数据可读。 */
    if (Ret > 0) // 有文件描述符就绪
    {
        if (FD_ISSET(ConnSockfd, &Readset))  // 检查是否连接套接字就绪
        {
            // 使用非阻塞方式接收数据
            if ((RecvBytes = recv(ConnSockfd, RxBuf, BufLen, MSG_DONTWAIT)) == -1)/* 如果`select`返回大于0，并且连接socket在可读集合中，则调用`recv`（使用`MSG_DONTWAIT`标志，非阻塞接收）读取数据。 */
            {
                perror("recv");
                return -1;
            }
        }
    }
    else
    {
        return -1;
    }

    return RecvBytes;  /* 返回实际读取的字节数。 */
}

/* 阻塞接收数据。
- 参数：`ConnSockfd`（连接socket），`RxBuf`（接收缓冲区），`BufLen`（缓冲区长度）。
- 返回值：调用`recv`的返回值。
步骤：
直接调用`recv`，使用阻塞模式（标志为0）。
注意：由于没有设置超时，如果一直没有数据，会一直阻塞。 */
int TcpBlockingRecv(int ConnSockfd, void *RxBuf, uint16_t BufLen)
{
    return recv(ConnSockfd, RxBuf, BufLen, 0);
}


/* 发送数据。
- 参数：`ConnSockfd`（连接socket），`TxBuf`（发送数据缓冲区），`BufLen`（数据长度）。
- 返回值：调用`send`的返回值。
步骤：
直接调用`send`，使用阻塞模式（标志为0）。 */
int TcpSend(int ConnSockfd, uint8_t *TxBuf, uint16_t BufLen)
{
    return send(ConnSockfd, TxBuf, BufLen, 0);
}

/* 关闭socket。
- 参数：`Sockfd`（要关闭的socket描述符）。
步骤：
调用`close`关闭。 */
void TcpClose(int Sockfd)
{
    close(Sockfd);
}