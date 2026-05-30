/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-24 15:33:43
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-09 08:36:20
 * @FilePath: /82225-EPC/src/NumericKeypad.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _NUMERIC_KEYPAD_H_
#define _NUMERIC_KEYPAD_H_
#include "time.h"

#define KEYPAD_BUFFER_SIZE 8

typedef enum
{
    KEY0,
    KEY1,
    KEY2,
    KEY3,
    KEY4,
    KEY5,
    KEY6,
    KEY7,
    KEY8,
    KEY9,
    KEYSTAR,
    KEYPOUND,
} KeyVaule;
typedef enum
{
    KeyStandby,
    KeyCodeUnlock,
    KeyStandbyReady,
    KeyStandbyReturn,
    KeyReturn,

    KeyNewCardCode,
    KeyAffiCardCode,
    KeyCardCodeVerify,

    KeyAdmin,

    KeyNewAdminCode,
    KeyAffiAdminCode,
    KeyAdminCodeVerify,

    KeyNewLockCode,
    KeyAffiLockCode,
    KeyLockCodeVerify,

    KeyNewGateCode,
    KeyAffiGateCode,
    KeyGateCodeVerify,

    KeyUnlockTime,
    KeyUngateTime,

    KeyReset,

    KeyBacklightTime,

    KeyLockWay,
    KeySafeMode,
    KeyPublicUnlockEn,

    KeyAddCard,
    KeyAddMoreCard,
    KeyDelCard,

    KeyLanguage,
    ActionTotal
} KeyAction;

typedef struct
{
    int Cursor;
    char Buff[KEYPAD_BUFFER_SIZE];
    struct timespec Time;
} Keyboard;

typedef struct ActionRoute
{
    char *ActionStr;
    char *Command;
    int (*Process)(Keyboard *KeyAttr, struct ActionRoute *CurrRoute);
    void (*ErrorHandle)(void);
    int (*ExitHandle)(Keyboard *KeyAttr, struct ActionRoute *CurrRoute);
    KeyAction NextRoute[ActionTotal];
    char NextRouteCount;
    void *RouteData;
} ActionRoute;

typedef struct
{
    ActionRoute *Routes[ActionTotal];
} RouteMap;

typedef struct
{
    ActionRoute *Routes[ActionTotal];
    int CurrentRoute;
} RouteStack;

void KeypadLightDisable(void *us);

void KeypadLightEnable(void);

int KeypadProess(Keyboard *KeyAttr);

int NumericKeypadInit(void);
#endif