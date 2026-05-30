#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H


#define MOTION_DETECT_CH        VIDEO_DEV0

int motion_detect_init(int dev_id);

int motion_detect_result_get(void);

int motion_detect_start(void);
#endif