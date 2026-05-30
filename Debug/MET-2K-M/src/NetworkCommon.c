#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "AudioOutput.h"
#include "NetworkCommon.h"
#include "NetMsgComm.h"
#include "GeneralInterface.h"
#include "InfraredDetect.h"
#include "LightControl.h"
#include "DeviceUpgrade.h"
#include "VoiceRingPlay.h"
#include "VoiceDecode.h"
#include "UserConfig.h"
#include "Timer.h"
#include "Unlock.h"
#include "MotionDetect.h"
#include "AdcDetect.h"
#include "AudioInput.h"
#include "VideoInput.h"
static void CommunicateOuttime(void *us)
{
    KeyLightControl(-1);
    TalkLightControl(0);
}

/**********************************************************网络消息处理************************************************************8*/

#define EventList(EVENT)          \
    EVENT(IdRepeatEvent)          \
    EVENT(UnlockEvent)            \
    EVENT(StreamStatusEvent)      \
    EVENT(OutdoorTalkEvent)       \
    EVENT(OutdoorHangEvent)       \
    EVENT(OutdoorResetEvent)      \
    EVENT(UpgraedOutdoorEvent)    \
    EVENT(MotionSensitivityEvent) \
    EVENT(DefaultUnlockTimeEvent)

#define DefineEventFunc(_EVENT) [_EVENT].proc = _EVENT##Func, [_EVENT].str = #_EVENT,

static void IdRepeatEventFunc(NetworkMsgPacket Packet)
{
    static int HeratCount = 0;
    struct timespec time;
    struct tm t = FetchCompileTime();
    if (DiffClockTimeMs(&time) > 500)
    {
        GetClockTimeMs(&time);
        NetworkMsgData Data;
        HeratCount = (HeratCount + 1) % 3;
        switch (HeratCount)
        {
        case 0:
#ifdef FINGERPRINT_ENABLE
#define FINGERPRINT_EN 1
#else
#define FINGERPRINT_EN 0
#endif
            Data.Device = DEVICE_ALL;
            Data.Cmd = StreamStatusEvent;
            Data.Arg1 = ((t.tm_mon + 1) << 4) | (FINGERPRINT_EN /* 指纹使能 */ << 2) | (TimerEnablestatus(MDTimer) ? *((int *)TimerGet(MDTimer)->Data) : 0) | (TimerEnablestatus(CommunicateTimer) << 1) | TimerEnablestatus(SVPTimer); // 通知移动侦测结果
            Data.Arg2 = t.tm_mday % 100;
            break;

        case 1:
            Data.Device = DEVICE_ALL;
            Data.Cmd = CompileTimeEvent;
            Data.Arg1 = (char)(((t.tm_mon + 1) * 100 + t.tm_mday) & 0xFF);
            Data.Arg2 = (char)(((t.tm_year - 100) << 3) | (((t.tm_mon + 1) * 100 + t.tm_mday) >> 8));
            break;

        case 2:
            Data.Device = DEVICE_ALL;
            Data.Cmd = DefaultUnlockTimeEvent;
            Data.Arg1 = UserConfigGet()->UnlockTime;
            Data.Arg2 = UserConfigGet()->UngateTime;
            break;
        default:
            break;
        }
        NetworkMsgSend(Data);
    }
}

static void UnlockEventFunc(NetworkMsgPacket Packet)
{
#define UNLOCK_VOICE_INDEX (UnlockEng + ((Packet.Data.Arg[1] & 0x3C) >> 2))
#define UNLOCK_TIME (Packet.Data.Arg[0])
#define UNLOCK_TYPE ((Packet.Data.Arg[1] & 0x03))
#define UNLOCK_VOICE ((Packet.Data.Arg[1] & 0x80))
    int TmpLanguage = UserConfigGet()->Language;
    UserConfigGet()->Language = ((Packet.Data.Arg[1] & 0x3C) >> 2);
    UnlockL(UNLOCK_TIME, UNLOCK_TYPE, UNLOCK_VOICE);
    UserConfigGet()->Language = TmpLanguage;
    UserConfigSave();
}

extern int network_stream_count;
static void StreamStatusEventFunc(NetworkMsgPacket Packet)
{
    // Debug("\n\n");
#define KEY_FRAME_REQUEST (!(Packet.Data.Arg[0] & 0x01))
#define LEAVE_MESSAGE_ENABLE (Packet.Data.Arg[0] & 0x02)
#define TUYA_MONIOTR_ENABLE (Packet.Data.Arg[0] & 0x08)
#define AUDIO_TALK_VOLUME ((Packet.Data.Arg[1]) * 3 + 66)

    network_stream_count = 0;
    if (is_network_audio_send_package_open() == 0)
    {
        network_audio_send_package_start();
    }
    if (is_network_video_send_package_open() == 0)
    {
        network_video_send_package_start();
    }

    if (KEY_FRAME_REQUEST)
    {
        extern void VideoKeyFrameRequest(void);
        VideoKeyFrameRequest();
    }

    if (LEAVE_MESSAGE_ENABLE)
    {
#define LEAVE_MSG_VOICE_INDEX (LeaveMsgEng + (Packet.Data.Arg[0] >> 2))
        static struct timespec time;
        if (DiffClockTimeMs(&time) > 6 * 1000 && !TimerEnablestatus(CommunicateTimer))
        {
            VoiceRingPlay(LEAVE_MSG_VOICE_INDEX, 100);
        }
        GetClockTimeMs(&time);
    }
    else if (TUYA_MONIOTR_ENABLE)
    {
        static char CommDev;
        if (!TimerEnablestatus(CommunicateTimer))
        {

            CommDev = Packet.SendDev;
            SetTimer(5000, CommunicateTimer, CommunicateOuttime, &CommDev);
            TimerDestroy(MonitorTimer);
        }
    }

    if (TimerEnablestatus(CommunicateTimer))
    {
        char *ConmmDevice = TimerGet(CommunicateTimer)->Data;
        if (ConmmDevice && *ConmmDevice == Packet.SendDev)
        {
            RefreshTimer(5000, CommunicateTimer);
        }
        TalkLightControl(1);
        AudioOutputVolumeSet(AUDIO_TALK_VOLUME);
    }
    else if (!TimerEnablestatus(MonitorTimer))
    {
        if (TimerEnablestatus(CallBusyTimer))
        {
            TimerDestroy(CallBusyTimer);
        }

        SetTimer(5000, MonitorTimer, CommunicateOuttime, NULL);
    }
    else
    {
        RefreshTimer(5000, MonitorTimer);
    }
}

static void OutdoorTalkEventFunc(NetworkMsgPacket Packet)
{
    static char CommDev;
    //char CommFloor = Packet.Data.Arg[1];
    char CommCh = Packet.Data.Arg[0];
    if (!TimerEnablestatus(CommunicateTimer) && (DEVICE_OUTDOOR_1+CommCh-1) == NetLocalDeviceIDGet())
    {
        CommDev = Packet.SendDev;
        SetTimer(5000, CommunicateTimer, CommunicateOuttime, &CommDev);
        TimerDestroy(MonitorTimer);

        //CallKeyLightCtrl(CommFloor);
    }
}

static void OutdoorHangEventFunc(NetworkMsgPacket Packet)
{
}

static void OutdoorResetEventFunc(NetworkMsgPacket Packet)
{
    UserConfigReset();
}

static void UpgraedOutdoorEventFunc(NetworkMsgPacket Packet)
{
#define UpgradeLongPack (Packet.DataLen > 8)
#define CheckOnlineStatus (Packet.Data.Arg[0] & 0x01)
#define UpdateOver (Packet.Data.Arg[0] & 0x02)
#define UpdataFinish (Packet.Data.Arg[1] & 0x01)
#define UpdataFail (Packet.Data.Arg[1] & 0x02)
    if (UpgradeLongPack)
    {
        // printf("[%s]DataLen:%d\n", __func__, Packet.DataLen);
        int arg1 = Packet.Data.DP[0] << 24 | Packet.Data.DP[1] << 16 | Packet.Data.DP[2] << 8 | Packet.Data.DP[3];
        // buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];

        int arg2 = Packet.Data.DP[4] << 24 | Packet.Data.DP[5] << 16 | Packet.Data.DP[6] << 8 | Packet.Data.DP[7];
        // buf[8] << 24 | buf[9] << 16 | buf[10] << 8 | buf[11];
        if (ReceiveUpgradePack(arg1, arg2, &Packet.Data.DP[8]) == 0)
        {
            NetworkMsgData Data;
            Data.Device = Packet.SendDev;
            Data.Cmd = UpgraedOutdoorEvent;
            Data.Arg1 = 3; // 升级失败
            Data.Arg2 = 1;
            NetworkMsgSend(Data);
        }
    }
    else
    {
        // printf("[%s]Arg0:%d,Arg1:%d\n", __func__, Packet.Data.Arg[0], Packet.Data.Arg[1]);
        if (CheckOnlineStatus)
        {
            NetworkMsgData Data;
            Data.Device = Packet.SendDev;
            Data.Cmd = UpgraedOutdoorEvent;
            Data.Arg1 = 1;
            Data.Arg2 = 1;
            NetworkMsgSend(Data);
        }
        else if (UpdateOver)
        {
            if (UpdataFinish)
            {
                NetworkMsgData Data;
                Data.Device = Packet.SendDev;
                Data.Cmd = UpgraedOutdoorEvent;
                Data.Arg1 = 2;
                Data.Arg2 = 1;
                VoiceRingPlay(Unlock16k, VoiceDefVol);
                sleep(2);
                if (!UpgradeProcess(1))
                {
                    Data.Arg1 = 3;
                    UpgradeProcess(0);
                }
                NetworkMsgSend(Data);
            }
            else if (UpdataFail)
            {
                UpgradeProcess(0);
            }
        }
    }
}

static void MotionSensitivityEventFunc(NetworkMsgPacket Packet)
{
    int Sensitivity = Packet.Data.Arg[0];
    MotionDetectSensitivitySet(Sensitivity);
}

static void DefaultUnlockTimeEventFunc(NetworkMsgPacket Packet)
{
    if (Packet.SendDev == DEVICE_OUTDOOR_1 || Packet.SendDev == DEVICE_OUTDOOR_2)
        return;

    UserConfigGet()->UnlockTime = Packet.Data.Arg[0];
    UserConfigGet()->UngateTime = Packet.Data.Arg[1];
    UserConfigSave();

    NetworkMsgData Data;
    Data.Device = DEVICE_ALL;
    Data.Cmd = DefaultUnlockTimeEvent;
    Data.Arg1 = UserConfigGet()->UnlockTime;
    Data.Arg2 = UserConfigGet()->UngateTime;
    NetworkMsgSend(Data);
}
NetworkEventHandle HandleFuncGroup[TotalEvent] = {EventList(DefineEventFunc)};
