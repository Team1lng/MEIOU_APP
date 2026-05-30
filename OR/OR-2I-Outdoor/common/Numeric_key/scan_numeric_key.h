#ifndef _SCAN_NUMERIC_KEY_H_
#define _SCAN_NUMERIC_KEY_H_

#define KEY_PRESS_DEL   10
#define KEY_PRESS_AFFIRM  11
#define KEY_PRESS_FAMILY_L   12
#define KEY_PRESS_FAMILY_R  13
#define KEY_NONE_VOL    (16)

#define DETECT_PIN_1    6
#define DETECT_PIN_2    7
#define DETECT_PIN_3    8

typedef void (*KEY_EVENT_CALLBACK)(unsigned int arg1,unsigned int arg2);

typedef enum
{
    KEY_VOL_0,
    KEY_VOL_1,
    KEY_VOL_2,
    KEY_VOL_3,
    KEY_VOL_4,
    KEY_VOL_5,
    KEY_VOL_6 = 7,
    KEY_VOL_7 = 10,
    KEY_VOL_8 = 13,
    KEY_VOL_9 = 12,
    KEY_VOL_DEL = 15,
    KEY_VOL_AFFIRM = 9,
    KEY_VOL_FAMILY_L = 8,
    KEY_VOL_FAMILY_R = 6,
    KEY_VOL_NONE = KEY_NONE_VOL,
    KEY_VOL_TOTAL = 24,
}KEY_VOL_INDEX;

typedef enum
{
    NONE_KEY_EVENT,
    KEY_PRESS,
    KEY_HOLD,
    KEY_RELEASE
}KEY_PRESS_EVENT;

void Numeric_key_init(void);
void key_press_register(KEY_EVENT_CALLBACK func);
void key_timer_register(KEY_EVENT_CALLBACK func);
#endif