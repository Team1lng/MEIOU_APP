/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2025-07-02 15:11:49
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2025-07-02 18:30:17
 * @FilePath: /11/DebugModule.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include <stdlib.h>
#include "LogModule.h"
#include "CmdModule.h"
#include "DebugModule.h"

#define RegisterCmdList(Cmd)       do{  \
    Cmd(LogModule)       \
    Cmd(LogLevel)       \
}while(0);

#define RegisterCmd(CMD) DebugCmdRegister(#CMD,CMD##Cmd);

static void LogLevelCmd(int argc,char * argv[])
{
    char *LogStr[DEBUG_TOTAL] = {"FATAL","ERROR","WARNING","NORMAL","DEBUG","TRACE"};

    DebugLogModuleInfo *Info = DebugLogInfo();
    if(argc != 1)
    {
        DebugLog(0, DEBUG_FATAL, "****************************************");
        DebugLog(0, DEBUG_FATAL, "Examle:");
        for(int i = DEBUG_FATAL;i < DEBUG_TOTAL;i ++)
        {
            DebugLog(0, DEBUG_FATAL, "%d        %s",i, LogStr[i]);
        }
        DebugLog(0, DEBUG_FATAL, "LogLevel Level(DEBUG_FATAL ~ DEBUG_TRACE)");
        DebugLog(0, DEBUG_FATAL, "LogLevel 3");
        PRINTF_LOG_LEVEL:
        DebugLog(0, DEBUG_FATAL, "****************************************");

        DebugLog(0, DEBUG_FATAL, "LogLevel [%s]\n",LogStr[Info->CurrLevel]);
        return;
    }

	unsigned int level  = (unsigned int)atoi(argv[0]);
    if(level >= DEBUG_TOTAL)
    {
        DebugLog(0, DEBUG_ERROR, "Undefined Log Level!!!");
        return;
    }

    DebugLevelSet(level);
    goto PRINTF_LOG_LEVEL;
}

static void LogModuleCmd(int argc,char * argv[])
{
    DebugLogModuleInfo *Info = DebugLogInfo();
    if(argc != 2)
    {
        DebugLog(0, DEBUG_FATAL, "****************************************");
        DebugLog(0, DEBUG_FATAL, "Examle:");
        DebugLog(0, DEBUG_FATAL, "LogModule Log(0 - LogModuleMax) Status(Enable 1 - 0)");
        DebugLog(0, DEBUG_FATAL, "LogModule 0 1");

        PRINTF_LOG_STATUS:
        DebugLog(0, DEBUG_FATAL, "****************************************");

        DebugLog(0, DEBUG_FATAL, "%-5s        %-15s        %-5s","Index", "Log", "Status");
        for(int i = 0; i < Info->RegNum; i ++)
        {
            DebugLog(0, DEBUG_FATAL, "%-5d        %-15s        [%-5s]",i, Info->Ptr[i].LogName, Info->Ptr[i].LogEnable ? "Open" : "Close");
        }
        printf("\n");
        return ;
    }
    
	unsigned int index  = (unsigned int)atoi(argv[0]);
	unsigned int enable = (unsigned int)atoi(argv[1]);
    if(index >= Info->RegNum)
    {
        DebugLog(0, DEBUG_ERROR, "Unregistered Log Module!!!");
        return;
    }
    Info->Ptr[index].LogEnable = enable ? 1 : 0;
    goto PRINTF_LOG_STATUS;
}

/**
 * @description: 调试模块初始化
 * @param {DebugLevel} DefaultLevel 默认日志等级
 * @param {unsigned int} SupportModuleMax   支持日志模块最大个数
 * @param {unsigned int} CmdModuleMax   支持指令最大个数
 * @param {char} *CmdShellPath  RShell可执行文件路径
 * @param {char} *CmdLinkPath   指令软连接RShell文件路径
 * @return {*}
 */
int DebugModuleInit(DebugLevel DefaultLevel,unsigned int LogModuleMax,unsigned int CmdModuleMax, char *CmdShellPath, char *CmdLinkPath)
{   
    static int ModuleInitEd = 0;
    if(ModuleInitEd)
    {
        printf("\033[33m [Warning]  DebugLogModule has been initialized!!!\033[37m \n");
        return -1;
    }

    LogModuleInit(DefaultLevel, LogModuleMax);

    if(CmdModuleInit(CmdModuleMax, CmdShellPath, CmdLinkPath) == 0)
	{
        RegisterCmdList(RegisterCmd)
	}
    ModuleInitEd = 1;
    return 0;
}