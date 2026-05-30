/*******************************************************************
 * @Descripttion   :
 * @version        : 1.0.0
 * @Author         : wxj
 * @Date           : 2022-11-05 09:32
 * @LastEditTime   : 2022-11-05 09:52
 *******************************************************************/
#include "user_data.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define USER_DATA_PATH "/etc/config/user_data.cfg"

static user_data_info user_data = {0};
static user_data_info user_data_default = {
    .app_version = APP_VERSION,
    .public_code_enable = true,

    .open_way = CARD_OR_CODE,
    .safe_mode = CLOSE_SAFE_MODE,
    .operate_mode = USER_STANDBY_MODE,
    .lock_unlock_time = 3,
    .gate_unlock_time = 1,
    .exit_button_lock = true,
    .exit_button_gate1 = false,

    .language = POLISH,
#if (USER_CODE_LEN == 6)
    .public_lock_code = {1, 2, 3, 4, 5, 6, '\0'},
    .public_gate1_code = {4, 5, 6, 7, 8, 9, '\0'},
    .public_code = {{0}},
#else
    .public_lock_code = {1, 2, 3, 4, '\0'},
    .public_gate1_code = {5, 6, 7, 8, '\0'},
    .public_code = {{0}},
#endif
    .talk_volume = 7,
    .key_led_time = 0,
};

void user_code_permission_set(int index, char permission)
{
    if (index < USER_CODE_GROUP)
    {
        if (user_data.public_code[index].type != permission)
        {
            user_data.public_code[index].type = permission;
            user_data_save();
        }
    }
}

void user_code_add(int password)
{
    for (int i = 0; i < USER_CODE_GROUP; i++)
    {
        if (user_data.public_code[i].type == 0)
        {
            memset(user_data.public_code[i].password, 0, USER_CODE_LEN);
            for (int j = USER_CODE_LEN - 1; j >= 0; j--)
            {
                user_data.public_code[i].password[j] = password % 10;
                password = password / 10;
            }

            user_data.public_code[i].type = 3;
            break;
        }
    }
    user_data_save();
}

void user_code_del(int index)
{
    if (index == 200)
    {
        memset(user_data.public_code, 0, sizeof(user_code_info) * USER_CODE_GROUP);
    }
    else if (index < USER_CODE_GROUP)
    {
        memset(user_data.public_code[index].password, 0, USER_CODE_LEN);
        user_data.public_code[index].type = 0;
    }
    user_data_save();
}

bool user_data_save(void)
{
    int fd = open(USER_DATA_PATH, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        printf("write open %s fail \n", USER_DATA_PATH);
        return false;
    }

    write(fd, &user_data, sizeof(user_data_info));

    close(fd);
    system("sync");
    return true;
}

user_data_info *user_data_get(void)
{
    return &user_data;
}

bool user_data_init(void)
{
    int fd = open(USER_DATA_PATH, O_RDONLY);
    if (fd < 0)
    {
        user_data = user_data_default;
        user_data_save();
        system("sync");
        printf("[%s]read open %s fail \n", __func__, USER_DATA_PATH);
        return false;
    }

    read(fd, &user_data, sizeof(user_data_info));
    if (user_data.app_version != user_data_default.app_version)
    {
        user_data_reset();
        // user_swiping_card_reset();
    }

    close(fd);

    return true;
}

bool user_data_reset(void)
{
    user_data = user_data_default;
    printf("%s===========++>>,%d,%d\n", __func__, user_data.lock_unlock_time, user_data.gate_unlock_time);
    user_data_save();

    void Default_unlock_time_packet(void);
    Default_unlock_time_packet();

    void Default_exit_button_packet(void);
    Default_exit_button_packet();
    return true;
}
