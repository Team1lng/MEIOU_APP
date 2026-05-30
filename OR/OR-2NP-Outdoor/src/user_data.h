/*******************************************************************
 * @Descripttion   :
 * @version        : 1.0.0
 * @Author         : wxj
 * @Date           : 2022-11-05 09:32
 * @LastEditTime   : 2022-11-05 11:34
 *******************************************************************/
#ifndef _USER_DATA_H_
#define _USER_DATA_H_
#include "stdbool.h"
#include <stdbool.h>
#define UAER_CARD_MAX 40
#define USER_DATA_PATH "/etc/config/user_data.cfg"
#define USER_DATA_ECTYPE_PATH "/etc/config/user_data_ectype.cfg"
#define USER_CODE_LEN 5
#define USER_CODE_GROUP 10
#define APP_VERSION 9
#define USER_CARD_NUMBER_SIZE 4    // byte,卡号
#define USER_CARD_SERIAL_NO_SIZE 1 // byte,序号
#define CARD_DATA_TOTAL_SIZE (UAER_CARD_MAX * (USER_CARD_NUMBER_SIZE + USER_CARD_SERIAL_NO_SIZE))

typedef enum
{
    CARD_ONLY = 1,
    CARD_OR_CODE,
    CARD_AND_CODE,
    TOTAL_WAY
} open_door_way;

typedef enum
{
    CLOSE_SAFE_MODE,
    LOCKED_MODE,
    ALARM_MODE,
    TOTAL_MODE
} user_safe_mode;

typedef enum
{
    USER_STANDBY_MODE,
    USER_ADMIN_MODE,
    USER_ADD_CARD,
    USER_DEL_CARD,
} user_operation_mode;

typedef enum
{
    ENGLISH = 0,
    CHINESE = 1,
    GERMANY,
    HEBREW,
    POLISH,
    PORTUGAL,
    SPAIN,
    FRENCH,
    JAPANESE,
    ITALIAN,
    CZECH,
    SLOVAK,
    MAGYAR,
    ROMANIAN,
    SLOVENIAN,
    LANGUAGE_TOTAL,
} user_language;

struct _CARD_DATA_
{
    unsigned char seriel_number;
    char card_number[USER_CARD_NUMBER_SIZE];
};
typedef struct
{
    char total;
    struct _CARD_DATA_ data[UAER_CARD_MAX];
} user_card_info;

typedef struct
{
    unsigned char type;
    char password[USER_CODE_LEN];
} user_code_info;

typedef struct
{
    int app_version;
    bool public_code_enable;
    user_card_info card;
    open_door_way open_way;
    user_safe_mode safe_mode;
    user_operation_mode operate_mode;

    int lock_unlock_time;
    int gate_unlock_time;
    bool exit_button_lock;
    bool exit_button_gate1;

    user_language language;

    char public_lock_code[USER_CODE_LEN + 1];
    char public_gate1_code[USER_CODE_LEN + 1];
    user_code_info public_code[USER_CODE_GROUP];

    int talk_volume;
    int key_led_time;
} user_data_info;

void user_code_permission_set(int index, char permission);
void user_code_add(int password);
void user_code_del(int password);

bool user_data_save(void);
bool user_data_init(void);

user_data_info *user_data_get(void);
bool user_data_reset(void);

#endif