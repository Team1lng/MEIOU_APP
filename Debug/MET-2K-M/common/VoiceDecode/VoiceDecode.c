/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-14 08:06:04
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-05 11:10:24
 * @FilePath: /project_3/common/VoiceRingTone.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>
#include <pthread.h>
#include "mad.h"
#include "CircularList.h"
#include "VoiceDecode.h"
#include <sys/prctl.h>

#define VOICE_CACHE_MAX 4 * 1024

typedef struct
{
    FILE *Fp;
    FILE *BackFp;
    unsigned char *Start;
    unsigned long Length;

    unsigned long SeqNo;
    unsigned int CacheLen;
    unsigned char Cache[VOICE_CACHE_MAX];
} Mp3Mad;

static int VoiceDecodeFinsh = 1;
/**
 * @description: 获取语音铃声信息 [需要函数重定义]
 * @param {VoiceInfo} info   节点信息
 * @return {*}
 */
__attribute__((weak)) int VoiceInfoImport(VoiceInfo *info)
{
    printf("This is the weak VoiceInfoImport function.\n");
    return -1;
}

/**
 * @description: 铃声音频数据写入 [需要函数重定义]
 * @param {VoiceFrame} frame   音频数据
 * @return {*}
 */
__attribute__((weak)) int VoiceDataExport(VoiceFrame *frame)
{
    printf("This is the weak VoiceDataExport function.\n");
    return -1;
}

/**
 * @description: 铃声音频开始回调 [需要函数重定义]
 * @param {void} arg   私有数据
 * @return {*}
 */
__attribute__((weak)) void VoiceDecodeStart(void *arg)
{
    VoiceCallBackFunc Func = (VoiceCallBackFunc)arg;
    if (Func)
    {
        Func(NULL);
    }
}

/**
 * @description: 铃声音频结束回调 [需要函数重定义]
 * @param {void} arg   私有数据
 * @return {*}
 */
__attribute__((weak)) void VoiceDecodeEnd(void *arg)
{
    VoiceCallBackFunc Func = (VoiceCallBackFunc)arg;
    if (Func)
    {
        Func(NULL);
    }
}

static inline signed int Mp3Scale(mad_fixed_t sample)
{
    sample += (1L << (MAD_F_FRACBITS - 16));

    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static enum mad_flow Mp3Input(void *data, struct mad_stream *stream)
{
    Mp3Mad *Buffer = data;
    if (stream->next_frame != NULL)
    {
        Buffer->Length = &Buffer->Start[Buffer->Length] - stream->next_frame;
        memmove(Buffer->Start, stream->next_frame, Buffer->Length);
    }
    int ReadLen = fread(&Buffer->Start[Buffer->Length], sizeof(char), VOICE_CACHE_MAX - Buffer->Length, Buffer->Fp);
    if (ReadLen <= 0)
    {
        return MAD_FLOW_STOP;
    }

    Buffer->Length += ReadLen;
    mad_stream_buffer(stream, Buffer->Start, Buffer->Length);
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow Mp3Output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
    unsigned int Nchannels, Nsamples;
    mad_fixed_t const *LeftCh, *RightCh;
    pcm->channels = 1;
    Nchannels = pcm->channels;
    Nsamples = pcm->length;
    LeftCh = pcm->samples[0];
    RightCh = pcm->samples[1];

    Mp3Mad *Buffer = data;

    while (Nsamples--)
    {
        signed int Sample;

        /* output Sample(s) in 16-bit signed little-endian PCM */

        Sample = Mp3Scale(*LeftCh++);
        Buffer->Cache[Buffer->CacheLen++] = (Sample >> 0) & 0xff;
        Buffer->Cache[Buffer->CacheLen++] = (Sample >> 8) & 0xff;

        if (Nchannels == 2)
        {
            Sample = Mp3Scale(*RightCh++);
            Buffer->Cache[Buffer->CacheLen++] = (Sample >> 0) & 0xff;
            Buffer->Cache[Buffer->CacheLen++] = (Sample >> 8) & 0xff;
        }

        /* output Sample(s) in 16-bit signed little-endian PCM */
    }

    /*
     *   音频满1152 * 3大小后直接送入AO
     *   送入AO有一定延迟，因此送入AO的数据持续
     *   时长要大于延迟时间否则会出现音频卡顿
     *   每帧数据大小1152字节，因此不能完整存到
     *   VOICE_CACHE_MAX，否则会出现数组越界段错误问题
     */
    if (Buffer->CacheLen >= (VOICE_CACHE_MAX - (VOICE_CACHE_MAX % 1152)))
    {
        if (Buffer->BackFp != NULL)
        {
            fwrite(Buffer->Cache, Buffer->CacheLen, 1, Buffer->BackFp);
        }

        VoiceFrame Frame;
        Frame.Data = Buffer->Cache;
        Frame.Len = Buffer->CacheLen;
        Frame.SeqNo = Buffer->SeqNo;
        Buffer->SeqNo++;
        VoiceDataExport(&Frame);
        Buffer->CacheLen = 0;
    }
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow Mp3Error(void *data, struct mad_stream *stream, struct mad_frame *Frame)
{
    Mp3Mad *Buffer = data;
    /*
     *   若Mp3Output最后的数据没有满VOICE_CACHE_MAX
     *   需要在此处将最后一份数据送入AO
     */
    if (Buffer->CacheLen > 0)
    {
        VoiceFrame Frame;
        Frame.Data = Buffer->Cache;
        Frame.Len = Buffer->CacheLen;
        Frame.SeqNo = Buffer->SeqNo;
        Buffer->SeqNo++;
        VoiceDataExport(&Frame);
        Buffer->CacheLen = 0;
    }
    return MAD_FLOW_CONTINUE;
}

static int Mp3VoiceDataDecode(VoiceInfo *info)
{
    int result = -1;
    if (access(info->FilePath, F_OK) != 0)
    {
        printf("MP3 File %s Inexistence\n", info->FilePath);
        return -1;
    }

    struct mad_decoder Decodec;
    Mp3Mad Mp3Buff;
    memset(&Mp3Buff, 0, sizeof(Mp3Buff));
    if ((Mp3Buff.Fp = fopen(info->FilePath, "r")) == NULL)
    {
        return -1;
    }

    if ((Mp3Buff.Start = malloc(VOICE_CACHE_MAX)) == NULL)
    {
        goto ALLOC_MP3_BUF_FAIL;
    }

    VoiceDecodeStart(info);

    if (info->BackPcmFile != NULL)
        Mp3Buff.BackFp = fopen(info->BackPcmFile, "w");

    Mp3Buff.CacheLen = 0;
    Mp3Buff.Length = 0;
    Mp3Buff.SeqNo = 0;
    mad_decoder_init(&Decodec, &Mp3Buff, Mp3Input, 0, 0, Mp3Output, Mp3Error, 0);
    mad_decoder_run(&Decodec, MAD_DECODER_MODE_SYNC);
    mad_decoder_finish(&Decodec);

    VoiceDecodeEnd(info);

    free(Mp3Buff.Start);

    if (Mp3Buff.BackFp != NULL)
    {
        fclose(Mp3Buff.BackFp);
    }

    result = 0;
ALLOC_MP3_BUF_FAIL:
    fclose(Mp3Buff.Fp);
    return result;
}

static int PcmVoiceDataDecode(VoiceInfo *info)
{
    if (access(info->FilePath, F_OK) != 0)
    {
        printf("MP3 File %s Inexistence\n", info->FilePath);
        return -1;
    }

    FILE *Fp = fopen(info->FilePath, "r");
    if (Fp == NULL)
    {
        printf("MP3 fopen %s Fail\n", info->FilePath);
        return -1;
    }

    VoiceDecodeStart(info);

    int SeqNo = 0;
    int BufferLen = 0;
    unsigned char Buffer[VOICE_CACHE_MAX] = {0};

    while ((BufferLen = fread(Buffer, sizeof(char), VOICE_CACHE_MAX, Fp)) > 0)
    {
        VoiceFrame Frame;
        Frame.Data = Buffer;
        Frame.Len = BufferLen;
        Frame.SeqNo = SeqNo;
        SeqNo++;
        VoiceDataExport(&Frame);
    }

    VoiceDecodeEnd(info);
    fclose(Fp);
    return 0;
}

void VoiceNodeHandle(VoiceInfo *info)
{
    if (info->Type == VOICE_TYPE_PCM)
    {
        PcmVoiceDataDecode(info);
    }
    else if (info->Type == VOICE_TYPE_MP3)
    {
        Mp3VoiceDataDecode(info);
    }
}

static void *VoiceDecodeThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    while (1)
    {
        VoiceInfo info;
        if (VoiceInfoImport(&info) != -1)
        {
            // printf("**************\n[%s]\n", __func__);
            // printf("FilePath[%s]\n**************\n", info.FilePath);
            // printf("Type[%s]\n**************\n", info.Type == VOICE_TYPE_MP3 ? "MP3" : "PCM");
            // printf("End Func[%p]\n**************\n", info.End);
            VoiceDecodeFinsh = 0;
            VoiceNodeHandle(&info);
            VoiceDecodeFinsh = 1;
        }
    }
    return NULL;
}

/**
 * @description: 语音解码状态
 * @return {*}
 */
int VoiceDecodeStatus(void)
{
    return !VoiceDecodeFinsh;
}

/**
 * @description: 初始化语音铃声提示
 * @return {*}
 */
int VoiceDecodeInit(void)
{
    pthread_t thread;
    pthread_create(&thread, NULL, VoiceDecodeThread, NULL);
    pthread_detach(thread);
    return 0;
}