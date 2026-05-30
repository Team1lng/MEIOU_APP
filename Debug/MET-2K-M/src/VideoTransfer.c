/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-16 08:19:10
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-28 16:04:39
 * @FilePath: /project_3/src/VideoTransfer.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "SmartVisionPlatform.h"
#include"InfraredDetect.h"
#include "VideoTransfer.h"
#include "NetworkInet.h"
#include "VideoInput.h"
#include "Timer.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAIN_CHN_WIDTH 1920
#define MAIN_CHN_HEIGHT 1080
#define SUB_CHN_WIDTH 1280
#define SUB_CHN_HEIGHT 720
#define TRD_CHN_WIDTH 320
#define TRD_CHN_HEIGHT 180
/****************************************函数重载*******************************************/
int VideoInputParamInit(VideoInputParam *param)
{
    memset(param->IspPath, 0, sizeof(param->IspPath));
    sprintf(param->IspPath, "%s", ISP_PATH);
    param->DevId = VIDEO_DEV0;

    param->DevChn[VI_CHN_MAIN].EnChn = 1;
    param->DevChn[VI_CHN_MAIN].ChnAttr.chn_id = VIDEO_CHN0;
    param->DevChn[VI_CHN_MAIN].ChnAttr.res.width = 1920;
    param->DevChn[VI_CHN_MAIN].ChnAttr.res.height = 1080;
    param->DevChn[VI_CHN_MAIN].ChnAttr.frame_depth = 2;
    param->DevChn[VI_CHN_MAIN].ChnAttr.data_type = VI_DATA_TYPE_YUV420SP;

    param->DevChn[VI_CHN_SUB].EnChn = 1;
    param->DevChn[VI_CHN_SUB].ChnAttr.chn_id = VIDEO_CHN1;
    param->DevChn[VI_CHN_SUB].ChnAttr.res.width = 1280;
    param->DevChn[VI_CHN_SUB].ChnAttr.res.height = 720;
    param->DevChn[VI_CHN_SUB].ChnAttr.frame_depth = 2;
    param->DevChn[VI_CHN_SUB].ChnAttr.data_type = VI_DATA_TYPE_YUV420SP;

    param->DevChn[VI_CHN_TRD].EnChn = 1;
    param->DevChn[VI_CHN_TRD].ChnAttr.chn_id = VIDEO_CHN16;
    param->DevChn[VI_CHN_TRD].ChnAttr.res.width = 320;
    param->DevChn[VI_CHN_TRD].ChnAttr.res.height = 180;
    param->DevChn[VI_CHN_TRD].ChnAttr.frame_depth = 2;
    param->DevChn[VI_CHN_TRD].ChnAttr.data_type = VI_DATA_TYPE_RGB_LINEINTL;

    param->VencParam.width = param->DevChn[VI_CHN_MAIN].ChnAttr.res.width;
    param->VencParam.height = param->DevChn[VI_CHN_MAIN].ChnAttr.res.height;
    param->VencParam.fps = 25;
    param->VencParam.goplen = param->VencParam.fps * 2;
    param->VencParam.target_kbps = 2048;
    param->VencParam.max_kbps = 4096;
    param->VencParam.br_mode = BR_MODE_AVBR;
    param->VencParam.minqp = 25;
    param->VencParam.maxqp = 43;
    param->VencParam.initqp = 99 /* (param->VencParam.minqp + param->VencParam.maxqp) / 2 */;
    param->VencParam.jpeg_qlevel = JPEG_QLEVEL_DEFAULT;
    param->VencParam.chroma_mode = CHROMA_4_2_0;
    param->VencParam.max_picture_size = 0;
    param->VencParam.enc_level = 50;
    param->VencParam.smart_mode = 0;
    param->VencParam.smart_goplen = 100;
    param->VencParam.smart_quality = 50;
    param->VencParam.smart_static_value = 0;
    param->VencParam.enc_out_type = H264_ENC_TYPE;
    param->VencParam.profile = PROFILE_MAIN;
    return 0;
}

int VideoStreamExport(struct video_stream *stream)
{
    NetVideoPackageSend(stream->data, stream->len);
    return 0;
}

int SmartVisionPlatformCallback(int Total, AK_SVP_RECT_T *Src)
{
    struct ak_vi_box_group BoxInfo;
    memset(&BoxInfo, 0, sizeof(struct ak_vi_box_group));
    if (Src)
    {
        SetTimer(3000, SVPTimer, NULL, NULL);
        for (int j = 0; j < Total; j++)
        {
            BoxInfo.draw_frame_num = -1;
            BoxInfo.box[j].left = RectMap(Src[j].left, 0, TRD_CHN_WIDTH, 0, MAIN_CHN_WIDTH);
            BoxInfo.box[j].top = RectMap(Src[j].top, 0, TRD_CHN_HEIGHT, 0, MAIN_CHN_HEIGHT);
            BoxInfo.box[j].width = RectMap(Src[j].right - Src[j].left, 0, TRD_CHN_WIDTH, 0, MAIN_CHN_WIDTH);
            BoxInfo.box[j].height = RectMap(Src[j].bottom - Src[j].top, 0, TRD_CHN_HEIGHT, 0, MAIN_CHN_HEIGHT);
            BoxInfo.box[j].enable = 1;
            BoxInfo.box[j].color_id = Src[j].label;
            BoxInfo.box[j].line_width = 2;
            // printf("SVP [%d] :Mobile [%s] detect, score[%lu],Res is:left[%lu],top[%lu],right[%lu]:bottom[%lu]\n",
            //        j, Src[j].label == AK_SVP_HUMAN_SHAPE ? "Body" : "Face", Src[j].score, Src[j].left, Src[j].top, Src[j].right, Src[j].bottom);
        }
    }
    ak_vi_draw_box(VIDEO_CHN0, &BoxInfo);
    return AK_SUCCESS;
}

/*******************************************************************************************/
#define NETWORK_INTERFACE_NAME "eth0"
#define VIDEO_IP_ADDRES "255.255.255.255"
#define VIDEO_PACKAGE_SIZE_MAX 32 * 1024

static const char video_start_code[4] = {0x00, 0x00, 0x01, 0xfc};
static struct sockaddr_in VideoServAddr;
static int NetVideoProtocol = 0x1606;
static int NetVideoFd = -1;
#include <errno.h>
/**
 * @description: 视频数据包发送
 * @param {char} *Data  视频数据
 * @param {int} Size    视频数据大小
 * @return {*}
 */
void NetVideoPackageSend(unsigned char *Data, int Size)
{
    static unsigned char *Buffer = NULL;
    int SendSize = 0;
    int RemainSize = Size;

    /***** 分配视频帧内存 *****/
    if (Buffer == NULL)
    {
        Buffer = malloc(VIDEO_PACKAGE_SIZE_MAX);
        if (Buffer == NULL)
            return;
    }

    /***** 获取时间戳 *****/
    struct timespec Tv;
    clock_gettime(CLOCK_MONOTONIC, &Tv);
    unsigned long long Pts = Tv.tv_sec * 1000 + Tv.tv_nsec / 1000000;

    /***** 帧序号 *****/
    static unsigned long FrameIndex = 0;
    FrameIndex++;
    // printf("%s,NetVideoFd:%d,Size:%d\n", __func__, NetVideoFd, Size);
    /***** 帧类型 H264:0*****/
    char FrameType = 0;

    memset(&VideoServAddr, 0, sizeof(VideoServAddr));
    VideoServAddr.sin_family = AF_INET;
    VideoServAddr.sin_addr.s_addr = inet_addr(VIDEO_IP_ADDRES);
    VideoServAddr.sin_port = htons(NetVideoProtocol);

    while (RemainSize > 0)
    {
        if (SendSize == 0)
        {
            memcpy(&Buffer[0], video_start_code, 4);
            Buffer[4] = (RemainSize >> 24) & 0xFF;
            Buffer[5] = (RemainSize >> 16) & 0xFF;
            Buffer[6] = (RemainSize >> 8) & 0xFF;
            Buffer[7] = RemainSize & 0xFF;

            Buffer[8] = (Pts >> 24) & 0xFF;
            Buffer[9] = (Pts >> 16) & 0xFF;
            Buffer[10] = (Pts >> 8) & 0xFF;
            Buffer[11] = Pts & 0xFF;

            Buffer[12] = (FrameIndex >> 24) & 0xFF;
            Buffer[13] = (FrameIndex >> 16) & 0xFF;
            Buffer[14] = (FrameIndex >> 8) & 0xFF;
            Buffer[15] = FrameIndex & 0xFF;

            Buffer[16] = FrameType;

            if (RemainSize > (VIDEO_PACKAGE_SIZE_MAX - 17))
            {
                memcpy(&Buffer[17], Data, VIDEO_PACKAGE_SIZE_MAX - 17);

                if (sendto(NetVideoFd, Buffer, VIDEO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)&VideoServAddr, sizeof(VideoServAddr)) < 0)
                {
                    perror("Video Package send to fail  !!!!!~~~~!!!\n");
                }
                RemainSize -= (VIDEO_PACKAGE_SIZE_MAX - 17);
                SendSize += (VIDEO_PACKAGE_SIZE_MAX - 17);
            }
            else
            {
                memcpy(&Buffer[17], Data, RemainSize);
                if (sendto(NetVideoFd, Buffer, RemainSize + 17, 0, (struct sockaddr *)&VideoServAddr, sizeof(VideoServAddr)) < 0)
                {
                    perror("Video Package send to fail  \n");
                }
                break;
            }
        }
        usleep(1000);
        if (RemainSize > VIDEO_PACKAGE_SIZE_MAX)
        {
            if (sendto(NetVideoFd, &Data[SendSize], VIDEO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)&VideoServAddr, sizeof(VideoServAddr)) < 0)
            {
                perror("Video Package send to fail  \n");
            }
            RemainSize -= (VIDEO_PACKAGE_SIZE_MAX);
            SendSize += (VIDEO_PACKAGE_SIZE_MAX);
        }
        else
        {
            if (sendto(NetVideoFd, &Data[SendSize], RemainSize, 0, (struct sockaddr *)&VideoServAddr, sizeof(VideoServAddr)) < 0)
            {
                perror("Video Package send to fail  \n");
            }
            break;
        }
    }
}

static int NetVideoTransferOpen(void)
{
    assert(NetVideoFd == -1);
    if ((NetVideoFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("create socket error !!!\n");
        return -1;
    }

    if (InetNetInterfaceBind(NetVideoFd, NETWORK_INTERFACE_NAME) < 0)
    {
        perror("InetNetInterfaceBind fail !!!\n");
        return -1;
    }

    if (InetSockConfig(NetVideoFd, SO_REUSEADDR | SO_BROADCAST, 1) < 0)
    {
        perror("InetSockConfig fail !!!\n");
        return -1;
    }

    return 0;
}

/**
 * @description: 视频传输网络协议
 * @param {int} SlaveId 从机设备ID
 * @return {*}
 */
int VideoNetSocketProtocolSet(int SlaveId)
{
#define VIDEO_NET_PROTOCOL_BASE 0X1600
#define NetworkSlaveId(x) ((x & 0x00) | ((x) - 1))
    return (NetVideoProtocol = VIDEO_NET_PROTOCOL_BASE | NetworkSlaveId(SlaveId));
}

/**
 * @description: 视频网络传输初始化
 * @return {*}
 */
int NetVideoTransferInit(void)
{
    if (NetVideoTransferOpen() == -1)
        return -1;

    VideoInputInit();

#define MOTION_DETECT_TEST
#ifdef MOTION_DETECT_TEST
    SmartVisionPlatformInit(VideoInputChnAttrGet(VI_CHN_TRD));
#endif
    return 0;
}
