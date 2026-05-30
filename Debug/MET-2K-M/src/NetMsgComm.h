/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-23 21:37:18
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-06-11 11:41:23
 * @FilePath: /82225-EPC/src/NetMsgComm.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _NET_MSG_COMM_H_
#define _NET_MSG_COMM_H_

#define NET_COMMON_CMD_START 0XAA
#define NET_COMMON_CMD_END 0X55

typedef enum
{
    DEVICE_UNKONW,
    DEVICE_INDOOR_ID1 = 1,
    DEVICE_INDOOR_ID2,
    DEVICE_INDOOR_ID3,
    DEVICE_INDOOR_ID4,
    DEVICE_INDOOR_ID5,
    DEVICE_INDOOR_ID6,
    DEVICE_OUTDOOR_1,
    DEVICE_OUTDOOR_2,
    DEVICE_CCTV_1,
    DEVICE_CCTV_2,
    DEVICE_END,
    DEVICE_ALL = 0XFF,
    DEVICE_TOTAL
} NetworkDevice;

typedef enum
{
    LightEvent = 0x53,
    UnlockEvent,
    IdRepeatEvent,
    DoorbellEvent,
    OutdoorTalkEvent,
    IntercomCallEvent,
    StreamStatusEvent,

    DeviceBusyEvent = 0x60,
    MotionDelectEvent,
    UpgraedOutdoorEvent,
    MotionSensitivityEvent,
    AddDelCardEvent,
    OutdoorResetEvent,
    MailboxStatusEvent,

    CompileTimeEvent = 0x70,
    DefaultUnlockTimeEvent,
    ExitButtonTimeEvent,
    OutdoorHangEvent = 0x75,

    Gate2UnlockEvent = 0x99,
    TotalEvent,
} network_event;

typedef struct
{
    NetworkDevice Device;
    char Cmd;
    char Arg1;
    char Arg2;
} NetworkMsgData;

typedef struct
{
    unsigned char SendDev;
    unsigned char ReceiveDev;
    unsigned char Cmd;
    unsigned int DataLen;
    union
    {
        unsigned char Arg[2];
        unsigned char *DP;
    } Data;
} NetworkMsgPacket;

typedef struct
{
    const char *str;
    void (*proc)(NetworkMsgPacket packet);
} NetworkEventHandle;

/**
 * @description: 网络消息发送
 * @param {NetworkMsgData} Data 数据报结构
 * @return {*}
 */
void NetworkMsgSend(NetworkMsgData data);

/**
 * @description: 本地网络设备ID设置
 * @param {NetworkDevice} Id  设备ID
 * @return {*}
 */
void NetLocalDeviceIDSet(NetworkDevice Id);

/**
 * @description: 本地网络设备ID获取
 * @return {*}
 */
NetworkDevice NetLocalDeviceIDGet(void);

/**
 * @description: 网络信息通信初始化
 * @return {*}
 */
void NetMsgCommInit(void);
#endif