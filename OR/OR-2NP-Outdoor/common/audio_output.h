#ifndef _AUDIO_OUTPUT_H_
#define _AUDIO_OUTPUT_H_
#include <stdbool.h>
#include "ak_ao.h"

typedef struct
{
	unsigned char *data;
	int size;
	char ch;
} stream_info;

typedef struct
{
	long type;
	stream_info info;
} stream_data;


int get_ao_buf_remain_len(void);

/***
**  初始化音频输出设备
***/
bool audio_output_device_init(void);

/***
**  开启音频输出设备
**  返回false：打开失败
**  CH:音频通道（单通道/双通道）
**  rate:采样率设置
**  音频位深安凯平台只能16BIT
***/
bool audio_output_open(enum ak_audio_channel_type ch, enum ak_audio_sample_rate rate);

/***
**  关闭音频输出设备
***/
bool audio_output_close(void);

/***
**  写入音频数据到输出设备
***/
bool audio_output_write(unsigned char *data, int size);

/***
**  设置输出音量,音量范围（0-100）
***/
bool audio_output_volume_set(int volume);

/***
**  设置输出音量,音量范围（0-100）
***/
int audio_output_volume_get(void);

bool amp_status_get(void);

void amp_turn_off(void);

void amp_turn_on(void);

void pcm_play_audio(unsigned char *data, int size);

bool ao_play_finish(void);

void ao_howling_suppress_open(void);

void ao_howling_suppress_close(void);

void ao_howling_suppress_pause(void);
int get_ao_hand(void);

void audio_output_device_param_switch(bool param_type);

#endif
