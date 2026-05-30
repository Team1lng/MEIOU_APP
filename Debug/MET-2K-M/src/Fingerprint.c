/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-04 15:08:22
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-23 08:10:30
 * @FilePath: /project_3/common/Fingerprint/Fingerprint.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "GeneralInterface.h"
#include "UserFingerprint.h"
#include "UserNetManage.h"
#include "EpollGpioEvent.h"
#include "VoiceRingPlay.h"
#include "GpioControl.h"
#include "UartControl.h"
#include "CircularList.h"
#include "UserConfig.h"
#include "Fingerprint.h"
#include <pthread.h>
#include "Unlock.h"
#include <unistd.h>
#include <string.h>
#include "assert.h"
#include "Timer.h"
#include <fcntl.h>
#include <sys/prctl.h>
#include <stddef.h>
#include "PeripheralControl.h"
#include <stdint.h>
#include "FingerAndCardLock.h"
#define FINGER_DEBUG
#ifdef FINGER_DEBUG
#define DebugLog(format, ...) printf("\033[0;33;40m" format "\033[0m", ##__VA_ARGS__)
#define DebugInfo(format, ...) \
    printf("[%s][%d]");        \
    DebugLog(format, ...)

#else
#define DebugLog(format, ...)
#endif

#define RETURN_TIMEOUT 2000
#define FINGER_MODULE_POWER_DOWN_TIMER 0x01
static void SleepEventRegister(void *u);
static atomic_int FingerPowerOn = ATOMIC_VAR_INIT(0);

#define FINGER_LOCK_DURATION_MS 30000                             // 禁止上电时长：30秒
#define FINGER_ERROR_MAX_CNT 5 

/* ****************************************************************** 指纹事件注册接口 *********************************************************** */
DECLARE_LIST(FingerEventList)

/**
 * @description: 指纹事件注册
 * @return {*}
 */
int FingerEventRegister(FingerEvent *Event)
{
    assert(Event != NULL);
    ListNode *Node = CircularListRequest(&FingerEventList);
    if (Node != NULL)
    {
        Node->Data = Event;
        // printf("[%s] Event:%p\n", __func__, Event);
        atomic_store(&(Event->EvRelease), 0);
        if (CircularListWrite(&FingerEventList, Node) == -1)
        {
            printf("[%s] FingerEventList Write Fail\n", __func__);
            goto fail;
        }
        return 1;
    }

fail:
    atomic_store(&(Event->EvRelease), 1);
    return 0;
}

/* ****************************************************************** 指纹模块操作接口 *********************************************************** */
static int UartFd = -1;

static bool FingerReadData(FingerDataAck *Buffer)
{
    int ReadLen = 0;
    int timeout = 100;
    int FingerHeadLen = sizeof(FingerHead);
    memset(Buffer, 0, sizeof(FingerDataAck));
    if (UartBufferSize(UartFd) >= FingerHeadLen && (ReadLen = UartRead(UartFd, (char *)&(Buffer->Format), FingerHeadLen)) == FingerHeadLen) // 读取包头至包长度的数据
    {
        if (PACKAGE_HEADER == (uint16_t)((Buffer->Format.Header[0] << 8) | Buffer->Format.Header[1]))
        {
            if ((Buffer->Format.Addr[0] & Buffer->Format.Addr[1] & Buffer->Format.Addr[2] & Buffer->Format.Addr[3]) != 0xFF)
                return false;
            printf("-------------Type---------%2x\n", Buffer->Format.Type);
            if (Buffer->Format.Type != DATA_PACKAGE && Buffer->Format.Type != ACK_PACKAGE && Buffer->Format.Type != END_PACKAGE)
                return false;

            uint16_t PackLen = (((uint16_t)(Buffer->Format.Len[0]) << 8) | (uint16_t)(Buffer->Format.Len[1] & 0xFF));

            /* 等待数据完整传输 */
            while (UartBufferSize(UartFd) < PackLen && timeout--)
            {
                usleep(1000);
            }

            { /* 包长度不可大于接收缓存，否则会内存溢出，其次按协议分析也代表包数据是错误的 */
                size_t CacheOffset;
                if (Buffer->Format.Type != ACK_PACKAGE)
                    CacheOffset = offsetof(FingerDataAck, Data);
                else
                    CacheOffset = offsetof(FingerDataAck, Affirm);

                int CacheLen = sizeof(FingerDataAck) - CacheOffset;
                DebugLog("Receive Format:");
                DebugLog("[ %2x ][ %2x ]", Buffer->Format.Header[0], Buffer->Format.Header[1]);
                DebugLog("[ %2x ][ %2x ][ %2x ][ %2x ]", Buffer->Format.Addr[0], Buffer->Format.Addr[1], Buffer->Format.Addr[2], Buffer->Format.Addr[3]);
                DebugLog("[ %2x ]", Buffer->Format.Type);
                DebugLog("[ %2x ][ %2x ]\n", Buffer->Format.Len[0], Buffer->Format.Len[1]);
                if (PackLen > CacheLen)
                {
                    DebugWarning("Data Error,Data out of cache size!!!!,PackLen:%d,CacheLen:%d,UartBufferSize:%d\n", PackLen, CacheLen, UartBufferSize(UartFd));
                    return false;
                }
            }

            if ((UartRead(UartFd, Buffer->Format.Type != ACK_PACKAGE ? (char *)Buffer->Data : (char *)&(Buffer->Affirm), PackLen)) != PackLen)
                return false;

            uint32_t CheckSum = Buffer->Format.Type + PackLen + Buffer->Affirm;

            uint16_t DataLen = Buffer->Format.Type == ACK_PACKAGE ? PackLen - AFFIRM_CODE_LEN : PackLen;

            if ((DataLen - CHECK_CODE_LEN) >= (sizeof(Buffer->Data) / sizeof(uint8_t)))
            {
                return false;
            }

            for (int i = 0; i < DataLen - CHECK_CODE_LEN; i++)
            {
                CheckSum += Buffer->Data[i];
            }

            DebugLog("Receive Data:");
            // DebugLog("[ %2x ]", Buffer->Format.Type);
            // DebugLog("[ %2x ][ %2x ]", Buffer->Format.Len[0], Buffer->Format.Len[1]);
            DebugLog("[ %2x ]", Buffer->Affirm);

            for (int i = 0; i < DataLen - CHECK_CODE_LEN; i++)
            {
                DebugLog("[ %2x ]", Buffer->Data[i]);
            }
            DebugLog("[ %2x ][ %2x ]", Buffer->Data[DataLen - 2], Buffer->Data[DataLen - 1]);
            DebugLog("\n");
            if (CheckSum == (((uint16_t)(Buffer->Data[DataLen - 2]) << 8) | (uint16_t)(Buffer->Data[DataLen - 1] & 0xFF)))
            {
                Buffer->CheckSum[0] = Buffer->Data[DataLen - 2];
                Buffer->CheckSum[1] = Buffer->Data[DataLen - 1];

                return true;
            }
        }
    }
    else if (ReadLen > 0)
    {
        DebugLog("RECEIVE DATA ERROR LEN: %d ", ReadLen);
        DebugLog("[ %2x ][ %2x ]", Buffer->Format.Header[0], Buffer->Format.Header[1]);
        DebugLog("[ %2x ][ %2x ][ %2x ][ %2x ]\n", Buffer->Format.Addr[0], Buffer->Format.Addr[1], Buffer->Format.Addr[2], Buffer->Format.Addr[3]);
        DebugLog("\n");
    }
    return false;
}

static void FingerWriteCmd(uint8_t Type, uint8_t Cmd, uint8_t *Data, uint16_t Size)
{
    UartClear(UartFd);

    uint16_t DataLen = FINGER_BASE_DATA_LEN + Size;
    uint8_t Buffer[DataLen];
    uint32_t CheckSum = 0;
    Buffer[0] = (PACKAGE_HEADER >> 8) & 0xFF;
    Buffer[1] = PACKAGE_HEADER & 0xFF;
    Buffer[2] = (DEVICE_ADDR >> 24) & 0xFF;
    Buffer[3] = (DEVICE_ADDR >> 16) & 0xFF;
    Buffer[4] = (DEVICE_ADDR >> 8) & 0xFF;
    Buffer[5] = DEVICE_ADDR & 0xFF;
    Buffer[6] = Type;
    Buffer[7] = ((Size + 3) >> 8) & 0xFF;
    Buffer[8] = (Size + 3) & 0xFF;
    Buffer[9] = Cmd;
    CheckSum = Type + (Size + 3) + Cmd;
    for (int i = 0; i < Size; i++)
    {
        Buffer[10 + i] = Data[i];
        CheckSum += Data[i];
    }
    Buffer[DataLen - 2] = (CheckSum >> 8) & 0xFF;
    Buffer[DataLen - 1] = CheckSum & 0xFF;
    UartWrite(UartFd, (char *)Buffer, DataLen);

    DebugLog("\n\rSend Data :");
    for (int i = 0; i < DataLen; i++)
    {
        DebugLog("[ %x ]", Buffer[i]);
    }
    DebugLog("\n");
}

/* ****************************************************************** 指纹模块电源控制接口 *********************************************************** */

void FingerModulePowerUp(void)
{
    // printf("!!!!!!!!!!!!!!!!!!!!!!!1FingerModulePowerUp\n");
    unsigned char data = _74hc595dDevStatus();
    data = data | (0x01 << 4);
    _74hc595dDevWrite(data);
}

void FingerModulePowerDown(void)
{
    // printf("!!!!!!!!!!!!!!!!!!!!!!!1FingerModulePowerDown\n");
    unsigned char data = _74hc595dDevStatus();
    data = data & ~(0x01 << 4);
    _74hc595dDevWrite(data);
}

#if 1
static bool Cancel(void)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);
    FingerWriteCmd(CMD_PACKAGE, CODE_CANCEL, NULL, 0);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            Debug("指令中断退出%s\n", Buffer.Affirm == 0x00 ? "成功" : "失败");
            return (Buffer.Affirm == 0x00);
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("指令中断退出失败\n");
    return false;
}
#endif

static uint16_t ValidTemplateNum(void)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);
    FingerWriteCmd(CMD_PACKAGE, CODE_VALID_TEMPLATE_NUM, NULL, 0);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            uint16_t Nmber = (((uint16_t)Buffer.Data[0] << 8) | ((uint16_t)Buffer.Data[1]));
            Debug("有效模板数量获取%s 个数:%d\n", Buffer.Affirm == 0x00 ? "成功" : "失败", Nmber);
            return Buffer.Affirm == 0x00 ? Nmber : 0x00;
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("有效模板数量获取失败\n");
    return 0xff;
}

static uint16_t AutoVerifyFinger(uint16_t Id) // 自动验证
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);

    uint8_t Data[5];
    Data[0] = 0x02;
    Data[1] = (Id >> 8) & 0xFF;
    Data[2] = Id & 0xFF;
    Data[3] = 0x00;
    Data[4] = 0b00000010;
    FingerWriteCmd(CMD_PACKAGE, CODE_AUTO_IDENTIFY, Data, 5);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            if (Buffer.Affirm == 0x00 && Buffer.Data[0] == 0x05)
            {
                Debug("比对成功\n");
                return (((uint16_t)Buffer.Data[1] << 8) | ((uint16_t)Buffer.Data[2]));
            }
            else if (Buffer.Affirm == 0x00 && Buffer.Data[0] == 0x00)
                Debug("指令合法性检测成功\n");
            else if (Buffer.Affirm == 0x00 && Buffer.Data[0] == 0x01)
                Debug("录入指纹获取图像成功\n");
            else
                break;
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("比对失敗:0x%x\n", Buffer.Data[0]);
    return 0xFFFF;
}

static bool AutoEnrool(uint16_t Id)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);

    uint8_t Data[5];
    Data[0] = (Id >> 8) & 0xFF;
    Data[1] = Id & 0xFF;
    Data[2] = 0x02;
    Data[3] = 0x00;
    Data[4] = 0b00011000;
    FingerWriteCmd(CMD_PACKAGE, CODE_AUTO_ENROLL, Data, 5);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            if (Buffer.Data[0] == 0x00 && Buffer.Data[1] == 0x00)
            {
                if (Buffer.Affirm == 0x00)
                    Debug("指令合法性检测成功,并进入第一次指纹录入\n");
                else
                    break;
            }
            else if (Buffer.Affirm == 0x00 && Buffer.Data[0] == 0x01)
                Debug("等待第[%d]次彩图成功\n", Buffer.Data[1]);
            else if (Buffer.Data[0] == 0x02)
            {
                if (Buffer.Affirm == 0x00)
                    Debug("等待第[%d]次生成特征成功\n", Buffer.Data[1]);
                else
                    break;
            }
            else if (Buffer.Affirm == 0x00 && Buffer.Data[0] == 0x03)
                Debug("第[%d]次手指离开\n", Buffer.Data[1]);
            else if (Buffer.Data[0] == 0x04 && Buffer.Data[1] == 0xF0)
            {
                if (Buffer.Affirm == 0x00)
                    Debug("合成模板成功\n");
                else
                {
                    Debug("合成模板失败\n");
                    break;
                }
            }
            else if (Buffer.Data[0] == 0x05 && Buffer.Data[1] == 0xF1)
            {
                if (Buffer.Affirm == 0x00)
                    Debug("没有相同指纹\n");
                else if (Buffer.Affirm == 0x27)
                {
                    Debug("有相同指纹\n");
                    break;
                }
            }
            else if (Buffer.Data[0] == 0x06 && Buffer.Data[1] == 0xF2)
            {
                if (Buffer.Affirm == 0x00)
                {
                    Debug("模板数据存储成功：%d\n", Id);
                    return true;
                }
                else
                {
                    Debug("模板数据存储失败\n");
                    break;
                }
            }
            else if (Buffer.Affirm == 0x26)
            {
                Debug("超时\n");
                break;
            }
            else if (Buffer.Affirm == 0x22)
            {
                Debug("指纹模板非空\n");
                break;
            }
            else
            {
                Debug("\n");
                break;
            }
            GetClockTimeMs(&Time);
        }
        /* 指纹模块超时时间近8秒，因此八秒内无返回则退出 */
        else if (DiffClockTimeMs(&Time) > (RETURN_TIMEOUT * 4))
        {
            Debug("Exit after 8 seconds\n");
            break;
        }
        usleep(1000);
    }
    Cancel();
    Debug("指纹:%d 模板录入失败\n", Id);
    return false;
}

static void FingerModulePowerDownHandle(void *pData)
{
    FingerModulePowerDown();
}

static bool DeleteTemplate(uint16_t Page, uint16_t Num)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);

    uint8_t Data[4];
    Data[0] = (Page >> 8) & 0xFF;
    Data[1] = Page & 0xFF;
    Data[2] = (Num >> 8) & 0xFF;
    Data[3] = Num & 0xFF;
    FingerWriteCmd(CMD_PACKAGE, CODE_DELETE_CHAR, Data, 4);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            Debug("刪除指紋 %d %s\n", Num, Buffer.Affirm == 0x00 ? "成功" : "失败");
            return (Buffer.Affirm == 0x00);
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("刪除指紋 %d 失败\n", Num);
    return false;
}

static bool EmptyTemplate(void)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);
    FingerWriteCmd(CMD_PACKAGE, CODE_EMPTY, NULL, 0);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            Debug("清空指紋%s\n", Buffer.Affirm == 0x00 ? "成功" : "失败");
            return (Buffer.Affirm == 0x00);
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("清空指紋失败\n");
    return false;
}

static bool ReadIndexTable(uint8_t Page, FingerDataAck *DataAck)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);
    FingerWriteCmd(CMD_PACKAGE, CODE_READ_INDEX_TABLE, &Page, 1);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {

            Debug("索引列表读取%s\n", Buffer.Affirm == 0x00 ? "成功" : "失败");
            *DataAck = Buffer.Affirm == 0x00 ? Buffer : *DataAck;
            return (Buffer.Affirm == 0x00);
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("索引列表读取失败\n");
    return false;
}

#if (FINGER_MANUFACTURER == BLACK_FIRE)
static bool LightSetting(LightCode Code, uint8_t Speed, LightColor Color, uint8_t Times)
#elif (FINGER_MANUFACTURER == ML_FPM093A)
static bool LightSetting(LightCode Code, uint8_t StartColor, LightColor EndColor, uint8_t Times)
#endif
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);

    uint8_t Data[4];
    Data[0] = Code;
#if (FINGER_MANUFACTURER == BLACK_FIRE)
    Data[1] = Speed;
    Data[2] = Color;
#elif (FINGER_MANUFACTURER == ML_FPM093A)
    Data[1] = StartColor;
    Data[2] = EndColor;
#endif
    Data[3] = Times;
    FingerWriteCmd(CMD_PACKAGE, CODE_LIGHT_SETTING, Data, (sizeof(Data) / sizeof(uint8_t)));
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            Debug("灯光设置%s\n", Buffer.Affirm == 0x00 ? "成功" : "失败");
            return (Buffer.Affirm == 0x00);
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("灯光设置失败\n");
    return false;
}

static bool ModuleSleep(void)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);
    FingerWriteCmd(CMD_PACKAGE, CODE_SLEEP, NULL, 0);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            Debug("休眠指令%s\n", Buffer.Affirm == 0x00 ? "成功" : "失败");
            return (Buffer.Affirm == 0x00);
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("休眠指令失败\n");
    return false;
}

#if (FINGER_MANUFACTURER == ML_FPM093A)
static bool DetectFingerPress(void)
{
    FingerDataAck Buffer;
    struct timespec Time;
    GetClockTimeMs(&Time);
    FingerWriteCmd(CMD_PACKAGE, CODE_GET_IMAGE, NULL, 0);
    while (1)
    {
        if (FingerReadData(&Buffer))
        {
            Debug("获取图像指令:%s\n", Buffer.Affirm == 0x00 ? "成功" : (Buffer.Affirm == 0x02 ? "传感器无手指" : "失败"));
            return (Buffer.Affirm == 0x02);
        }
        else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
        {
            break;
        }
        usleep(1000);
    }
    Debug("获取图像指令失败\n");
    return false;
}
#endif

/* ****************************************************************** 指纹事件定义接口 *********************************************************** */

static int ReadFingerModuleData(void)
{
    uint8_t Num = ValidTemplateNum();
    if (Num == 0 && GetFingerInfo()->TotalNum)
    {
        FingerInfoFormat();
        return 0;
    }
    else if (Num == 0xFF)
    {
        if ((Num = ValidTemplateNum()) == 0xFF)
        {
            return -1;
        }
    }

    Debug("Vaild Num:%d\n", Num);

    GetFingerInfo()->NextEnptyIndex = GetFingerInfo()->TotalNum = Num;

    if (GetFingerInfo()->TotalNum)
    {

        FingerDataAck Buffer;

        if (ReadIndexTable(0, &Buffer))
        {
            for (int Byte = 0, Num = 0; Byte < 32; Byte++)
            {
                for (int Bit = 0; Bit < 8; Bit++)
                {
                    uint8_t Index = Byte * 8 + Bit;

                    if (Index >= FINGER_NUM_MAX)
                    {
                        GetFingerInfo()->TotalNum = Num;
                        goto Exit;
                    }

                    if (Buffer.Data[Byte] & (1 << Bit))
                    {
                        Num++;

                        GetFingerInfo()->Finger[Index].Perm = GetFingerInfo()->Finger[Index].Perm ? GetFingerInfo()->Finger[Index].Perm : LOCK_TYPE;
                        FingerDataInit(Index);
                    }
                    else
                    {
                        GetFingerInfo()->Finger[Index].Perm = NONE_TYPE;
                        FingerDataReset(Index);

                        if (GetFingerInfo()->NextEnptyIndex == GetFingerInfo()->TotalNum)
                            GetFingerInfo()->NextEnptyIndex = Index;
                    }

                    if (Num == GetFingerInfo()->TotalNum || Num == FINGER_NUM_MAX)
                    {
                        goto Exit;
                    }
                }
            }
        }
    Exit:
        FingerInfoSave();
        return 0;
    }
    return 0;
}

int FingerLightControl(int En, LightColor Color, uint8_t Times)
{
    Debug("Addr En:%p,Color:%p,Times:%p\n", &En, &Color, &Times);
    Debug("Vol En:%d,Color:%d,Times:%d\n", En, Color, Times);
    if (En)
    {
#if (FINGER_MANUFACTURER == BLACK_FIRE)
        LightSetting(FLASHING, 0x0A, Color, Times);
#elif (FINGER_MANUFACTURER == ML_FPM093A)
        LightSetting(FLASHING, Color, Color, Times);
#endif
    }
    else
    {

#if (FINGER_MANUFACTURER == BLACK_FIRE)
        // LightSetting(CLOSE_POWER_ON_LIGHT, 0, COLORFUL, 0);
        // LightSetting(SUCCEE_DEFAULT_SET, 0, COLORFUL, 0);
        // LightSetting(FAILED_DEFAULT_SET, 0, COLORFUL, 0);
        // LightSetting(BREATHING, 0, BLUE, 0);
#elif (FINGER_MANUFACTURER == ML_FPM093A)
        // LightSetting(BREATHING, 0, BLUE, 0);
#endif
    }
    Debug("\n");
    return 0;
}



/**
 * @brief 标记禁止指纹上电（核心函数）
 */
static void FingerForbidPowerUp(void)
{
    if (atomic_load(&LockFlag) == 1)
    {
        DebugLog("************* 指纹已禁止上电，无需重复标记 *************\n");
        return;
    }

    // 仅标记禁止上电，不主动断电（保留原有断电逻辑）
    atomic_store(&LockFlag, 1);
    struct timespec now_time;
    GetClockTimeMs(&now_time);
    uint64_t now_ms = (uint64_t)now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000;
    atomic_store(&LockStartMs, now_ms);

    DebugLog("************* 标记指纹禁止上电30秒 *************\n");
}
/* ======================================================== zio ================================================ */


void FingerPowerControl(void)
{
     // 禁止上电期间，无论什么模式都不上电 
    if (IsVerifyLocked() == 1)
    {
        // DebugLog("************* 指纹处于禁止期，拒绝网络管理模式下的上电操作 *************\n");
        return;
    }

    if (IsNetManageEntryState())
    {
        if (atomic_load(&FingerPowerOn) == 0)
        {
            FingerModulePowerUp();
            atomic_store(&FingerPowerOn, 1); // 标记为已上电
            printf("[Finger Power: ON (Enter Net Manage Mode)]\n");
        }
    }
    else
    {
        if (atomic_load(&FingerPowerOn) == 1)
        {
            FingerModulePowerDown();
            atomic_store(&FingerPowerOn, 0); // 标记为已断电
            printf("[Finger Power: OFF (Exit Net Manage Mode)]\n");
        }
    }
}

static int VerifyFingerPrintf(const FingerEvent *Event)
{
    // 禁止上电期间，直接拒绝验证
    if (IsVerifyLocked() == 1)
    {
        DebugLog("************* 禁止上电期间，拒绝指纹验证 *************\n");
        return 0xFFFF;
    }

    uint16_t Num = 0xFFFF;
    if ((Num = AutoVerifyFinger(0xFFFF)) == 0xFFFF)
    {
        Debug("*************该指纹不存在**************\n");
        FingerLightControl(1, RED, 3);
        VoiceRingPlay(Bi4, VoiceDefVol);
        int cur_error_cnt = atomic_fetch_add(&ErrorCnt, 1) + 1;

        if (cur_error_cnt >= FINGER_ERROR_MAX_CNT)
        {
            DebugLog("************* 连续错误5次,触发禁止上电逻辑 *************\n");
            FingerLightControl(1, YELLOW, 4);
            VoiceRingPlay(Bi4, VoiceDefVol);
            TriggerVerifyLock();
            FingerForbidPowerUp();
            atomic_store(&ErrorCnt, 0);
        }
    }
    else
    {
        Debug("*************该指纹已存在 指纹ID:%d**************\n", Num);
        FingerLightControl(1, GREEN, 1);
        VoiceRingPlay(Bi2, VoiceDefVol);

        // 验证成功，重置错误计数器
        ResetVerifyLockState();

        if (FingerPermGet(Num) & LOCK_TYPE)
            Unlock(UserConfigGet()->UnlockTime, LOCK_TYPE);
        if (FingerPermGet(Num) & GATE_TYPE)
            Unlock(UserConfigGet()->UngateTime, GATE_TYPE);
    }
    return Num;
}

/* ======================================================== zio ================================================ */


static int AddFingerPrintf(const FingerEvent *Event)
{
    // 录入指纹也需要检查禁止上电状态
    if (IsVerifyLocked() == 1)
    {
        DebugLog("************* 禁止上电期间，拒绝指纹录入 *************\n");
        return -1;
    }
    RefreshTimer(30 * 1000, AddFingerTimer);
    if (AutoEnrool(GetFingerInfo()->NextEnptyIndex))
    {
        GetFingerInfo()->Finger[GetFingerInfo()->NextEnptyIndex].Perm = LOCK_TYPE;
        FingerLightControl(1, GREEN, 1);
        ReadFingerModuleData();
        VoiceRingPlay(Bi2, VoiceDefVol);
        NetManageShortPack(1, ManageAddFinger, 1, 0);
    }
    else
    {
        FingerLightControl(1, RED, 3);
        VoiceRingPlay(Bi4, VoiceDefVol);
        NetManageShortPack(1, ManageAddFinger, 9, 0);
    }
    return 0;
}

static int EraseFingerprintf(const FingerEvent *Event)
{
    // 删除指纹也需要检查禁止上电状态
    if (IsVerifyLocked() == 1)
    {
        DebugLog("************* 禁止上电期间，拒绝指纹删除 *************\n");
        return -1;
    }
    if (Event->Arg1 == 200)
    {

        EmptyTemplate();
    }
    else
    {
        DeleteTemplate(Event->Arg1, Event->Arg2);
    }
    ReadFingerModuleData();
    return 1;
}


static int FingerDetectHandle(int Level)
{
    // 禁止上电期间，直接拒绝中断处理
    if (IsVerifyLocked() == 1)
    {
        // DebugLog("************* 禁止上电期间，拒绝指纹中断处理 *************\n");
        return -1;
    }

    static struct timespec DitherEliminaTime, FirIntervalTime;
    static int FirIntervalMs = 500;
    static int DitherEliminatMs = 200;
    unsigned long long IntervalMs = DiffClockTimeMs(&DitherEliminaTime);

    GetClockTimeMs(&DitherEliminaTime); // 更新 DitherEliminaTime 为当前时间
    if (IntervalMs < DitherEliminatMs)  /* 指紋模块100ms低13ms高周期性持续方波，需要等待持续第二次触发中断 ，避免电路干扰导致中断错误触发*/
    {
        IntervalMs = DiffClockTimeMs(&FirIntervalTime);
        GetClockTimeMs(&FirIntervalTime);
        if (IntervalMs > FirIntervalMs) /* 每次事件需间隔100ms */
        {

            static FingerEvent Event = {.Func = NULL, .EvRelease = ATOMIC_VAR_INIT(1)};
            if (atomic_load(&Event.EvRelease))
            {
                Debug("Trigger:%d,EvRelease:%d\n\n", Level, atomic_load(&Event.EvRelease));
                Event.EvStr = (char *)__func__;
                Event.Func = TimerEnablestatus(AddFingerTimer) ? AddFingerPrintf : VerifyFingerPrintf;
                FingerEventRegister(&Event);
                return 0;
            }
        }
    }

    return 0;
}

// int FingerDetectEpollEventInit(struct EpollEvent *Event)
// {
// #define FINGER_INT_GPIO 27   //31  指纹中断
//     if (GpioOpen(FINGER_INT_GPIO, GPIO_DIR_IN, true) == false)
//     {
//         return -1;
//     }
//     GpioEdge(FINGER_INT_GPIO, RISING_EDGE);
//     char Path[64] = {0};
//     memset(Path, 0, sizeof(Path));
//     sprintf(Path, "/sys/class/gpio/gpio%d/value", FINGER_INT_GPIO);
//     Event->Fd = open(Path, O_RDONLY);
//     Event->TriggerLevel = 1;
//     Event->ChatterTimeMs = 0;
//     Event->EpollEventHandle = FingerDetectHandle;
//     return 0;
// }

#if (FINGER_MANUFACTURER == ML_FPM093A)
static int DetectFingerReleaseHandle(const FingerEvent *Event)
{
    if (DetectFingerPress())
    {
        ModuleSleep();
        FingerModulePowerDown();
        return 0;
    }
    return 0;
}

static void SleepEventRegister(void *u)
{
    static FingerEvent Event = {.Func = DetectFingerReleaseHandle, .EvRelease = ATOMIC_VAR_INIT(1)};
    if (atomic_load(&Event.EvRelease))
    {
        Event.EvStr = (char *)__func__;
        Event.Func = DetectFingerReleaseHandle;
        FingerEventRegister(&Event);
    }
}

#endif

/**
 * author:zio
 * @description: 指纹模组复位
 * @return {*} true=复位成功，false=复位失败
 */
// bool FingerModuleReset(void)
// {
//     FingerDataAck Buffer;
//     struct timespec Time;
//     GetClockTimeMs(&Time);

//     // 发送复位指令
//     FingerWriteCmd(CMD_PACKAGE, CODE_RESET, NULL, 0);

//     while (1)
//     {
//         if (FingerReadData(&Buffer))
//         {
//             // Debug("指纹模组复位%s\n", Buffer.Affirm == 0x00 ? "成功" : "失败");
//             // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Buffer.Affirm: %02x\n", Buffer.Affirm);
//             usleep(500 * 1000);
//             return (Buffer.Affirm == 0x00);
//         }
//         else if (DiffClockTimeMs(&Time) > RETURN_TIMEOUT)
//         {
//             break;
//         }
//         usleep(1000);
//     }
//     // Debug("指纹模组复位失败（超时）\n");
//     return false;
// }

/**
 * author:zio
 * @description: 定时复位线程（每5分钟执行一次复位）
 * @param {void} *arg 线程参数
 * @return {*}
 */
// static void *FingerTimedResetThread(void *arg)
// {
//     prctl(PR_SET_NAME, "FingerTimedReset"); // 设置线程名，方便调试

//     // 5分钟 = 5 * 60 = 300秒，若需要更精确定时可使用定时器API
//     const int TIMED_RESET_INTERVAL = 3 * 3600;

//     while (1)
//     {
//         // 先休眠5分钟，再执行复位（避免程序启动立即复位）
//         sleep(TIMED_RESET_INTERVAL);

//         Debug("==================== 执行定时指纹模组复位 ====================\n");
//         // 执行复位
//         FingerModuleReset();

//         // 复位后重新读取指纹信息，确保数据同步
//         ReadFingerModuleData();
//         // 复位后重新设置灯光（可选，根据你的需求保留）
//         FingerLightControl(0, 0, 0);
//     }
//     return NULL;
// }

static void *DrvFingerprintThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    ModuleSleep();
    FingerLightControl(0, 0, 0);
    ReadFingerModuleData();
    while (1)
    {
        FingerEvent *Event = NULL;
        if (CircularListRead(&FingerEventList, (void *)&Event) != -1)
        {
            if (atomic_load(&Event->EvRelease) == 0)
            {
                if (Event->Func)
                    Event->Func(Event);
                atomic_store(&Event->EvRelease, 1);
                Debug("%s EvRelease:%d\n", Event->EvStr, atomic_load(&Event->EvRelease));
            }
        }
    }
    return NULL;
}

static void FingerModuleIdleHandle(void *u)
{
    SleepEventRegister(u);
}

static void FingerModulePowerHandle(void)
{
    // ========== 禁止上电期间，不执行FingerModulePowerUp ==========
    if (IsVerifyLocked() == 1)
    {
        // DebugLog("************* 禁止上电期间,拒绝执行FingerModulePowerUp *************\n");
        return;
    }
    FingerModulePowerUp();
    if (!TimerEnablestatus(FingerPowerTimer))
    {
        SetTimer(1000, FingerPowerTimer, FingerModuleIdleHandle, NULL);
    }
    else
    {
        RefreshTimer(1000, FingerPowerTimer);
    }
}





static void *DrvFingerIrqDetectThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    GPIO_LEVEL level = 0;
#define FINGER_INT_GPIO 27
    GpioOpen(FINGER_INT_GPIO, GPIO_DIR_IN, true);
    while (1)
    {
        FingerPowerControl();

        if (GpioLevelGet(FINGER_INT_GPIO, &level) && level == GPIO_LEVEL_HIGH)
        {
            FingerModulePowerHandle();
            FingerDetectHandle(1);
        }

        usleep(1000);
    }
    return NULL;
}

/**
 * @description: 删除指纹事件注册
 * @param {uint8_t} Arg1    删除起始索引
 * @param {uint8_t} Arg2    删除个数
 * @return {*}
 */
void DelFingerEvnetRegister(uint8_t Arg1, uint8_t Arg2)
{
    static FingerEvent Event = {.EvRelease = ATOMIC_VAR_INIT(1)};
    if (atomic_load(&Event.EvRelease))
    {
        Event.EvStr = (char *)__func__;
        Event.Func = EraseFingerprintf, Event.Arg1 = Arg1, Event.Arg2 = Arg2;
        FingerEventRegister(&Event);
    }
}

/**
 * @description: 指纹驱动初始化
 * @return {*}
 */
int FingerprintInit(void)
{
    UartFd = UartOpen("ttySAK1", 57600, 8, 1, 'n');
    if (UartFd < 0)
    {
        DebugLog("open ttySAK1 faild \n");
        usleep(1000 * 1000);
        return false;
    }

    FingerInfoInit();

    pthread_t Thread;
    CreateCircularList(&FingerEventList);
    pthread_create(&Thread, NULL, DrvFingerprintThread, NULL);
    pthread_detach(Thread);
    pthread_create(&Thread, NULL, DrvFingerIrqDetectThread, NULL);
    pthread_detach(Thread);

    // pthread_create(&Thread, NULL, FingerTimedResetThread, NULL);
    // pthread_detach(Thread);

    return 0;
}
