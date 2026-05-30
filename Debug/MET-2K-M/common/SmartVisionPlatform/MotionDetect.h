/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-21 15:08:46
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-04-28 14:13:08
 * @FilePath: /project_3/common/SmartVisionPlatform/MotionDetect.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _MOTIONDETECT_H_
#define _MOTIONDETECT_H_
#include "ak_common.h"
#include "ak_vi.h"
#include "ak_md.h"
#define MAX_BOX_NUM 5

struct SvpMdParam
{
    int DevId;
    int MdFps;
    int MoveSizeMin;
    int MoveSizeMax;
    int FltBig;
    int FltSmall;
};

struct MdBox
{
    int Left;
    int Top;
    int Right;
    int Bottom;
};

/**
 * @description:  SVP移动侦测模块参数初始化 [需要函数重定义]
 * @param {int} DevId   设备号
 * @param {SvpMdParam} Param   移动侦测属性结构
 * @return {*}
 */
__attribute__((weak)) int SvpMdParamInit(int DevId, struct SvpMdParam *Param);

/**
 * @description:  SVP移动侦测触发回调 [需要函数重定义]
 * @param {int} DevId   设备号
 * @return {*}
 */
__attribute__((weak)) int MotionDetectCallBack(int DevId);

/**
 * MotionDetectInit - md init .
 * @param[IN]:  DevId   设备ID
 * return: 0 - success; other error code;
 */
int MotionDetectInit(int DevId);

/**
 * MotionDetectGetResult - get md result.
 * @dev[IN]:  Dev id
 * @RetInfo[OUT]:  result info
 * @Timeout[IN]:  Timeout <0  block mode, ==0 non-block mode, >0 waittime
 * return: 0 - success;  other error code;
 */
int MotionDetectGetResult(int Dev, struct md_result_info *RetInfo, int Timeout);

/**
 * MotionDetectEnable - start or stop md .
 * @Dev[IN]:  Dev id
 * @Enable[IN]:  [0,1],  0 -> stop md; 1 -> start md
 * return: 0 - success; other error code;
 */
int MotionDetectEnable(int Dev, int Enable);

/**
 * MotionDetectFrameMode -  set the mrd to single frame mode
 * @Dev[IN]:  Dev id
 * @Mode[IN]:  [0,x],  x -> 单帧; 0 -> 多帧
 * return: 0 - success;
 */
int MotionDetectFrameMode(int Dev, int Mode);

/**
 * MotionDetectFiltersSet -   set filters for motion region detection
 * @Dev[IN]:  Dev id
 * @FltBig[IN]: big filter to roughly detect motion region
 * @FltSmall[IN]: small filter to expand rough detection result
 * return: 0 - success;
 */
int MotionDetectFiltersSet(int Dev, int FltBig, int FltSmall);

/*
 * MotionDetectUninit - free md resource and quit md .
 * @Dev[IN]:  Dev id
 * return: 0 - success; other error code;
 */
int MotionDetectUninit(int Dev);

#endif
