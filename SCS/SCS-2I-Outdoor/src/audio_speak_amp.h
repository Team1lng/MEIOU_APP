#ifndef _AUDIO_SPEAK_AMP_H_
#define _AUDIO_SPEAK_AMP_H_
#include <stdbool.h>

/***

**  初始化音频输出功能使能IO
***/
bool audio_speak_init(void);


/***

**  使能音频输出功能使能IO
***/
bool audio_speak_enable(void);


/***
**  失能音频输出功能使能IO
***/
bool audio_speak_disable(void);

#endif