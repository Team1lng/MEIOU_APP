/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-25 15:32:02
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-15 16:28:07
 * @FilePath: /project_3/src/PwmIdCard.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _PWM_ID_CARD_H_
#define _PWM_ID_CARD_H_

typedef enum
{
    PWM_ID_CARD_CHECK,
    PWM_ID_CARD_ADD,
} PwmIdAction;

typedef enum
{
    CARD_EVENT_NULL = 0x00,
    CARD_EVENT_START_ADD_CARD,
    CARD_EVENT_SUCCESS_ADD_CARD,
    CARD_EVENT_FAIL_ADD_CARD,
    CARD_EVENT_STOP_ADD_CARD,
    CARD_EVENT_SUCCESS_SWIPE_CARD,
    CARD_EVENT_FAIL_SWIPE_CARD,
} PwmIdEvent;

#define PWM_ID_CARD_LEN 4
#endif