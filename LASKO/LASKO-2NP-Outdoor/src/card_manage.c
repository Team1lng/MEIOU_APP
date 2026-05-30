/*******************************************************************
 * @Descripttion   :
 * @version        : 1.0.0
 * @Author         : wxj
 * @Date           : 2022-11-05 09:54
 * @LastEditTime   : 2022-11-07 15:42
 *******************************************************************/
#include "card_manage.h"
#include "audio_output.h"
#include "audio_input.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <memory.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/poll.h>
#include "ak_common.h"
#include "user_data.h"
#include "app_common.h"
#include "leo_pwm.h"
#include "gpio_base.h"
#include "network_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "tcp_network_cmd.h"

static int dev_fd;
static int RECEIVE_DATA_LEN;
static pthread_mutex_t card_event_mutex;
static bool card_scan_start(int fd);
static void printf_card_receive_data(char *data, int len);
static int receive_card_data_check(char *data, int len);
static void receive_buffer_clear(int fb);
static int receive_buffer_size(int fb);

card_data_group user_card_data;
bool user_swiping_card_save(void)
{
    int fd = open(USER_CARD_DATA_PATH, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        printf("write open %s fail \n", USER_CARD_DATA_PATH);
        return false;
    }

    write(fd, &user_card_data, sizeof(card_data_group));

    close(fd);
    system("sync");
    return true;
}

bool user_swiping_card_reset(void)
{
    memset(&user_card_data, 0, sizeof(user_card_data));
    user_swiping_card_save();
    system("sync");
    return true;
}

bool user_card_data_init(void)
{
    int fd = open(USER_CARD_DATA_PATH, O_RDONLY);
    if (fd < 0)
    {
        memset(&user_card_data, 0, sizeof(card_data_group));
        user_swiping_card_save();
        printf("[%s]read open %s fail \n", __func__, USER_CARD_DATA_PATH);
        return false;
    }

    read(fd, &user_card_data, sizeof(card_data_group));

    for (int i = 0; i < 8; i++)
    {
        printf("[%x]      [%x][%x][%x][%x][%x]\n", user_card_data.info[i].lock_type, user_card_data.info[i].card[0], user_card_data.info[i].card[1], user_card_data.info[i].card[2], user_card_data.info[i].card[3], user_card_data.info[i].card[4]);
    }
    close(fd);

    return true;
}

card_data_group *user_card_data_get(void)
{
    return &user_card_data;
}

bool card_data_add(char *data)
{
    int valid_Traversal = 0;                     // 当前遍历有效卡片个数
    int Null_index = user_card_data.card_number; // 默认当前空卡号位置为卡片最末位
    if (user_card_data.card_number < USER_DEFINE_CARD_MAX)
    {
        for (int i = 0; i < USER_DEFINE_CARD_MAX; i++)
        {
            if (user_card_data.info[i].lock_type != NULL_PERMISSION) // 若权限不为空即当前卡位已被占用
            {
                valid_Traversal++;
                if (memcmp(data, user_card_data.info[i].card, CARD_DATA_LEN) == 0) // 卡号已存在
                {
                    printf("[The card already exists ...]\n");
                    return false;
                }
            }
            else if (Null_index == user_card_data.card_number) // 记录第一个空卡位，待存放卡信息
            {
                Null_index = i;
            }

            if (valid_Traversal == user_card_data.card_number) // 当前所有有效卡遍历完，未检索到需要添加的卡片
            {
                break;
            }
        }

        /* 添加新卡信息 */
        user_card_data.info[Null_index].lock_type = GATE_PERMISSION | LOCK_PERMISSION;
        memcpy(user_card_data.info[Null_index].card, data, CARD_DATA_LEN);
        user_card_data.card_number++;

        user_swiping_card_save();
        printf("[Added successfully] card number index:%d   Current number of cards:%d\n", Null_index, user_card_data.card_number);
        return true;
    }
    return false;
}

bool card_data_delete(int index)
{
    if (index < 0 || index > USER_DEFINE_CARD_MAX)
    {
        printf("card_data_delete  card index error\n");
        return false;
    }

    if (index == USER_DEFINE_CARD_MAX)
    {
        user_swiping_card_reset();
        printf("[Delete all successfully] Current number of cards:%d\n", user_card_data.card_number);
        return true;
    }
    else if (user_card_data.info[index].lock_type != NULL_PERMISSION)
    {
        memset(&(user_card_data.info[index]), 0, sizeof(card_data));
        user_card_data.card_number--;
        printf("[Delete index :%d successfully] Current number of cards:%d\n", index, user_card_data.card_number);
        user_swiping_card_save();
        return true;
    }

    printf("[Delete fail] The card bit data is empty !!!!!!!!\n");
    return false;
}

bool set_card_permission(int index, char permission)
{
    if (index < USER_DEFINE_CARD_MAX)
    {
        if (user_card_data.info[index].lock_type != NULL_PERMISSION)
        {
            user_card_data.info[index].lock_type = permission;
            user_swiping_card_save();
            return true;
        }
    }
    return false;
}

#define PWM_DEVICE_ID 3 // 原理图上没有pwm0的话，pwm的索引就需要减1
#define RFID_DEV_PATH "/dev/rfid_control"
#define RFID_MODULE_KO "/usr/modules/rfid.ko"
#define SWIPE_SAME_CARD_DELAY 1000 // 刷同一张卡延时

static card_state_t card_manage_state = CARD_STATE_SWIPE_CARD;
static card_event_node card_event = {.type = CARD_EVENT_NULL, .arg1 = 0x00, .arg2 = 0x00};
static unsigned long long add_card_idle_time = 0;

void card_event_trigger(card_event_t event, unsigned int arg1, unsigned int arg2)
{
    // if(card_event.type != CARD_EVENT_NULL)
    //     return ;

    pthread_mutex_lock(&card_event_mutex);
    card_event.type = event;
    card_event.arg1 = arg1;
    card_event.arg2 = arg2;
    pthread_mutex_unlock(&card_event_mutex);
}

bool card_type_check(void)
{
    static GPIO_LEVEL level = GPIO_LEVEL_UNKOWN;

    if (level == GPIO_LEVEL_UNKOWN)
    {
        gpio_read(CARD_TYPE_PIN, &level);
        printf("%s ===================>>%s\n", __func__, level == GPIO_LEVEL_LOW ? "UART" : "PWM");
    }
    return level == GPIO_LEVEL_LOW ? true : false;
}

bool card_drive_init(void)
{
    int rfid_fd;
    if (card_type_check())
    {
        RECEIVE_DATA_LEN = IC_DATA_LEN;

        rfid_fd = uart_open("ttySAK1", 9600, 8, 1, 'n');
        if (rfid_fd < 0)
        {
            printf("open ttySAK1 faild \n");
            usleep(1000 * 1000);
            return false;
        }
    }
    else
    {

        RECEIVE_DATA_LEN = ID_DATA_LEN;
        if (ak_drv_pwm_open(PWM_DEVICE_ID) != 0)
        {
            printf("ak_drv_pwm_open===================>>fail\n");
            return false;
        }
        if (ak_drv_pwm_set(PWM_DEVICE_ID, 4000, 8000) != 0)
        {
            printf("ak_drv_pwm_set===================>>fail\n");
            return false;
        }
        system("insmod " RFID_MODULE_KO);
        rfid_fd = open(RFID_DEV_PATH, O_RDONLY);
        printf("%s ===================>>rfid_fd:%d\n", __func__, rfid_fd);
    }

    card_scan_start(rfid_fd);

    card_manage_state = CARD_STATE_SWIPE_CARD;
    card_event_trigger(CARD_EVENT_NULL, 0x00, 0x00);
    return true;
}

void network_card_event_receive(unsigned char event)
{
    printf("======================>>>> event:[%d]\n", event);
    card_event_trigger(event, 0x00, 0x00);
}

void card_event_handler(void)
{
    switch (card_event.type)
    {
    case CARD_EVENT_START_ADD_CARD:
        printf("===================>>> 进入加卡状态\n");
        card_manage_state = CARD_STATE_ADD_CARD;
        add_card_idle_time = os_get_ms();
        play_doorbell(RING_INDEX_KEYBOARD_BI3, 7);
        break;

    case CARD_EVENT_SUCCESS_ADD_CARD:
        printf("===================>>> 加卡成功\n");
        play_doorbell(RING_INDEX_KEYBOARD_BI2, 7);
        extern bool tcp_network_cmd_add_card_result_send(bool result);
        tcp_network_cmd_add_card_result_send(true);
        break;

    case CARD_EVENT_FAIL_ADD_CARD:
        printf("===================>>> 加卡失败\n");
        play_doorbell(RING_INDEX_KEYBOARD_BI4, 7);
        extern bool tcp_network_cmd_add_card_result_send(bool result);
        tcp_network_cmd_add_card_result_send(false);
        break;

    case CARD_EVENT_STOP_ADD_CARD:
        printf("===================>>> 退出加卡状态\n");
        card_manage_state = CARD_STATE_SWIPE_CARD;
        play_doorbell(RING_INDEX_KEYBOARD_BI4, 7);
        break;

    case CARD_EVENT_SUCCESS_SWIPE_CARD:
        printf("====>>> 刷卡成功,card_event.arg1:%d,gate_unlock_time:%d,lock_unlock_time:%d\n", card_event.arg1, user_data_get()->gate_unlock_time, user_data_get()->lock_unlock_time);

        play_doorbell(RING_INDEX_KEYBOARD_BI1, 7);

        if (card_event.arg1 & OUTDOOR_LOCK_2)
            start_unlock(user_data_get()->gate_unlock_time, OUTDOOR_LOCK_2);

        if (card_event.arg1 & OUTDOOR_LOCK_1)
            start_unlock(user_data_get()->lock_unlock_time, OUTDOOR_LOCK_1);
        break;

    case CARD_EVENT_FAIL_SWIPE_CARD:
        printf("===================>>> 刷卡失败\n");

        play_doorbell(RING_INDEX_KEYBOARD_BI4, 7);

        break;
    default:
        break;
    }

    card_event_trigger(CARD_EVENT_NULL, 0x00, 0x00);
}

void card_data_sync_handler(unsigned char total, char *buff)
{
    user_data_get()->card.total = total;
    printf("============>>> card total : [%d]\n", user_data_get()->card.total);

    memset(&(user_data_get()->card.data), 0, CARD_DATA_TOTAL_SIZE);
    memcpy(&(user_data_get()->card.data), buff, CARD_DATA_TOTAL_SIZE);

    for (int i = 0; i < UAER_CARD_MAX; i++)
    {
        if (user_data_get()->card.data[i].seriel_number != 0)
            printf("===NO.[%d]=====card num:[%02x %02x %02x %02x]\n",
                   user_data_get()->card.data[i].seriel_number,
                   user_data_get()->card.data[i].card_number[0],
                   user_data_get()->card.data[i].card_number[1],
                   user_data_get()->card.data[i].card_number[2],
                   user_data_get()->card.data[i].card_number[3]);
    }
    user_data_save();
}

static int card_number_check(char *data, int len)
{
    if (len != CARD_DATA_LEN)
        return -1;

    if (user_card_data_get()->card_number != 0)
    {
        for (int i = 0; i < USER_DEFINE_CARD_MAX; i++)
        {
            if (memcmp(data, user_card_data_get()->info[i].card, CARD_DATA_LEN) == 0)
                return i;
        }
    }

    return -1;
}

// 短时间多次刷同一张卡检测
// true:相同卡 false:不同卡
static bool same_card_number_check_time_out(char *data)
{
    static unsigned long long count = 0;
    static char prev_card[CARD_DATA_LEN] = {0};
    if (memcmp(data, prev_card, CARD_DATA_LEN) == 0)
    {
        if (os_get_ms() - count < SWIPE_SAME_CARD_DELAY)
        {
            return true;
        }
    }
    memcpy(prev_card, data, CARD_DATA_LEN);
    count = os_get_ms();
    return false;
}

static bool card_scan_task_running = false;
static void *card_scan_task(void *arg)
{
    char receive_data[32] = {0};
    char data[CARD_DATA_LEN] = {0};
    int rfid_fd = *(int *)arg;
    int vaild_index;
    int receive_len;
    int receive_buffer_len;
    if (rfid_fd < 0)
    {
        printf("open failed : [%s] \n", __func__);
        pthread_exit(NULL);
    }
    while (card_scan_task_running)
    {
        memset(receive_data, 0, sizeof(receive_data));

        // printf("%s======================================>>>>uart_buffer_size:%d\n",__func__,uart_buffer_size(rfid_fd));
        // ak_sleep_ms(100);
        // continue;
        receive_buffer_len = receive_buffer_size(rfid_fd);
        if (receive_buffer_len > RECEIVE_DATA_LEN)
        {
            // printf("%s======================================>>>>uart_buffer_size:%d\n", __func__, receive_buffer_len);
            receive_buffer_clear(rfid_fd);
        }

        if (!(receive_buffer_len % RECEIVE_DATA_LEN) && (receive_len = read(rfid_fd, receive_data, RECEIVE_DATA_LEN)) == RECEIVE_DATA_LEN) // 卡有效数据只有4字节，为了兼容之前的刷卡管理功能，将存储大小更改至5字节，末尾为0
        {
            printf_card_receive_data(receive_data, receive_len);
            if ((vaild_index = receive_card_data_check(receive_data, RECEIVE_DATA_LEN)) != -1)
            {
                memcpy(data, &receive_data[vaild_index], sizeof(data));
            }
            // printf("%s======================================>>>>receive_len:%d  vaild_index:%d\n",__func__,receive_len,vaild_index);
            if (same_card_number_check_time_out(data) == false && vaild_index != -1)
            {
                printf("CARD_MANAGE_STATE :%d    card number : [%02x %02x %02x %02x]\n", card_manage_state, data[0], data[1], data[2], data[3]);
                if (card_manage_state == CARD_STATE_SWIPE_CARD)
                {
                    int card_index = -1;
                    if ((card_index = card_number_check(data, sizeof(data))) != -1)
                    {
                        card_event_trigger(CARD_EVENT_SUCCESS_SWIPE_CARD, user_card_data.info[card_index].lock_type, 0x00);

                        printf("CARD_EVENT_SUCCESS_SWIPE_CARD : [%02x %02x %02x %02x] intdex :%d     type:%d\n", data[0], data[1], data[2], data[3], card_index, user_card_data.info[card_index].lock_type);
                    }
                    else
                    {
                        card_event_trigger(CARD_EVENT_FAIL_SWIPE_CARD, 0x00, 0x00);
                    }
                }
                else if (card_manage_state == CARD_STATE_ADD_CARD)
                {
                    add_card_idle_time = os_get_ms();
                    // network_sendaddcard_cmd_data(data);
                    printf("CARD_STATE_ADD_CARD : [%02x %02x %02x %02x]\n", data[0], data[1], data[2], data[3]);
                    if (card_data_add(data))
                    {
                        card_event_trigger(CARD_EVENT_SUCCESS_ADD_CARD, 0x00, 0x00);
                    }
                    else
                    {
                        card_event_trigger(CARD_EVENT_FAIL_ADD_CARD, 0x00, 0x00);
                    }
                }
            }
        }

        if (card_manage_state == CARD_STATE_ADD_CARD && os_get_ms() - add_card_idle_time > 30000)
        {
            card_event_trigger(CARD_EVENT_STOP_ADD_CARD, 0x00, 0x00);

            printf("[Exit the add card mode]\n\n");
            add_card_idle_time = 0;
        }

        // card_event_handler();
        ak_sleep_ms(50);
    }
    pthread_exit(NULL);
    return NULL;
}

static bool card_scan_start(int fd)
{
    if (card_scan_task_running == true)
    {
        return false;
    }
    card_scan_task_running = true;
    pthread_t add_card_pthread_id;
    dev_fd = fd;
    pthread_mutex_init(&card_event_mutex, NULL);
    pthread_create(&add_card_pthread_id, NULL, card_scan_task, &dev_fd);
    return true;
}

void card_scan_stop(void)
{
    card_scan_task_running = false;
}

card_state_t card_manage_state_get(void)
{
    return card_manage_state;
}

static void printf_card_receive_data(char *data, int len)
{
    printf("[receive card data]\n");
    for (int i = 0; i < len; i++)
        printf("0x%02x ", data[i]);
    printf("\n\n");
}

static int receive_card_data_check(char *data, int len)
{
    if (card_type_check())
    {
        if ((data[0] ^ data[1] ^ data[2] ^ data[3]) == data[4])
        {
            printf("%s   start:0x%02x over:0x%02x\n", __func__, data[0], data[len - 1]);
            return 0;
        }

        return -1;
    }
    else
        return 0;
}

static void receive_buffer_clear(int fb)
{
    if (card_type_check())
        uart_clear(fb);

    return;
}

static int receive_buffer_size(int fb)
{
    if (card_type_check())
        return uart_buffer_size(fb);
    else
        return 0;
}