/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-12 09:05:38
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-22 14:48:35
 * @FilePath: /project_3/src/GeneralInterface.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _GENERAL_INTERFACE_H_
#define _GENERAL_INTERFACE_H_
#include "time.h"

#define Debug (printf("\n\033[0;32;40m[***%s***]:%u\033[0m \t", __PRETTY_FUNCTION__, __LINE__), printf)
#define DebugWarning (printf("\n\033[0;31;40m[***%s***]:%u\033[0m \t", __PRETTY_FUNCTION__, __LINE__), printf)

/**
 * @description: 获取时间差
 * @param {timespec} *last_time 基准时间
 * @return {*}  时间差值
 */
unsigned long long DiffClockTimeMs(struct timespec *last_time);

/**
 * @description: 获取时间
 * @param {timespec} *time 时间缓存
 * @return {*}  时间差值
 */
void GetClockTimeMs(struct timespec *time);

/**
 * @description: 获取当前文件编译时间
 * @return {*}
 */
struct tm FetchCompileTime(void);
#endif