/*******************************************************************
 * @Descripttion   : 
 * @version        : 1.0.0
 * @Author         : wxj
 * @Date           : 2022-11-05 09:54
 * @LastEditTime   : 2022-11-07 13:36
*******************************************************************/
#ifndef _CARD_MANAGE_H_
#define _CARD_MANAGE_H_

#include <stdbool.h>

#include"uart_ctrl.h"
#define IC_DATA_LEN   5
#define ID_DATA_LEN   4

#define USER_CARD_DATA_PATH "/etc/config/user_card_data.cfg"
#define CARD_DATA_LEN   5
#define USER_DEFINE_CARD_MAX 200


#define NULL_PERMISSION 0
#define LOCK_PERMISSION 1
#define GATE_PERMISSION 2

#define CARD_TYPE_PIN   31

typedef enum
{
    CARD_STATE_SWIPE_CARD = 0x00,
    CARD_STATE_ADD_CARD
} card_state_t;

typedef struct 
{
    char lock_type;//用户卡所支持卡的类型
    char card[CARD_DATA_LEN];
}card_data;

typedef struct 
{
    unsigned int card_number;
    card_data info[USER_DEFINE_CARD_MAX];
}card_data_group;

typedef enum
{
    CARD_EVENT_NULL = 0x00,
    CARD_EVENT_START_ADD_CARD,
    CARD_EVENT_SUCCESS_ADD_CARD,
    CARD_EVENT_FAIL_ADD_CARD,
    CARD_EVENT_STOP_ADD_CARD,
    CARD_EVENT_SUCCESS_SWIPE_CARD,
    CARD_EVENT_FAIL_SWIPE_CARD,
} card_event_t;

typedef struct 
{
    card_event_t type;
    unsigned int arg1;
    unsigned int arg2;
}card_event_node;

bool user_card_data_init(void);
card_data_group* user_card_data_get(void);
bool card_data_delete(int index);
bool set_card_permission(int index,char permission);
void card_event_trigger(card_event_t event,unsigned int arg1,unsigned int arg2);

bool card_drive_init(void);
void network_card_event_receive(unsigned char event);
void card_data_sync_handler(unsigned char total, char *buff);
void card_scan_stop(void);
card_state_t card_manage_state_get(void);

#endif