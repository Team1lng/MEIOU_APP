/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-18 11:28:59
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-16 11:51:15
 * @FilePath: /project_3/src/AudioTransfer.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "AudioTransfer.h"
#include "AudioOutput.h"
#include "AudioParam.h"
#include "AudioPlay.h"
#include "NetworkRaw.h"
#include "NetworkInet.h"
#include "AudioInput.h"
#include "CircularList.h"
#include "g711_table.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "stdbool.h"
#include <unistd.h>
#include <assert.h>
#include "ak_ao.h"
#include "ak_ai.h"
#include <string.h>
#include <stdio.h>
#include "stdlib.h"
#include <sys/prctl.h>

#define NETWORK_INTERFACE_NAME "eth0"
#define AUDIO_FRAME_CACHE_MAX 10
#define AUDIO_PACKAGE_SIZE_MAX 1510 // 10*1024//1510

DECLARE_LIST(AudioSendList)

static int NetAudioSendFd = -1;
static int NetAudioReceiveFd = -1;
static int NetAudioProtocol = 0x2666;
static struct sockaddr_ll AudioServAddr; // 原始套接字地址结构
static AudioOutputFrame AudioFrameCache[AUDIO_FRAME_CACHE_MAX];
static const char AudioStartCode[4] = {0x00, 0x00, 0x01, 0xfc};

/****************************************函数重载*******************************************/
int AIDataIxport(AudioInputFrame *frame)
{
    ListNode *Node = CircularListRequest(&AudioSendList);
    if (Node != NULL)
    {
        AudioInputFrame *AudioFrame = malloc(sizeof(AudioInputFrame));
        *AudioFrame = *frame;
        AudioFrame->Data = malloc(frame->Len);
        memcpy(AudioFrame->Data, frame->Data, frame->Len);
        Node->Data = AudioFrame;
        // printf("%s,%lu\n", __func__, frame->SeqNo);
        if (CircularListWrite(&AudioSendList, Node) == -1)
        {
            printf("AudioSendList Write Fail\n");
            free(AudioFrame->Data);
            free(AudioFrame);
        }
    }
    return 0;
}

int SetupAudioInputArgument(int HandleId)
{
    ak_ai_set_nr_attr(HandleId, &default_ai_nr_attr);

    ak_ai_set_agc_attr(HandleId, &default_ai_agc_attr);

    ak_ai_set_aec_attr(HandleId, &default_ai_aec_attr);

    ak_ai_set_aslc_attr(HandleId, &default_ai_aslc_attr);

    ak_ai_set_eq_attr(HandleId, &default_ai_eq_attr);

    ak_ai_set_gain(HandleId, default_ai_gain);
    return 0;
}

int SetupAudioOutputArgument(int HandleId)
{
    ak_ao_set_nr_attr(HandleId, &default_ao_nr_attr);
    ak_ao_set_aslc_attr(HandleId, &default_ao_aslc_attr);
    ak_ao_set_eq_attr(HandleId, &default_ao_eq_attr);

    ak_ao_set_gain(HandleId, default_ao_gain);
    return 0;
}
/*******************************************************************************************/

static int NetAudioReceiveOpen(void)
{
    assert(NetAudioReceiveFd == -1);
    printf("NetAudioProtocol:0x%x\n", NetAudioProtocol);
    if ((NetAudioReceiveFd = socket(PF_PACKET, SOCK_RAW, htons(NetAudioProtocol))) < 0)
    {
        perror("create socket error !!!\n");
        return -1;
    }

    return InetSockConfig(NetAudioReceiveFd, SO_RCVBUF, 10 * 1024);
}

static int AudioReceiveHandle(unsigned char *AudioFrame, int Framesize)
{
/*
 *   数据丢入AO存在延时，延时时长可能比音频数据持续时长还长，可能导致播放时出现断续，
 *   因此将数据缓存起来，达到一定大小后丢入，减少丢入次数和延时时间
 *    AO每次最大数据为4096，网络接收为G711，需要解压，解压后大小*2
 */
#define AUDIO_FRAME_SIZE 2048
    int result = 0;
    static int i = 0;
    static unsigned int AlawSize = 0;
    static unsigned char AlawBuffer[AUDIO_FRAME_SIZE] = {0};

    if (AlawSize + Framesize >= AUDIO_FRAME_SIZE)
    {
        if (AudioFrameCache[i].Data != NULL)
        {
            return -1;
        }
        
        ListNode *Node = CircularListRequest(AudioOutputListGet());
        if (Node != NULL)
        {

            AudioOutputFrame *AudioFrame = &AudioFrameCache[i];
            i = (i + 1) % AUDIO_FRAME_CACHE_MAX;

            AudioFrame->Len = AlawSize * 2;
            AudioFrame->Data = malloc(AudioFrame->Len);
            alaw_to_pcm16(AlawSize, (const char *)AlawBuffer, (char *)(AudioFrame->Data));
            Node->Data = AudioFrame;
            // printf("%s,%d\n", __func__, i);
            if (CircularListWrite(AudioOutputListGet(), Node) == -1)
            {
                printf("[%s] AudioOutputList Write Fail,pthread_self:%lu\n", __func__, pthread_self());
                free(AudioFrame->Data);
                result = -1;
            }
        }
        else
        {
            // printf("[%s] AudioOutputList Request Fail,pthread_self:%lu\n", __func__, pthread_self());
        }
        AlawSize = 0;
    }
    memcpy(&AlawBuffer[AlawSize], AudioFrame, Framesize);
    AlawSize += Framesize;
    return result;
}

static void *AudioReceiveThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    if (NetAudioReceiveOpen() == -1)
        return NULL;

    alaw_pcm16_tableinit();

    int RecvLen = -1;
    bool VaildFrameStart = false;
    unsigned int ReceiveFrameLen = 0;
    unsigned int ReceiveFrameSize = 0;
    unsigned char *ReceiveVaildData = NULL;
    unsigned char FrameBuffer[2048] = {0};
    unsigned char ReceiveBuffer[2048] = {0};
    while (1)
    {
        memset(ReceiveBuffer, 0, sizeof(ReceiveBuffer));
        if ((RecvLen = RawPacketReceive(NetAudioReceiveFd, ReceiveBuffer, sizeof(ReceiveBuffer), 5)) > 0)
        {
            ReceiveVaildData = ReceiveBuffer;
            while (RecvLen > 0)
            {
                if (memcmp(ReceiveVaildData, AudioStartCode, 4) == 0)
                {
                    ReceiveFrameLen = 0;
                    ReceiveFrameSize = (ReceiveVaildData[4] << 24) | (ReceiveVaildData[5] << 16) | (ReceiveVaildData[6] << 8) | ReceiveVaildData[7];
                    ReceiveVaildData += 17;
                    RecvLen -= 17;

                    VaildFrameStart = (RecvLen < 0) ? false : true;
                }

                if (VaildFrameStart)
                {
                    if ((ReceiveFrameLen + RecvLen) <= ReceiveFrameSize)
                    {
                        memcpy(&FrameBuffer[ReceiveFrameLen], ReceiveVaildData, RecvLen);
                        ReceiveFrameLen += RecvLen;
                        RecvLen = 0;
                        if (ReceiveFrameLen == ReceiveFrameSize)
                        {
                            VaildFrameStart = false;
                            AudioReceiveHandle(FrameBuffer, ReceiveFrameSize);
                        }
                    }
                    else
                    {
                        RecvLen = 0;
                    }
                }
                else
                {
                    printf("audio unknow data len = %d\n", RecvLen);
                    RecvLen = -1;
                }
            }
        }
    }
    return NULL;
}

/**
 * @description: 音频数据包发送
 * @param {char} *Data  音频数据
 * @param {int} Size    音频数据大小
 * @return {*}
 */
void NetAudioPackageSend(unsigned char *Data, int Size)
{
    static unsigned char *Buffer = NULL;
    int SendSize = 0;
    int RemainSize = Size;

    /***** 分配音频帧内存 *****/
    if (Buffer == NULL)
    {
        Buffer = malloc(AUDIO_PACKAGE_SIZE_MAX);
        if (Buffer == NULL)
            return;
    }

    /***** 获取时间戳 *****/
    struct timespec Tv;
    clock_gettime(CLOCK_MONOTONIC, &Tv);
    unsigned long long Pts = Tv.tv_sec * 1000 + Tv.tv_nsec / 1000000;

    /***** 帧序号 *****/
    static unsigned long FrameIndex = 0;
    FrameIndex++;
    /***** 帧类型 PCM*****/
    char FrameType = 0;
    while (RemainSize > 0)
    {
        RawPacketHead(Buffer, NETWORK_INTERFACE_NAME, NetAudioProtocol);
        if (SendSize == 0)
        {
            memcpy(&Buffer[60], AudioStartCode, 4);
            Buffer[64] = (RemainSize >> 24) & 0xFF;
            Buffer[65] = (RemainSize >> 16) & 0xFF;
            Buffer[66] = (RemainSize >> 8) & 0xFF;
            Buffer[67] = RemainSize & 0xFF;

            Buffer[68] = (Pts >> 24) & 0xFF;
            Buffer[69] = (Pts >> 16) & 0xFF;
            Buffer[70] = (Pts >> 8) & 0xFF;
            Buffer[71] = Pts & 0xFF;

            Buffer[72] = (FrameIndex >> 24) & 0xFF;
            Buffer[73] = (FrameIndex >> 16) & 0xFF;
            Buffer[74] = (FrameIndex >> 8) & 0xFF;
            Buffer[75] = FrameIndex & 0xFF;

            Buffer[76] = FrameType;

            if (RemainSize > (AUDIO_PACKAGE_SIZE_MAX - 77))
            {
                memcpy(&Buffer[77], Data, AUDIO_PACKAGE_SIZE_MAX - 77);

                if (sendto(NetAudioSendFd, Buffer, AUDIO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)&AudioServAddr, sizeof(AudioServAddr)) < 0)
                {
                    perror("Audio Package send to fail \n");
                }
                RemainSize -= (AUDIO_PACKAGE_SIZE_MAX - 77);
                SendSize += (AUDIO_PACKAGE_SIZE_MAX - 77);
            }
            else
            {
                memcpy(&Buffer[77], Data, RemainSize);
                if (sendto(NetAudioSendFd, Buffer, RemainSize + 77, 0, (struct sockaddr *)&AudioServAddr, sizeof(AudioServAddr)) < 0)
                {
                    perror("Audio Package send to fail  \n");
                }
                break;
            }
        }
        usleep(1000);
        if (RemainSize > AUDIO_PACKAGE_SIZE_MAX)
        {
            memcpy(&Buffer[60], &Data[SendSize], RemainSize);
            if (sendto(NetAudioSendFd, Buffer, AUDIO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)&AudioServAddr, sizeof(AudioServAddr)) < 0)
            {
                perror("Audio Package send to fail \n");
            }
            RemainSize -= (AUDIO_PACKAGE_SIZE_MAX);
            SendSize += (AUDIO_PACKAGE_SIZE_MAX);
        }
        else
        {
            memcpy(&Buffer[60], &Data[SendSize], RemainSize);
            if (sendto(NetAudioSendFd, Buffer, RemainSize + 60, 0, (struct sockaddr *)&AudioServAddr, sizeof(AudioServAddr)) < 0)
            {
                perror("Audio Package send to fail  \n");
            }
            break;
        }
    }
}

/**
 * @description: 音频传输网络协议
 * @param {int} DevId   本地设备ID
 * @param {int} SlaveId 从机设备ID
 * @return {*}
 */
int AudioNetSocketProtocolSet(int DevId)
{
#define AUDIO_NET_PROTOCOL_BASE 0X2600
#define NetworkSlaveId(x) ((x & 0x00) | ((x) - 1))
    int MstartId = NetworkSlaveId(DevId);
    int Vol = 0;
    Vol = (MstartId << 4) | MstartId;
    return (NetAudioProtocol = AUDIO_NET_PROTOCOL_BASE | Vol);
}

static int NetAudioSendOpen(void)
{
    assert(NetAudioSendFd == -1);

    if ((NetAudioSendFd = socket(PF_PACKET, SOCK_RAW, htons(NetAudioProtocol))) < 0)
    {
        perror("create socket error !!!\n");
        return -1;
    }

    return RawNetIfrAddrConfig(NetAudioSendFd, NETWORK_INTERFACE_NAME, &AudioServAddr);
}

static void *AudioSendThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    if (NetAudioSendOpen() == -1)
        return NULL;

    pcm16_alaw_tableinit();
    while (1)
    {
        AudioInputFrame *Frame = NULL;
        if (CircularListRead(&AudioSendList, (void *)&Frame) != -1)
        {
            int AlawSize = Frame->Len / 2;
            char *AlawBuffer = malloc(AlawSize);
            pcm16_to_alaw(Frame->Len, (const char *)Frame->Data, AlawBuffer);
            NetAudioPackageSend((unsigned char *)AlawBuffer, AlawSize);
            free(Frame->Data);
            free(Frame);
            free(AlawBuffer);
        }
    }
    return NULL;
}

/**
 * @description: 音频网络传输初始化
 * @return {*}
 */
int NetAudioTransferInit(void)
{
    CreateCircularList(&AudioSendList);

    AudioPlayInit();

    AudioInputInit();

    pthread_t ReceiveThread;
    pthread_create(&ReceiveThread, NULL, AudioReceiveThread, NULL);
    pthread_detach(ReceiveThread);
    pthread_t SendThread;
    pthread_create(&SendThread, NULL, AudioSendThread, NULL);
    pthread_detach(SendThread);
    return 0;
}