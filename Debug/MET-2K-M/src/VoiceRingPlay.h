/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-19 09:08:59
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-08-19 08:40:31
 * @FilePath: /project_3/src/VoiceRingPlay.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _VOICERINGPLAY_H_
#define _VOICERINGPLAY_H_
#include "VoiceDecode.h"

#define VOICE_BASE_PATH "/app/voice/"

#define VOICE_LIST(VOICE) \
    VOICE(Unlock16k)      \
    VOICE(LeaveMsgEng)    \
    VOICE(LeaveMsgChi)    \
    VOICE(LeaveMsgGer)    \
    VOICE(LeaveMsgHeb)    \
    VOICE(LeaveMsgPol)    \
    VOICE(LeaveMsgPor)    \
    VOICE(LeaveMsgSpa)    \
    VOICE(LeaveMsgFre)    \
    VOICE(LeaveMsgJap)    \
    VOICE(LeaveMsgIta)    \
    VOICE(UnlockEng)      \
    VOICE(UnlockChi)      \
    VOICE(UnlockGer)      \
    VOICE(UnlockHeb)      \
    VOICE(UnlockPol)      \
    VOICE(UnlockPor)      \
    VOICE(UnlockSpa)      \
    VOICE(UnlockFre)      \
    VOICE(UnlockJap)      \
    VOICE(UnlockIta)      \
    VOICE(CallBusy)       \
    VOICE(Bi1)            \
    VOICE(Bi2)            \
    VOICE(Bi3)            \
    VOICE(Bi4)            \
    VOICE(LongBi)         \
    VOICE(Dio)

#define DEFINE_VOICE(VOICE) VOICE,

typedef enum
{
    VOICE_LIST(DEFINE_VOICE)
        VoiceTotal
} VoiceList;

#define VoiceDefVol 90
#define VoiceRingPlay(Index, Volume) VoiceRingPlayL((Index), (Volume), NULL, NULL);

void VoiceRingPlayL(VoiceList Index, int Volume, VoiceCallBackFunc Start, VoiceCallBackFunc End);

void VoiceRingPlayInit(void);
#endif
