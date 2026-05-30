#ifndef _AUDIO_PLAY_H_
#define _AUDIO_PLAY_H_

#include"stdbool.h"
#include"ak_common_audio.h"

typedef void (*audio_play_callback)(void);
typedef struct
{
    char* file_path;
	enum ak_audio_channel_type ch;
	enum ak_audio_sample_rate rate;
	enum ak_audio_type type;
	
	int volume;

	audio_play_callback start;
	audio_play_callback end;
}audio_info;


bool audio_play_init(void);


bool is_audio_play_ing(void);


bool audio_play_stop_set(void);


void play_doorbell(int index,int volume);
#endif