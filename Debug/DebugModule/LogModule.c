#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "LogModule.h"

#define LOG_MODULE_MAX		Info.RegMax

static DebugLogModuleInfo Info = {.CurrLevel = DEBUG_NORMAL,.Ptr = NULL};

static struct 
{
    char *color;
    char *prefix;
}Debugdef[] = {
    [DEBUG_FATAL].color = "\033[36m", [DEBUG_FATAL].prefix = "[F]",
    [DEBUG_ERROR].color = "\033[31m", [DEBUG_ERROR].prefix = "[E]",
    [DEBUG_WARNING].color = "\033[33m", [DEBUG_WARNING].prefix = "[W]",
    [DEBUG_NORMAL].color = "\033[37m", [DEBUG_NORMAL].prefix = "[I]",
    [DEBUG_DEBUG].color = "\033[32m", [DEBUG_DEBUG].prefix = "[D]",
    [DEBUG_TRACE].color = "\033[34m", [DEBUG_TRACE].prefix = "[T]",
};

// 检查模块和日志级别是否有效（提取公共逻辑）
static inline int IsDebugLogValid(int LogFd, DebugLevel level) {
    return !(level >= DEBUG_TOTAL || Info.CurrLevel < level || Info.Ptr == NULL || 
             LogFd >= Info.RegNum || !Info.Ptr[LogFd].LogEnable);
}

/**
 * @description: 日志输出接口(弱函数专用)
 * @param {int} LogFd   日志模块句柄
 * @param {DebugLevel} level    日志输出等级
 * @param {char} *format    
 * @param {va_list} args    
 * @return {*} -1 失败      0 成功
 */
int DebugLogV(int LogFd, DebugLevel level, const char *format, va_list args) {
    if (!IsDebugLogValid(LogFd, level)) {
        return -1;
    }

    // 打印颜色前缀
    printf("%s", Debugdef[level].color);

    printf("%s", Debugdef[level].prefix);

    // 打印模块名前缀（如果有）
    if (Info.Ptr[LogFd].LogName && strlen(Info.Ptr[LogFd].LogName) > 0) {
        printf("[%s] ", Info.Ptr[LogFd].LogName);
    }


    // 输出日志内容
    vprintf(format, args);

    // 重置颜色
    printf("%s\n", Debugdef[DEBUG_NORMAL].color);

    return 0;
}

/**
 * @description: 日志输出接口
 * @param {int} LogFd   日志模块句柄
 * @param {DebugLevel} level 日志输出等级
 * @param {char} *format
 * @return {*} -1 失败      0 成功
 */
int DebugLog(int LogFd, DebugLevel level, const char *format, ...) {
    if (!IsDebugLogValid(LogFd, level)) {
        return -1;
    }

    va_list args;
    va_start(args, format);
    int ret = DebugLogV(LogFd, level, format, args);
    va_end(args);

    return ret;
}

/**
 * @description: 获取当前日志等级
 * @return {*} 日志等级
 */
DebugLevel DebugLevelGet(void)
{
    return Info.CurrLevel;
}

/**
 * @description: 设置日志输出等级
 * @param {DebugLevel} level 等级
 * @return {*} -1 失败      0 成功
 */
int DebugLevelSet(DebugLevel level)
{
    if(level < DEBUG_TOTAL)
    {
        Info.CurrLevel = level;
        return 0;
    }
    return -1;
}

/**
 * @description: 调试日志信息获取
 * @return {*}  日志信息结构体
 */
DebugLogModuleInfo *DebugLogInfo(void)
{   
    return &Info;
}

/**
 * @description: 调试日志注册函数
 * @param {char} *LogName   日志模块名
 * @param {int} Enable  模块使能
 * @return {*} -1 失败      成功 返回日志模块句柄LogFd(用于DebugLog、DebugLogV)
 */
int DebugLogRegister(char *LogName, int Enable)
{
    if(Info.RegNum < LOG_MODULE_MAX)
    {
        Info.Ptr[Info.RegNum].LogEnable = Enable;
        Info.Ptr[Info.RegNum].LogName = LogName;
        return Info.RegNum ++;
    }
    DebugLog(0,DEBUG_ERROR,"%s Log Module Unable to Register,The registration quota has been reached.!!!",LogName);
    return -1;
}

int LogModuleInit(DebugLevel DefaultLevel,unsigned int LogModuleMax)
{
    static int InitEd = 0;
    if(InitEd)
    {
        return -1;
    }

    if(LogModuleMax > 0)
    {
        Info.Ptr = calloc(LogModuleMax,sizeof(DebugLogModule));
        if(Info.Ptr == NULL)
        {
            printf("%s [Error]  Memory allocation for DebugLogModule failed!!! %s ",Debugdef[DEBUG_ERROR].color,Debugdef[DEBUG_NORMAL].color);
        }
        else
        {
            Info.Ptr[0].LogEnable = 1;
            Info.Ptr[0].LogName = "DebugModule";
            Info.RegNum = 1;
            Info.RegMax = LogModuleMax;
            Info.CurrLevel = DefaultLevel;

            InitEd = 1;
        }
    }

    DebugLog(0,DEBUG_FATAL,"LogModuleInit Succeed.!!!");

    return 0;
}
