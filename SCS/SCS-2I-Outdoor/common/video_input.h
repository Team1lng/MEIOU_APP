/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-08-08 15:59:35
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-10-17 08:43:26
 * @FilePath: /outdoor_pro/common/video_input.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _VIDEO_INPUT_H_
#define _VIDEO_INPUT_H_
#include <stdbool.h>



/***
**  date:2022/04/19
**  author:刘炼
**  初始化视频采集设备
***/
bool video_input_device_init(void);


/***
**	date:2022/04/19
**	author:刘炼
**	打开视频采集设备
***/
bool video_input_open(void);


/***
**  date:2022/04/19
**  author:刘炼
**  关闭视频采集设备
***/
bool video_input_close(void);


/***
**  date:2022/04/19
**  author:刘炼
**  采集视频数据
***/
bool video_input_read(unsigned char** data,int* size,char *ch);




/***
**	date:2022/04/19
**	author:刘炼
**	视频采集暂停/开始
***/
bool video_input_capture_pause(bool yeno);

#endif

