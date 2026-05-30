/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-16 08:19:15
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-06-22 14:48:13
 * @FilePath: /project_3/src/VideoTransfer.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _VIDEOTRANSFER_H_
#define _VIDEOTRANSFER_H_

static inline int RectMap(int x, int min_in, int max_in, int min_out, int max_out)
{
    if (x >= max_in)
        return max_out;
    if (x <= min_in)
        return min_out;

    int delta_in = max_in - min_in;
    int delta_out = max_out - min_out;

    return ((x - min_in) * delta_out) / delta_in + min_out;
}

/**
 * @description: 视频数据包发送
 * @param {char} *Data  视频数据
 * @param {int} Size    视频数据大小
 * @return {*}
 */
void NetVideoPackageSend(unsigned char *Data, int Size);

/**
 * @description: 视频传输网络协议
 * @param {int} SlaveId 从机设备ID
 * @return {*}
 */
int VideoNetSocketProtocolSet(int SlaveId);

/**
 * @description: 视频网络传输初始化
 * @return {*}
 */
int NetVideoTransferInit(void);
#endif