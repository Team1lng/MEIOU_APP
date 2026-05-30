#include "stdio.h"
#include "user_data.h"
#include "scan_numeric_key.h"
#include "string.h"
#include "app_common.h"
#include "ak_common.h"
#include "detection_call.h"

#define INPUT_BUFFER_SIZE 4
#define KEYBOARD_TIMER (1000 * 30)

static struct
{
    char buffer[INPUT_BUFFER_SIZE];
    unsigned int vaild_len;
    struct ak_timeval trigger_tv;
} numeric_keyborad = {{0}};

void keyboard_event_handle(unsigned int arg1, unsigned int arg2)
{
    if (arg2 != KEY_PRESS)
    {
        return;
    }

    switch (arg1)
    {
    case KEY_PRESS_DEL:
        ak_get_ostime(&numeric_keyborad.trigger_tv);
        if (numeric_keyborad.vaild_len > 0)
        {
            numeric_keyborad.vaild_len--;
            play_doorbell(RING_INDEX_KEYBOARD_BI1, 2);
        }
        else
        {
            play_doorbell(RING_INDEX_KEYBOARD_BI3, 2);
        }
        // printf("[");
        for (int i = 0; i < INPUT_BUFFER_SIZE; i++)
        {
            if (i < numeric_keyborad.vaild_len)
                printf("%d", numeric_keyborad.buffer[i]);
            else
                printf(" ");
        }
        printf("]\r");
        fflush(stdout);
        break;
    case KEY_PRESS_AFFIRM:
        ak_get_ostime(&numeric_keyborad.trigger_tv);
        printf("***KEY_PRESS_AFFIRM***\n");
        for (int i = 0; i < numeric_keyborad.vaild_len; i++)
        {
            printf("[%d]", numeric_keyborad.buffer[i]);
        }
        printf("\n\n");
        if (numeric_keyborad.vaild_len != INPUT_BUFFER_SIZE)
        {
            play_doorbell(RING_INDEX_KEYBOARD_BI4, 2);
        }
        else
        {
            int result = 0;
            for (int i = 0; i < 10; i++)
            {
                if (memcmp(numeric_keyborad.buffer, &user_data_get()->public_code[i].password, numeric_keyborad.vaild_len) == 0 && user_data_get()->public_code[i].type != 0x00)
                {
                    if (user_data_get()->public_code[i].type & OUTDOOR_LOCK_1)
                    {
                        start_unlock(user_data_get()->lock_unlock_time, OUTDOOR_LOCK_1);
                    }
                    if (user_data_get()->public_code[i].type & OUTDOOR_LOCK_2)
                    {
                        start_unlock(user_data_get()->gate_unlock_time, OUTDOOR_LOCK_2);
                    }
                    result = 1;
                    break;
                }
            }
            play_doorbell(result ? RING_INDEX_KEYBOARD_BI2 : RING_INDEX_KEYBOARD_BI4, 2);
        }
        numeric_keyborad.vaild_len = 0;
        break;
    case KEY_PRESS_FAMILY_L:
        // ak_get_ostime(&numeric_keyborad.trigger_tv);
        // key_trigger_set(2);
        break;
    case KEY_PRESS_FAMILY_R:
        // ak_get_ostime(&numeric_keyborad.trigger_tv);
        // key_trigger_set(1);
        break;

    default:
        ak_get_ostime(&numeric_keyborad.trigger_tv);
        if (numeric_keyborad.vaild_len >= INPUT_BUFFER_SIZE)
        {
            printf("***The password length is out of range !!!!***\n");
            play_doorbell(RING_INDEX_KEYBOARD_BI4, 2);
            numeric_keyborad.vaild_len = 0;
        }
        else if (arg1 >= 0 && arg1 <= 9)
        {
            numeric_keyborad.buffer[numeric_keyborad.vaild_len++] = arg1;
            play_doorbell(RING_INDEX_KEYBOARD_BI1, 2);
            printf("[");
            for (int i = 0; i < INPUT_BUFFER_SIZE; i++)
            {
                if (i < numeric_keyborad.vaild_len)
                    printf("%d", numeric_keyborad.buffer[i]);
                else
                    printf(" ");
            }
            printf("]\n");
            fflush(stdout);
        }
        break;
    }
}

void keyboard_timer_handle(unsigned int arg1, unsigned int arg2)
{
    struct ak_timeval cur_tv;
    ak_get_ostime(&cur_tv);
    if (numeric_keyborad.vaild_len != 0 && ak_diff_ms_time(&cur_tv, &numeric_keyborad.trigger_tv) > KEYBOARD_TIMER)
    {
        numeric_keyborad.trigger_tv = cur_tv;
        numeric_keyborad.vaild_len = 0;
        play_doorbell(RING_INDEX_KEYBOARD_BI3, 2);
        printf("***If no operation is performed for a long time, the input data is cleared !!!!***\n");
    }
}

void numeric_keyboard_init(void)
{
    Numeric_key_init();
    key_press_register(keyboard_event_handle);
    key_timer_register(keyboard_timer_handle);
}