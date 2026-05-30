#ifndef _OUTDOOR_DEVICE_H_
#define _OUTDOOR_DEVICE_H_


/***
** date: 2022/04/18
** author：刘炼
** 读取户外机设备
** 默认处理：0:outdoor1 1:outdoor2
** 返回-1:读取错误
***/
int outdoor_device_read(void);


/***
** date: 2022/04/18
** author：刘炼
** 读取户外机设备
** 返回true：设备发生改变
***/
bool outdoor_device_change(void);

#endif