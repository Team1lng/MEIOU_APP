/*
 * main.c
 *
 *  Created on: Dec 1, 2020
 *      Author: ad
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define SHELL_SERVER_PORT		8822

int main(int argc,char * argv[])
{
	int shellSockFd = -1;
    struct sockaddr_in shellServAddr;
    size_t shellServAddrLen = sizeof(struct sockaddr_in);

    shellSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if( shellSockFd < 0 )
    {
    	printf("Create socket failed\n");
    	return 0;
    }

    bzero( &shellServAddr, sizeof(struct sockaddr_in) );
    shellServAddr.sin_family = AF_INET;
    shellServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    shellServAddr.sin_port = htons(SHELL_SERVER_PORT);

    char *shellCmdStr = NULL;
    int i = 0;
    int shellCmdStrLen = 0;
    // 解析参数长度
    for( i = 0; i < argc; i++ )
    {
    	shellCmdStrLen += strlen(argv[i]);
    }
    /* 根据参数个数算上空格长度 */
    shellCmdStrLen += argc;

    shellCmdStr = (char* )malloc(shellCmdStrLen);
    if( shellCmdStr != NULL )
    {
    	memset( shellCmdStr, 0x00, shellCmdStrLen );

        for( i = 0; i < argc; i++ )
        {
        	strcat( shellCmdStr, argv[i] );
        	strcat( shellCmdStr, " " );
        }

        if( sendto(shellSockFd, shellCmdStr, strlen(shellCmdStr)+1, 0, (struct sockaddr *)&shellServAddr, shellServAddrLen) >= 0 )
        {
        	//printf("Exec cmd: %s\n", shellCmdStr);
        }
        else
        {
        	printf("Send cmd failed, error:%s\n", strerror(errno));
        }

        free(shellCmdStr);
    }
    close(shellSockFd);

	return 0;
}