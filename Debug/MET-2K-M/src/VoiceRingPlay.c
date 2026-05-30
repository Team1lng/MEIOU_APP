/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-19 09:08:55
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-21 16:54:14
 * @FilePath: /project_3/src/VoiceRingPlay.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "VoiceRingPlay.h"
#include "VoiceDecode.h"
#include "AudioOutput.h"
#include "CircularList.h"
#include "AudioPlay.h"

#define DefinePath(Voice) [Voice] = #Voice,

#define AUDIO_FRAME_CACHE_MAX 10

DECLARE_LIST(VoiceToneList)

static VoiceInfo InfoList[AUDIO_FRAME_CACHE_MAX] = {[0 ... AUDIO_FRAME_CACHE_MAX - 1].Valid = ATOMIC_VAR_INIT(0)};
static VoiceFrame AudioFrameCache[AUDIO_FRAME_CACHE_MAX];
static char *VoiceName[VoiceTotal] = {VOICE_LIST(DefinePath)};

/****************************************函数重载*******************************************/
int VoiceInfoImport(VoiceInfo *Info)
{
    VoiceInfo *TmpInfo = NULL;
    if (CircularListRead(&VoiceToneList, (void **)&TmpInfo) != -1)
    {
        AudioOutputVolumeSet(TmpInfo->Volume);
        memcpy(Info, TmpInfo, sizeof(VoiceInfo));
        atomic_store(&TmpInfo->Valid, 0);
        // printf("[%s]info->End:%p\n", __func__, Info->End);
        return 0;
    }
    return -1;
}

int VoiceDataExport(VoiceFrame *frame)
{
    static int i = 0;
    if (AudioFrameCache[i].Data != NULL)
    {
        return -1;
    }
    
    ListNode *Node = CircularListRequest(AudioOutputListGet());
    if (Node != NULL)
    {

        VoiceFrame *AudioFrame = &AudioFrameCache[i];
        i = (i + 1) % AUDIO_FRAME_CACHE_MAX;

        *AudioFrame = *frame;
        AudioFrame->Data = malloc(frame->Len);
        memcpy(AudioFrame->Data, frame->Data, frame->Len);
        Node->Data = AudioFrame;
        // printf("%s,%lu\n", __func__, frame->SeqNo);
        if (CircularListWrite(AudioOutputListGet(), Node) == -1)
        {
            printf("[%s] AudioOutputList Write Fail,pthread_self:%lu\n", __func__, pthread_self());
            free(AudioFrame->Data);
        }
    }
    else
    {
        // printf("[%s] AudioOutputList Request Fail,pthread_self:%lu\n", __func__, pthread_self());
        return -1;
    }
    /*
     *   每帧持续时长 ms = 每帧大小/一秒总数据量(采样率*(位深/8)*通道数 ) * 1000(1000ms)
     */
    int FrameDuration = (float)(frame->Len) / (16000 * 16 / 8) * 1000;

    usleep(1000 * (FrameDuration / 2));
    if (AudioOutputRemainLen() > (frame->Len * 3))
    {
        usleep(1000 * (FrameDuration));
    }
    return 0;
}
/*******************************************************************************************/

void VoiceDecodeStart(void *arg)
{
    VoiceInfo *Info = arg;
    CircularListLock(AudioOutputListGet());
    VoiceCallBackFunc Func = (VoiceCallBackFunc)(Info->Start);
    if (Func)
    {
        Func(NULL);
    }
    // printf("%s CircularListLock:%d\n", __func__, CircularListLock(AudioOutputListGet()));
}

void VoiceDecodeEnd(void *arg)
{
    VoiceInfo *Info = arg;
    CircularListUnlock(AudioOutputListGet());
    VoiceCallBackFunc Func = (VoiceCallBackFunc)(Info->End);
    if (Func)
    {
        Func(NULL);
    }
    // printf("%s CircularListUnlock:%d\n", __func__, CircularListUnlock(AudioOutputListGet()));
}

static VoiceType CheakVoiceFileType(const char *File)
{
    const char *Extension = strrchr(File, '.');
    if (Extension == NULL)
    {
        printf("Invalid file name: %s\n", File);
        return -1;
    }

    // 判断文件后缀
    if (strcmp(Extension, ".pcm") == 0)
    {
        return VOICE_TYPE_PCM; // PCM文件
    }
    else if (strcmp(Extension, ".mp3") == 0)
    {
        return VOICE_TYPE_MP3; // MP3文件
    }
    return VOICE_TYPE_UNKNOWN;
}

void VoiceRingPlayL(VoiceList Index, int Volume, VoiceCallBackFunc Start, VoiceCallBackFunc End)
{
    if (!atomic_load(&VoiceToneList.Inited))
    {
        return;
    }

    assert(Index < VoiceTotal);
    VoiceInfo *Info = NULL;
    for (int i = 0; i < (sizeof(InfoList) / (sizeof(VoiceInfo))); i++)
    {
        if (atomic_load(&InfoList[i].Valid) == 0)
        {
            Info = &InfoList[i];
            memset(Info, 0, sizeof(VoiceInfo));
            sprintf(Info->FilePath, "%s%s%s", VOICE_BASE_PATH, VoiceName[Index], ".pcm");
            if (access(Info->FilePath, F_OK) != 0)
            {
                memset(Info->FilePath, 0, sizeof(Info->FilePath));
                sprintf(Info->FilePath, "%s%s%s", VOICE_BASE_PATH, VoiceName[Index], ".mp3");
                if (access(Info->FilePath, F_OK) != 0)
                {
                    printf("%s:%s Not found!!!\n", __func__, VoiceName[Index]);
                    return;
                }
            }

            ListNode *Node = CircularListRequest(&VoiceToneList);
            if (Node != NULL)
            {
                // printf("%s:%s\n", __func__, Info->FilePath);
                Info->Volume = Volume;
                Info->Type = CheakVoiceFileType(Info->FilePath);
                Info->Start = Start;
                Info->End = End;
                atomic_store(&Info->Valid, 1);

                Node->Data = Info;
                if (CircularListWrite(&VoiceToneList, Node) == -1)
                {
                    printf("VoiceToneList Write Fail\n");
                }
            }
            break;
        }
    }
}

void VoiceRingPlayInit(void)
{
    CreateCircularList(&VoiceToneList);
    memset(InfoList, 0, sizeof(VoiceInfo));
    memset(AudioFrameCache, 0, sizeof(AudioFrameCache));
    VoiceDecodeInit();
}