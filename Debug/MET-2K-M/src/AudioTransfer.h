/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-18 11:29:04
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-01-23 17:42:26
 * @FilePath: /project_3/src/AudioTransfer.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _AUDIOTRANSFER_H_
#define _AUDIOTRANSFER_H_

/**
 * @description: 音频传输网络协议
 * @param {int} DevId   本地设备ID
 * @param {int} SlaveId 从机设备ID
 * @return {*}
 */
int AudioNetSocketProtocolSet(int DevId);

/**
 * @description: 音频网络传输初始化
 * @return {*}
 */
int NetAudioTransferInit(void);
#endif