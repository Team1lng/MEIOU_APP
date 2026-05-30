#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ak_common.h"

#include "detection_call.h"
#include "outdoor_device.h"
#include "outdoor_unlock.h"
#include "device_led.h"
#include "audio_speak_amp.h"
#include "audio_output.h"

#include "network_common.h"
#include "video_input.h"
#include "audio_input.h"
#include "ak_md.h"
#include "ak_drv_gpio.h"
#include "app_common.h"
#include "ircut.h"

#include "ak_vpss.h"
#include "ak_log.h"

#include "motion_detect.h"
#include "ak_its.h"

#include "ak_ats.h"

#include "card_manage.h"

#include "audio_play.h"
#include "scan_numeric_key.h"
#include "ak_thread.h"
unsigned long g_reboot_timestamp = 0; // 重启计时起点（全局，不重置）
bool g_waiting_for_reboot = false;	  // 重启等待标记（全局）
#define REBOOT_INTERVAL_SEC 43200
static void ak_platform_sdk_init(void)
{
	/* init sdk running */
	sdk_run_config config;
	memset(&config, 0, sizeof(config));
	config.mem_trace_flag = SDK_RUN_NORMAL;
	config.audio_tool_server_flag = 0;
	config.isp_tool_server_flag = 0;
	ak_sdk_init(&config);
	// ak_its_start(8765);
}

/////////////////////////////////////////////////////////////////////////////

void detect_message_status(void)
{
	static bool prev_status = false;
	struct ak_timeval curr_tv;
	ak_get_ostime(&curr_tv);
	if (prev_status != message_status.open_message)
	{
		prev_status = message_status.open_message;
		if (prev_status)
		{
			/* 播放留言提示音 */
			printf("play ring message start.......................%d\n\r", RING_INDEX_MESSAGE_EH + message_status.message_language);
			play_doorbell(RING_INDEX_MESSAGE_EH + message_status.message_language, 10);
		}
	}
	else if (prev_status != false && is_network_video_send_package_open() == false)
	{
		message_status.open_message = prev_status = false;
		printf("play ring message end.......................%d\n\r", RING_INDEX_MESSAGE_EH + message_status.message_language);
	}
}

static network_device outdoor_get_device(void)
{
#define ROOM_ID_PIN (80)
	char value[FILE_PATH_MAX] = {0};
	memset(value, 0, FILE_PATH_MAX);

	system("rmmod ak_gpio_keys");

	ak_drv_gpio_open(ROOM_ID_PIN);
	ak_drv_gpio_write(ROOM_ID_PIN, NODE_PULL_ENABLE, "0", strlen("0"));
	ak_drv_gpio_read(ROOM_ID_PIN, NODE_VALUE, value, FILE_PATH_MAX);
	char *stop_str;
	int int_value = strtol(value, &stop_str, 0);
	if (int_value == 1)
	{
		printf("DEVICE : NET CAMERA 1\n");
		return DEVICE_OUTDOOR_1;
	}
	else
	{
		printf("DEVICE : NET CAMERA 2\n");
		return DEVICE_OUTDOOR_2;
	}
}

void detect_id_pin_change(void)
{
	char *stop_str;
	char value[8] = {0};
	ak_drv_gpio_read(ROOM_ID_PIN, NODE_VALUE, value, FILE_PATH_MAX);
	int int_value = strtol(value, &stop_str, 0);
	if (int_value != (network_local_device_get() == DEVICE_OUTDOOR_1 ? 1 : 0))
	{
		ak_sleep_ms(100);
		ak_drv_gpio_read(ROOM_ID_PIN, NODE_VALUE, value, FILE_PATH_MAX);
		int int_value_1 = strtol(value, &stop_str, 0);
		if (int_value == int_value_1)
		{
			printf("ROOM id %d  %d\n\r", int_value, network_local_device_get());
			system("reboot");
		}
	}
}
//////////////////////////////////////////////////////////////////////////////
#if 1
static void feed_dog(void)
{

	static unsigned long sec = 0;
	struct ak_timeval timeval;
	ak_get_ostime(&timeval);
	if (timeval.sec != sec)
	{
		watch_dog_feed();
		sec = timeval.sec;
	}
}
#endif
#if 0
static void check_talk_status(void)
{

	if (is_network_audio_receive_connected() == false)
	{
		if ((is_play_ring() == false) && (ao_play_finish() == true))
		{
			audio_speak_disable();
		}
	}
}

int main(int argc ,char* argv[])
{
    /***** 初始化安凯平台中间件 *****/
    ak_platform_sdk_init();
	/***** 初始化视频设备 *****/
	video_input_device_init();

	usleep(1000*1000);
	video_input_open();
	while(1)
	{
		usleep(10000);
	}
}
#else

static void *main_task(void *arg)
{
	while (1)
	{
		{
			// printf("----------------------------%d \n",__LINE__);
			/***** CALL处理 *****/
			key_event_deal_with();
			// printf("----------------------------%d \n",__LINE__);
			extern int outdoor_unlock_detect(void);
			if (!outdoor_unlock_detect())
			{
				printf("----------------------------%d \n", __LINE__);
				if (user_data_get()->exit_button_lock)
					start_unlock(user_data_get()->lock_unlock_time, OUTDOOR_LOCK_1);
				if (user_data_get()->exit_button_gate1)
					start_unlock(user_data_get()->gate_unlock_time, OUTDOOR_LOCK_2);
				printf("----------------------------%d \n", __LINE__);
			}
			// printf("----------------------------%d \n",__LINE__);
			wait_unlock_finish();
			// printf("----------------------------%d \n",__LINE__);
			// wait_doorbell_stop();
			wait_call_light_ctrl_finish();
			// printf("----------------------------%d \n",__LINE__);
			// motion_detect();
			motion_detect_start();
			// printf("----------------------------%d \n",__LINE__);
			detect_message_status();
			feed_dog();

#if (IRCUT_MODE == 0)
			ircut_ctrl_handler();
			// extern int IR_detect_detect(void);
			// IR_detect_detect();
#else
#endif

			extern void card_event_handler(void);
			card_event_handler();

			extern bool curr_audio_param_is_tuya;
			if (curr_audio_param_is_tuya == true && is_network_video_send_package_open() == false) // 當前是tuya音頻參數 且 無視頻，還會室內機音頻參數
			{
				ao_howling_suppress_close(); // 關閉嘯叫抑制
				audio_output_device_param_switch(false);
				audio_input_device_param_switch(false);
				ak_ao_restart(get_ao_hand());
			}

			ak_sleep_ms(10);
			// printf("----------------------------%d \n",__LINE__);
			// printf("=========================>>>>> 1 : %lld   \n", os_get_ms());
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{

	printf("\n\n###########################[%s]########################\n", IPC_MODEL);
	printf("# Compile Time:%s-%s\n", __DATE__, __TIME__);
	printf("####################################################################\n\n");
	// system("rmmod /usr/modules/ak_eth.ko");
	// ak_sleep_ms(100);
	// system("insmod /usr/modules/ak_eth.ko");
	/***** 初始化安凯平台中间件 *****/
	ak_platform_sdk_init();

	/***** 初始化音频输出设备 *****/
	audio_output_device_init();

	/***** 初始化音频采集设备 *****/
	audio_input_device_init();

// 设置消回声网络端口
#ifdef APP_ATS_OPEN
	ak_ats_start(8012);
#endif

	/***** 初始化视频设备 *****/
	video_input_device_init();

	// 开启自动夜视功能
#if (IRCUT_MODE == 0)
	ircut_gpio_init();
	printf("\n*****************************\n");
	printf("\n************硬件光敏**********\n");
	printf("\n*****************************\n");
#else
	ircut_start();
	printf("\n*****************************\n");
	printf("\n************软件光敏**********\n");
	printf("\n*****************************\n");
#endif

	/*****移动侦测初始化 *****/
	motion_detect_init(MOTION_DETECT_CH);
	/***** 初始化网络处理 *****/

	gpio_pin_init();

	/***** 开锁相关的初始化处理 *****/
	// device_unlock_init();

	/***** led 相关初始化 *****/
	// device_led_init();
	light_init();

	audio_play_init();

	 watchdog_open();

	/***** audio speak gpio 初始化 *****/
	audio_speak_init();

	/***** 读取本机设备（0:门口机1 1:门口机2） *****/
	network_device self_device = /*DEVICE_OUTDOOR_2;*/ outdoor_get_device();
	network_init(self_device);

	/***** 获取对方的ID *****/
	int slave_id = (self_device == DEVICE_OUTDOOR_1) ? network_get_id_outdoor1(self_device) : network_get_id_outdoor2(self_device);

	/***** 初始化视频套接字 *****/
	int eth_p_id = network_common_socket_eth_p_get(0, slave_id);

	/***** 打开视频发送任务 *****/
	network_video_send_package_open(eth_p_id);

	/***** 初始化音频套接字 *****/
	eth_p_id = network_common_socket_eth_p_get(1, slave_id);
	// printf("read device is:%d,slave_id:0x%x,eth_v_id:0x%x,eth_a_id:0x%x \n", self_device, slave_id, network_common_socket_eth_p_get(0, slave_id), network_common_socket_eth_p_get(1, slave_id));

	/***** 打开音频发送任务 *****/
	network_audio_send_package_open(eth_p_id);
	//====================================================

	/***** 打开音频接收任务 *****/
	network_audio_receive_package_open(eth_p_id);

	audio_output_open(AUDIO_CHANNEL_MONO, AK_AUDIO_SAMPLE_RATE_16000);

	user_data_init();

#ifdef TCP_NETWORK_MANAGEMENT
	extern void tcp_netwrok_init(void);
	tcp_netwrok_init();
#endif

#ifdef CARD_MODE
	card_drive_init();
#endif

	amp_turn_on();

	user_card_data_init();

#ifdef NUMERIC_KEY_MODE
	void numeric_keyboard_init(void);
	numeric_keyboard_init();
#endif
#ifdef TCP_NETWORK_MANAGEMENT
extern unsigned char g_operation_in_progress;
#endif
	g_reboot_timestamp = os_get_second();
	/***** 初始化call检测设备结点 *****/
	key_init();

	ak_pthread_t pthread_id;
	ak_thread_create(&pthread_id, main_task, NULL, 512 * 1024, -1);
	while (1)
	{
		ak_sleep_ms(30 * 1000);

		bool allow_reboot_check = false;

		if (is_network_video_send_package_open() == false		  // 视频未打开
			&& user_data_get()->operate_mode == USER_STANDBY_MODE // 设备待机
			&& password_keyboard_using == false
			&& (g_operation_in_progress == 0))
		{
			allow_reboot_check = true; // 允许检查重启时间
    }
    else
    {
        	allow_reboot_check = false;
	}
		if (allow_reboot_check)
		{
			if (os_get_second() - g_reboot_timestamp >= REBOOT_INTERVAL_SEC)
			{
				if (!g_waiting_for_reboot)
				{
					printf("--------------------reboot---------------\n");
					g_waiting_for_reboot = true;
					// 延迟10秒重启（可选，确保日志输出）
					sleep(10);
					// 执行实际重启操作
					system("reboot");
				}
			}
		}
		else
		{
			// 任意条件不满足，重置计时起点和标记
			// g_reboot_timestamp = os_get_second();
			g_waiting_for_reboot = false;
			// printf("重启计时重置（原因：视频打开/设备非待机/密码键盘使用中）\n");
		}
			
	}	
		return 0;
}

#endif
