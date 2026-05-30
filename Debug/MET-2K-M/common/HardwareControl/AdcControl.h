/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-22 16:27:14
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-01-22 20:55:23
 * @FilePath: /82225-EPC/common/AkGpioControl/AdcControl.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _ADC_CONTROL_H_
#define _ADC_CONTROL_H_

/**
 * @description: ADC检测处理函数
 * @param {int} Voltage ADC采样值
 * @return {*}
 */
__attribute__((weak)) int AdcDetectHandle(int Voltage);

/**
 * @description: ADC检测初始化
 * @return {*}
 */
int AkDrvAdcInit(void);
#endif