/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-09-28 13:49:08
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-28 15:08:57
 * @FilePath: /2CD-ME2K-M/src/MotionDetect.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "ak_common.h"

#include "ak_vi.h"
#include "ak_md.h"
#include "ak_log.h"
#include "ak_vpss.h"

#include "Timer.h"
#include "stdbool.h"

#define MOTION_DETECT_CH VIDEO_DEV0

static bool MDEn = false;
static int MDStatus = false;
static int AreaMode = false;
static int ViDevID = VIDEO_DEV0;

static void MotionDetectTimer(void *u)
{
    struct md_result_info RetInfo = {0};
    if (AK_SUCCESS == ak_md_get_result(MOTION_DETECT_CH, &RetInfo, 0))
    {
        MDStatus = (RetInfo.result > 0);
        // ak_print_normal(MODULE_ID_APP, "dev0 detected moving, num: %d\n", RetInfo.move_box_num);
    }
    SetTimer(200, MDTimer, MotionDetectTimer, &MDStatus);
}

int MotionDetectInit_Set(void)
{
    ak_print_set_syslog_level(MODULE_ID_APP, LOG_LEVEL_NORMAL);
    int Ret = AK_FAILED;
    MDEn = false;

    ak_print_normal(MODULE_ID_APP, "*** start to init md module.***\n");
    /*
     * step 1:md init
     * default parameters: detect in 10 fps
     */
    if (AK_SUCCESS != ak_md_init(ViDevID))
    {
        ak_print_normal(MODULE_ID_APP, "dev %d md init fail\n", ViDevID);
        return Ret;
    }

    /*
     * step 2:set Sensitivity
     */
    if (AK_SUCCESS != ak_md_set_sensitivity(ViDevID, 60))
    {
        ak_print_normal(MODULE_ID_APP, "dev %d set_sensitivity fail\n", ViDevID);
        return Ret;
    }

    /*
     * step 3:set area
     */
    char AreaMask[VPSS_MD_DIMENSION_V_MAX * VPSS_MD_DIMENSION_H_MAX] = {0};
    int v, h;

    switch (AreaMode)
    {
    case 0: // whole image

        break;
    case 1: // left half area
        for (v = 0; v < VPSS_MD_DIMENSION_V_MAX; v++)
        {
            for (h = 0; h < VPSS_MD_DIMENSION_H_MAX / 2; h++)
            {
                AreaMask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    case 2: // right half area
        for (v = 0; v < VPSS_MD_DIMENSION_V_MAX; v++)
        {
            for (h = VPSS_MD_DIMENSION_H_MAX / 2; h < VPSS_MD_DIMENSION_H_MAX; h++)
            {
                AreaMask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    case 3: // top half area
        for (v = 0; v < VPSS_MD_DIMENSION_V_MAX / 2; v++)
        {
            for (h = 0; h < VPSS_MD_DIMENSION_H_MAX; h++)
            {
                AreaMask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    case 4: // bottom half area
        for (v = VPSS_MD_DIMENSION_V_MAX / 2; v < VPSS_MD_DIMENSION_V_MAX; v++)
        {
            for (h = 0; h < VPSS_MD_DIMENSION_H_MAX; h++)
            {
                AreaMask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    default:
        ak_print_normal(MODULE_ID_APP, "detect_mode error! must be 0-4.\n");
        return Ret;
        break;
    }

    if (AreaMode > 0)
    {
        if (AK_SUCCESS != ak_md_set_area(ViDevID, AreaMask))
        {
            ak_print_normal(MODULE_ID_APP, "dev %d set_area_sensitivity fail\n", ViDevID);
            return Ret;
        }
    }

    if (AK_SUCCESS != ak_md_set_filters(ViDevID, 2000 /* 40000 */, 1000 /* 15000 */))
    {
        ak_print_normal(MODULE_ID_APP, "dev %d ak_md_set_filters fail\n", ViDevID);
        return Ret;
    }
    /*
     * step 4:md enable
     */
    if (AK_SUCCESS != ak_md_enable(ViDevID, 1))
    {
        ak_print_normal(MODULE_ID_APP, "dev %d md_enable fail\n", ViDevID);
        return Ret;
    }

    MDEn = true;
    SetTimer(200, MDTimer, MotionDetectTimer, &MDStatus);
    return AK_SUCCESS;
}

int MotionDetectSensitivitySet(int Sensitivity)
{
    if (MDEn == false || Sensitivity == 0)
    {
        return false;
    }

    int Filters[4][2] = {{0, 0}, {40000, 10000}, {30000, 15000}, {30000, 20000}};
    int Sen[4] = {0, 80, 80, 80};

    if (AK_SUCCESS != ak_md_set_sensitivity(MOTION_DETECT_CH, Sen[Sensitivity]))
    {
        ak_print_normal(MODULE_ID_APP, "dev %d set_sensitivity fail\n", MOTION_DETECT_CH);
        return false;
    }

    if (AK_SUCCESS != ak_md_set_filters(MOTION_DETECT_CH, Filters[Sensitivity][0], Filters[Sensitivity][1]))
    {
        ak_print_normal(MODULE_ID_APP, "dev %d set_filters fail\n", MOTION_DETECT_CH);
        return false;
    }

    printf(" Motion Sensitivity Big Filter  : %d        Small Filter :%d\n\r", Filters[Sensitivity][0], Filters[Sensitivity][1]);
    return true;
}