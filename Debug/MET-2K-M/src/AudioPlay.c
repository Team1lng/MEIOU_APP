/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-19 10:59:05
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-16 11:57:48
 * @FilePath: /project_3/src/AudioPlay.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "AudioOutput.h"
#include "CircularList.h"
#include "SpeakAmp.h"
#include "Timer.h"

DECLARE_LIST(AudioOutputList)

static void AudioAmpDisable(void *u)
{
    SpeakAmpDisable();
}

CircularList *AudioOutputListGet(void)
{
    return &AudioOutputList;
}

int AODataExport(AudioOutputFrame **frame)
{
    int ret = CircularListRead(&AudioOutputList, (void **)frame);
    if (!ret)
    {
        SpeakAmpEnable();
        if (TimerEnablestatus(AmpTimer))
        {
            RefreshTimer(3000, AmpTimer);
        }
        else
        {
            SetTimer(3000, AmpTimer, AudioAmpDisable, NULL);
        }
    }
    return ret;
}

int AODataFree(AudioOutputFrame *frame)
{
    free(frame->Data);
    frame->Data = NULL;
    return 0;
}

void AudioPlayInit(void)
{
    CreateCircularList(&AudioOutputList);
    AudioOutputInit();
    SpeakAmpGpioInit();
    SpeakAmpDisable();
}