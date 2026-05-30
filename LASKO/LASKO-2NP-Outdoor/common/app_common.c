#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ak_common.h>
#include "app_common.h"
#include "audio_output.h"
// #include "audio_package.h"
#include "gpio_base.h"
#include "network_common.h"
#include "ak_drv_wdt.h"
#include "ak_log.h"
// #include "audio_capture_encode.h"
#include <unistd.h>
#include <fcntl.h>
#include "ak_drv_gpio.h"
#include "ak_thread.h"

#define PCM_READ_LEN 1024 * 20

static bool play_ring_task_run = false;
static ak_mutex_t audio_play_mutex;
#if 1
#define DOORBELL_BI1 "/etc/config/cbin/bi1.pcm"
#define DOORBELL_BI2 "/etc/config/cbin/bi2.pcm"
#define DOORBELL_BI3 "/etc/config/cbin/bi3.pcm"
#define DOORBELL_BI4 "/etc/config/cbin/bi4.pcm"
// #define DOORBELL_RING_1 "/etc/config/cbin/melody1_16k.pcm"
// #define DOORBELL_RING_2 "/etc/config/cbin/melody2_16k.pcm"
// #define DOORBELL_RING_3 "/etc/config/cbin/melody3_16k.pcm"
// #define DOORBELL_RING_4 "/etc/config/cbin/melody4_16k.pcm"
// #define DOORBELL_RING_5 "/etc/config/cbin/melody5_16k.pcm"
// #define DOORBELL_RING_6 "/etc/config/cbin/melody6_16k.pcm"
#define DOORBELL_ALARM "/etc/config/cbin/alarm_16k.pcm"
// #define DOORBELL_UNLOCK "/etc/config/cbin/unlock_16k.pcm"
// #define DOORBELL_CALL_LIGHT_OFF "/etc/config/cbin/call_light_off_16k.pcm"
// #define DOORBELL_MESSAGE_CH "/etc/config/cbin/liuyan_ch.pcm"
#define DOORBELL_MESSAGE_EN "/etc/config/cbin/liuyan_en.pcm"
// #define DOORBELL_MESSAGE_GER "/etc/config/cbin/liuyan_ger.pcm"
// #define DOORBELL_MESSAGE_HEB "/etc/config/cbin/liuyan_heb.pcm"
// #define DOORBELL_MESSAGE_POL "/etc/config/cbin/liuyan_pol.pcm"
// #define DOORBELL_UNLOCK_CH "/etc/config/cbin/lock_en.pcm"
// #define DOORBELL_UNLOCK_EN "/etc/config/cbin/lock_en.pcm"
// #define DOORBELL_UNLOCK_GER "/etc/config/cbin/lock_en.pcm"
// #define DOORBELL_UNLOCK_HEB "/etc/config/cbin/lock_heb.pcm"
// #define DOORBELL_UNLOCK_POL "/etc/config/cbin/lock_pol.pcm"
#define DOORBELL_CALL_BUSY "/etc/config/cbin/knock.pcm"
#else
#define DOORBELL_RING_1 "/usr/share/melody1_16k.pcm"
#define DOORBELL_RING_2 "/usr/share/melody2_16k.pcm"
#define DOORBELL_RING_3 "/usr/share/melody3_16k.pcm"
#define DOORBELL_RING_4 "/usr/share/melody4_16k.pcm"
#define DOORBELL_RING_5 "/usr/share/melody5_16k.pcm"
#define DOORBELL_RING_6 "/usr/share/melody6_16k.pcm"
#define DOORBELL_ALARM "/usr/share/alarm_16k.pcm"
#define DOORBELL_UNLOCK "/usr/share/unlock_16k.pcm"
#endif

const char *ring_group[] = {
	DOORBELL_BI1,
	DOORBELL_BI2,
	DOORBELL_BI3,
	DOORBELL_BI4,
	DOORBELL_ALARM,
	// DOORBELL_UNLOCK,
	// DOORBELL_CALL_LIGHT_OFF,
	DOORBELL_MESSAGE_EN,
	// DOORBELL_MESSAGE_CH,
	// DOORBELL_MESSAGE_GER,
	// DOORBELL_MESSAGE_HEB,
	// DOORBELL_MESSAGE_POL,
	// DOORBELL_UNLOCK_EN,
	// DOORBELL_UNLOCK_CH,
	// DOORBELL_UNLOCK_GER,
	// DOORBELL_UNLOCK_HEB,
	// DOORBELL_UNLOCK_POL,
	DOORBELL_CALL_BUSY};

static int play_ring_flag = 0;
static bool ring_play_run = false;
static bool ring_play_doorbell_flag = false;
static FILE *ring_fp = NULL;
unsigned long long os_get_us(void)
{
	struct ak_timeval tv;
	ak_get_ostime(&tv);
	return tv.usec + tv.sec * 1000 * 1000;
}

unsigned long long os_get_ms(void)
{
	struct ak_timeval tv;
	ak_get_ostime(&tv);
	return (tv.usec / 1000 + tv.sec * 1000);
}

unsigned long os_get_second(void)
{
	struct ak_timeval tv;
	ak_get_ostime(&tv);
	return tv.sec;
}

static bool audio_file_path = false;
static FILE *open_doorbell_ring_file(unsigned int index)
{
	ak_print_normal(MODULE_ID_APP, "RING: %s\n\r", ring_group[index]);
	printf("RING: %s\n\r", ring_group[index]);
	FILE *fp = fopen(ring_group[index], "r");
	if (fp == NULL)
	{
		ak_print_error_ex(MODULE_ID_APP, "open doorbell ring file error!\n\r");
		return NULL;
	}
	return fp;
}

#if 1
static void *read_play_ring_data(void *arg)
{
	unsigned char *data = (unsigned char *)malloc(PCM_READ_LEN);
	// int total_length = 0;
	audio_output_open(AUDIO_CHANNEL_MONO, AK_AUDIO_SAMPLE_RATE_16000);
	while (1)
	{
		ak_thread_mutex_lock(&audio_play_mutex);
		if (ring_play_run)
		{
			if (ring_fp)
			{
				// audio_speak_enable();
				play_ring_flag = 1;
				memset(data, 0x00, PCM_READ_LEN);
				// static int count = 0;
				// printf("==================================>>>> os_get_ms1111:[%llu]\n", os_get_ms() - count);
				// count = os_get_ms();
				int read_length = fread(data, sizeof(char), PCM_READ_LEN, ring_fp);
				if (read_length > 0)
				{
					audio_output_write(data, read_length);
				}
				else if (read_length == 0)
				{
					ak_print_normal(MODULE_ID_APP, "end of ring file! \n\r");
					ring_play_run = false;
				}
				else
				{
					ak_print_error(MODULE_ID_APP, "\nread, %s\n", strerror(errno));
					ring_play_run = false;
				}
			}
		}
		else if (ring_fp)
		{
			// audio_speak_disable();
			fclose(ring_fp);
			ring_fp = NULL;
		}
		ak_thread_mutex_unlock(&audio_play_mutex);

		ak_sleep_ms(1);
	}
	play_ring_flag = 0;
	ak_thread_exit();
	return NULL;
}

int get_mtd_num(void)
{
	static int count = 0;
	if (count != 0)
	{
		return count;
	}

	FILE *fp;
	char line[256];
	// 打开/proc/mounts文件
	fp = fopen("/proc/mtd", "r");
	if (fp == NULL)
	{
		printf("无法打开/proc/mtd 文件\n");
		return -1;
	}
	// 逐行读取文件内容，查找包含"mtd"关键字的行
	while (fgets(line, sizeof(line), fp))
	{
		if (strstr(line, "mtd") != NULL)
		{
			count++;
		}
	}
	// 关闭文件
	fclose(fp);
	printf("MTD个数:%d\n", count);
	return count;
}
void play_ring_init(void)
{
	if (play_ring_task_run == true)
	{
		return;
	}

	if (get_mtd_num() > 8)
	{
		printf("The /app directory exists.\n");
		audio_file_path = true;
	}
	else
	{
		printf("The /app directory does not exist.\n");
		audio_file_path = false;
	}
	play_ring_task_run = true;
	ak_thread_mutex_init(&audio_play_mutex, NULL);
	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, read_play_ring_data, NULL, 100 * 1024, -1);
	ak_thread_detach(thread_id);
}
void play_ring(unsigned int index)
{
	// watch_dog_close();
	// ao_howling_suppress_pause();
	if (index < RING_INDEX_MAX)
	{
		ak_thread_mutex_lock(&audio_play_mutex);
		if (ring_fp != NULL)
		{
			fclose(ring_fp);
			ring_fp = NULL;
		}
		ring_play_run = true;
		ring_fp = open_doorbell_ring_file(index);
		ak_thread_mutex_unlock(&audio_play_mutex);
	}
	// watchdog_open();
	// ao_howling_suppress_close();
}
#else
static void read_play_ring_data(FILE *fp)
{
	unsigned char data[PCM_READ_LEN] = {0};
	// int total_length = 0;
	play_ring_flag = 1;
	ring_play_run = true;
	audio_output_open(AUDIO_CHANNEL_MONO, AK_AUDIO_SAMPLE_RATE_16000);
	while (ring_play_run)
	{

		memset(data, 0x00, sizeof(data));
		int read_length = fread(data, sizeof(char), PCM_READ_LEN, fp);
		if (read_length > 0)
		{
			audio_output_write(data, read_length);
		}
		else if (read_length == 0)
		{
			ak_print_normal(MODULE_ID_APP, "end of ring file! \n\r");
			break;
		}
		else
		{
			ak_print_error(MODULE_ID_APP, "\nread, %s\n", strerror(errno));
			break;
		}
	}
	if (fp)
	{
		fclose(fp);
	}
	play_ring_flag = 0;
	ring_play_run = false;
}

void play_ring(unsigned int index)
{
	// watch_dog_close();
	ao_howling_suppress_pause();
	if (index < 12)
	{
		FILE *fp = open_doorbell_ring_file(index);
		if (fp)
		{
			read_play_ring_data(fp);
		}
	}
	// watchdog_open();
	ao_howling_suppress_close();
}
#endif
void stop_doorbell_ring(void)
{
	ring_play_doorbell_flag = false;
	ak_thread_mutex_lock(&audio_play_mutex);
	ring_play_run = false;
	ak_thread_mutex_unlock(&audio_play_mutex);
}

bool is_play_ring(void)
{
	int result;
	ak_thread_mutex_lock(&audio_play_mutex);
	if (play_ring_flag && ao_play_finish() == true)
	{
		play_ring_flag = false;
	}
	result = play_ring_flag;
	ak_thread_mutex_unlock(&audio_play_mutex);
	return result;
}

void play_doorbell1(int ring, int vol)
{
	if (is_network_audio_receive_connected() == false)
	{
		ring_play_doorbell_flag = true;
		// set_tone_volume(4);
		ak_print_normal(MODULE_ID_APP, ">>> [CURRENT TONE VOLUME] VOLUME < %d > \n\r", 4 /*音量*/);
		audio_output_volume_set(vol * 5 + 46);
		play_ring(ring - 1);
	}
}

void wait_doorbell_stop(void)
{
	if (ring_play_doorbell_flag == true)
	{
		if (is_network_audio_receive_connected() == true)
		{
			stop_doorbell_ring();
		}
	}
}

#define KEY1_LINGHT_GPIO_PIN 6
#define KEY2_LINGHT_GPIO_PIN 7
#define KEY3_LINGHT_GPIO_PIN 8
#define KEY4_LINGHT_GPIO_PIN 9

#define DELECT_IR_GPIO_PIN 65
#define IR_LIGHT_GPIO_PIN 81
// #define AUTO_LINGHT_GPIO_PIN 66
#ifdef NUMERIC_KEY_MODE
#define UNGATE_LINGHT_GPIO_PIN 69
#define UNLOCK_LINGHT_GPIO_PIN 41
#else
#define UNGATE_LINGHT_GPIO_PIN 69
#define UNLOCK_LINGHT_GPIO_PIN 69
#endif
#define INDOOR_LINGHT_GPIO_PIN 7

#define OPEN_LOCK1_GPIO_PIN 32
#define OPEN_INDOOR_LOCK2_GPIO_PIN 33

typedef struct
{
	int pin_index;
	int gpio_pin;
	int direction;
} GPIO_GROUP;

GPIO_GROUP GPIO[] = {
#ifndef NUMERIC_KEY_MODE
	{0, KEY1_LINGHT_GPIO_PIN, GPIO_DIR_OUT},
	{1, KEY2_LINGHT_GPIO_PIN, GPIO_DIR_OUT},
	{0, KEY3_LINGHT_GPIO_PIN, GPIO_DIR_OUT},
#endif
	{1, KEY4_LINGHT_GPIO_PIN, GPIO_DIR_OUT},
	{2, OPEN_LOCK1_GPIO_PIN, GPIO_DIR_LOW},
	{3, OPEN_INDOOR_LOCK2_GPIO_PIN, GPIO_DIR_LOW},
	// {4,DELECT_IR_GPIO_PIN,GPIO_DIR_IN},
	// {5,IR_LIGHT_GPIO_PIN,GPIO_DIR_OUT},
	{6, UNLOCK_LINGHT_GPIO_PIN, GPIO_DIR_OUT},
	{7, UNGATE_LINGHT_GPIO_PIN, GPIO_DIR_OUT},

	// {8,UNLOCK_DETECT_GPIO_PIN,GPIO_DIR_IN},
	// {9,UNGATE_DETECT_GPIO_PIN,GPIO_DIR_IN},
	// {10,OPEN_LOCK_GPIO_PIN,GPIO_DIR_OUT},
	// {11,OPEN_GATE1_GPIO_PIN,GPIO_DIR_OUT},
	// {12,I2C_INTERRUPT_GPIO_PIN,GPIO_DIR_IN},
	// {13,IRCUT_INA_GPIO_PIN,GPIO_DIR_OUT},
	// {14,IRCUT_INB_GPIO_PIN,GPIO_DIR_OUT},
	// {13,CARD_LIGHT_GPIO_PIN,GPIO_DIR_OUT},
};
void gpio_pin_init(void)
{
	int size = sizeof(GPIO) / sizeof(GPIO_GROUP);
	for (int i = 0; i < size; i++)
	{
		gpio_export(GPIO[i].gpio_pin);
		if (GPIO[i].direction != GPIO_DIR_RETAIN)
		{
			//
			// if(GPIO[i].gpio_pin == OPEN_LOCK_GPIO_PIN || GPIO[i].gpio_pin == OPEN_GATE1_GPIO_PIN)
			// {
			// 	gpio_level_set(GPIO[i].gpio_pin,GPIO_LEVEL_LOW);
			// }
			// else
			{
				gpio_direction_set(GPIO[i].gpio_pin, GPIO[i].direction);
			}
		}
	}
	// gpio_level_set(INFRARED_GPIO_PIN, GPIO_LEVEL_HIGH);
}
bool door_light_control(bool light_en, int light)
{

	// printf("door_light_control  %d================>>> %d\n\r", light, light_en);
	if (light == AUTO_LIGHT)
	{
		// gpio_set(AUTO_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	else if (light == TALKING_LIGHT)
	{
		// gpio_set(TALK_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	else if (light == UNLOCK_LIGHT)
	{
		gpio_set(UNLOCK_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	else if (light == UNGATE_LIGHT)
	{
		gpio_set(UNGATE_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
// else if (light == INDOOR_LIGHT)
// {
// 	gpio_set(INDOOR_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
// }
#ifndef NUMERIC_KEY_MODE
	else if (light == KEY1_LED)
	{
		gpio_set(KEY1_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	else if (light == KEY2_LED)
	{
		gpio_set(KEY2_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	else if (light == KEY3_LED)
	{
		gpio_set(KEY3_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
#endif
	else if (light == KEY4_LED)
	{
		gpio_set(KEY4_LINGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	else if (light == IR_LED)
	{
		gpio_set(IR_LIGHT_GPIO_PIN, light_en == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	return true;
}

void light_init(void)
{
	door_light_control(1, AUTO_LIGHT);
	door_light_control(0, TALKING_LIGHT);
	door_light_control(0, UNLOCK_LIGHT);
	door_light_control(0, UNGATE_LIGHT);
	door_light_control(0, INDOOR_LIGHT);
	door_light_control(0, KEY1_LED);
	door_light_control(0, KEY2_LED);
	door_light_control(0, KEY3_LED);
	door_light_control(0, KEY4_LED);
	door_light_control(0, IR_LED);
}

static unsigned long long start_call_light_ctrl_ms = 0;

void call_light_ctrl(int light)
{
#ifndef NUMERIC_KEY_MODE
	door_light_control(0, KEY1_LED);
	door_light_control(0, KEY2_LED);
	door_light_control(0, KEY3_LED);
#endif
	door_light_control(0, KEY4_LED);
	if (light != 0)
	{
		door_light_control(1, light);
		start_call_light_ctrl_ms = os_get_ms();
	}
}

void start_call_light_ctrl_ms_set(unsigned long long ms)
{
	start_call_light_ctrl_ms = ms;
}
void wait_call_light_ctrl_finish(void)
{
	if (start_call_light_ctrl_ms > 0)
	{
		if ((os_get_ms() - start_call_light_ctrl_ms) > 10000)
		{
			if (is_network_video_send_package_open() == false)
			{
				call_light_ctrl(0);
				start_call_light_ctrl_ms = 0;
			}

			// printf("%s ====================>%d\n\r",__func__,__LINE__);
			// stop_doorbell_ring();
			// printf("%s ====================>%d\n\r",__func__,__LINE__);
		}
	}
}

int IR_detect_detect(void)
{
	static GPIO_LEVEL prev_level = GPIO_LEVEL_HIGH;
	GPIO_LEVEL level;

	gpio_read(DELECT_IR_GPIO_PIN, &level);
	if (prev_level != level)
	{
		{
			prev_level = level;

			if (level == GPIO_LEVEL_LOW)
			{
				door_light_control(true, IR_LED);
			}
			else if (level == GPIO_LEVEL_HIGH)
			{
				door_light_control(false, IR_LED);
			}
		}
	}

	return 0;
}

#define UNLOCK_DETECT_GPIO_PIN 68

#include "time.h"

/**
 * @description: 获取时间
 * @param {timespec} *time 时间缓存
 * @return {*}  时间差值
 */
void GetClockTimeMs(struct timespec *time)
{
	clock_gettime(CLOCK_MONOTONIC, time);
	return;
}

/**
 * @description: 获取时间差
 * @param {timespec} *last_time 基准时间
 * @return {*}  时间差值
 */
unsigned long long DiffClockTimeMs(struct timespec *last_time)
{
	struct timespec curr_time;
	unsigned long long diff;
	clock_gettime(CLOCK_MONOTONIC, &curr_time);
	diff = (curr_time.tv_sec - last_time->tv_sec) * 1000 + (curr_time.tv_nsec - last_time->tv_nsec) / 1000000;

	return diff;
}

int outdoor_unlock_detect(void)
{
	extern int door_panel_lock_state(void);
	static GPIO_LEVEL prev_level = GPIO_LEVEL_HIGH;
	// static bool detect_beating = false;
	GPIO_LEVEL level;
	static struct timespec time;
	gpio_read(UNLOCK_DETECT_GPIO_PIN, &level);
	if (prev_level != level)
	{
		if (DiffClockTimeMs(&time) > 100)
		{
			prev_level = level;
			if (level == GPIO_LEVEL_LOW)
			{
				return 1;
			}
			else if (level == GPIO_LEVEL_HIGH)
			{
				return 0;
			}
		}
	}
	else
	{
		GetClockTimeMs(&time);
	}

	return -1;
}

static int door_opened_flag = 0;

static bool door_panel_lock_control(bool unlock, int lock)
{

	if (lock == OUTDOOR_LOCK_2)
	{
		gpio_level_set(OPEN_INDOOR_LOCK2_GPIO_PIN, unlock == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}
	else if (OUTDOOR_LOCK_1 == lock)
	{
		gpio_level_set(OPEN_LOCK1_GPIO_PIN, unlock == true ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	}

	if (unlock)
	{
		door_opened_flag |= lock;
	}
	else
	{
		door_opened_flag &= (~lock);
	}
	return true;
}

int door_panel_lock_state(void)
{
	return door_opened_flag;
}

static unsigned long long start_unlock_ms = 0;
static unsigned long long start_ungate_ms = 0;
static unsigned long long lock_delay = 0;
static unsigned long long gate_delay = 0;

bool start_unlock(char delay_mode, int lock)
{
	if (lock == OUTDOOR_LOCK_1)
	{
		if (start_unlock_ms > 0)
		{
			printf("[ door_panel_lock_status ERROR !!!!!] \n\r");
			return false;
		}
		start_unlock_ms = os_get_ms();
		if (delay_mode <= 0)
		{
			lock_delay = 5000;
		}
		else
		{
			lock_delay = delay_mode * 1000;
		}
	}
	else if (lock == OUTDOOR_LOCK_2)
	{
		if (start_ungate_ms > 0)
		{
			printf("[ door_panel_lock_status ERROR !!!!!] \n\r");
			return false;
		}
		start_ungate_ms = os_get_ms();
		if (delay_mode <= 0)
		{
			gate_delay = 5000;
		}
		else
		{
			gate_delay = delay_mode * 1000;
		}
	}
	printf("\n\r DOOR OPENED - LOCK TYPE[%s] DELAY[%llu] \n\r", lock == OUTDOOR_LOCK_1 ? "lock" : "gate", lock == OUTDOOR_LOCK_1 ? lock_delay : gate_delay);

	// play_ring(RING_INDEX_UPDATE_SUCCESS);
	door_light_control(true, lock == OUTDOOR_LOCK_1 ? UNLOCK_LIGHT : UNGATE_LIGHT);
	return door_panel_lock_control(true, lock);
}

void wait_unlock_finish(void)
{
	if ((start_unlock_ms > 0) && door_opened_flag & OUTDOOR_LOCK_1)
	{
		if ((os_get_ms() - start_unlock_ms) > lock_delay)
		{
			printf("\n\r DOOR LOCK CLOSED - DELAY[%llu] \n\r", lock_delay);
			door_panel_lock_control(false, OUTDOOR_LOCK_1);
			start_unlock_ms = 0;
			door_light_control(false, UNLOCK_LIGHT);
		}
	}

	if ((start_ungate_ms > 0) && door_opened_flag & OUTDOOR_LOCK_2)
	{
		if ((os_get_ms() - start_ungate_ms) > gate_delay)
		{
			printf("\n\r DOOR GATE CLOSED - DELAY[%llu] \n\r", gate_delay);
			door_panel_lock_control(false, OUTDOOR_LOCK_2);
			start_ungate_ms = 0;
			door_light_control(false, UNGATE_LIGHT);
		}
	}
}

bool network_call_send_cmd(int key, char ringback)
{
	/***** 通话状态下，call机不做任何处理 *****/
	printf("---->>key == %d\n\n", key);
	// if (is_network_audio_receive_connected() == true)
	// {
	// 	return false;
	// }

	network_cmd_data data;
	data.device = DEVICE_ALL;
	data.cmd = NET_COMMON_CMD_OUTDOOR_CALL;
	data.arg1 = ringback;
	data.arg2 = key;

	network_send_cmd_data(&data);

	return true;
}

static bool watch_dog_flag = false;

void watchdog_open(void)
{
	ak_drv_wdt_open(5);
	watch_dog_flag = true;
}

void watch_dog_close(void)
{
	if (watch_dog_flag)
	{
		ak_drv_wdt_close();
		watch_dog_flag = false;
	}
}

void watch_dog_feed(void)
{
	if (watch_dog_flag == true)
	{
		ak_drv_wdt_feed();
	}
}

void app_log_level(void)
{
	ak_print_set_level(MODULE_ID_MEMORY, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_THREAD, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_LOG, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_COMMON, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_IPCSRV, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AI, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AO, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_VI, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_VO, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_TIMER, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_VPSS, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_DRV, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_TDE, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_VENC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_VDEC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AENC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_ADEC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_OSD, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_MUX, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_DEMUX, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_DVR, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_MD, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_GUI, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AED, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AUDIO_TOOL, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_VQE, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_APP_VIDEO, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_DVR_IDX, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_NET, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_OSAL, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_APP_MEM, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_APP_AUDIO, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_UUID, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_ATC_ECHO, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_ATC_ADEC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_ATC_AENC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AKV_VENC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AKV_VDEC, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AKV_MUX, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_AKV_DEMUX, LOG_LEVEL_WARNING);
	ak_print_set_level(MODULE_ID_APP, LOG_LEVEL_WARNING);
}
