#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include"DebugModule.h"

#define SHELL_SERVER_PORT		8822
#define CMD_MODULE_MAX		CmdModuleRegMax


static unsigned int CmdModuleRegNum = 0;
static unsigned int CmdModuleRegMax;
static DebugCmdModule *CmdModulePtr = NULL;
static char *CmdShellDafultPath = "/home/share/11/RShell";
static char *CmdLinkDafultPath = "/usr/local/bin/";

/**
 * @description: 调试指令注册函数
 * @param {char} *Cmd	指令字符串
 * @param {CmdHandle} CallbackFunc	指令处理回调
 * @return {*} -1 失败       成功 返回当前支持指令个数
 */
int DebugCmdRegister(char *Cmd, CmdHandle CallbackFunc)
{
	if(0 != access(CmdLinkDafultPath, 0))
	{
	    // int ret = system(CmdLinkDafultPath);
        // if(!ret)
        // {
            DebugLog(0,DEBUG_ERROR,"Failed to create the %s directory!!!",CmdLinkDafultPath);
            return -1;
        // }
	}

	if(0 != access(CmdShellDafultPath, 0))
    {
        DebugLog(0,DEBUG_ERROR,"The %s shell debug path does not exist.!!!",CmdShellDafultPath);
        return -1;
    }

    if(CmdModuleRegNum < CMD_MODULE_MAX)
    {
        CmdModulePtr[CmdModuleRegNum].Cmd = Cmd;
        CmdModulePtr[CmdModuleRegNum].CallbackFunc = CallbackFunc;

		char softlink[128] = {0};
		snprintf( softlink, 128, "/usr/local/bin/%s", CmdModulePtr[CmdModuleRegNum].Cmd);
		symlink(CmdShellDafultPath, softlink);
		DebugLog(0,DEBUG_FATAL,"%s Cmd Register Succeed.!!!",Cmd);
        return CmdModuleRegNum ++;
    }
    DebugLog(0,DEBUG_ERROR,"%s Cmd Module Unable to Register,The registration quota has been reached.!!!",Cmd);
    return -1;
}

static void ExecShellCmd(const char* shell, int argc,char * argv[])
{
	unsigned int loop = 0;
	for( loop = 0; loop < CmdModuleRegNum; loop++ )
	{
		if( !strcmp(CmdModulePtr[loop].Cmd, shell) )
		{
			CmdModulePtr[loop].CallbackFunc(argc, argv);
			break;
		}
	}
}

static void *DebugCmdHandle(void *arg)
{
    prctl(PR_SET_NAME, __func__);
    pthread_detach(pthread_self());

	/* 主循环，处理Shell指令 */
	int shellSockFd = -1;
	struct sockaddr_in shellClientaddr;
	socklen_t addrlen = sizeof(shellClientaddr);

	shellSockFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (shellSockFd == -1)
	{
        DebugLog(0,DEBUG_ERROR,"Create socket fail, error:%s!!!",strerror(errno));
		return NULL;
	}

	int reuse = 1;
	setsockopt(shellSockFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	char shellCmdBuf[1024];
	ssize_t recvbytes = 0;
	struct sockaddr_in shellServaddr;
	shellServaddr.sin_family = AF_INET;
	shellServaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	shellServaddr.sin_port = htons(SHELL_SERVER_PORT);

	if (bind(shellSockFd, (const struct sockaddr *)&shellServaddr, sizeof(shellServaddr)) == -1)
	{
        DebugLog(0,DEBUG_ERROR,"Bind shell socket fail, error:%s!!!",strerror(errno));
		close( shellSockFd );
		return NULL;
	}

	DebugLog(0,DEBUG_FATAL,"DebugCmdHandle Start......");
	
	while(1)
	{
		memset( shellCmdBuf, 0x00, sizeof(shellCmdBuf) );
		memset( &shellClientaddr, 0, sizeof(struct sockaddr_in) );

		recvbytes = recvfrom(shellSockFd, shellCmdBuf, sizeof(shellCmdBuf), 0, (struct sockaddr *)&shellClientaddr, &addrlen);
		if (recvbytes < 0)
		{
			if (errno == EINTR)
			{
				usleep(10);
				continue;
			}
			else
			{
                DebugLog(0,DEBUG_ERROR,"recvfrom shell socket failed!!!");
				break;
			}
		}

        // DebugLog(0,DEBUG_NORMAL,"Recv RShell : %s",shellCmdBuf);

		/* 解析Shell指令 */
		int shellArgc = 0;
		char * shellArgv[32]; /* 最多支持32个参数 */
		char delim[] = " ";
		char *token;
		for(token = strtok(shellCmdBuf, delim); token != NULL; token = strtok(NULL, delim))
		{
			shellArgv[shellArgc++] = token;
			if( shellArgc >= 31 )
			{
                DebugLog(0,DEBUG_WARNING,"Too many shell args");
				break;
			}
		}

		if( shellArgc >= 1 )
		{
			/* 执行Shell命令 */
			ExecShellCmd(shellArgv[0], (shellArgc-1), (shellArgv+1) );
		}
	}

	close( shellSockFd );

	return NULL;
}

/**
 * @description: 调试指令模块初始化
 * @param {unsigned int} CmdModuleMax 支持指令最大各个数
 * @param {char} *CmdShellPath  RShell可执行文件路径
 * @param {char} *CmdLinkPath 指令软连接RShell文件路径
 * @return {*}
 */
int CmdModuleInit(unsigned int CmdModuleMax, char *CmdShellPath, char *CmdLinkPath)
{
    static int InitEd = 0;
    if(InitEd)
    {
		DebugLog(0,DEBUG_ERROR,"DebugCmdModule has been initialized!!!");
        return -1;
    }

    if(CmdModuleMax > 0)
    {
        CmdModulePtr = calloc(CmdModuleMax,sizeof(DebugCmdModule));
        if(CmdModulePtr == NULL)
        {
            DebugLog(0,DEBUG_ERROR,"CmdModuleMax parameter is abnormal!!!");
            return -1;
        }
        else
        {
            CmdModuleRegMax = CmdModuleMax;
            CmdShellDafultPath = CmdShellPath ? CmdShellPath : CmdShellDafultPath;
			CmdLinkDafultPath = CmdLinkPath ? CmdLinkPath : CmdLinkDafultPath;
            pthread_t tid;
            
            if (pthread_create(&tid, NULL, DebugCmdHandle, NULL) != 0) {
                DebugLog(0,DEBUG_ERROR,"Failed to create DebugModuleInit thread:%s!!!",strerror(errno));
                return -1;
            }
        }
    }

    DebugLog(0,DEBUG_FATAL,"CmdModuleInit Succeed.!!!");
    InitEd = 1;
    return 0;
}
