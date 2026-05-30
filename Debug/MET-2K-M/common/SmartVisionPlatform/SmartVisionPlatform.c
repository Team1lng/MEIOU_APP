/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-21 11:14:42
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-11 14:42:48
 * @FilePath: /project_3/common/SmartVisionPlatform/SmartVisionPlatform.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "SmartVisionPlatform.h"
#include "MotionDetect.h"
#include "ak_object_filter.h"
#include "ak_thread.h"
#include "ak_mem.h"
#include "ak_log.h"
#include "ak_md.h"
#include "string.h"

#define SVP_PATH "/usr/sbin/dt320_param.bin"
#define OBJECT_FILTER_ENABLE
static AK_SVP_CHN_ATTR_T SvpChnAttr;
static SVP_PARAM SvpParam;

typedef struct BOX
{
    int x1;
    int y1;
    int x2;
    int y2;
} BOX;

/**
 * @description:  SVP模块参数初始化 [需要函数重定义]
 * @param {VI_CHN_ATTR_EX} info   通道属性结构
 * @param {SVP_PARAM} SvpParam   SVP属性结构
 * @return {*}
 */
__attribute__((weak)) int SmartVisionPlatformPrameInit(AK_SVP_CHN_ATTR_T *SvpChnAttr, SVP_PARAM *SvpParam)
{
    SvpChnAttr->target_type = AK_SVP_HUMAN_SHAPE_AND_FACE;
    SvpChnAttr->model_type = AK_SVP_MODEL_NORMAL;
    // /* set svp classsify threshold, default is 700, range[400-900], the less, the more sensitive, but the less accurate */
    SvpChnAttr->threshold.classify_threshold = 700;
    /* set the IoU threshold, range [3-10], default is 3*/
    SvpChnAttr->threshold.IoU_threshold = 3;

    SvpParam->Dev = VIDEO_DEV0;            // 使用视频设备0
    SvpParam->ChnId = SVP_CHN_0;           // 通道ID为0
    SvpParam->SvpRate = 4;                 // 处理帧率比例（每4帧处理1帧）
    SvpParam->FilterOpt = SVP_FILTER_MD;   // 过滤选项：运动检测
    SvpParam->SvpThrld = 7;                // SVP通用阈值
    SvpParam->SvpMdThrld = 40;             // 运动检测阈值
    SvpParam->EnableCallback = 1;          // 启用回调函数
    return AK_FAILED;
}

/**
 * @description:  SVP触发回调 [需要函数重定义]
 * @param {int} Total   总输出块数
 * @param {AK_SVP_RECT_T} Src   人行坐标信息 [NULL -  检测无效]   [!NULL - 检测有效]
 * @return {*}
 */
__attribute__((weak)) int SmartVisionPlatformCallback(int Total, AK_SVP_RECT_T *Src)
{
    for (int j = 0; j < Total; j++)
    {
        ak_print_normal(MODULE_ID_SVP, "SVP %s [%d] :Mobile [%s] detect, score[%lu],Res is:left[%lu],top[%lu],right[%lu]:bottom[%lu]\n",
                        SvpParam.FilterOpt == NOLY_SVP ? "" : SvpParam.FilterOpt == SVP_MD ? " Alarm"
                                                                                           : "Filter Alarm",
                        j, Src->label == AK_SVP_HUMAN_SHAPE ? "Body" : "Face", Src->score, Src->left, Src->top, Src->right, Src->bottom);
    }
    return AK_SUCCESS;
}

/*
 * * @BRIEF  计算两个矩形框的交集分针占源BOX的百分比
 * * @PARAM  a: one box
 * * @PARAM  b: one box
 * * @PARAM  r_IOUa: IoU ratio of Box a
 * * @PARAM  r_IOUb: IoU ratio of Box b
 * * @NOTE:
 * */
static void compute_iou2(BOX a, BOX b, double *r_IOUa, double *r_IOUb)
{
    int MaxX = 0;
    int MaxY = 0;
    int MinX = 0;
    int MinY = 0;
    double IOU_I;
    double A, B;
    double IOUA, IOUB;
    // ak_print_normal_ex(MODULE_ID_SVP, "A[%d,%d,%d,%d],B[%d,%d,%d,%d]\n", a.x1, a.y1, a.x2, a.y2, b.x1, b.y1, b.x2, b.y2);
    MaxX = (a.x1 > b.x1) ? a.x1 : b.x1;
    MaxY = (a.y1 > b.y1) ? a.y1 : b.y1;
    MinX = (a.x2 < b.x2) ? a.x2 : b.x2;
    MinY = (a.y2 < b.y2) ? a.y2 : b.y2;

    MaxX = ((MinX - MaxX) > 0) ? (MinX - MaxX) : 0;
    MaxY = ((MinY - MaxY) > 0) ? (MinY - MaxY) : 0;

    IOU_I = MaxX * MaxY;
    A = (a.x2 - a.x1) * (a.y2 - a.y1);
    B = (b.x2 - b.x1) * (b.y2 - b.y1);

    // ak_print_normal_ex(MODULE_ID_SVP, "IOU_I[%lf], A[%lf],B[%lf]\n",IOU_I, A,B);
    IOUA = IOU_I / A;
    IOUB = IOU_I / B;

    *r_IOUa = IOUA;
    *r_IOUb = IOUB;
}

static void *SmartVisionPlatformThread(void *arg)
{
    VI_CHN_ATTR_EX *ViChnAttr = (VI_CHN_ATTR_EX *)arg;

    int count = 0;
    int SvpRate = 3;
    struct video_input_frame Frame;
    AK_SVP_IMG_INFO_T Input = {0};

    /* fill in input structure info */
    Input.img_type = (ViChnAttr->data_type == VI_DATA_TYPE_YUV420SP ? AK_SVP_IMG_YUV420SP : AK_SVP_IMG_RGB_LI);
    Input.width = Input.pos_info.width = ViChnAttr->res.width;
    Input.height = Input.pos_info.height = ViChnAttr->res.height;
    Input.pos_info.left = 0;
    Input.pos_info.top = 0;
    unsigned long ViCnt = 0;

    OBJECT_BOX *ObjBox = NULL;
    void *FilterHandle = NULL;
    if (SvpParam.FilterOpt == SVP_FILTER_MD)
    {
        FilterHandle = ak_object_filter_init();
        ak_object_filter_set_frames(FilterHandle, 3, 2, 4);
        ak_object_filter_set_distance_enhancement_params(FilterHandle, 2, 8, 6);
        ak_object_filter_set_md_level(FilterHandle, 2);
        ak_object_filter_set_continous_enhancement_params(FilterHandle, 1, 5);
        ak_object_filter_set_false_record_params(FilterHandle, 1, 7);
        ak_print_notice(MODULE_ID_SVP, "libfilter version = %s\r\n", ak_object_filter_get_version());

        ObjBox = ak_mem_alloc(MODULE_ID_SVP, sizeof(OBJECT_BOX) * OBJECT_CAP);
        memset(ObjBox, 0, sizeof(OBJECT_BOX) * OBJECT_CAP);
    }

    ak_print_normal(MODULE_ID_SVP, "SVP Process start\n");
    /*
     * To get frame by loop
     */
    while (1)
    {
        memset(&Frame, 0x00, sizeof(Frame));

        /* to get frame according to the channel number */
        int Ret = ak_vi_get_frame(ViChnAttr->chn_id, &Frame);
        if (!Ret)
        {
            if (SvpRate != 0 && ++ViCnt % SvpRate == 0)
            {
                AK_SVP_OUTPUT_T Output = {0};
                Input.vir_addr = Frame.vi_frame.data;
                Input.phy_addr = Frame.phyaddr;
                /* invoke the svp process to do detection */
                Ret = ak_svp_process(SvpParam.ChnId, &Input, &Output);
                if (Ret == AK_SUCCESS)
                {
                    if (Output.total_num > 0)
                    {
                        /* draw forever */
                        int i = 0;

                        /* printf the svp process result base on the detect chn resolution, the third chn, chn16*/
                        if (SvpParam.FilterOpt == NOLY_SVP)
                        {
                            goto CALLBACK;
                        }
                        else if (SvpParam.FilterOpt == SVP_FILTER_MD)
                        {
                            for (; i < Output.total_num; i++)
                            {
                                AK_SVP_RECT_T *Src = &Output.target_boxes[i];
                                ObjBox[i].left = Src->left;
                                ObjBox[i].top = Src->top;
                                ObjBox[i].right = Src->right;
                                ObjBox[i].bottom = Src->bottom;
                                ObjBox[i].count = 1;
                                ObjBox[i].md = 0;
                            }
                        }

                        /* if Md is enable, used Md result to help to do the correction,
                        and will increase the accuracy of the mobile humam detection*/
                        if (SvpParam.FilterOpt != NOLY_SVP)
                        {
                            struct md_result_info RetInfo = {0};
                            /* get the Md result */
                            if (AK_SUCCESS == MotionDetectGetResult(SvpParam.Dev, &RetInfo, 0))
                            {
                                /* get Md result success and check the valid Md boxes */
                                if (RetInfo.result > 0)
                                {
                                    int j = 0;
                                    /* go through each svp info*/
                                    for (j = 0; j < Output.total_num; j++)
                                    {
                                        AK_SVP_RECT_T *Src = &Output.target_boxes[j];
                                        BOX Svp = {0};
                                        Svp.x1 = Src->left;
                                        Svp.y1 = Src->top;
                                        Svp.x2 = Src->right;
                                        Svp.y2 = Src->bottom;
                                        /* go throuth each Md box to do the correction*/
                                        for (i = 0; i < RetInfo.move_box_num; i++)
                                        {
                                            BOX Md = {0};
                                            /* convert the MD result to MD box , according to the chn resolution that SVP used*/
                                            Md.x1 = RetInfo.boxes[i].left * ViChnAttr->res.width / 32;
                                            Md.y1 = RetInfo.boxes[i].top * ViChnAttr->res.height / 24;
                                            Md.x2 = (RetInfo.boxes[i].right == RetInfo.boxes[i].left ? RetInfo.boxes[i].right + 1 : RetInfo.boxes[i].right) * ViChnAttr->res.width / 32;
                                            Md.y2 = (RetInfo.boxes[i].bottom == RetInfo.boxes[i].top ? RetInfo.boxes[i].bottom + 1 : RetInfo.boxes[i].bottom) * ViChnAttr->res.height / 24;
                                            double IoU_md = 0, IoU_svp = 0;
                                            /*calc the MD box and SVP box IOU value*/
                                            compute_iou2(Md, Svp, &IoU_md, &IoU_svp);
                                            // ak_print_normal_ex(MODULE_ID_SVP,"IoU_MD[%lf],Iou_SVP[%lf]\n", IoU_md, IoU_svp);

                                            /* it the percentage of the overlapped area for MD and SVP are over the threshold,mobile human detect */
                                            if (IoU_svp * 100 >= SvpParam.SvpThrld && IoU_md * 100 >= SvpParam.SvpMdThrld)
                                            {
                                                /* mobile human detection , user can invode the alarm process */
                                                // ak_print_normal(MODULE_ID_SVP, "SVP Alarm[%d] :Mobile [%s] detect, score[%lu],Res is:left[%lu],top[%lu],right[%lu]:bottom[%lu]\n",
                                                //                 j, Src->label == AK_SVP_HUMAN_SHAPE ? "Body" : "Face", Src->score, Src->left, Src->top, Src->right, Src->bottom);

                                                if (SvpParam.FilterOpt == SVP_FILTER_MD)
                                                {
                                                    ObjBox[j].md = 1;
                                                }
                                                else
                                                {
                                                    goto CALLBACK;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (SvpParam.FilterOpt == SVP_FILTER_MD)
                                {
                                    int Ret = -1;
                                    if ((Ret = ak_object_filter_alarm(FilterHandle, ObjBox, Output.total_num)) != 0)
                                    {
                                        goto CALLBACK;
                                    }
                                }
                            }
                        }
                        goto RELEASE;
                    }

                    SmartVisionPlatformCallback(Output.total_num, NULL);
                    goto RELEASE;

                CALLBACK:
                    if (SvpParam.EnableCallback)
                    {
                        // printf("[%s][%d]total_num:%d,boxes:%p\n", __func__, __LINE__, Output.total_num, Output.target_boxes);
                        SmartVisionPlatformCallback(Output.total_num, Output.target_boxes);
                    }
                RELEASE:
                    /* release Svp result*/
                    Ret = ak_svp_release(&Output);
                    if (Ret != AK_SUCCESS)
                    {
                        ak_print_error_ex(MODULE_ID_SVP, "SVP rlease Output failed!\n");
                    }
                }
                else
                {
                    ak_svp_reset_hw();
                }
                /*
                 * in this context, this frame was useless,
                 * release frame data
                 */
                count++;
            }
            ak_vi_release_frame(ViChnAttr->chn_id, &Frame);
            ak_sleep_ms(10);
        }
        else
        {
            /*
             *    If getting too fast, it will have no data,
             *    just take breath.
             */
            ak_print_normal_ex(MODULE_ID_SVP, "get frame failed!\n");
            ak_sleep_ms(10);
        }
    }

#ifdef OBJECT_FILTER_ENABLE
    if (ObjBox != NULL)
    {
        ak_mem_free(ObjBox);
        ObjBox = NULL;
    }
    ak_object_filter_destroy(FilterHandle);

#endif

    ak_print_normal(MODULE_ID_SVP, "capture finish\n\n");
    ak_thread_exit();
    return NULL;
}

/**
 * @description: 智能视觉平台初始化
 * @param {VI_CHN_ATTR_EX} *ViChnAttr   视频采集通道属性
 * @return {*}
 */
int SmartVisionPlatformInit(VI_CHN_ATTR_EX *ViChnAttr)
{
    if (ViChnAttr == NULL)
        return -1;

    int Ret = AK_FAILED;
    /* init the svp attr */
    SmartVisionPlatformPrameInit(&SvpChnAttr, &SvpParam);

    Ret = ak_svp_init();
    if (Ret != AK_SUCCESS)
    {
        ak_print_error_ex(MODULE_ID_SVP, "Init SVP failed!\n");
        return Ret;
    }

    Ret = ak_svp_create_chn(SvpParam.ChnId, &SvpChnAttr, NULL, SVP_PATH);
    if (Ret != AK_SUCCESS)
    {
        ak_print_error_ex(MODULE_ID_SVP, "Create SVP chn failed\n");
        ak_svp_deinit();
        return Ret;
    }
    /* create the svp_process*/
    ak_pthread_t thread;
    if (ak_thread_create(&thread, SmartVisionPlatformThread,
                         (void *)ViChnAttr, ANYKA_THREAD_NORMAL_STACK_SIZE, -1))
    {

        ak_print_error_ex(MODULE_ID_MD, "create svp detect thread failed.\n");
        return AK_FAILED;
    }
    ak_thread_detach(thread);

    if (SvpParam.FilterOpt != NOLY_SVP)
    {
        int Ret = MotionDetectInit(SvpParam.Dev);
        if (Ret)
        {
            SvpParam.FilterOpt = NOLY_SVP;
        }
        else
        {
            MotionDetectEnable(0, 1);
        }
    }
    return Ret;
}

/**
 * SvpMdFiltersSet -   set filters for motion region detection
 * @FltBig[IN]: big filter to roughly detect motion region
 * @FltSmall[IN]: small filter to expand rough detection result
 * return: 0 - success;
 */
int SvpMdFiltersSet(int FltBig, int FltSmall)
{
    if (!FltBig || !FltSmall)
    {
        SvpParam.EnableCallback = 0;
        return 0;
    }

    SvpParam.EnableCallback = 1;
    return MotionDetectFiltersSet(SvpParam.Dev, FltBig, FltSmall);
}