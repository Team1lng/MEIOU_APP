#include"stdio.h"
#include <unistd.h>
#include"gpio_base.h"
#include"ak_thread.h"
#include"ak_common.h"
#include"scan_numeric_key.h"

#define KEY_SCAN_KO "/usr/modules/key_scan.ko"
#define DEV_NAME    "/dev/GpioScanKey"
static int dev_fb = -1;

#define KEY_SHAKE_TIME    (20)
#define KEY_MASK(x)    ((1 << (x)) - 1)

static KEY_EVENT_CALLBACK key_event_callback = NULL;
static KEY_EVENT_CALLBACK key_timer_callback = NULL;
static char (*scan_key_read)(void) = NULL;

// 全局标志位：true=键盘正在使用（不计入重启时间），false=闲置（计入重启时间）
bool password_keyboard_using = false;
// 记录最后一次按键的时间（用于判断闲置超时）
unsigned long password_keyboard_last_active = 0;
#define PASSWORD_KEYBOARD_IDLE_TIMEOUT 30 // 单位：秒

typedef enum
{
    DETECTION_INDEX_1,
    DETECTION_INDEX_2,
    DETECTION_INDEX_3,
    DETECTION_TOATL,
}DETECT_KEY_PIN;


static const char key_group[DETECTION_TOATL] = {DETECT_PIN_1,DETECT_PIN_2,DETECT_PIN_3};

static char key_vol_tag[KEY_VOL_TOTAL] = {
    [KEY_VOL_0] = 0,
    [KEY_VOL_1] = 1,
    [KEY_VOL_2] = 2,
    [KEY_VOL_3] = 3,
    [KEY_VOL_4] = 4,
    [KEY_VOL_5] = 5,
    [KEY_VOL_6] = 6,
    [KEY_VOL_7] = 7,
    [KEY_VOL_8] = 8,
    [KEY_VOL_9] = 9,
    [KEY_VOL_DEL] = KEY_PRESS_DEL,
    [KEY_VOL_AFFIRM] = KEY_PRESS_AFFIRM,
    [KEY_VOL_FAMILY_L] = 12,
    [KEY_VOL_FAMILY_R] = 13,
    [KEY_VOL_NONE] = KEY_NONE_VOL,
};

void key_press_register(KEY_EVENT_CALLBACK func)
{
    key_event_callback = func;
}

void key_timer_register(KEY_EVENT_CALLBACK func)
{
    key_timer_callback = func;
}

static char scan_key_read_1(void)
{
    char vol;
    if(read(dev_fb,&vol,1) && vol < KEY_VOL_TOTAL)
    {  
        // printf("scan_key_read_1:%d\n",key_vol_tag[(int)vol]); 
        return key_vol_tag[(int)vol];
    }
    return KEY_NONE_VOL;
}

static char scan_key_read_2(void)
{
    int vol = 0;
    int vol_temp = 0;
    
    for(int i = DETECTION_INDEX_1;i < DETECTION_TOATL;i++)
    {
         gpio_direction_set(key_group[i],GPIO_DIR_IN);
    }

    for(int i = DETECTION_TOATL - 1;i >= DETECTION_INDEX_1;i--)
    {
        GPIO_LEVEL level;
        gpio_direction_set(key_group[i],GPIO_DIR_IN);
        if(gpio_level_read(key_group[i],&level))
        {
            vol_temp = vol_temp << 1;
            vol_temp = (vol_temp|(level == GPIO_LEVEL_LOW ? 0 : 1));
        }
    }
    vol += vol_temp;
    
    #if 1
    for(int traversal = 0;traversal < DETECTION_TOATL;traversal ++)
    {
        int detect_key_num = traversal == 0 ? DETECTION_TOATL : DETECTION_TOATL - 1;

        if((vol_temp&KEY_MASK(detect_key_num)) == (KEY_MASK(detect_key_num)))
        {
            vol_temp = 0;
            for(int i = DETECTION_INDEX_1;i < DETECTION_TOATL;i++)
            {
                if(traversal == i){
                    gpio_direction_set(key_group[i],GPIO_DIR_OUT);
                    gpio_level_set(key_group[i], GPIO_LEVEL_LOW);
                }
                else
                    gpio_direction_set(key_group[i],GPIO_DIR_IN);
            }

            for(int i = DETECTION_INDEX_1;i < DETECTION_TOATL;i++)
            {
                if(traversal == i)   continue;

                GPIO_LEVEL level;
                gpio_direction_set(key_group[i],GPIO_DIR_IN);
                if(gpio_level_read(key_group[i],&level))
                {
                    vol_temp = vol_temp << 1;
                    vol_temp = (vol_temp|(level == GPIO_LEVEL_LOW ? 0 : 1));
                }
            }
            vol += vol_temp;
        }
        else
        {
            break;
        }
    }
    #endif
    // printf("scan_key_read_2:%d\n",key_vol_tag[(int)vol]); 
    return key_vol_tag[vol];
}


static void *scan_key_task(void *arg)
{
    printf("****************%s start****************\n",__func__);
    int key;
    static int prev_key;
    static int shake_key;
    KEY_PRESS_EVENT event = NONE_KEY_EVENT;
    while (1)
    {
        key = scan_key_read();
        // ========== 新增：更新密码键盘使用状态 ==========
        // 1. 检测到有效按键（非无按键）→ 标记为使用中，记录最后活跃时间
        if(key != KEY_NONE_VOL)
        {
            password_keyboard_using = true;
            password_keyboard_last_active = ak_get_os_timestamp(); // 获取当前秒数
            //printf("密码键盘按下：%d,标记为使用中\n", key);
        }
        // 2. 无按键时，判断是否闲置超时
        else
        {
            if(password_keyboard_using)
            {
                unsigned long curr_sec = ak_get_os_timestamp();
                if(curr_sec - password_keyboard_last_active >= PASSWORD_KEYBOARD_IDLE_TIMEOUT)
                {
                    password_keyboard_using = false;
                    printf("密码键盘闲置超时，标记为闲置\n");
                }
            }
        }
        // =================================================  
        if(key != prev_key)
        {
            shake_key = key;
            ak_sleep_ms(KEY_SHAKE_TIME);
            key = scan_key_read();  
            if(key == shake_key)
            {
                prev_key = key;
                if(key == KEY_NONE_VOL)
                {
                    event = KEY_RELEASE;
                    if(key_event_callback != NULL)  key_event_callback(key,event);
                }
                else
                {
                    event = KEY_PRESS;
                    if(key_event_callback != NULL)  key_event_callback(key,event);
                }
            }
        }

        if(key_timer_callback)  key_timer_callback(0,0);

        ak_sleep_ms(KEY_SHAKE_TIME);
    }
	ak_thread_exit();
    return NULL;
}

#include <fcntl.h>
#include <unistd.h>
void Numeric_key_init(void)
{
    if(access(KEY_SCAN_KO,F_OK) == 0)
    {
        printf("Key Drive scan mode =============>>>\n\n\n");
        system("insmod " KEY_SCAN_KO" scan_pin=6,7,8");
        scan_key_read = scan_key_read_1;
        
        dev_fb = open(DEV_NAME, O_RDWR);
        if(dev_fb < 0) {
            printf("file %s open failed!\r\n", DEV_NAME);
            return;
        }
    }
    else
    {
        printf("Key Apply scan mode =============>>>\n\n\n");
        for(int i = DETECTION_INDEX_1;i < DETECTION_TOATL;i++)
        {
            gpio_export(key_group[i]);
        }
        scan_key_read = scan_key_read_2;
    }

	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, scan_key_task, NULL, 100*1000, -1);
    ak_thread_detach(thread_id);
}