/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-14 19:43:29
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-20 08:13:33
 * @FilePath: /project_3/common/AudioOutput/AudioOutput.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _AUDIOOUTPUT_H_
#define _AUDIOOUTPUT_H_

typedef struct
{
    unsigned char *Data;   // frame data
    unsigned int Len;      // frame len in bytes
    unsigned long long Ts; // timestamp(ms)
    unsigned long SeqNo;   // current frame sequence no.
} AudioOutputFrame;

/**
 * @description: 获取输出音频数据 [需要函数重定义]
 * @param {AudioOutputFrame} frame   节点信息
 * @return {*}
 */
__attribute__((weak)) int AODataExport(AudioOutputFrame **frame);

/**
 * @description: 释放音频数据缓存 [需要函数重定义]
 * @param {AudioOutputFrame} frame   节点信息
 * @return {*}
 */
__attribute__((weak)) int AODataFree(AudioOutputFrame *frame);

/**
 * @description: 设置音频输出参数
 * @param {int} HandleId 音频输出句柄
 * @return {*}
 */
__attribute__((weak)) int SetupAudioOutputArgument(int HandleId);

/**
 * @description: 获取AO剩余数据
 * @return {*}  AO剩余数据
 */
int AudioOutputRemainLen(void);

/**
 * @description: 设置AO音量大小
 * @param {int} volume  音量大小
 * @return {*}
 */
void AudioOutputVolumeSet(int volume);

/**
 * @description: 初始化音频输出
 * @return {*}
 */
int AudioOutputInit(void);
#endif