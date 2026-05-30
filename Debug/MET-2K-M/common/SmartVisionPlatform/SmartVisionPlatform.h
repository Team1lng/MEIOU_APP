/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-21 11:14:48
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-06-22 16:30:46
 * @FilePath: /project_3/common/SmartVisionPlatform/SmartVisionPlatform.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _SMARTVISIONPLATFORM_H_
#define _SMARTVISIONPLATFORM_H_
#include "ak_common.h"
#include "ak_svp.h"
#include "ak_vi.h"

/* 人形滤波功能(filter+md)，0：disable(only svp)，1：svp+md，2: svp+filter+md" */
typedef enum
{
    NOLY_SVP,
    SVP_MD,
    SVP_FILTER_MD
} FILTER_OPTION;

typedef enum
{
    SVP_CHN_0,
    SVP_CHN_1,
    SVP_CHN_2,
    SVP_CHN_3,
} SVP_CHN_ID;

typedef enum
{
    SENSITIVITY_CLOSE,
    SENSITIVITY_LOW,
    SENSITIVITY_MIDDLE,
    SENSITIVITY_HIGH,
    SENSITIVITY_TOTAL,
} SVP_MD_SENSITIVITY;

typedef struct
{
    int Dev;

    int SvpRate; /* 每个多少帧检测一次 */
    SVP_CHN_ID ChnId;
    FILTER_OPTION FilterOpt;
    /*
     * percentage of overlapped area for svp threshold, valid range [1-100],
     * the less, the more sensitive, but the less accurate, default 7,means 7%
     */
    int SvpThrld;
    /*
     * percentage of overlapped area for md threshold, valid range [1-100],
     * the less, the more sensitive, but the less accurate, default 40, means 40%
     */
    int SvpMdThrld;

    int EnableCallback;
} SVP_PARAM;

/**
 * @description:  SVP模块参数初始化 [需要函数重定义]
 * @param {VI_CHN_ATTR_EX} info   通道属性结构
 * @param {SVP_PARAM} SvpParam   SVP属性结构
 * @return {*}
 */
__attribute__((weak)) int SmartVisionPlatformPrameInit(AK_SVP_CHN_ATTR_T *SvpChnAttr, SVP_PARAM *SvpParam);

/**
 * @description:  SVP回调 [需要函数重定义]
 * @param {int} Total   总输出块数
 * @param {AK_SVP_RECT_T} Src   人行坐标信息 [NULL -  检测无效]   [!NULL - 检测有效]
 * @return {*}
 */
__attribute__((weak)) int SmartVisionPlatformCallback(int Total, AK_SVP_RECT_T *Src);
/**
 * @description: 智能视觉平台初始化
 * @param {VI_CHN_ATTR_EX} *ViChnAttr   视频采集通道属性
 * @return {*}
 */
int SmartVisionPlatformInit(VI_CHN_ATTR_EX *ViChnAttr);

/**
 * SvpMdFiltersSet -   set filters for motion region detection
 * @FltBig[IN]: big filter to roughly detect motion region
 * @FltSmall[IN]: small filter to expand rough detection result
 * return: 0 - success;
 */
int SvpMdFiltersSet(int FltBig, int FltSmall);
#endif