/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-10-10 10:03:37
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-12 08:47:32
 * @FilePath: /project_2/main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "CircularList.h"
#include "GeneralInterface.h"
#include "ak_common.h"
#include "GpioControl.h"
#include "UserConfig.h"
#include "UserCard.h"
#include <fcntl.h>
#include "ak_log.h"

#define NETMSG_ENABLE
#define VOICE_AO_ENABLE
#define VIDEO_TRANSFER_ENABLE
#define AUDIO_TRANSFER_ENABLE
#define ADC_DETECT_ENABLE
#define EPOLL_GPIO_ENABLE
#define TIMER_ENABLE
#define PERIPHERAL_CONTROL
#define USER_MANAGEMENT
#define MOTION_DELECT

#ifdef ATS_ENABLE
#undef VOICE_AO_ENABLE
// #include "ak_its.h"
// #include "ak_ats.h"
#endif

#ifdef USER_MANAGEMENT
#include "UserNetManage.h"
#endif

#ifdef PERIPHERAL_CONTROL
#include "PeripheralControl.h"
#endif

#ifdef TIMER_ENABLE
#include "Timer.h"
#endif

#ifdef EPOLL_GPIO_ENABLE
#include "EpollGpioEvent.h"
#endif

#ifdef ADC_DETECT_ENABLE
#include "AdcControl.h"
#endif

#ifdef FINGERPRINT_ENABLE
#include "Fingerprint.h"
#endif

#ifdef KEYPAD_ENABLE
#include "DrvNumericKeypad.h"
#endif

#ifdef CARD_ENABLE
#include "DrvSwipeCard.h"
#endif

#ifdef VOICE_AO_ENABLE
#include "VoiceDecode.h"
#include "VoiceRingPlay.h"
#include "AudioPlay.h"
#endif

#ifdef NETMSG_ENABLE
#include "NetMsgComm.h"
#endif

#ifdef VIDEO_TRANSFER_ENABLE
#include "VideoInput.h"
#include "VideoTransfer.h"
#else
#undef MOTION_DELECT
#endif

#ifdef AUDIO_TRANSFER_ENABLE
#include "AudioTransfer.h"
#endif

#ifdef ITS_DEBUG
#include "ak_its.h"
#endif

#ifdef MOTION_DELECT
#include "MotionDetect.h"
#endif


/******************************** 内核报错缓存区满报错测试并重启 *****************************************/
// kernel_errno_text
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h> 
#define LOG_FILE "/proc/kmsg"
#define BUFFER_SIZE 2048
static volatile int g_stop_monitor = 0;
/*内核错误标识*/
static const char *CrashKeywords[] = {
    "Unable to handle kernel",
    "Internal error: Oops",
    "Kernel panic",
    "Out of memory:",
    "Null pointer dereference",
    NULL
};
/*停止日志进程，避免写入磁盘*/
static void StopLogDaemons(void) 
{
    const char *daemons[] = {"klogd", "syslogd", "rsyslogd", "ulogd", NULL};
    
    for (int i = 0; daemons[i] != NULL; i++) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "killall -9 %s 2>/dev/null", daemons[i]);
        system(cmd);
        printf("[INFO] Stopped %s (if exists)\n", daemons[i]);
    }
}
/*监控内核日志*/
static void* MonitorKernelLogs(void* arg) {
    FILE *log_file = NULL;
    char buffer[BUFFER_SIZE];

    StopLogDaemons();

    log_file = fopen(LOG_FILE, "r");
    if (log_file == NULL) {
        fprintf(stderr, "[ERROR] Open %s failed: %s (errno: %d)\n", 
                LOG_FILE, strerror(errno), errno);
        // 重试一次（应对临时文件系统问题）
        sleep(2);
        log_file = fopen(LOG_FILE, "r");
        if (log_file == NULL) {
            fprintf(stderr, "[ERROR] Reopen %s failed, exit monitor\n", LOG_FILE);
            return NULL;
        }
    }
    printf("[INFO] Monitor started: exclusive read %s\n", LOG_FILE);
    printf("[INFO] Watching crash keywords: ");
    for (int i = 0; CrashKeywords[i]; i++) {
        printf("%s, ", CrashKeywords[i]);
    }
    printf("\n");

    while (!g_stop_monitor) {
        memset(buffer, 0, BUFFER_SIZE);
        if (fgets(buffer, BUFFER_SIZE - 1, log_file) == NULL) {
            fprintf(stderr, "[WARN] Read %s failed: %s, retry...\n", 
                    LOG_FILE, strerror(errno));
            fclose(log_file);
            sleep(2);
            log_file = fopen(LOG_FILE, "r");
            if (log_file == NULL) {
                fprintf(stderr, "[ERROR] Reopen %s failed, exit monitor\n", LOG_FILE);
                break;
            }
            continue;
        }

        printf("[DEBUG] Received kernel log: %s\n", buffer);

        for (int i = 0; CrashKeywords[i] != NULL; i++) {
            if (strstr(buffer, CrashKeywords[i]) != NULL) {
                printf("[ALERT] Crash detected! Keyword: %s\n", CrashKeywords[i]);
                printf("[ALERT] Log content: %s\n", buffer);
                system("echo '[ALERT] Crash detected, sync and reboot' > /dev/kmsg");
                sync();  // 同步文件系统
                sleep(1);
                system("reboot -f");  // 强制重启
                fclose(log_file);
                return NULL;
            }
        }
    }

    fclose(log_file);
    printf("[INFO] Monitor stopped\n");
    return NULL;
}
/************************************************************************ */

static void AKPlatformSdkInit(void)
{
    sdk_run_config Config;
    memset(&Config, 0, sizeof(Config));
    Config.mem_trace_flag = SDK_RUN_NORMAL;
    Config.isp_tool_server_flag = 0;
    Config.audio_tool_server_flag = 0;
#ifdef ITS_ENABLE
    Config.isp_tool_server_flag = 1;
#endif
#ifdef ATS_ENABLE
    Config.audio_tool_server_flag = 1;
#endif
    ak_sdk_init(&Config);
}

#ifdef EPOLL_GPIO_ENABLE
static int HouseSwitchHandle(int Level)
{
    printf("[%s][%d]\n", __func__, Level);
    system("reboot");
    return 0;
}
int HouseSwitchEpollEventInit(struct EpollEvent *Event)
{
#define HOUSE_SWITCH_GPIO 26
    GPIO_LEVEL Level = GPIO_LEVEL_UNKNOWN;
    if (GpioOpen(HOUSE_SWITCH_GPIO, GPIO_DIR_IN, true) == false)
    {
        return -1;
    }
    GpioLevelGet(HOUSE_SWITCH_GPIO, &Level);
    GpioEdge(HOUSE_SWITCH_GPIO, Level == GPIO_LEVEL_LOW ? RISING_EDGE : FALLING_EDGE);
#ifdef NETMSG_ENABLE
    NetworkDevice DevId = Level == GPIO_LEVEL_LOW ? DEVICE_OUTDOOR_1 : DEVICE_OUTDOOR_2;
    NetLocalDeviceIDSet(DevId);
#endif
#ifdef VIDEO_TRANSFER_ENABLE
    VideoNetSocketProtocolSet(DevId);
#endif
#ifdef AUDIO_TRANSFER_ENABLE
    AudioNetSocketProtocolSet(DevId);
#endif
    if (Event == NULL)
    {
        return 0;
    }

    // char Path[64] = {0};
    // memset(Path, 0, sizeof(Path));
    // sprintf(Path, "/sys/class/gpio/gpio%d/value", HOUSE_SWITCH_GPIO);
    Event->Fd = -1;//open(Path, O_RDONLY);
    Event->TriggerLevel = 0;//!Level;
    Event->EpollEventHandle = NULL;//HouseSwitchHandle;
    return -1;
}
#endif

static void HardwareInitConfig(void)
{
#ifdef EPOLL_GPIO_ENABLE
    EpollGpioEventInit();
#endif

#ifdef PERIPHERAL_CONTROL
    PeripheralControlInit();
#endif

#ifdef CARD_ENABLE
    DrvSwipeCardInit();
#endif

#ifdef KEYPAD_ENABLE
    DrvNumericKeypadInit();
#endif

#ifdef ADC_DETECT_ENABLE
    AkDrvAdcInit();
#endif

#ifdef FINGERPRINT_ENABLE
    FingerprintInit();
#endif

    extern int LightGpioInit(void);
    LightGpioInit();

    extern void LockGpioInit(void);
    LockGpioInit();
}

//为了防止启动被load_isp_conf 损坏，增加心跳检测
#include <sys/time.h>
static unsigned long long CurrentSystemTimetamp = 0;
static unsigned long long GetTimestmap(void){
    struct timeval val;
    gettimeofday(&val,NULL);
    return val.tv_sec*1000 + val.tv_usec/1000;
}
static void ResetSystemTimestamp(void){
    CurrentSystemTimetamp = GetTimestmap();
}

static void* SystemtickThrad(void* arg){

    while(1){
        unsigned long long timestamp = GetTimestmap();
        if((timestamp - CurrentSystemTimetamp) > 10*1000){
            printf("\n#####################################\n");
            printf("--------system bad ....;reboot...\n");
            printf("#####################################\n");
            exit(0);
            while(1);
        }
        usleep(1000*100);
    }
    return  NULL;
}

static void SystemtickInit(void){

    pthread_t tid;
    ResetSystemTimestamp();
    pthread_create(&tid,NULL,SystemtickThrad,NULL);
}

int main(int argc, char *argv[])
{
    printf("\n\n###########################[%s]########################\n", IPC_MODEL);
    printf("# Compile Time:%s-%s\n", __DATE__, __TIME__);
    printf("####################################################################\n\n");

    AKPlatformSdkInit();
    //leo modiy
    SystemtickInit();

    HardwareInitConfig();

    UserConfigInit();

    UserDeckInit();

#ifdef VIDEO_TRANSFER_ENABLE
    NetVideoTransferInit();
#endif

#ifdef AUDIO_TRANSFER_ENABLE
    NetAudioTransferInit();
#endif


#ifdef NETMSG_ENABLE
    NetMsgCommInit();
#endif

#ifdef VOICE_AO_ENABLE
    VoiceRingPlayInit();
#else
    // ak_its_start(8012);
    // ak_its_start(8765);
#endif

#ifdef USER_MANAGEMENT
    UserNetManageInit();
#endif

    pthread_t monitor_thread;
    int ret;

    // 启动内核崩溃监控线程
    ret = pthread_create(&monitor_thread, NULL, MonitorKernelLogs, NULL);
    pthread_detach(monitor_thread);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] Create monitor thread failed: %s\n", strerror(ret));
        return -1;
} 


#ifdef MOTION_DELECT
    // MotionDetectInit_Set();    //hare 2025.6.4
#endif
    GPIO_LEVEL pre_level = GPIO_LEVEL_LOW;
    GpioLevelGet(HOUSE_SWITCH_GPIO, &pre_level);
    while (1)
    {
        usleep(1000 * 1000);

        GPIO_LEVEL level = GPIO_LEVEL_LOW;
        if(GpioLevelGet(HOUSE_SWITCH_GPIO, &level) && level != pre_level)
        {
            pre_level = level;
            HouseSwitchHandle(level);
        }

        //leo modiy
        ResetSystemTimestamp();
    }

    return 0;
}