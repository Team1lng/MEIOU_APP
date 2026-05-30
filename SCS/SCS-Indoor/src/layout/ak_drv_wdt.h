/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-10-31 15:56:20
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-10-31 15:56:28
 * @FilePath: /two-wire-indoor/src/layout/ak_drv_wdt.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _AK_DRV_WDT_H_
#define _AK_DRV_WDT_H_

/**
 * ak_drv_wdt_open - open watch dog and watch dog start work.
 * @feed_timeout: [in] second, [1, 357]
 * return: 0 - success; otherwise error code;
 * note:
 */
int ak_drv_wdt_open(unsigned int feed_timeout);

/**
 * ak_drv_wdt_feed - feed watch dog.
 * return:0 - success; otherwise error code;
 */
int ak_drv_wdt_feed(void);

/**
 * ak_drv_wdt_close - close watch dog.
 * return:0 - success; otherwise error code;
 */
int ak_drv_wdt_close(void);

#endif
