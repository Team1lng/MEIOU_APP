#ifndef _OUTDOOR_UNLOCK_H_
#define _OUTDOOR_UNLOCK_H_

/***
** date:2022/04/18
** author:刘炼
** 上电初始化开锁相关的IO
** 将开锁的电平置为关闭状态
***/
bool device_unlock_init(void);


/***
** date:2022/04/18
** author:刘炼
** 开锁检测以及处理，包括开锁时间到自动关闭锁处理
***/
void device_unlock_process(void);



/***
**  date:2022/04/19
**  author:刘炼
**  接收到室内机发送指令开锁
**  unlock_dev:1：door unlock,2:gate unlock
***/
bool device_unlock_cmd_process(int unlock_dev);
#endif