/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-14 19:43:25
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-05 11:09:58
 * @FilePath: /project_3/common/AudioOutput/AudioOutput.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include "AudioOutput.h"
#include "ak_common_audio.h"
#include "ak_common.h"
#include "ak_ao.h"
#include <math.h>
#include <string.h>
#include <sys/prctl.h>

#define PCM_SIZE_MAX 4096

struct
{
    int HandleId;
    atomic_int AudioVolume;
    struct ak_audio_out_param param;
} AO = {
    .AudioVolume = ATOMIC_VAR_INIT(90),
    .param.dev_id = DEV_ADC,
    .param.pcm_data_attr.sample_rate = AK_AUDIO_SAMPLE_RATE_16000,
    .param.pcm_data_attr.channel_num = AUDIO_CHANNEL_MONO,
    .param.pcm_data_attr.sample_bits = AK_AUDIO_SMPLE_BIT_16};

/**
 * @description: 获取输出音频数据 [需要函数重定义]
 * @param {AudioOutputFrame} frame   节点信息
 * @return {*}
 */
__attribute__((weak)) int AODataExport(AudioOutputFrame **frame)
{
    printf("This is the weak AODataExport function.\n");
    return -1;
}

/**
 * @description: 释放音频数据缓存 [需要函数重定义]
 * @param {AudioOutputFrame} frame   节点信息
 * @return {*}
 */
__attribute__((weak)) int AODataFree(AudioOutputFrame *frame)
{
    printf("This is the weak AODataExport function.\n");
    return -1;
}

/**
 * @description: 获取AO剩余数据
 * @return {*}  AO剩余数据
 */
int AudioOutputRemainLen(void)
{
    struct ak_dev_buf_status Status;
    ak_ao_get_buf_status(AO.HandleId, &Status);
    return Status.buf_remain_len;
}

/**
 * @description: 设置AO音量大小
 * @param {int} volume  音量大小
 * @return {*}
 */
void AudioOutputVolumeSet(int volume)
{
    if (volume <= 0 || volume == 100)
        return;

    if (volume == atomic_load(&AO.AudioVolume))
        return;

    atomic_store(&AO.AudioVolume, volume);
}

/**
 * @description: 设置PCM音量大小
 * @param {unsigned char} *src  源数据
 * @param {int} size    数据长度
 * @param {int} volume  音量
 * @return {*}
 */
static int Pcm16bitVolumeCover(unsigned char *src, int size, int volume)
{
    static int PrevVolume = 0;
    float Multiplier = pow(10, (float)(volume - 96) / 20);
    if (PrevVolume != volume)
    {
        PrevVolume = volume;
    }

    unsigned char Dst[PCM_SIZE_MAX] = {0};

    if (size > PCM_SIZE_MAX)
    {
        return -1;
    }

    memset(Dst, 0, PCM_SIZE_MAX);
    for (int i = 0; i < size; i += 2)
    {
        short SrcData = (src[i + 1] << 8) | (src[i] & 0xFF);

        SrcData = SrcData * Multiplier;

        if (SrcData > 32767)
        {
            SrcData = 32767;
        }
        else if (SrcData < -32768)
        {
            SrcData = -32768;
        }

        Dst[i] = SrcData & 0xFF;
        Dst[i + 1] = (SrcData >> 8) & 0xFF;
    }

    memcpy(src, Dst, size);
    return 0;
}

/**
 * @description: 设置音频输出参数
 * @param {int} HandleId 音频输出句柄
 * @return {*}
 */
__attribute__((weak)) int SetupAudioOutputArgument(int HandleId)
{
    // struct ak_audio_nr_attr default_ai_nr_attr = {-40, 0, 1};
    // struct ak_audio_agc_attr default_ai_agc_attr = {24576, 4, 0, 20, 0, 1};
    // struct ak_audio_aec_attr default_ai_aec_attr = {0, 1024, 1024, 0, 512, 1};
    // struct ak_audio_aslc_attr default_ai_aslc_attr = {9830, 0, 0};

    struct ak_audio_nr_attr default_ao_nr_attr = {-40, 0, 1};
    // struct ak_audio_aslc_attr default_ao_aslc_attr = {9830, 0, 0};

    ak_ao_set_nr_attr(AO.HandleId, &default_ao_nr_attr);

    ak_ao_set_gain(HandleId, 3);
    ak_ao_set_volume(HandleId, 0);
    return 0;
}

static int AudioOutputOpen(void)
{
    if (ak_ao_open(&AO.param, &AO.HandleId))
    {
        printf("ak_ao_open failed\n");
        return -1;
    }

    SetupAudioOutputArgument(AO.HandleId);
    return 0;
}

static void *AudioOutputThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    if (AudioOutputOpen() == -1)
        return NULL;

    while (1)
    {
        AudioOutputFrame *Frame;
        if (AODataExport(&Frame) != -1)
        {
            Pcm16bitVolumeCover(Frame->Data, Frame->Len, AO.AudioVolume);
            ak_ao_send_frame(AO.HandleId, Frame->Data, Frame->Len, NULL);
            AODataFree(Frame);
        }
        usleep(1000*10);
    }
    return NULL;
}

/**
 * @description: 初始化音频输出
 * @return {*}
 */
int AudioOutputInit(void)
{
    pthread_t thread;
    pthread_create(&thread, NULL, AudioOutputThread, NULL);
    pthread_detach(thread);
    return 0;
}