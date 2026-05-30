/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-25 14:11:16
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-12-25 14:58:41
 * @FilePath: /project_3/common/GpioControl/PwmControl.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _PWM_CONTROL_H_
#define _PWM_CONTROL_H_

/*
 * AkDrvPwmOpen - open pwm device
 * device_no[IN]: pwm device minor-number,[0-4]
 * return: 0 - success; otherwise error code;
 */
int AkDrvPwmOpen(int device_no);

/*
 * AkDrvPwmSet - set pwd working param
 * device_no[IN]: pwm device minor-number,[0-4]
 * duty_ns[IN]: pwm duty time in ns.
 * period_ns[IN]: pwm period time in ns.
 * return: 0 on success, otherwise error code.
 */
int AkDrvPwmSet(int device_no, long long duty_ns, long long period_ns);

/*
 * ak_drv_pwm_close - close pwm
 * device_no[IN]: pwm device minor-number,[0-4]
 * return: 0 on success, otherwise error code.
 */
int AkDrvPwmClose(int device_no);

#endif
