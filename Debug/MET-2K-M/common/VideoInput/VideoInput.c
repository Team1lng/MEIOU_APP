/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-15 16:07:30
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-18 14:22:32
 * @FilePath: /project_3/common/VideoInput/VideoInput.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
#include "VideoInput.h"
#include <stdatomic.h>

#include <unistd.h>

static int VencId = -1;
static int CurChn = VIDEO_CHN0;
static VideoInputParam Param = {.DevId = -1};
static atomic_int VideoModeFlag = ATOMIC_VAR_INIT(0);
static atomic_int VideoReqIdrFlag = ATOMIC_VAR_INIT(0);
static pthread_mutex_t video_input_mutex;
static bool is_video_input_enable = false;
/**
 * @description:  视频采集参数初始化 [需要函数重定义]
 * @param {VideoInputParam} param   视频采集参数
 * @return {*}
 */
__attribute__((weak)) int VideoInputParamInit(VideoInputParam *param)
{
    printf("This is the weak VideoInputParamInit function.\n");
    return AK_FAILED;
}

/**
 * @description: 视频数据写入 [需要函数重定义]
 * @param {VoiceInfo} info   视频数据
 * @return {*}
 */
__attribute__((weak)) int VideoStreamExport(struct video_stream *stream)
{
    printf("This is the weak VideoStreamExport function.\n");
    return AK_FAILED;
}

/**
 * @description: 获取视频采集通道属性
 * @param {VI_CHN_NUM} Chn  通道号
 * @return {*}
 */
VI_CHN_ATTR_EX *VideoInputChnAttrGet(VI_CHN_NUM Chn)
{
    return &(Param.DevChn[Chn].ChnAttr);
}

/**
 * @description: 请求编码关键帧
 * @return {*}
 */
void VideoKeyFrameRequest(void)
{
    atomic_store(&VideoReqIdrFlag, 1);
}

static int VideoInputDevInit(void)
{
    /*
     * step 1: open video input device
     */
    printf("[=======%s:%d===========]\n",__func__,__LINE__);
    if (ak_vi_open(Param.DevId))
    {
        printf("ak_vi_open fail!!!\n");
        goto VI_FAIL;
    }

    /*
     * step 2: load isp config
     */
    printf("[=======%s:%d===========]\n",__func__,__LINE__);
    if (ak_vi_load_sensor_cfg(Param.DevId, Param.IspPath))
    {
        printf("ak_vi_load_sensor_cfg fail!!!\n");
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }

    /*
     * step 3: get sensor support max resolution
     */
    RECTANGLE_S Res; // max sensor resolution
    VI_DEV_ATTR DevAttr;
    memset(&DevAttr, 0, sizeof(VI_DEV_ATTR));
    DevAttr.dev_id = Param.DevId;
    /* init the interface mode according to the input param*/
    DevAttr.max_width = Param.DevChn[VI_CHN_MAIN].ChnAttr.res.width;
    DevAttr.max_height = Param.DevChn[VI_CHN_MAIN].ChnAttr.res.height;
    DevAttr.sub_max_width = Param.DevChn[VI_CHN_SUB].ChnAttr.res.width;
    DevAttr.sub_max_height = Param.DevChn[VI_CHN_SUB].ChnAttr.res.height;
    DevAttr.crop.width = Param.DevChn[VI_CHN_MAIN].ChnAttr.res.width;
    DevAttr.crop.height = Param.DevChn[VI_CHN_MAIN].ChnAttr.res.height;
    printf("[=======%s:%d===========]\n",__func__,__LINE__);
    /* get sensor resolution */
    if (ak_vi_get_sensor_resolution(Param.DevId, &Res))
    {
        printf("Can't get dev[%d]resolution\n", Param.DevId);
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }
    else
    {
        printf("get dev res w:[%d]h:[%d]\n", Res.width, Res.height);
        if (DevAttr.crop.width > Res.width)
            DevAttr.crop.width = Res.width;
        if (DevAttr.crop.height > Res.height)
            DevAttr.crop.height = Res.height;
    }

    /*
     * step 4: set vi device working parameters
     * default parameters: 25fps, day mode
     */
    printf("[=======%s:%d===========]\n",__func__,__LINE__);
    if (ak_vi_set_dev_attr(Param.DevId, &DevAttr))
    {
        printf("vi device %d set device attribute failed!\n", Param.DevId);
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }
    printf("[=======%s:%d===========]\n",__func__,__LINE__);
    /*
     * step 5: set main channel attribute
     */
    if (ak_vi_set_chn_attr_ex(Param.DevChn[VI_CHN_MAIN].ChnAttr.chn_id, &Param.DevChn[VI_CHN_MAIN].ChnAttr))
    {
        printf("vi device %d set channel [%d] attribute failed!\n", Param.DevId, Param.DevChn[VI_CHN_MAIN].ChnAttr.chn_id);
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }

    /*
     * step 6: set sub channel attribute
     */
    if (ak_vi_set_chn_attr_ex(Param.DevChn[VI_CHN_SUB].ChnAttr.chn_id, &Param.DevChn[VI_CHN_SUB].ChnAttr))
    {
        printf("vi device %d set channel [%d] attribute failed!\n", Param.DevId, Param.DevChn[VI_CHN_SUB].ChnAttr.chn_id);
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }

    /*
     * step 7: set third channel attribute
     */
    if (ak_vi_set_chn_attr_ex(Param.DevChn[VI_CHN_TRD].ChnAttr.chn_id, &Param.DevChn[VI_CHN_TRD].ChnAttr))
    {
        printf("vi device %d set channel [%d] attribute failed!\n", Param.DevId, Param.DevChn[VI_CHN_TRD].ChnAttr.chn_id);
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }

    /*
     * step 8: enable vi device
     */
    if (ak_vi_enable_dev(Param.DevId))
    {
        printf("vi device %d enable device  failed!!\n", Param.DevId);
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }

    /*
     * step 9: enable vi main channel
     */
    if (Param.DevChn[VI_CHN_MAIN].EnChn)
    {
        if (ak_vi_enable_chn(Param.DevChn[VI_CHN_MAIN].ChnAttr.chn_id))
        {
            printf("vi channel[%d] enable failed!\n", Param.DevChn[VI_CHN_MAIN].ChnAttr.chn_id);
            ak_vi_close(Param.DevId);
            goto VI_FAIL;
        }
    }

    /*
     * step 10: enable vi sub channel
     */
    if (Param.DevChn[VI_CHN_SUB].EnChn)
    {
        if (ak_vi_enable_chn(Param.DevChn[VI_CHN_SUB].ChnAttr.chn_id))
        {
            printf("vi channel[%d] enable failed!\n", Param.DevChn[VI_CHN_SUB].ChnAttr.chn_id);
            ak_vi_close(Param.DevId);
            goto VI_FAIL;
        }
    }

    /*
     * step 11: enable vi third channel
     */
    if (Param.DevChn[VI_CHN_TRD].EnChn)
    {
        if (ak_vi_enable_chn(Param.DevChn[VI_CHN_TRD].ChnAttr.chn_id))
        {
            printf("vi channel[%d] enable failed!\n", Param.DevChn[VI_CHN_TRD].ChnAttr.chn_id);
            ak_vi_close(Param.DevId);
            goto VI_FAIL;
        }
    }

    if (ak_venc_open(&Param.VencParam, &VencId))
    {
        printf("ak_venc_open fail!!!\n");
        ak_vi_close(Param.DevId);
        goto VI_FAIL;
    }

    unsigned int ColorTable[] = {
        0xff00ff00};
    ak_vi_set_box_color_table(Param.DevId, ColorTable);
    return AK_SUCCESS;

VI_FAIL:
    return AK_FAILED;
}

static int VideoModuleDetect(const char *ModuleName)
{
    char Cmd[256];
    FILE *Fp;

    snprintf(Cmd, sizeof(Cmd), "lsmod | grep ^%s ", ModuleName);
    Fp = popen(Cmd, "r");

    if (Fp == NULL)
    {
        fprintf(stderr, "Failed to run lsmod command\n");
        return -1;
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer) - 1, Fp) != NULL)
    {
        pclose(Fp);
        return 0; // 模块已加载
    }

    pclose(Fp);
    return -1;
}

static int VideoModuleLoad(const char *ModulePath, const char *ModuleName, const char *ModuleParam)
{
    if (ModuleName == NULL || strlen(ModuleName) == 0 || ModulePath == NULL || strlen(ModulePath) == 0)
    {
        fprintf(stderr, "Invalid module path or name\n");
        return -1;
    }

    // 检查模块是否已加载
    if (VideoModuleDetect(ModuleName) == 0)
    {
        printf("Module %s is already loaded\n", ModuleName);
        return 0; // 模块已加载，无需重新加载
    }

    // 尝试加载模块
    char Cmd[64] = {0};
    snprintf(Cmd, sizeof(Cmd), "insmod %s%s.ko %s", ModulePath, ModuleName, ModuleParam ? ModuleParam : "");

    int Ret = system(Cmd);
    if (Ret != 0)
    {
        fprintf(stderr, "Failed to load module %s: %s\n", ModuleName, strerror(errno));
        return -1;
    }

    printf("Module %s loaded successfully\n", ModuleName);
    return 0;
}

static int VideoInputOpen(void)
{
    int ret = AK_FAILED;
    if (VideoModuleLoad(VIDEO_MODULE_PATH, VIDEO_ISP_MODLE_KO, NULL))
    {
        return AK_FAILED;
    }
    if (VideoModuleLoad(VIDEO_MODULE_PATH, VIDEO_SNESOR_MODLE_KO, VIDEO_SNESOR_MODLE_DAAR))
    {
        return AK_FAILED;
    }

    usleep(2000*1000);

    if (VideoInputParamInit(&Param) == AK_FAILED)
    {
        printf("Video Input Param Uninitialized!!!\n");
        return ret;
    }
    if ((ret = VideoInputDevInit()) == AK_FAILED)
    {
        Param.DevId = -1;
    }
    return ret;
}

static void *VideoInputThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    struct video_input_frame Frame;
    struct video_stream Stream;
    static int CurrVideoModeFlag = 0;
    while (1)
    {
     if((is_video_input_enable == true))
     {
        memset(&Frame, 0x00, sizeof(struct video_input_frame));
        if (ak_vi_get_frame(CurChn, &Frame) == AK_SUCCESS)
        {
            memset(&Stream, 0, sizeof(struct video_stream));
            if (ak_venc_encode_frame(VencId, Frame.vi_frame.data, Frame.vi_frame.len, NULL, &Stream) == AK_SUCCESS)
            {
                if ((Stream.data) && (Stream.len > 0))
                {
                    VideoStreamExport(&Stream);
                    ak_venc_release_stream(VencId, &Stream);
                }
            }
            ak_vi_release_frame(CurChn, &Frame);

            if(atomic_load(&VideoReqIdrFlag))
            {
                atomic_store(&VideoReqIdrFlag, 0);
                ak_venc_request_idr(VencId);
            }

            if(atomic_load(&VideoModeFlag) != CurrVideoModeFlag)
            {
                CurrVideoModeFlag = atomic_load(&VideoModeFlag);
                ak_vi_switch_mode(Param.DevId, CurrVideoModeFlag);
            }
        }
    }
    ak_sleep_ms(10);
 }
    return NULL;
}

/**
 * @description: 切换夜间模式
 * @param {int} daynight    夜间模式标志
 * @return {*}
 */
void VideoSwitchMode(int daynight)
{
    atomic_store(&VideoModeFlag, daynight);
}

/**
 * @description: 视频采集初始化
 * @return {*}
 */
int VideoInputInit(void)
{
    if (VideoInputOpen() == AK_FAILED)
    {
        return -1;
    }
    pthread_t thread;
    pthread_create(&thread, NULL, VideoInputThread, NULL);
    pthread_detach(thread);
    return 0;
}

/****************************************hare set (视频信息传输标志位)********************************************************************/
/***
**	date:2025.10.13
**	author:hare
**	打开视频采集设备
***/
static bool video_input_open(void)
{
	pthread_mutex_lock(&video_input_mutex);
	if (is_video_input_enable == true)
	{
		pthread_mutex_unlock(&video_input_mutex);
		return true;
	}

	is_video_input_enable = true;
	pthread_mutex_unlock(&video_input_mutex);
	return true;
}

/***
**  date:2025.10.13
**  author:hare
**  关闭视频采集设备
***/
static bool video_input_close(void)
{
	pthread_mutex_lock(&video_input_mutex);
	if (is_video_input_enable == false)
	{
		pthread_mutex_unlock(&video_input_mutex);
		return false;
	}
	is_video_input_enable = false;
	pthread_mutex_unlock(&video_input_mutex);
	return true;
}

static int network_video_send_ready = 0;
int is_network_video_send_package_open(void)
{
	return network_video_send_ready;
}

void network_video_send_package_start(void)
{
	printf("==========>>> video send package start <<<==========\n");
	network_video_send_ready = true;
	video_input_open();
	// set_network_i_frame_request_param(true);
}

void network_video_send_package_stop(void)
{
	printf("==========>>> video send package stop <<<==========\n");
	network_video_send_ready = false;
	video_input_close();
}
