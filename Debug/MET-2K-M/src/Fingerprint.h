/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-04 15:08:26
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-22 08:41:17
 * @FilePath: /project_3/common/Fingerprint/Fingerprint.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _FINGERPRINT_H_
#define _FINGERPRINT_H_

#include <stdatomic.h>

#define ML_FPM093A 1
#define BLACK_FIRE 2

#define FINGER_MANUFACTURER ML_FPM093A

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

#define FINGER_NUM_MAX 100
#define FINGER_BASE_DATA_LEN 12

#define PACKAGE_HEADER 0xEF01
#define DEVICE_ADDR 0xFFFFFFFF

#define AFFIRM_CODE_LEN 1
#define CHECK_CODE_LEN 2

#define CMD_PACKAGE 0x01
#define DATA_PACKAGE 0x02
#define ACK_PACKAGE 0x07
#define END_PACKAGE 0x08

typedef struct FingerEvent
{
    int (*Func)(const struct FingerEvent *);
    char *EvStr;
    uint8_t Arg1;
    uint8_t Arg2;
    atomic_int EvRelease;   //原子标志，标记事件是否可释放（0=未释放，1=可释放）
} FingerEvent;

typedef struct
{
    uint8_t Perm;
    uint8_t Data[5];
} Fingerprintf;

typedef struct
{
    uint8_t NextEnptyIndex;
    uint8_t TotalNum;
    Fingerprintf Finger[FINGER_NUM_MAX];
} FingerInfo;

typedef struct
{
    uint8_t Header[2];
    uint8_t Addr[4];
    uint8_t Type;
    uint8_t Len[2];
} FingerHead;

typedef struct
{
    FingerHead Format;
    uint8_t Affirm;
    uint8_t Data[35];
    uint8_t CheckSum[2];
} FingerDataAck;

typedef enum
{
    CODE_NULL = 0x00,
    CODE_GET_IMAGE = 0x01,          // 获取图像
    CODE_GEN_CHAR = 0x02,           // 生成特征
    CODE_SEARCH = 0x04,             // 搜索指纹
    CODE_LOAD_CHAR = 0x07,          // 读出模板
    CODE_UPLOAD_CHAR = 0x08,        // 上传模板
    CODE_DELETE_CHAR = 0x0C,        // 删除模板
    CODE_EMPTY = 0x0D,              // 清空模板
    CODE_WRITE_REG = 0x0E,          // 写系统参数
    CODE_READ_SYS_PARA = 0x0F,      // 读系统参数
    CODE_HIGH_SPEED_SEARCH = 0x1B,  // 高速搜索
    CODE_VALID_TEMPLATE_NUM = 0x1D, // 有效模板数量
    CODE_READ_INDEX_TABLE = 0x1F,   // 读索索引表
    CODE_CANCEL = 0x30,             // 取消指令
    CODE_AUTO_ENROLL = 0x31,        // 自动注册模板
    CODE_AUTO_IDENTIFY = 0x32,      // 自动验证指纹
    CODE_SLEEP = 0x33,              // 休眠
    CODE_LIGHT_SETTING = 0x3C,      // 灯光设置
    CODE_RESET = 0xF3,              // 复位
    CODE_HANDSHAKE = 0x55,           // 握手
} CodeEventType;

// 指令列表
typedef enum
{
    BREATHING = 0X01,  //呼吸灯
    FLASHING,       //闪烁灯
    CONSTANT,       //灯常开
    LOSED,          //灯常闭
    GRADUAL_OPEN,   //灯渐开
    GRADUAL_CLOSE,  //灯渐灭

#if (FINGER_MANUFACTURER == BLACK_FIRE)
    CLOSE_POWER_ON_LIGHT = 0XF5,
    OPEN_POWER_ON_LIGHT = 0XF6,
    POWER_ON_DEFAULT_SET = 0XFA,
    SUCCEE_DEFAULT_SET = 0XFB,
    FAILED_DEFAULT_SET = 0XFC,
#endif
} LightCode;

typedef enum
{
    NONE = 0,
    RED = 0X01,
    GREEN,
    YELLOW,
    BLUE,
    PINK,
    SKY_BLUE,
    WHITE,
    THREE_GUN = 0X20,
    COLORFUL = 0X30,
} LightColor;


/**
 * @description: 删除指纹事件注册
 * @param {uint8_t} Arg1    删除起始索引
 * @param {uint8_t} Arg2    删除个数
 * @return {*}
 */
void DelFingerEvnetRegister(uint8_t Arg1, uint8_t Arg2);

/**
 * @description: 指纹驱动初始化
 * @return {*}
 */
int FingerprintInit(void);

void FingerModulePowerUp(void);

void FingerModulePowerDown(void);

void FingerPowerControl(void);

int FingerLightControl(int En, LightColor Color, uint8_t Times);

#endif