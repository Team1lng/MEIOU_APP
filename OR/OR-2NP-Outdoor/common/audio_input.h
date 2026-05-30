#ifndef _AUDIO_INPUT_H_
#define _AUDIO_INPUT_H_
#include "ak_vi.h"


/***
**  初始化音频采集设备
***/
bool audio_input_device_init(void);


/***

**	打开音频采集设备
***/
bool audio_input_open(enum ak_audio_channel_type ch,enum ak_audio_sample_rate rate);

/***

**	控制消回声使能
***/
void audio_input_aec_control(bool enable);

/***
**  关闭音频采集设备
***/
bool audio_input_close(void);


/***
**  采集音频数据
***/
bool audio_input_read(unsigned char** data,int* size);


/**
**  设置采集音量,音量范围（0-100）
***/
bool audio_input_volume_set(int volume);


/***
**  设置采集音量,音量范围（0-100）
***/
int audio_input_volume_get(void);



/***
**	音量采集暂停/开始
***/
bool audio_input_capture_pause(bool yeno);

void audio_input_device_param_switch(bool param_type);

#endif

