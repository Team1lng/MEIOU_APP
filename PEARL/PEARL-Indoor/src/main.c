#include "lvgl.h"
#include <stdio.h>
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ak_thread.h"
#include "ak_mem.h"

#include "layout_define.h"
#include "leo_api.h"
#include "user_data.h"

#include <stdlib.h>
#include <sys/signal.h>

// 默认读写一个关闭的socket会触发sigpipe信号 该信号的默认操作是关闭进程 有时候这明显是我们不想要的
// 所以此时我们需要重新设置sigpipe的信号回调操作函数  比如忽略操作等  使得我们可以防止调用它的默认操作
// 信号的处理是异步操作 也就是说 在这一条语句以后继续往下执行中如果碰到信号依旧会调用信号的回调处理函数
// 处理sigpipe信号

void handle_pipe(int sig)
{
    printf("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", __func__);
    fflush(stdout);
}
void pipe_info_func(int signo, siginfo_t *info, void *p)
{
    printf("\n\n%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", __func__);
    printf("signo=%d\n", signo);

    printf("sender sigal pid=%d\n", info->si_pid);
    printf("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n\n", __func__);
}
void handle_for_sigpipe()
{
    struct sigaction sa; // 信号处理结构体
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN; // 设置信号的处理回调函数 这个SIG_IGN宏代表的操作就是忽略该信号
    sa.sa_flags = 0;
    // sa.sa_sigaction = pipe_info_func;
    if (sigaction(SIGPIPE, &sa, NULL)) // 将信号和信号的处理结构体绑定
        return;
}

/**
 *  @brief print memory useage state
 *  @param[in] void
 *  @returnval  void
 */
void memory_print(void)
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("used: %6d (%3d %%), frag: %3d %%, biggest free: %6d used_cnt:%d, max_used = %d free_size:%d free_cnt:%d ,total_size:%d\n", (int)mon.total_size - mon.free_size,
           mon.used_pct,
           mon.frag_pct,
           (int)mon.free_biggest_size, mon.used_cnt, mon.max_used, mon.free_size, mon.free_cnt, mon.total_size);
}

static void *lvgl_titck_task(void *arg)
{
    struct ak_timeval tv1, tv2;

    ak_get_ostime(&tv1);
    ak_get_ostime(&tv2);
    while (1)
    {
        ak_get_ostime(&tv1);
        lv_tick_inc(tv1.sec * 1000 + tv1.usec / 1000 - tv2.sec * 1000 - tv2.usec / 1000);
        ak_get_ostime(&tv2);
        ak_sleep_ms(1);
    }
    ak_thread_exit();
    return NULL;
}

static void standby_timeout_callback(void)
{
    // Debug("==========================================================================\n");
    ring_init();

    if (current_layout_get() != &layout_standby &&
        current_layout_get() != &layout_monitor &&
        // current_layout_get() != &layout_call &&
        current_layout_get() != &layout_video &&
        current_layout_get() != &layout_tuya_register &&
        // current_layout_get() != &layout_setting_senior &&
        current_layout_get() != &layout_cctv &&
        current_layout_get() != &layout_dev_busy)
    {
        Debug("\n\n\n\n");
        goto_layout(pLAYOUT(standby));
    }
}

#define ETH2_STATIC_IP "192.168.188.1"
static void *network_pairing_init_task(void *arg)
{
    Debug("pairing_mode ,%d\n", user_data_get()->pairing_mode);
    /*     if(user_data_get()->pairing_mode == WLAN_NET)
        {
            system("ifconfig eth2 down");

            system("ifconfig wlan0 up");
        }
        else */
    if (user_data_get()->pairing_mode == WIRED_NET)
    {
        system("ifconfig eth2 up");
        Debug("ifconfig eth2 up");
        ak_sleep_ms(100);

        system("ifconfig wlan0 down");
        Debug("ifconfig wlan0 down");
        ak_sleep_ms(100);

        system("killall udhcpc");
        Debug("killall udhcpc");
        ak_sleep_ms(1000);

        if (user_data_get()->allocation_mode == UDHCPC_ALLOC)
        {
            system("udhcpc -i eth2 &");
            Debug("udhcpc -i eth2 &");
            ak_sleep_ms(100);
        }
        else if (user_data_get()->allocation_mode == STATIC_ALLOC)
        {
            system("ifconfig eth2 192.168.188.1");
            Debug("ifconfig eth2 192.168.188.1");
            ak_sleep_ms(100);
        }

        system("route add -net 224.0.0.0 netmask 224.0.0.0 eth2");
        Debug("route add -net 224.0.0.0 netmask 224.0.0.0 eth2\n");
    }

    tuya_network_dev_set(&user_data_get()->pairing_mode);
    *((ak_pthread_t *)arg) = -1;
    ak_thread_exit();
    return NULL;
}

static void network_pairing_int(void)
{
    if (wifi_usb_module_enable())
    {
        static ak_pthread_t pthread_id = -1;
        if (pthread_id == -1)
            ak_thread_create(&pthread_id, network_pairing_init_task, &pthread_id, ANYKA_THREAD_MIN_STACK_SIZE, -1);
    }
}

/*
 *   这段函数是为了实现卡片指纹管理功能，旧的ak_eth.ko不能同时用ICMP协议通讯两个PHY
 *   需要将添加了混杂模式的ko重新加载一次
 * */
static void ak_eth_reload(void)
{
#ifdef BCOM_OID_VERSION

#define AK_ETH_PATH_1 "/tmp/ak_eth.ko"
#define AK_ETH_PATH_2 "/etc/config/ak_eth.ko"

    if (access(AK_ETH_PATH_1, F_OK) == 0)
    {
        Debug("fine %s ,reload now!\n", AK_ETH_PATH_1);
        system("rmmod ak_eth.ko");
        ak_sleep_ms(200);
        system("insmod " AK_ETH_PATH_1 "  yt8510_rate_model=0x01");
    }
    else if (access(AK_ETH_PATH_2, F_OK) == 0)
    {
        Debug("fine %s ,reload now!\n", AK_ETH_PATH_2);
        system("rmmod ak_eth.ko");
        ak_sleep_ms(200);
        system("insmod " AK_ETH_PATH_2);
    }
#endif
}

void main_device_monitor_busy_func(unsigned long arg1, unsigned long arg2)
{
#if (defined(MEIOU_VERSION))
    if (current_layout_get() != &layout_dev_busy && monitor_enter_way_get() != MONITOR_ENTER_TUYA)
    {
        Debug("\n\n\n\n");
        goto_layout(pLAYOUT(dev_busy));
    }
#else
    if (current_layout_get() != &layout_standby && monitor_enter_way_get() != MONITOR_ENTER_TUYA)
    {
        Debug("\n\n\n\n");
        goto_layout(pLAYOUT(standby));
    }
#endif
}

int lcd_reset_pin_higt(void)
{
    system("echo 34 > /sys/class/gpio/export");
    system("echo 1 > /sys/class/gpio/gpio34/value");
    return 1;
}

void mute_poll_task(lv_task_t *task_t)
{
    if (user_data_get()->mute.schedule)
    {
        time_t seconds = time(NULL);
        struct tm tm = {0};
        localtime_r(&seconds, &tm);
        int curr_time = tm.tm_hour * 100 + tm.tm_min;
        if (((user_data_get()->mute.timer_start > user_data_get()->mute.timer_end) && (curr_time > user_data_get()->mute.timer_start || curr_time < user_data_get()->mute.timer_end)) ||
            ((user_data_get()->mute.timer_start < user_data_get()->mute.timer_end) && (curr_time > user_data_get()->mute.timer_start && curr_time < user_data_get()->mute.timer_end)))
        {
            user_data_get()->other.model = MUTE_PATTERN;
            printf("Switch Mode: MUTE_PATTERN!!!!\n");
        }
        else
        {
            user_data_get()->other.model = AT_HOME_PATTERN;
            printf("Switch Mode: AT_HOME_PATTERN!!!!\n");
        }
    }
}

/* ==================== 每 2 分钟执行一次：系统时间 → 硬件 RTC ==================== */
static void hwclock_sync_task(lv_task_t *task_t)
{
    // printf(" =================== SYS -> RTC ==============\n");
    system("hwclock -w");
}

void hwclock_sync_timer_init(void)
{
    lv_task_create(hwclock_sync_task, 120 * 1000, LV_TASK_PRIO_LOW, NULL);
}

static lv_task_t *door_chime_det_task_t = NULL;
int main(int argv, char **argc)
{
    // char * buffer = malloc(1000*1000);
    // if(buffer)
    //     printf("Example Apply for one M memory\n");

    // // free(buffer);
    // return 0;

    Debug("INDOOR VERSION:%s-%s\n", __DATE__, __TIME__);
    system("echo 0 > /proc/sys/vm/oom_dump_tasks");
    system("hwclock -w");
    handle_for_sigpipe();

    ak_eth_reload();

    void upgrade_check_firmware(void);
    upgrade_check_firmware();

    wifi_usb_module_init();

    network_pairing_int();
    /* 必须放在wifi、usb初始化后面，此线程中也wifi操作 */
    user_data_init();

    lv_init();            // lvgl 系统初始化
    lv_port_disp_init();  // lvgl 显示接口初始化,放在 lv_init()的后面
    lv_port_indev_init(); // lvgl 输入接口初始化,放在 lv_init()的后面

    leo_api_init();

    // extern void PWM1_AVDD(void);
    // PWM1_AVDD();
    network_devices_enable_init();

    device_monitor_busy_register(main_device_monitor_busy_func);

    device_gate2_unlock_register(default_gate2_unlock_callback);

    device_id_repeat_register(device_id_repeat_func);

    door_chime_event_register(door_chime_func);
    // goto_layout(pLAYOUT(home));

    standby_timer_open(60000, standby_timeout_callback);

    ak_pthread_t pthread_id;
    ak_thread_create(&pthread_id, lvgl_titck_task, NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);

    extern void hardware_detect_task(lv_task_t * task_t);
    door_chime_det_task_t = lv_task_create(hardware_detect_task, 50, LV_TASK_PRIO_MID, NULL);

    /* 时间漂移补偿任务，每 60 秒检查一次，每 6 小时自动校准 */
    extern void time_calibrate_task(lv_task_t *task);
    lv_task_t *time_cal_task = lv_task_create(time_calibrate_task, 60 * 1000, LV_TASK_PRIO_MID, NULL);
    time_cal_task->clean_lock = false;

    // 2 分钟一次的系统时间与 RTC 同步任务，防止系统时间漂移过大
    hwclock_sync_timer_init();

    // extern void printf_str(void);
    // printf_str();
    // int count = 0;
    struct ak_timeval tv1;
    bool speak_status = false;
    speak_enable_set(0);
    while (1)
    {
        lv_task_handler();
        ak_get_ostime(&tv1);
        struct ak_timeval tv2 = audio_output_time_get();
        if (speak_status)
        {
            if (abs(tv1.sec - tv2.sec) > 3)
            {
                speak_enable_set(0);
                speak_status = false;
            }
        }
        else
        {
            if (abs(tv1.sec - tv2.sec) < 3)
            {
                speak_enable_set(1);
                speak_status = true;
            }
        }

        ak_sleep_ms(1);
        // count++;
        // if(count > 100)
        // {
        // 	count = 0;
        // system("sync");
        // system("echo 3 > /proc/sys/vm/drop_caches");
        // system("free");
        // memory_print();
        //  }
    }
}
