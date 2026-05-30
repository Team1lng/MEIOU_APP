#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#include "../../layout/layout_define.h"

/********************************hlf:检测氛围灯是否接入*********************** */
#define SARADC_THRESHOLD 2800
#define FILE_PATH_MAX 64
#define AK_SAR_ADC_PATCH "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw"

static bool atmosphere_connect = false;

bool atmosphere_connect_get(void)
{
    return atmosphere_connect;
}

static int ak_sardac_read_channel_data(int channel, char *result, int count)
{
    int fd = -1;
    int ret = 0;
    char path[FILE_PATH_MAX] = {0};

    memset(path, 0, FILE_PATH_MAX);
    sprintf(path, AK_SAR_ADC_PATCH, channel);

    if (!access(path, F_OK)) 
    {
        fd = open(path, O_RDONLY);
        if (fd < 0) 
        {
            printf("open %s: %s\n", path, strerror(errno));
            return -EPERM;
        }

        ret = read(fd, result, count);
        if (ret < 0) 
        {
            close(fd);
            printf("read %s: %s\n", path, strerror(errno));
            return ret;
        }

        close(fd);
    }
    else 
    {
        return -ENOENT;
    }
    return 0;
}

static void *saradc_voltage_det_thread(void *arg)
{
    int opt_val = 0;
    char value[FILE_PATH_MAX] = {0};
    int channel = 0;
    int voltage = 0;

    while(1)
    {
        memset(value, 0, FILE_PATH_MAX);
        opt_val = ak_sardac_read_channel_data(channel, value, FILE_PATH_MAX);
        if (opt_val)
        {
            if (opt_val == (-ENOENT))
            {
                printf("CHANNEL_%d: no such channel\n", channel);
            }
            else if (opt_val == (-EPERM))
            {
                printf("CHANNEL_%d: no permission\n", channel);
            }
            else
            {
                printf("CHANNEL_%d fail to get value\n", channel);
            }
        }

        voltage = atoi(value);

        if(voltage > SARADC_THRESHOLD)
        {
            atmosphere_connect = false;
            if(atmosphere_state_read() == true)
            {
                atmosphere_ctrl(false);
            }
                
        }
        else
        {
            atmosphere_connect = true;
        }

        usleep(1000*1000);
    }
    return NULL;
}

void init_saradc(void)
{
    system("insmod /usr/modules/ak_saradc.ko");

    pthread_t thread_t;
    pthread_create(&thread_t, NULL, saradc_voltage_det_thread, NULL);

}