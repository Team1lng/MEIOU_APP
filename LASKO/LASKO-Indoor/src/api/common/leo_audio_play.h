#ifndef _LEO_AUDIO_PLAY_H_
#define _LEO_AUDIO_PLAY_H_
#include "stdbool.h"
void touch_sound_play(int volume);
void alarm_sound_play(int volume);
void chime_sound_play(int ring_index,int volume,audio_play_callback start,audio_play_callback end);

bool door_ring_play(int ring_index,int volume,bool send_net,audio_play_callback start,audio_play_callback end);

bool custom_music_play(char* music_name,int volume,bool send_net,audio_play_callback start,audio_play_callback end);
bool interphone_ring_play(int volume,bool send_net,audio_play_callback start,audio_play_callback end);

bool open_door_ring_play(int volume);
bool audio_volume_set(int vol);


#endif

