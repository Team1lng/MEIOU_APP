#include "audio_output.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <memory.h>
#include <unistd.h>

#include "audio_config.h"

#include "ak_ai.h"
#include "ak_mem.h"
#include "audio_input.h"

static pthread_mutex_t audio_input_mutex;
static pthread_t audio_input_pthread_id;

static int audio_input_device = -1;
/***
** 音频输出使能判断标志
***/
static bool is_audio_input_enable = false;

/***
**	暂停采集
***/
static bool is_audio_input_pause = false;

/***
**  音量输出参数设置
***/
static enum ak_audio_channel_type audio_input_ch = AUDIO_CHANNEL_RESERVED;
static enum ak_audio_sample_rate audio_input_rate = AK_AUDIO_SAMPLE_RATE_16000;
static int audio_input_volume = 0;

/***
**	音频输出队列创建
***/
static int audio_input_queue_head = -1;

/***
**	采集队列计数器
***/
#define AUDIO_INPUT_QUEUE_MAX 10
static int audio_input_queue_count = 0;

static void audio_input_queue_create(void)
{
	key_t key = ftok("/tmp", 0666);
	audio_input_queue_head = msgget(key, 0666 | IPC_CREAT);

	stream_data data;
	while (msgrcv(audio_input_queue_head, (void *)&data, sizeof(stream_data), 0, IPC_NOWAIT) > 0)
		;
}

#if 0
static bool audio_input_device_reset()
{
	ak_ai_stop_capture(audio_input_device);
	struct ak_audio_in_param par;
	par.dev_id = DEV_ADC;
	par.pcm_data_attr.channel_num = audio_input_ch;
	par.pcm_data_attr.sample_bits = AK_AUDIO_SMPLE_BIT_16;
	par.pcm_data_attr.sample_rate = audio_input_rate;
	ak_ai_reset_params(audio_input_device, &par, 0);
	return true;
}
#endif

/***
**	打开实际的音频输出设备
***/
static bool audio_input_device_open(void)
{
	struct ak_audio_in_param par;
	par.dev_id = DEV_ADC;
	par.pcm_data_attr.channel_num = audio_input_ch;
	par.pcm_data_attr.sample_bits = AK_AUDIO_SMPLE_BIT_16;
	par.pcm_data_attr.sample_rate = audio_input_rate;
	ak_ai_open(&par, &audio_input_device);

	/*
	struct ak_audio_agc_attr user_ai_agc = {0};
	user_ai_agc.agc_level = USER_AI_AGC_LEVEL;
	user_ai_agc.agc_max_gain = USER_AI_AGC_MAX_GAIN;
	user_ai_agc.agc_min_gain = USER_AI_AGC_MIN_GAIN;
	user_ai_agc.near_sensitivity = USER_AI_NEAR_SENSITIVE;
	user_ai_agc.enable = ENABLE;
	*/
	struct ak_audio_nr_attr user_ai_nr_attr = {USER_AI_NOISE_SUP_DB, 0, ENABLE};
	struct ak_audio_agc_attr user_ai_agc_attr = {USER_AI_AGC_LEVEL, USER_AI_AGC_MAX_GAIN, USER_AI_AGC_MIN_GAIN, 1, 0, ENABLE};
	struct ak_audio_aec_attr user_ai_aec_attr = {0, USER_AEC_OUT_DIGI_GAIN, USER_AEC_IN_DIGI_GAIN, 0, USER_AEC_TAIL, ENABLE};
	struct ak_audio_aslc_attr user_ai_aslc_attr = {USER_AI_LIMIT, USER_AI_DB, ENABLE};
	struct ak_audio_eq_attr user_ai_eq_attr = {
		0,
		10,
		{50, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{717, 717, 717, 717, 717, 717, 717, 717, 717, 717},
		{TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

	ak_ai_set_agc_attr(audio_input_device, &user_ai_agc_attr);

	/*
		struct ak_audio_nr_attr user_ai_nr_attr = {0};
		user_ai_nr_attr.noise_suppress_db = USER_AI_NOISE_SUP_DB;
		user_ai_nr_attr.enable = ENABLE;
	*/
	ak_ai_set_nr_attr(audio_input_device, &user_ai_nr_attr);

	/*
		struct ak_audio_aec_attr user_aec_attr = {0};
		user_aec_attr.audio_in_digi_gain = USER_AEC_IN_DIGI_GAIN;
		user_aec_attr.audio_out_digi_gain = USER_AEC_OUT_DIGI_GAIN;
		user_aec_attr.audio_out_threshold = USER_AEC_OUT_THRESHLD;
		user_aec_attr.enable = ENABLE;
		user_aec_attr.tail = 512; //don't need to set this value now
	*/
	ak_ai_set_aec_attr(audio_input_device, &user_ai_aec_attr);

	/*
		struct ak_audio_aslc_attr user_ai_aslc_attr = {0};
		user_ai_aslc_attr.aslc_db = USER_AI_DB;
		user_ai_aslc_attr.limit = USER_AI_LIMIT;
	*/
	ak_ai_set_aslc_attr(audio_input_device, &user_ai_aslc_attr);

	/*
		struct ak_audio_eq_attr user_ai_eq_attr = {0};
		//user_ai_eq_attr.pre_gain = 12280;

		user_ai_eq_attr.bands = 1;
		user_ai_eq_attr.bandfreqs[0] = 2000;
		user_ai_eq_attr.bandgains[0] = (signed short) (-18 * (1 << 10));
		user_ai_eq_attr.bandQ[0] = (unsigned short) (1 * (1 << 10));
		user_ai_eq_attr.band_types[0] = TYPE_LPF;

		ak_ai_enable_eq(audio_input_device, 1);
	*/
	ak_ai_set_eq_attr(audio_input_device, &user_ai_eq_attr);

	ak_ai_set_source(audio_input_device, AI_SOURCE_MIC);

	ak_ai_set_gain(audio_input_device, USER_AI_GAIN);
	ak_ai_set_volume(audio_input_device, USER_AI_VOLUME);
	ak_ai_start_capture(audio_input_device);
	printf("%s =====================>>%d\n\r", __func__, audio_input_device);
	return true;
}

void set_ai_gain(int gain)
{
	static int default_gain = USER_AI_GAIN;
	if (gain == -1)
	{
		gain = USER_AI_GAIN;
	}
	if (default_gain != gain)
	{
		default_gain = gain;
		ak_ai_set_gain(audio_input_device, gain);
	}
}
/***
**	关闭实际的音频输出设备
***/
static bool audio_input_device_close(void)
{
	ak_ai_stop_capture(audio_input_device);
	ak_ai_close(audio_input_device);

	// audio_input_ch = AUDIO_CHANNEL_RESERVED;
	// audio_input_rate = AK_AUDIO_SAMPLE_RATE_16000;
	audio_input_device = -1;
	printf("%s =====================>>\n\r", __func__);
	return true;
}

bool is_network_audio_send_package_open(void);
static void *audio_input_task(void *arg)
{
	struct frame pcm_frame = {0};
	stream_data audio_data;
	bool ai_capture = false;
	// bool ai_reset = true;
	// while (1)
	// {
	// 	usleep(100);
	// }

	while (1)
	{
		pthread_mutex_lock(&audio_input_mutex);

		if (is_network_audio_send_package_open() == true && ai_capture == false)
		{
			printf("#####################################>>>audio_input_device_start\n");
			// ak_ai_start_capture(audio_input_device);
			// audio_input_device_open();
			// audio_input_device_open();
			// ak_ai_start_capture(audio_input_device);
			ai_capture = true;
		}
		else if (is_network_audio_send_package_open() == false && ai_capture == true)
		{
			printf("#####################################>>>audio_input_device_end\n");
			// ak_ai_stop_capture(audio_input_device);
			// audio_input_device_close();
			// audio_input_device_reset();
			audio_input_device_close();
			ak_sleep_ms(1);
			audio_input_device_open();
			ai_capture = false;
		}
		else
			// if(is_network_audio_send_package_open() != ai_reset)
			// {
			// 	if(ai_reset == true)
			// 	{
			// 		// audio_input_device_reset();
			// 		audio_input_device_close();
			// 		usleep(100);
			// 		audio_input_device_open();
			// 	}
			// 	else
			// 	{

			// 	}
			// 	ai_reset = is_network_audio_send_package_open();
			// }
			if (audio_input_device != -1)
			{
				memset(&pcm_frame, 0, sizeof(struct frame));
				if ((ak_ai_get_frame(audio_input_device, &pcm_frame, 0) == 0) && (pcm_frame.data != NULL) && (pcm_frame.len > 0))
				{
					if ((is_audio_input_pause == false) && ai_capture && (audio_input_queue_count < AUDIO_INPUT_QUEUE_MAX) && is_network_audio_send_package_open())
					{
						// unsigned long long os_get_ms(void);
						// unsigned long long  x= os_get_ms();
						audio_data.type = sizeof(stream_info);
						audio_data.info.size = pcm_frame.len;
						audio_data.info.data = ak_mem_alloc(MODULE_ID_AI, pcm_frame.len);
						if (audio_data.info.data == NULL)
						{
							printf("audio data get mem fail....\n\r");
							ak_ai_release_frame(audio_input_device, &pcm_frame);
							pthread_mutex_unlock(&audio_input_mutex);
							usleep(1);
							continue;
						}
						memcpy(audio_data.info.data, pcm_frame.data, pcm_frame.len);
						msgsnd(audio_input_queue_head, &audio_data, sizeof(stream_info), IPC_NOWAIT);
						// printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA22222222222:%d\n",dev_buf_size);
						audio_input_queue_count++;
					}

					// ak_ai_print_runtime_status(audio_input_device);
					ak_ai_release_frame(audio_input_device, &pcm_frame);
				}
				else
				{
				}
			}

		pthread_mutex_unlock(&audio_input_mutex);
		usleep(10);
	}
	return NULL;
}

void audio_input_aec_control(bool enable)
{
	return;
	printf("==========>>> audio_input_aec %s <<<==========\n", enable ? "open" : "close");
	static bool en = false;

	if (en != enable)
		ak_ai_enable_aec(audio_input_device, enable);

	en = enable;
}

/***
**  date:2022/04/19
**  author:刘炼
**  初始化音频输出设备
***/
bool audio_input_device_init(void)
{
	/***** 只能初始化一次 *****/
	static bool inited = false;
	if (inited == true)
	{
		return false;
	}

	inited = true;

	audio_input_queue_create();

	audio_input_open(AUDIO_CHANNEL_MONO, AK_AUDIO_SAMPLE_RATE_16000);

	audio_input_device_open();

	pthread_mutex_init(&audio_input_mutex, NULL);
	pthread_create(&audio_input_pthread_id, NULL, audio_input_task, NULL);
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  开启音频采集设备
**  返回false：打开失败
**  CH:音频通道（单通道/双通道）
**  rate:采样率设置
**  音频位深安凯平台只能16BIT
***/
bool audio_input_open(enum ak_audio_channel_type ch, enum ak_audio_sample_rate rate)
{
	pthread_mutex_lock(&audio_input_mutex);
	if (is_audio_input_enable == true)
	{
		if ((ch == audio_input_ch) && (rate == audio_input_rate))
		{
			pthread_mutex_unlock(&audio_input_mutex);
			return true;
		}
		// audio_input_device_close();
	}
	audio_input_ch = ch;
	audio_input_rate = rate;

	is_audio_input_pause = false;
	is_audio_input_enable = true;
	pthread_mutex_unlock(&audio_input_mutex);
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  关闭音频采集设备
***/
bool audio_input_close(void)
{
	printf("%s========================>>>\n\r", __func__);
	pthread_mutex_lock(&audio_input_mutex);
	if (is_audio_input_enable == false)
	{
		pthread_mutex_unlock(&audio_input_mutex);
		return false;
	}
	is_audio_input_enable = false;
	pthread_mutex_unlock(&audio_input_mutex);
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  读取采集的音频数据，读取成功后需要手动释放数据
***/
bool audio_input_read(unsigned char **data, int *size)
{
	bool reslut = false;
	pthread_mutex_lock(&audio_input_mutex);
	if (is_audio_input_enable == false)
	{
		pthread_mutex_unlock(&audio_input_mutex);
		return false;
	}

	stream_data audio_data;
	if (msgrcv(audio_input_queue_head, (void *)&audio_data, sizeof(stream_data), sizeof(stream_info), IPC_NOWAIT) > 0)
	{
		*data = audio_data.info.data;
		*size = audio_data.info.size;
		audio_input_queue_count--;
		reslut = true;
	}
	pthread_mutex_unlock(&audio_input_mutex);
	return reslut;
}

/***
**  date:2022/04/19
**  author:刘炼
**  采集到的音量,音量范围（0-100）
***/
bool audio_input_volume_set(int volume)
{
	audio_input_volume = volume;
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  获取采集音量,音量范围（0-100）
***/
int audio_input_volume_get(void)
{
	return audio_input_volume;
}

/***
**	date:2022/04/19
**	author:刘炼
**	音量采集暂停/开始
***/
bool audio_input_capture_pause(bool yeno)
{
	is_audio_input_pause = yeno;
	return true;
}

void tuya_audio_in_vol_switch(bool tuya)
{
	static bool tuya_state = false;
	if (tuya_state != tuya)
	{
		ak_ai_set_gain(audio_input_device, tuya ? TUYA_USER_AI_GAIN : USER_AI_GAIN);
		ak_ai_set_volume(audio_input_device, tuya ? TUYA_USER_AI_VOLUME : USER_AI_VOLUME);
		printf("[%s============================%s\n]", __func__, tuya ? "TUYA" : "NORMAL");
		tuya_state = tuya;
	}
}

void audio_input_device_param_switch(bool param_type)
{
	struct ak_audio_eq_attr user_ai_eq_attr = {
		0,
		10,
		{50, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{717, 717, 717, 717, 717, 717, 717, 717, 717, 717},
		{TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

	if (param_type == false)
	{
		struct ak_audio_nr_attr user_ai_nr_attr = {USER_AI_NOISE_SUP_DB, 0, ENABLE};
		struct ak_audio_agc_attr user_ai_agc_attr = {USER_AI_AGC_LEVEL, USER_AI_AGC_MAX_GAIN, USER_AI_AGC_MIN_GAIN, 1, 0, ENABLE};
		struct ak_audio_aec_attr user_ai_aec_attr = {0, USER_AEC_OUT_DIGI_GAIN, USER_AEC_IN_DIGI_GAIN, 0, USER_AEC_TAIL, ENABLE};
		struct ak_audio_aslc_attr user_ai_aslc_attr = {USER_AI_LIMIT, USER_AI_DB, ENABLE};
		ak_ai_set_agc_attr(audio_input_device, &user_ai_agc_attr);
		ak_ai_set_nr_attr(audio_input_device, &user_ai_nr_attr);
		ak_ai_set_aec_attr(audio_input_device, &user_ai_aec_attr);
		ak_ai_set_aslc_attr(audio_input_device, &user_ai_aslc_attr);
		ak_ai_set_eq_attr(audio_input_device, &user_ai_eq_attr);
		ak_ai_set_source(audio_input_device, AI_SOURCE_MIC);
		ak_ai_set_gain(audio_input_device, USER_AI_GAIN);
		ak_ai_set_volume(audio_input_device, USER_AI_VOLUME);
		// ak_ai_start_capture(audio_input_device);
		return;
	}

	struct ak_audio_nr_attr user_ai_nr_attr = {TUYA_USER_AI_NOISE_SUP_DB, 0, ENABLE};
	struct ak_audio_agc_attr user_ai_agc_attr = {TUYA_USER_AI_AGC_LEVEL, TUYA_USER_AI_AGC_MAX_GAIN, TUYA_USER_AI_AGC_MIN_GAIN, 1, 0, ENABLE};
	struct ak_audio_aec_attr user_ai_aec_attr = {0, TUYA_USER_AEC_OUT_DIGI_GAIN, TUYA_USER_AEC_IN_DIGI_GAIN, 0, TUYA_USER_AEC_TAIL, ENABLE};
	struct ak_audio_aslc_attr user_ai_aslc_attr = {TUYA_USER_AI_LIMIT, TUYA_USER_AI_DB, ENABLE};
	ak_ai_set_agc_attr(audio_input_device, &user_ai_agc_attr);
	ak_ai_set_nr_attr(audio_input_device, &user_ai_nr_attr);
	ak_ai_set_aec_attr(audio_input_device, &user_ai_aec_attr);
	ak_ai_set_aslc_attr(audio_input_device, &user_ai_aslc_attr);
	ak_ai_set_eq_attr(audio_input_device, &user_ai_eq_attr);
	ak_ai_set_source(audio_input_device, AI_SOURCE_MIC);
	ak_ai_set_gain(audio_input_device, TUYA_USER_AI_GAIN);
	ak_ai_set_volume(audio_input_device, TUYA_USER_AI_VOLUME);
	// ak_ai_start_capture(audio_input_device);
}