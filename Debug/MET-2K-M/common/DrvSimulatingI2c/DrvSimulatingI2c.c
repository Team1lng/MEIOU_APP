/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-11-08 10:33:06
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-28 11:20:24
 * @FilePath: /82227-EPC-FHD/common/sim_i2c_drive_api.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "DrvSimulatingI2c.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#define Debug (printf("\n\033[0;32;40m[***%s***]:%u\033[0m \t", __PRETTY_FUNCTION__, __LINE__), printf)

#define SIMULATED_I2C_DRIVE_PATH "/usr/modules/sim_idrv.ko"
#define SIMULATED_I2C_DEV_PATH "/dev/SIMULAT_I2C"

static int SimI2cFd = -1;
static pthread_mutex_t SimI2cMutex = PTHREAD_MUTEX_INITIALIZER;
;

/**
 * @description: 模拟I2C驱动初始化
 * @return {*}
 */
int DrvSimI2cInit(void)
{
    if (SimI2cFd != -1)
    {
        return -1;
    }

    if (access(SIMULATED_I2C_DRIVE_PATH, F_OK) != 0)
    {
        Debug("%s no exist\n", SIMULATED_I2C_DRIVE_PATH);
        return -1;
    }

    system("insmod " SIMULATED_I2C_DRIVE_PATH);

    SimI2cFd = open(SIMULATED_I2C_DEV_PATH, O_RDWR);
    if (SimI2cFd == -1)
    {
        Debug("open i2c(%s) devices fail \n\r", SIMULATED_I2C_DEV_PATH);
        return -1;
    }
    else
    {
        Debug("open i2c(%s) devices succeed! \n\r", SIMULATED_I2C_DEV_PATH);
    }

    return 0;
}

/**
 * @description: 写数据到模拟I2C设备
 * @param {char} addr：从机地址
 * @param {char} raw：寄存器地址
 * @param {unsigned char} value：数据
 * @return {*}
 */
int DrvSimI2cWrite(unsigned char addr, unsigned char raw, unsigned char value)
{
    int Reslut = 0;
    i2c_data Data = {.SlaAddr = addr, .RawAddr = raw, .Data[0] = value};
    pthread_mutex_lock(&SimI2cMutex);
    write(SimI2cFd, &Data, sizeof(i2c_data));
    pthread_mutex_unlock(&SimI2cMutex);
    return Reslut;
}

/**
 * @description: 读取模拟I2C设备
 * @param {char} addr：从机地址
 * @param {char} raw：寄存器地址
 * @param {char} *buf：数据缓冲
 * @return {*}
 */
int DrvSimI2cRead(unsigned char addr, unsigned char raw, unsigned char *buf, unsigned int len)
{
    int Reslut = 0;
    int ReadLen = sizeof(i2c_data);
    i2c_data Data = {.SlaAddr = addr, .RawAddr = raw, .DataLen = len};
    pthread_mutex_lock(&SimI2cMutex);

    if ((ReadLen = read(SimI2cFd, &Data, ReadLen)) != len)
    {
        Debug("read addr:%02x fail ,ReadLen:%d,len:%d\n\r", addr, ReadLen, len);
        Reslut = -1;
        goto finish;
    }

    memcpy(buf, Data.Data, ReadLen);
finish:
    pthread_mutex_unlock(&SimI2cMutex);
    return Reslut;
}

/**
 * @description: 添加模拟I2C互斥锁
 * @return {*}
 */
void DrvSimI2cMutexUnlock(void)
{
    pthread_mutex_unlock(&SimI2cMutex);
}

/**
 * @description: 释放模拟I2C互斥锁
 * @return {*}
 */
void DrvSimI2cMutexLock(void)
{
    pthread_mutex_lock(&SimI2cMutex);
}
