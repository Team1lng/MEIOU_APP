/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-24 15:33:36
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-10 09:07:24
 * @FilePath: /82225-EPC/src/NumericKeypad.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifdef KEYPAD_ENABLE

#include "NumericKeypad.h"
#include "VoiceRingPlay.h"
#include "LightControl.h"
#include "GpioControl.h"
#include "UserConfig.h"
#include "UserCard.h"
#include "Unlock.h"
#include "Timer.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
static void PushRouteStack(KeyAction Actiom);
static void PopRouteStack(void);
static RouteStack ActionStack;
static ActionRoute RoutesMap[ActionTotal] = {};

static void PushRouteStack(KeyAction Actiom)
{
    assert(Actiom >= KeyStandby && Actiom < ActionTotal);
    ActionRoute **CurrRoute = &(ActionStack.Routes[ActionStack.CurrentRoute]);

    if (Actiom == KeyStandby)
    {
        if (ActionStack.CurrentRoute == KeyStandby)
        {
            (*CurrRoute) = &RoutesMap[Actiom];
            printf("Enter %s\n", (*CurrRoute)->ActionStr);
        }
    }
    else
    {
        for (int i = 0; i < (*CurrRoute)->NextRouteCount; i++)
        {
            if ((*CurrRoute)->NextRoute[i] == Actiom)
            {
                if ((*CurrRoute)->ExitHandle)
                    (*CurrRoute)->ExitHandle(NULL, NULL);

                RoutesMap[Actiom].RouteData = (*CurrRoute)->RouteData;
                ActionStack.CurrentRoute++;
                printf("%s", (*CurrRoute)->ActionStr);
                ActionStack.Routes[ActionStack.CurrentRoute] = &RoutesMap[Actiom];
                printf(" => %s\n", ActionStack.Routes[ActionStack.CurrentRoute]->ActionStr);
                break;
            }
        }
    }
}

static void PopRouteStack(void)
{
    if (ActionStack.CurrentRoute)
    {
        if (ActionStack.Routes[ActionStack.CurrentRoute]->ExitHandle)
            ActionStack.Routes[ActionStack.CurrentRoute]->ExitHandle(NULL, NULL);
        ActionStack.CurrentRoute--;
        printf("%s\n", ActionStack.Routes[ActionStack.CurrentRoute]->ActionStr);
    }
}

int KeypadProess(Keyboard *KeyAttr)
{
    assert(KeyAttr->Cursor > 0);
    ActionRoute **CurrRoute = &(ActionStack.Routes[ActionStack.CurrentRoute]);
    for (int i = 0; i < (*CurrRoute)->NextRouteCount; i++)
    {
        ActionRoute Route = RoutesMap[(*CurrRoute)->NextRoute[i]];
        if (Route.Process)
        {
            if (Route.Command == NULL || memcmp(Route.Command, KeyAttr->Buff, strlen(Route.Command)) == 0)
            {
                if (Route.Process(KeyAttr, (*CurrRoute)))
                {
                    return 1;
                }
            }
        }
    }

    if ((*CurrRoute)->ErrorHandle)
        (*CurrRoute)->ErrorHandle();

    return 0;
}

int NumericKeypadInit(void)
{
    memset(&ActionStack, 0, sizeof(ActionStack));
    PushRouteStack(KeyStandby);
    return 1;
}

#endif