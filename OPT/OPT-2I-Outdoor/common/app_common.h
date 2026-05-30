//
// Created by hemao on 2021/9/22.
//

#ifndef NET_CAMERA_APP_COMMON_H
#define NET_CAMERA_APP_COMMON_H

#include <stdbool.h>
#define OUTDOOR_LOCK_1 1
#define OUTDOOR_LOCK_2 2

enum
{
    AUTO_LIGHT = 1,
    TALKING_LIGHT,
    UNLOCK_LIGHT,
    UNGATE_LIGHT,
    INDOOR_LIGHT,
    KEY1_LED,
    KEY2_LED,
    KEY3_LED,
    KEY4_LED,
    IR_LED,
};

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

enum
{
    // RING_INDEX_1,
    // RING_INDEX_2,
    // RING_INDEX_3,
    // RING_INDEX_4,
    // RING_INDEX_5,
    // RING_INDEX_6,
    RING_INDEX_UNLOCK,
    RING_INDEX_MESSAGE_EH,
    RING_INDEX_MESSAGE_CH,
    RING_INDEX_MESSAGE_GER,
    RING_INDEX_MESSAGE_HEB,
    RING_INDEX_MESSAGE_POL,
    RING_INDEX_MESSAGE_POR,
    RING_INDEX_MESSAGE_SPA,
    RING_INDEX_MESSAGE_FRE,
    RING_INDEX_MESSAGE_JAP,
    RING_INDEX_MESSAGE_ITA,
    RING_INDEX_UNLOCK_EH,
    RING_INDEX_UNLOCK_CH,
    RING_INDEX_UNLOCK_GER,
    RING_INDEX_UNLOCK_HEB,
    RING_INDEX_UNLOCK_POL,
    RING_INDEX_UNLOCK_POR,
    RING_INDEX_UNLOCK_SPA,
    RING_INDEX_UNLOCK_FRE,
    RING_INDEX_UNLOCK_JAP,
    RING_INDEX_UNLOCK_ITA,
    RING_INDEX_CALL_BUSY,
    RING_INDEX_KEYBOARD_BI1,
    RING_INDEX_KEYBOARD_BI2,
    RING_INDEX_KEYBOARD_BI3,
    RING_INDEX_KEYBOARD_BI4,
    RING_INDEX_KEYBOARD_LONG_BI,
    RING_INDEX_KEYBOARD_DIO,
    RING_INDEX_MAX
};

void gpio_pin_init(void);

unsigned long long os_get_us(void);

unsigned long long os_get_ms(void);

unsigned long os_get_second(void);

void play_ring(unsigned int index);

bool is_play_ring(void);

bool is_audio_play_ing(void);

void play_doorbell(int ring, int vol);

void wait_doorbell_stop(void);

void stop_doorbell_ring(void);

bool door_light_control(bool light_en, int light);

void light_init(void);
void call_light_ctrl(int light);
void wait_call_light_ctrl_finish(void);

void start_call_light_ctrl_ms_set(unsigned long long ms);

// int outdoor_unlock_detect(void);

bool start_unlock(char delay_mode, int lock);

void wait_unlock_finish(void);

unsigned int eth0_mac_read(unsigned char *mac);

void watchdog_open(void);

void watch_dog_close(void);

void watch_dog_feed(void);

int get_mtd_num(void);

void play_ring_init(void);

void app_log_level(void);

bool network_call_send_cmd(int key, char ringback);

#endif // NET_CAMERA_APP_COMMON_H
