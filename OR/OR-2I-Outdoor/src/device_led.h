#ifndef _DEVICE_LED_H_
#define _DEVICE_LED_H_

/***
** date:2022/04/19
** author:刘炼
** 上电初始化LED相关的IO
** 将LED的电平置为关闭状态
***/
// bool device_led_init(void);



/***
**  date:2022/04/19
**  author:刘炼
**  LED检测处理
**  LED标志位开启时，启动计时，倒计时间到后自动将LED关闭
***/
// void device_led_process(void);




/***
** date:2022/04/19
** author:刘炼
** 外部使能led状态。
***/
bool device_led_open_enable(void);

#endif