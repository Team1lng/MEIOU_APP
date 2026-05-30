/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-30 09:01:30
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-01-30 11:30:22
 * @FilePath: /82225-EPC/src/DeviceUpgrade.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _DEVICE_UPGRADE_H_
#define _DEVICE_UPGRADE_H_

/**
 * @description: 接收软件更新包
 * @param {int} index    数据包索引
 * @param {int} len    数据包长度
 * @param {char} *buf   更新包数据
 * @return {*}
 */
int ReceiveUpgradePack(int index, int len, unsigned char *buf);

/**
 * @description:开始更新
 * @param {int} Flag    升级结果
 * @return {*}
 */
int UpgradeProcess(int Flag);
#endif