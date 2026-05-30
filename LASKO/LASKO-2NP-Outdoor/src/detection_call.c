#include "detection_call.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include "ak_common.h"
#include <pthread.h>
#include "app_common.h"
#include "device_led.h"
#include "card_manage.h"

#define DET_CALL_KO_PATH "/usr/modules/ak_saradc.ko"
#define DET_CALL_DEV "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
static int det_call_dev_fd = -1;

#define DET_CALL_KEY4_ACTION_LEVEL 1760
#define DET_CALL_KEY3_ACTION_LEVEL 1390
#ifdef NUMERIC_KEY_MODE
#define DET_CALL_KEY2_ACTION_LEVEL 2200
#define DET_CALL_KEY1_ACTION_LEVEL 560
#else
#define DET_CALL_KEY2_ACTION_LEVEL 560
#define DET_CALL_KEY1_ACTION_LEVEL 2200
#endif
#define FREEVALUE 3299
#define VOLTAGE_INTERVAL 200

static char last_det_level = 5;
static struct ak_timeval tv_cur, tv_pre;

static char det_call_pin_read(void)
{
	char temp[64] = {0};
	lseek(det_call_dev_fd, 0, SEEK_SET);
	// printf("==============>>%d\n\n\n", __LINE__);
	if (read(det_call_dev_fd, temp, sizeof(temp)) < 0)
	{
		printf("read det call value failed \n");
		return -1;
	}
	// printf("==============>>%d\n\n\n", __LINE__);
	/***** 将字符串转换为10进制的数值 *****/
	int value = 0;
	sscanf(temp, "%d", &value);

	// printf("==============>>%d,%d\n\n\n", value, DET_CALL_KEY1_ACTION_LEVEL);
	ak_get_ostime(&tv_cur);
	if (ak_diff_ms_time(&tv_cur, &tv_pre) >= 500)
	{

		if (last_det_level >= 5 || (value > FREEVALUE - VOLTAGE_INTERVAL && value < FREEVALUE + VOLTAGE_INTERVAL))
		{

			if (value > DET_CALL_KEY1_ACTION_LEVEL - VOLTAGE_INTERVAL && value < DET_CALL_KEY1_ACTION_LEVEL + VOLTAGE_INTERVAL)
			{
				tv_pre = tv_cur;
				last_det_level = 1;
				return 1;
			}
			else if (value > DET_CALL_KEY2_ACTION_LEVEL - VOLTAGE_INTERVAL && value < DET_CALL_KEY2_ACTION_LEVEL + VOLTAGE_INTERVAL)
			{
				tv_pre = tv_cur;
				last_det_level = 2;
				return 2;
			}
			else if (value > DET_CALL_KEY3_ACTION_LEVEL - VOLTAGE_INTERVAL && value < DET_CALL_KEY3_ACTION_LEVEL + VOLTAGE_INTERVAL)
			{
				tv_pre = tv_cur;
				last_det_level = 3;
				return 3;
			}
			else if (value > DET_CALL_KEY4_ACTION_LEVEL - VOLTAGE_INTERVAL && value < DET_CALL_KEY4_ACTION_LEVEL + VOLTAGE_INTERVAL)
			{
				tv_pre = tv_cur;
				last_det_level = 4;
				return 4;
			}
			else
			{
				last_det_level = 5;
				return 5;
			}
		}
	}

	last_det_level = 6;
	return 6;
}

bool det_call_pin_init(void)
{
	/***** 安装AVIN 的驱动文件 *****/
	system("insmod " DET_CALL_KO_PATH);

	/***** 打开设备结点 ******/
	det_call_dev_fd = open(DET_CALL_DEV, O_RDONLY);
	if (det_call_dev_fd < 0)
	{
		printf("open %s failed \n", DET_CALL_DEV);
		return false;
	}
	printf("init det call pin success \n");
	return true;
}

bool det_call_pin_uninit(void)
{
	if (det_call_dev_fd < 0)
	{
		return true;
	}

	close(det_call_dev_fd);
	det_call_dev_fd = -1;
	system("remmod " DET_CALL_KO_PATH);
	return true;
}

char det_call_pin_call(void)
{
	char level = det_call_pin_read();
	if (level < 5)
	{
		printf("======22222========>>%d\n\n\n", level);
		device_led_open_enable();
		return level;
	}
	return 5;
}

static pthread_mutex_t key_mutex;
static char key_trigger_num = 5;

static void *key_task(void *arg)
{
	struct ak_timeval detect_time;
	unsigned long pre_detect_t = 0;
	// struct ak_timeval detect_time2;
	// unsigned long pre_detect_t2 = 0;
	if (!det_call_pin_init())
	{
		printf("Key init failed \n");
		return NULL;
	}

	ak_get_ostime(&tv_pre);

	while (1)
	{

		ak_get_ostime(&detect_time);
		if (detect_time.sec != pre_detect_t)
		{
			pre_detect_t = detect_time.sec;
			extern void detect_id_pin_change(void);
			detect_id_pin_change();
		}

		char tmp;

		tmp = det_call_pin_read();

		// ak_get_ostime(&detect_time2);
		// if(detect_time2.sec - pre_detect_t2 > 50)
		// {
		// 	pre_detect_t2 = detect_time2.sec;
		// 	tmp = 3;
		// }
		// printf("[TRIGGER CALL]key_trigger_num :==============>%d\n", tmp);
		if (tmp <= 4 && tmp >= 1)
		{
			pthread_mutex_lock(&key_mutex);
			printf("[TRIGGER CALL]key_trigger_num :==============>%d\n", key_trigger_num);
			key_trigger_num = tmp;
			pthread_mutex_unlock(&key_mutex);
		}

		ak_sleep_ms(5);
	}
}

void key_init(void)
{
	pthread_mutex_init(&key_mutex, NULL);

	static pthread_t key_pthread_id;
	pthread_create(&key_pthread_id, NULL, key_task, NULL);
}

void key_trigger_set(int family)
{
	pthread_mutex_lock(&key_mutex);
	key_trigger_num = family;
	pthread_mutex_unlock(&key_mutex);
}

void key_event_deal_with(void)
{
	static unsigned long long key_call_time = 0;
	// unsigned long long curr_call_time = 0;
	pthread_mutex_lock(&key_mutex);
	// printf("======****************************========>>\n\n\n");
	if (key_trigger_num <= 4 && key_trigger_num >= 1)
	{
		printf("======2222========>>%d\n\n\n", key_trigger_num);
		if (card_manage_state_get() == CARD_STATE_ADD_CARD)
		{
			key_trigger_num = 0;
			pthread_mutex_unlock(&key_mutex);
			return;
		}
		// call_light_ctrl(KEY1_LED + key_trigger_num == 1 ? KEY1_LED : key_trigger_num + 4);
		call_light_ctrl(KEY1_LED + key_trigger_num - 1);
		bool is_network_audio_receive_connected(void);
		network_call_send_cmd(key_trigger_num, 1 << is_network_audio_receive_connected());
		key_trigger_num = 0;
		key_call_time = os_get_ms();
	}

	pthread_mutex_unlock(&key_mutex);

	bool is_network_video_send_package_open(void);
	if (key_call_time && (os_get_ms() - key_call_time > 2000))
	{
		key_call_time = 0;
		if (is_network_video_send_package_open() == false)
			play_doorbell(RING_INDEX_CALL_BUSY, 7);
	}
}
