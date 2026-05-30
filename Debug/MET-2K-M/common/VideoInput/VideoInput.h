/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-15 16:07:45
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-15 10:57:13
 * @FilePath: /project_3/common/VideoInput/VideoInput.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _VIDEOINPUT_H_
#define _VIDEOINPUT_H_
#include "ak_common.h"
#include "ak_venc.h"
#include "ak_vi.h"

#define VIDEO_MODULE_PATH  "/usr/modules/"
#define VIDEO_SNESOR_MODLE_KO  "sensor_gc2083"//"sensor_gc3003"/* "sensor_gc2063" *//* "sensor_gc2083" */
#define VIDEO_SNESOR_MODLE_DAAR " SENSOR_I2C_ADDR=0x37"
#define VIDEO_ISP_MODLE_KO "ak_isp"
#define ISP_PATH  "/app/isp_gc2083-2024121107.conf"    /*  isp_gc2083_mipi_2lane_h3b_101101.conf */     /*  isp_gc2063_mipi_2lane_h3b_101101.conf */ /* isp_gc3003_mipi_2lane_av100_hp.conf */
typedef enum
{
    VI_CHN_MAIN,
    VI_CHN_SUB,
    VI_CHN_TRD,
    VI_CHN_TOATL,
} VI_CHN_NUM;

typedef struct
{
    int EnChn;
    VI_CHN_ATTR_EX ChnAttr;
} ViChnAttr;

typedef struct
{
    char IspPath[128];
    int DevId;

    ViChnAttr DevChn[VI_CHN_TOATL];
    struct venc_param VencParam;
} VideoInputParam;

/**
 * @description:  视频采集参数初始化 [需要函数重定义]
 * @param {VideoInputParam} param   视频采集参数
 * @return {*}
 */
__attribute__((weak)) int VideoInputParamInit(VideoInputParam *param);

/**
 * @description: 视频数据写入 [需要函数重定义]
 * @param {VoiceInfo} info   视频数据
 * @return {*}
 */
__attribute__((weak)) int VideoStreamExport(struct video_stream *stream);

/**
 * @description: 获取视频采集通道属性
 * @param {VI_CHN_NUM} Chn  通道号
 * @return {*}
 */
VI_CHN_ATTR_EX *VideoInputChnAttrGet(VI_CHN_NUM Chn);

/**
 * @description: 请求编码关键帧
 * @return {*}
 */
void VideoKeyFrameRequest(void);

/**
 * @description: 切换夜间模式
 * @param {int} daynight    夜间模式标志
 * @return {*}
 */
void VideoSwitchMode(int daynight);

/**
 * @description: 视频采集初始化
 * @return {*}
 */
int VideoInputInit(void);
#endif

int is_network_video_send_package_open(void);
void network_video_send_package_start(void);
void network_video_send_package_stop(void);
