#ifndef LEO_PWM_H
#define LEO_PWM_H

int ak_drv_pwm_open(int device_no);
int ak_drv_pwm_set(int device_no, long long duty_ns, long long period_ns);
int ak_drv_pwm_close(int device_no);

#endif
