/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-07-09 11:08:06
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-18 13:48:26
 * @FilePath: /Doorbell/common/HardwareControl/Peripheral.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef PeripheralControl_h
#define PeripheralControl_h

/**
 * @description: 韦根数据写入
 * @param {char} *Data 数据
 * @param {int} Size    数据长度
 * @return {*}
 */
int WiegandDevWrite(char *Data, int Size);

/**
 * @description: 获取74hc595d当前状态
 * @return {*}
 */
char _74hc595dDevStatus(void);

/**
 * @description:74hc595d设备写入
 * @param {char} Data   写入数据
 * @return {*}
 */
int _74hc595dDevWrite(char Data);

/**
 * @description: 外设控制器初始化
 * @return {*}
 */
int PeripheralControlInit(void);
#endif