/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-22 16:27:09
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-05 11:10:13
 * @FilePath: /82225-EPC/common/AkGpioControl/AdcControl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "AdcControl.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>

#define AK_SARADC_PATH "/usr/modules/ak_saradc.ko"
#define SARADC_DEV "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define DEFAULT_VALUE 3299

/**
 * @description: ADC检测处理函数
 * @param {int} Voltage ADC采样值
 * @return {*}
 */
__attribute__((weak)) int AdcDetectHandle(int Voltage)
{
    // printf("[%s] Voltage:%d\n", __func__, Voltage);
    return 0;
}

static void *DrvAdcDetectThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    assert((access(SARADC_DEV, F_OK) == 0));
    int Fd;
    char raw[64] = {0};
    int Voltage = DEFAULT_VALUE;
    if ((Fd = open(SARADC_DEV, O_RDONLY)) < 0)
    {
        return NULL;
    }
    while (1)
    {
        lseek(Fd, 0, SEEK_SET);
        memset(raw, 0, sizeof(raw));
        if (read(Fd, raw, sizeof(raw)) > 0)
        {
            Voltage = atoi(raw);
            AdcDetectHandle(Voltage);
        }

        usleep(1000 * 10);
    }
    close(Fd);
    return NULL;
}

/**
 * @description: ADC检测初始化
 * @return {*}
 */
int AkDrvAdcInit(void)
{
    assert(access(AK_SARADC_PATH, F_OK) == 0);
    system("insmod " AK_SARADC_PATH);
    pthread_t Thread;
    pthread_create(&Thread, NULL, DrvAdcDetectThread, NULL);
    pthread_detach(Thread);
    return 0;
}