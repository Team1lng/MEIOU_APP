/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-07-10 11:30:39
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-11 09:49:32
 * @FilePath: /Doorbell/src/UserFingerprint.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef UserFingerprint_h
#define UserFingerprint_h
#include "Fingerprint.h"

/**
 * @description: 保存指纹信息
 * @return {*}
 */
int FingerInfoSave(void);

/**
 * @description: 指纹数据初始化
 * @param {int} Index 指纹存储索引
 * @return {*}
 */
int FingerDataInit(int Index);

/**
 * @description: 指纹数据复位
 * @param {int} Index 指纹存储索引
 * @return {*}
 */
int FingerDataReset(int Index);

/**
 * @description:  指纹信息格式化
 * @return {*}
 */
int FingerInfoFormat(void);

/**
 * @description: 指纹信息初始化
 * @return {*}
 */
int FingerInfoInit(void);

/**
 * @description: 设置权限，指纹权限为0时删除指纹
 * @param {int} index   指纹存储索引
 * @param {int} Perm   指纹权限
 * @return {*}0-失败，1-成功
 */
int FingerSetPerm(int index, char Perm);

/**
 * @description: 获取指纹权限
 * @param {int} index 指纹存储索引
 * @return {*} 权限
 */
int FingerPermGet(int index);

/**
 * @description: 获取指纹信息
 * @return {*}
 */
FingerInfo *GetFingerInfo(void);

/**
 * @description: 获取指纹权限及数据
 * @param {uint8_t} *Deck  指纹缓存
 * @return {*} 指纹数据大小
 */
int UserFingerPermGet(uint8_t **Deck);
#endif