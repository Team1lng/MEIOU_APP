/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-21 15:08:42
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-18 14:22:27
 * @FilePath: /project_3/common/SmartVisionPlatform/MotionDetect.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "MotionDetect.h"
#include <stdlib.h>
#include <string.h>

#include "ak_log.h"
#include "ak_mem.h"
#include "ak_thread.h"
#include "ak_vpss.h"
#include "ak_mrd.h"

enum
{
    MD_THREAD_EXIT = 0,
    MD_THREAD_STOP,
    MD_THREAD_RUN,
};

struct HwMdResult
{
    ak_mutex_t Lock;
    struct md_result_info RetInfo;
};

struct HwMdCtrl
{
    int ThreadStat;
    ak_pthread_t MdTid;
    ak_sem_t SendSem;
    int Dev;
    int MdFps; // msecond. interval time data to detect
    void *MrdHandle;
};

static struct HwMdResult MdResult[VIDEO_DEV_MUX];
static struct HwMdCtrl MdCtrl[VIDEO_DEV_MUX];
static int MdInitFlag[VIDEO_DEV_MUX] = {0};
static struct SvpMdParam Param[VIDEO_DEV_MUX];

/**
 * @description:  SVP移动侦测模块参数初始化 [需要函数重定义]
 * @param {int} DevId   设备号
 * @param {SvpMdParam} Param   移动侦测属性结构
 * @return {*}
 */
__attribute__((weak)) int SvpMdParamInit(int DevId, struct SvpMdParam *Param)
{
    Param->DevId = DevId;
    Param->MoveSizeMin = 2;
    Param->MoveSizeMax = 300;
    Param->MoveSizeMin = 1500;
    Param->FltSmall = 1500;
    Param->FltBig = 7000;
    Param->FltSmall = 3500;
    ak_vpss_get_sensor_fps(DevId, &Param->MdFps);
    return AK_SUCCESS;
}

/**
 * @description:  SVP移动侦测触发回调 [需要函数重定义]
 * @param {int} DevId   设备号
 * @return {*}
 */
__attribute__((weak)) int MotionDetectCallBack(int DevId)
{
    return AK_SUCCESS;
}

/**
 * MotionDetectGetStat: get motion detection stat params
 * @Dev[IN]: Dev id
 * @md[OUT]: md params
 * return: 0 - success; other error code;
 * notes:
 */
static int MotionDetectGetStat(int Dev, struct vpss_md_info *Md)
{
    if (Dev >= VIDEO_DEV_MUX)
    {
        ak_print_error_ex(MODULE_ID_MD, "Dev:%d error!\n", Dev);
        return ERROR_TYPE_INVALID_ARG;
    }

    if (NULL == Md)
    {
        ak_print_error_ex(MODULE_ID_MD, "md is null!\n");
        return ERROR_TYPE_POINTER_NULL;
    }

    /*get isp 3d stat info*/
    int Ret = ak_vpss_md_get_stat(Dev, Md);

    if (0 != Ret)
    {
        ak_print_error_ex(MODULE_ID_MD, "get 3d nr stat info fail\n");
    }

    return Ret;
}

/**
 * MotionDetectCheck: judge function
 * @MdCtrl[IN]: md ctrl struct pointer
 * return: 0 no move, 1 has move
 * notes:
 */
static int MotionDetectCheck(struct HwMdCtrl *MdCtrl)
{
    int Ret = 0;
    int Hasmove = 0;
    struct vpss_md_info Md = {{{0}}};
    MRD_RECTANGLE boxes[MAX_CAP] = {{0}};
    int Dev = MdCtrl->Dev;

    if (MotionDetectGetStat(Dev, &Md))
    {
        ak_print_warning_ex(MODULE_ID_MD, "get md stat fail.\n");
        return 0;
    }

    Ret = ak_mrd(MdCtrl->MrdHandle, Md.stat, VPSS_MD_DIMENSION_V_MAX, boxes);

    if (Ret > 0)
    {
        // ak_print_normal(MODULE_ID_MD, "box 0: [%d, %d, %d, %d]\n",
        // boxes[0].left, boxes[0].top, boxes[0].right, boxes[0].bottom);
        ak_thread_mutex_lock(&MdResult[Dev].Lock);

        memset(&MdResult[Dev].RetInfo.boxes, 0, MAX_BOX_NUM * sizeof(struct MdBox));
        if (Ret > MAX_BOX_NUM)
            MdResult[Dev].RetInfo.move_box_num = MAX_BOX_NUM;
        else
            MdResult[Dev].RetInfo.move_box_num = Ret;

        for (int i = 0; i < MdResult[Dev].RetInfo.move_box_num; i++)
        {
            MdResult[Dev].RetInfo.boxes[i].left = boxes[i].left;
            MdResult[Dev].RetInfo.boxes[i].top = boxes[i].top;
            MdResult[Dev].RetInfo.boxes[i].right = boxes[i].right;
            MdResult[Dev].RetInfo.boxes[i].bottom = boxes[i].bottom;
        }

        ak_thread_mutex_unlock(&MdResult[Dev].Lock);
        Hasmove = 1;
    }

    return Hasmove;
}

/**
 * MotionDetectThread - move detect thread, do detect thing here
 * compare move detect success over 2 times, then one detect message is emit really
 */
static void *MotionDetectThread(void *Arg)
{
    int DetectInterval = 0;
    struct ak_timeval MdTv = {0}, NowTv = {0};
    struct ak_date Date = {0};
    int MoveCount = 0;

    ak_thread_set_name("svp_md");

    ak_print_normal_ex(MODULE_ID_MD, "thread id : %ld\n", ak_thread_get_tid());

    int Dev = *(int *)Arg;

    while (MdCtrl[Dev].ThreadStat)
    {
        DetectInterval = 1000 / MdCtrl[Dev].MdFps;

        /*
         * stop mode ,don't run to check, wait here
         */
        do
        {
            ak_sleep_ms(DetectInterval);
            if (MD_THREAD_STOP == MdCtrl[Dev].ThreadStat)
            {
                MoveCount = 0;
            }
        } while (MD_THREAD_STOP == MdCtrl[Dev].ThreadStat);

        if (MotionDetectCheck(&MdCtrl[Dev]))
        {
            MoveCount++;
            if (MoveCount >= 1)
            {
                MotionDetectCallBack(Dev);
                MoveCount = 0;
                ak_get_ostime(&MdTv);
                ak_get_localdate(&Date);
                ak_thread_mutex_lock(&MdResult[Dev].Lock);
                MdResult[Dev].RetInfo.md_sec = ak_date_to_seconds(&Date);
                MdResult[Dev].RetInfo.result = 1;
                ak_thread_mutex_unlock(&MdResult[Dev].Lock);

                ak_thread_sem_post(&MdCtrl[Dev].SendSem);
            }
        }
        else
        {
            /*
             * alarm module do not take away md result and it is over 4 second,
             * clean it
             */
            ak_thread_mutex_lock(&MdResult[Dev].Lock);
            if (1 == MdResult[Dev].RetInfo.result)
            {
                ak_get_ostime(&NowTv);
                if ((NowTv.sec > (MdTv.sec + 4)) || (NowTv.sec < MdTv.sec))
                {
                    MdResult[Dev].RetInfo.result = 0;
                }
            }
            ak_thread_mutex_unlock(&MdResult[Dev].Lock);
            MoveCount = 0;
        }
    }

    ak_print_normal_ex(MODULE_ID_MD, "### thread id: %ld exit ###\n",
                       ak_thread_get_tid());
    ak_thread_exit();
    return NULL;
}

/**
 * MotionDetectInit - md init .
 * @param[IN]:  SvpMdParam
 * return: 0 - success; other error code;
 */
int MotionDetectInit(int DevId)
{

    SvpMdParamInit(DevId, &Param[DevId]);

    int Dev = DevId;

    if (MdInitFlag[Dev])
    {
        ak_print_error_ex(MODULE_ID_MD, "have been init \n");
        return AK_SUCCESS;
    }

    memset(&MdCtrl[Dev], 0, sizeof(struct HwMdCtrl));
    memset(&MdResult[Dev], 0, sizeof(struct HwMdResult));

    /* init the ak_mrd lib*/
    MdCtrl[Dev].MrdHandle = ak_mrd_init();
    if (NULL == MdCtrl[Dev].MrdHandle)
    {
        ak_print_error_ex(MODULE_ID_MD, "ak_mrd_init failed!\n");
        return AK_FAILED;
    }

    MdCtrl[Dev].Dev = Dev;
    MdCtrl[Dev].ThreadStat = MD_THREAD_STOP;
    MdCtrl[Dev].MdFps = Param->MdFps;
    ak_thread_sem_init(&MdCtrl[Dev].SendSem, 0);

    /* set the mrd to single frame mode */
    ak_mrd_set_mode(MdCtrl[Dev].MrdHandle, 1);

    /* set mrd filter Param*/
    ak_mrd_set_filters(MdCtrl[Dev].MrdHandle, Param->FltBig, Param->FltSmall);

    /* set motion region size*/
    ak_mrd_set_motion_region_size(MdCtrl[Dev].MrdHandle, Param->MoveSizeMin, Param->MoveSizeMax);

    /* set mrd floating Param */
    ak_mrd_set_floating_wadding_params(MdCtrl[Dev].MrdHandle, 3, 1, 3, 36, 2, 2);

    /* create the svp_md_process*/
    if (ak_thread_create(&MdCtrl[Dev].MdTid, MotionDetectThread,
                         &(MdCtrl[Dev].Dev), ANYKA_THREAD_NORMAL_STACK_SIZE, -1))
    {
        ak_print_error_ex(MODULE_ID_MD, "create move detect thread failed.\n");
        return AK_FAILED;
    }
    ak_thread_detach(MdCtrl[Dev].MdTid);

    MdInitFlag[Dev] = 1;
    return AK_SUCCESS;
}

/**
 * submit - get md result.
 * @Dev[IN]:  Dev id
 * @RetInfo[OUT]:	result info
 * return: 0 - success; other error code;
 */
int GetMotionDetectResult(int Dev, struct md_result_info *RetInfo)
{
    if (Dev >= VIDEO_DEV_MUX)
    {
        ak_print_error_ex(MODULE_ID_MD, "Dev:%d error!\n", Dev);
        return ERROR_TYPE_INVALID_ARG;
    }

    if (NULL == RetInfo)
    {
        ak_print_error_ex(MODULE_ID_MD, "RetInfo is null!\n");
        return ERROR_TYPE_POINTER_NULL;
    }

    memcpy(RetInfo, &MdResult[Dev].RetInfo, sizeof(struct md_result_info));

    memset(&MdResult[Dev].RetInfo, 0, sizeof(struct md_result_info));

    return AK_SUCCESS;
}

/**
 * MotionDetectGetResult - get md result.
 * @Dev[IN]:  Dev id
 * @RetInfo[OUT]:  result info
 * @Timeout[IN]:  Timeout <0  block mode, ==0 non-block mode, >0 waittime
 * return: 0 - success;  other error code;
 */
int MotionDetectGetResult(int Dev, struct md_result_info *RetInfo, int Timeout)
{
    int Ret = 0;

    if (Dev >= VIDEO_DEV_MUX)
    {
        ak_print_error_ex(MODULE_ID_MD, "Dev:%d error!\n", Dev);
        return ERROR_TYPE_INVALID_ARG;
    }

    if (NULL == RetInfo)
    {
        ak_print_error_ex(MODULE_ID_MD, "RetInfo is null!\n");
        return ERROR_TYPE_POINTER_NULL;
    }

    memset(RetInfo, 0, sizeof(struct md_result_info));

    if (0 == MdInitFlag[Dev])
    {
        ak_print_error_ex(MODULE_ID_MD, "fail,not init\n");
        return AK_FAILED;
    }

    if (MD_THREAD_RUN != MdCtrl[Dev].ThreadStat)
    {
        ak_print_error_ex(MODULE_ID_MD, "fail,not run\n");
        return AK_FAILED;
    }

    /*md have been triggered*/
    ak_thread_mutex_lock(&MdResult[Dev].Lock);
    if (MdResult[Dev].RetInfo.result)
    {
        GetMotionDetectResult(Dev, RetInfo);
        ak_thread_mutex_unlock(&MdResult[Dev].Lock);
        return AK_SUCCESS;
    }
    ak_thread_mutex_unlock(&MdResult[Dev].Lock);

    if (0 == Timeout)
        return 0;
    else if (Timeout > 0)
    {
        while (Timeout > 0)
        {
            ak_sleep_ms(20);
            ak_thread_mutex_lock(&MdResult[Dev].Lock);
            if (MdResult[Dev].RetInfo.result)
            {
                GetMotionDetectResult(Dev, RetInfo);
                ak_thread_mutex_unlock(&MdResult[Dev].Lock);
                return AK_SUCCESS;
            }
            ak_thread_mutex_unlock(&MdResult[Dev].Lock);
            Timeout -= 20;
        }
    }
    else if (Timeout < 0)
    {
        Ret = ak_thread_sem_wait(&MdCtrl[Dev].SendSem);
    }

    /* sem_wait exit normally, md is triggered*/
    if (0 == Ret)
    {
        ak_thread_mutex_lock(&MdResult[Dev].Lock);
        if (MdResult[Dev].RetInfo.result)
        {
            GetMotionDetectResult(Dev, RetInfo);
            ak_thread_mutex_unlock(&MdResult[Dev].Lock);
            return AK_SUCCESS;
        }
        ak_thread_mutex_unlock(&MdResult[Dev].Lock);
    }

    return AK_SUCCESS;
}

/**
 * MotionDetectEnable - start or stop md .
 * @Dev[IN]:  Dev id
 * @Enable[IN]:  [0,1],  0 -> stop md; 1 -> start md
 * return: 0 - success; other error code;
 */
int MotionDetectEnable(int Dev, int Enable)
{
    if (Dev >= VIDEO_DEV_MUX)
    {
        ak_print_error_ex(MODULE_ID_MD, "Dev:%d error!\n", Dev);
        return ERROR_TYPE_INVALID_ARG;
    }

    if ((Enable != 0) && (Enable != 1))
    {
        ak_print_error_ex(MODULE_ID_MD, "Enable:%d error!\n", Enable);
        return ERROR_TYPE_INVALID_ARG;
    }

    if (0 == MdInitFlag[Dev])
    {
        ak_print_error_ex(MODULE_ID_MD, "fail,no init\n");
        return AK_FAILED;
    }

    ak_print_normal_ex(MODULE_ID_MD, "Enable:%d\n", Enable);

    if ((1 == Enable) && (MD_THREAD_STOP == MdCtrl[Dev].ThreadStat))
    {
        MdCtrl[Dev].ThreadStat = MD_THREAD_RUN;
    }
    else if ((0 == Enable) && (MD_THREAD_RUN == MdCtrl[Dev].ThreadStat))
    {
        MdCtrl[Dev].ThreadStat = MD_THREAD_STOP;
    }

    return AK_SUCCESS;
}

/**
 * MotionDetectFrameMode -  set the mrd to single frame mode
 * @Dev[IN]:  Dev id
 * @mode[IN]: 0, multiple frames mode with floating wadding method
             otherwise, single frame mode without floating wadding method
 * return: 0 - success;
 */
int MotionDetectFrameMode(int Dev, int Mode)
{
    if (Dev >= VIDEO_DEV_MUX)
    {
        return AK_FAILED;
    }

    if (0 == MdInitFlag[Dev])
    {
        ak_print_error_ex(MODULE_ID_MD, "fail,no init\n");
        return AK_FAILED;
    }

    ak_mrd_set_mode(MdCtrl[Dev].MrdHandle, Mode);
    return AK_SUCCESS;
}

/**
 * MotionDetectFiltersSet -   set filters for motion region detection
 * @Dev[IN]:  Dev id
 * @FltBig[IN]: big filter to roughly detect motion region
 * @FltSmall[IN]: small filter to expand rough detection result
 * return: 0 - success;
 */
int MotionDetectFiltersSet(int Dev, int FltBig, int FltSmall)
{

    if (Dev >= VIDEO_DEV_MUX)
    {
        return AK_FAILED;
    }

    if (0 == MdInitFlag[Dev])
    {
        ak_print_error_ex(MODULE_ID_MD, "fail,no init\n");
        return AK_FAILED;
    }

    ak_mrd_set_filters(MdCtrl[Dev].MrdHandle, FltBig, FltSmall);
    return AK_SUCCESS;
}

/*
 * MotionDetectUninit - free md resource and quit md .
 * @Dev[IN]:  Dev id
 * return: 0 - success; other error code;
 */
int MotionDetectUninit(int Dev)
{
    int Ret = 0;

    if (Dev >= VIDEO_DEV_MUX)
    {
        ak_print_error_ex(MODULE_ID_MD, "Dev:%d error!\n", Dev);
        return ERROR_TYPE_INVALID_ARG;
    }

    if (0 == MdInitFlag[Dev])
    {
        ak_print_error_ex(MODULE_ID_MD, "fail, no init\n");
        return AK_FAILED;
    }

    MdInitFlag[Dev] = 0;

    if (MD_THREAD_STOP == MdCtrl[Dev].ThreadStat)
    {
        MdCtrl[Dev].ThreadStat = MD_THREAD_EXIT;
    }
    else
        MdCtrl[Dev].ThreadStat = MD_THREAD_EXIT;

    Ret = ak_thread_join(MdCtrl[Dev].MdTid);
    ak_print_normal_ex(MODULE_ID_MD, "disable, %s\n", Ret ? "failed" : "success");

    ak_mrd_destroy(MdCtrl[Dev].MrdHandle);
    MdCtrl[Dev].MrdHandle = NULL;

    ak_thread_sem_destroy(&MdCtrl[Dev].SendSem);
    ak_thread_mutex_destroy(&MdResult[Dev].Lock);

    return AK_SUCCESS;
}

/* end of file */
