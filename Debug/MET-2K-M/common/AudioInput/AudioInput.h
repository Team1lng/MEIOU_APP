/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-14 19:43:29
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-20 08:13:19
 * @FilePath: /project_3/common/AudioOutput/AudioInput.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _AUDIOINPUT_H_
#define _AUDIOINPUT_H_

typedef struct
{
    unsigned char *Data;   // frame data
    unsigned int Len;      // frame len in bytes
    unsigned long long Ts; // timestamp(ms)
    unsigned long SeqNo;   // current frame sequence no.
} AudioInputFrame;

/**
 * @description: 获取输入音频数据 [需要函数重定义]
 * @param {AudioOutputFrame} frame   节点信息
 * @return {*}
 */
__attribute__((weak)) int AIDataExport(AudioInputFrame *frame);

/**
 * @description: 设置音频输入参数
 * @param {int} HandleId 音频输入句柄
 * @return {*}
 */
__attribute__((weak)) int SetupAudioInputArgument(int HandleId);

/**
 * @description: 初始化音频输入
 * @return {*}
 */
int AudioInputInit(void);

/**
 * @description: 反初始化音频输入
 * @return {*}
 */
int AudioInputDeinit(void);

/**
 * @description: 音频输入线程状态
 * @return {*}
 */
int AudioInputThreadState(void);
#endif

int is_network_audio_send_package_open(void);
void network_audio_send_package_start(void);
void network_audio_send_package_stop(void);