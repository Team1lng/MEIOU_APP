/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-11-08 11:19:09
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-27 17:27:19
 * @FilePath:  DrvSimulatingI2c.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _DRV_SIMULATING_I2C_H_
#define _DRV_SIMULATING_I2C_H_

typedef struct
{
    unsigned char SlaAddr;
    unsigned char RawAddr;
    unsigned char Data[4];
    unsigned char DataLen;
} i2c_data;

/**
 * @description: 模拟I2C驱动初始化
 * @return {*}
 */
int DrvSimI2cInit(void);

/**
 * @description: 写数据到模拟I2C设备
 * @param {char} addr：从机地址
 * @param {char} raw：寄存器地址
 * @param {unsigned char} value：数据
 * @return {*}
 */
int DrvSimI2cWrite(unsigned char addr, unsigned char raw, unsigned char value);

/**
 * @description: 读取模拟I2C设备
 * @param {char} addr：从机地址
 * @param {char} raw：寄存器地址
 * @param {char} *buf：数据缓冲
 * @return {*}
 */
int DrvSimI2cRead(unsigned char addr, unsigned char raw, unsigned char *buf, unsigned int len);
/**
 * @description: 添加模拟I2C互斥锁
 * @return {*}
 */
void DrvSimI2cMutexUnlock(void);

/**
 * @description: 释放模拟I2C互斥锁
 * @return {*}
 */
void DrvSimI2cMutexLock(void);
#endif