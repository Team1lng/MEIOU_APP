#include"stdbool.h"
#include"ak_vpss.h"
#include"ak_common.h"
#include"ak_md.h"
#include"ak_vi.h"
#include"ak_log.h"
#include"motion_detect.h"

static bool  motion_en_flag = false;
static int area_mode = 0;

int motion_detect_init(int dev_id)
{
    int ret = AK_FAILED;
    
    ak_print_normal(MODULE_ID_APP, "*** start to init md module.***\n");
    /*
    * step 1:md init
    * default parameters: detect in 10 fps
    */
    if (AK_SUCCESS != ak_md_init(dev_id)) 
    {
        ak_print_normal(MODULE_ID_APP, "dev %d md init fail\n", dev_id);
        return ret;
    }

    /*
    * step 2:set sensitivity
    */
    if (AK_SUCCESS != ak_md_set_sensitivity(dev_id, 60)) 
    {
        ak_print_normal(MODULE_ID_APP, "dev %d set_sensitivity fail\n", dev_id);
        return ret;
    }

    /*
    * step 3:set area
    */
    char area_mask[VPSS_MD_DIMENSION_V_MAX * VPSS_MD_DIMENSION_H_MAX] = {0};
    int v, h;

    
    switch (area_mode) 
    {
    case 0: //whole image

        break;
    case 1: //left half area
        for (v=0; v<VPSS_MD_DIMENSION_V_MAX; v++) 
        {
            for(h=0; h<VPSS_MD_DIMENSION_H_MAX/2; h++) 
            {
                area_mask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    case 2: //right half area
        for (v=0; v<VPSS_MD_DIMENSION_V_MAX; v++) 
        {
            for(h=VPSS_MD_DIMENSION_H_MAX/2; h<VPSS_MD_DIMENSION_H_MAX; h++) 
            {
                area_mask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    case 3: //top half area
        for (v=0; v<VPSS_MD_DIMENSION_V_MAX/2; v++) 
        {
            for(h=0; h<VPSS_MD_DIMENSION_H_MAX; h++) 
            {
                area_mask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    case 4: //bottom half area
        for (v=VPSS_MD_DIMENSION_V_MAX/2; v<VPSS_MD_DIMENSION_V_MAX; v++) 
        {
            for(h=0; h<VPSS_MD_DIMENSION_H_MAX; h++) 
            {
                area_mask[v * VPSS_MD_DIMENSION_H_MAX + h] = 1;
            }
        }
        break;
    default:
        ak_print_normal(MODULE_ID_APP, "detect_mode error! must be 0-4.\n");
        return ret;
        break;
    }

    if (area_mode > 0)
    {
        if (AK_SUCCESS != ak_md_set_area(dev_id, area_mask)) 
        {
            ak_print_normal(MODULE_ID_APP, "dev %d set_area_sensitivity fail\n", dev_id);
            return ret;
        }
    }

    if (AK_SUCCESS != ak_md_set_filters(dev_id,40000/* 40000 */,15000/* 15000 */)) 
    {
        ak_print_normal(MODULE_ID_APP, "dev %d ak_md_set_filters fail\n", dev_id);
        return ret;
    }
    /*
    * step 4:md enable
    */
    if (AK_SUCCESS != ak_md_enable(dev_id, 1)) 
    {
        ak_print_normal(MODULE_ID_APP, "dev %d md_enable fail\n", dev_id);
        return ret;
    }
	
	motion_en_flag = true;
    return AK_SUCCESS;
}

bool motion_detect_sensitivity_set(int sensitivity)
{
    if(motion_en_flag == false || sensitivity == 0)
    {
        return false;
    }

    int filters[4][2] = {{0,0},{40000,10000},{30000,15000},{30000,20000}};
    int sen[4] = {0,80,80,80};

    if (AK_SUCCESS != ak_md_set_sensitivity(MOTION_DETECT_CH, sen[sensitivity])) 
    {
        ak_print_normal(MODULE_ID_APP, "dev %d set_sensitivity fail\n", MOTION_DETECT_CH);
        return false;
    }

    if (AK_SUCCESS != ak_md_set_filters(MOTION_DETECT_CH, filters[sensitivity][0], filters[sensitivity][1])) 
    {
        ak_print_normal(MODULE_ID_APP, "dev %d set_filters fail\n", MOTION_DETECT_CH);
        return false;
    }

	printf("net_common motion_sensitivity BIG FILTER  : %d        SMALL FILTER :%d\n\r", filters[sensitivity][0], filters[sensitivity][1]);
    return true;
}

static int motion_detect_result = 0;
int motion_detect_result_get(void)
{
    int result = motion_detect_result;
    motion_detect_result = 0;
    return result;
}

int motion_detect_start(void)
{
    int ret = AK_FAILED;

	if(motion_en_flag == false)
	{
		ak_md_destroy(MOTION_DETECT_CH);
		
		ak_print_normal(MODULE_ID_APP, "*** md exit ***\n");
		goto EXIT;
	}

    static struct ak_timeval prev_tv;
    struct ak_timeval curr_tv;
    struct md_result_info ret_info = {0};

    ak_get_ostime(&curr_tv);
    /*
    * step 5:get result
    * you can create a new thread to do it 
    */
   if(curr_tv.usec - prev_tv.usec > 200000)//200ms获取一次移动检测结果
   {
	   prev_tv = curr_tv;

		if (AK_SUCCESS == ak_md_get_result(MOTION_DETECT_CH, &ret_info, 0)) 
		{
			if(ret_info.result)
            {
                motion_detect_result = ret_info.result;
				// ak_print_normal(MODULE_ID_APP, "dev0 detected moving, num: %d\n", ret_info.move_box_num);
            }
		}
   }

    ret = AK_SUCCESS;
    
EXIT:

    return ret;
}

