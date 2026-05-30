/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2025-07-01 16:09:48
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2025-07-03 08:59:27
 * @FilePath: /11/DebugModule.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef DEBUG_MODULE_H
#define DEBUG_MODULE_H
#include <stdarg.h>

typedef enum {
    DEBUG_FATAL,
    DEBUG_ERROR,        
    DEBUG_WARNING,      
    DEBUG_NORMAL,     
    DEBUG_DEBUG,     
    DEBUG_TRACE,     
    DEBUG_TOTAL,    
} DebugLevel;

typedef struct {
    int LogEnable;
    char *LogName;
} DebugLogModule;


typedef void (* CmdHandle)(int argc,char * argv[]);
typedef struct {
    char *Cmd;
    CmdHandle CallbackFunc;
} DebugCmdModule;

/**
 * @description: 日志输出接口
 * @param {int} LogFd   日志模块句柄
 * @param {DebugLevel} level 日志输出等级
 * @param {char} *format
 * @return {*} -1 失败      0 成功
 */
int DebugLog(int LogFd, DebugLevel level, const char *format, ...);

/**
 * @description: 日志输出接口(弱函数专用)
 * @param {int} LogFd   日志模块句柄
 * @param {DebugLevel} level    日志输出等级
 * @param {char} *format    
 * @param {va_list} args    
 * @return {*} -1 失败      0 成功
 */
int DebugLogV(int LogFd, DebugLevel level, const char *format, va_list args);

/**
 * @description: 调试日志注册函数
 * @param {char} *LogName   日志模块名
 * @param {int} Enable  模块使能
 * @return {*} -1 失败      成功 返回日志模块句柄LogFd(用于DebugLog、DebugLogV)
 */
int DebugLogRegister(char *LogName, int Enable);

/**
 * @description: 调试指令注册函数
 * @param {char} *Cmd	指令字符串
 * @param {CmdHandle} CallbackFunc	指令处理回调
 * @return {*} -1 失败       成功 返回当前支持指令个数
 */
int DebugCmdRegister(char *CmdName, CmdHandle CallbackFunc);

/**
 * @description: 调试模块初始化
 * @param {DebugLevel} DefaultLevel 默认日志等级
 * @param {unsigned int} SupportModuleMax   支持日志模块最大个数
 * @param {unsigned int} CmdModuleMax   支持指令最大个数
 * @param {char} *CmdShellPath  RShell可执行文件路径
 * @param {char} *CmdLinkPath   指令软连接RShell文件路径
 * @return {*}
 */
int DebugModuleInit(DebugLevel DefaultLevel,unsigned int SupportModuleMax,unsigned int CmdModuleMax, char *CmdShellPath, char *CmdLinkPath);
#endif