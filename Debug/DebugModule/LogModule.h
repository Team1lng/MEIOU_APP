/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2025-07-02 18:08:10
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2025-07-02 18:33:36
 * @FilePath: /11/LogModule.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef LOG_MODULE_H
#define LOG_MODULE_H
#include "DebugModule.h"

typedef struct {
    unsigned int RegNum;
    unsigned int RegMax;
    DebugLevel CurrLevel;
    DebugLogModule *Ptr;
} DebugLogModuleInfo;

/**
 * @description: 获取当前日志等级
 * @return {*} 日志等级
 */
DebugLevel DebugLevelGet(void);

/**
 * @description: 设置日志输出等级
 * @param {DebugLevel} level 等级
 * @return {*} -1 失败      0 成功
 */
int DebugLevelSet(DebugLevel level);

/**
 * @description: 调试日志信息获取
 * @return {*}  日志信息结构体
 */
DebugLogModuleInfo *DebugLogInfo(void);

/**
 * @description: 日志模块初始化
 * @param {DebugLevel} DefaultLevel 默认输出等级
 * @param {unsigned int} LogModuleMax   支持模块最大个数
 * @return {*} -1 失败      0 成功
 */
int LogModuleInit(DebugLevel DefaultLevel,unsigned int LogModuleMax);
#endif