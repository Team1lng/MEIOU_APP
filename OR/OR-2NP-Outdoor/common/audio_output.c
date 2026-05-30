
#include "audio_output.h"
#include "pthread.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <unistd.h>
#include <memory.h>

#include "ak_ai.h"
#include "ak_mem.h"
#include "ak_ats.h"

#include "audio_speak_amp.h"
#include "audio_config.h"

#define PCM_SIZE_MAX 4 * 1024

static pthread_mutex_t audio_output_mutex;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_t audio_output_pthread_id;

static int audio_output_device = -1;
/***
** 音频输出使能判断标志
***/
static bool is_audio_output_enable = false;

/***
**  音量输出参数设置
***/
static enum ak_audio_channel_type audio_output_ch = AUDIO_CHANNEL_RESERVED;
static enum ak_audio_sample_rate audio_output_rate = AK_AUDIO_SAMPLE_RATE_16000;
static int audio_output_volume = 87;

/***
**	音频输出队列创建
***/
static int audio_output_queue_head = -1;
static void audio_output_queue_create(void)
{
	key_t key = ftok("/tmp", 01);
	printf("[AUDIO] key=0x%x\n", key);
	audio_output_queue_head = msgget(key, 0666 | IPC_CREAT);
	printf("[AUDIO] queue_head=%d\n", audio_output_queue_head);
	stream_data data;
	while (msgrcv(audio_output_queue_head, (void *)&data, sizeof(stream_data), 0, IPC_NOWAIT) > 0)
		;
}

/***
**	打开实际的音频输出设备
***/
static bool audio_output_device_open(void)
{
	struct ak_audio_out_param par;
	par.dev_id = DEV_DAC;
	par.pcm_data_attr.channel_num = audio_output_ch;
	par.pcm_data_attr.sample_bits = AK_AUDIO_SMPLE_BIT_16;
	par.pcm_data_attr.sample_rate = audio_output_rate;
	ak_ao_open(&par, &audio_output_device);

	struct ak_audio_nr_attr user_ao_nr_attr = {USER_AO_NOISE_SUP_DB, 0, ENABLE};
	struct ak_audio_aslc_attr user_ao_aslc_attr = {USER_AO_LIMIT, USER_AO_DB, ENABLE};

	struct ak_audio_eq_attr user_ao_eq_attr = {
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

	/*
	struct ak_audio_nr_attr user_ao_nr_attr = {0};
	user_ao_nr_attr.noise_suppress_db = USER_AO_NOISE_SUP_DB;
	user_ao_nr_attr.enable = ENABLE;
	*/
	ak_ao_set_nr_attr(audio_output_device, &user_ao_nr_attr);

	/*
		struct ak_audio_aslc_attr user_ao_aslc_attr = {0};
		user_ao_aslc_attr.aslc_db = USER_AO_DB;
		user_ao_aslc_attr.limit = USER_AO_LIMIT;
		ak_ao_enable_nr(audio_output_device, 1);

	*/
	ak_ao_set_aslc_attr(audio_output_device, &user_ao_aslc_attr);

	/*

	struct ak_audio_eq_attr user_ao_eq_attr = {0};
	user_ao_eq_attr.bands = 1;
	user_ao_eq_attr.bandfreqs[0] = 2000;
	user_ao_eq_attr.bandgains[0] = (signed short) (-18 * (1 << 10));
	user_ao_eq_attr.bandQ[0] = (unsigned short) (1 * (1 << 10));
	user_ao_eq_attr.band_types[0] = TYPE_LPF;
	ak_ao_enable_eq(audio_output_device, 1);
	*/
	ak_ao_set_eq_attr(audio_output_device, &user_ao_eq_attr);
	ak_ao_enable_eq(audio_output_device, 1);

	ak_ao_set_gain(audio_output_device, USER_AO_GAIN);
	ak_ao_set_volume(audio_output_device, USER_AO_VOLUME);

	amp_turn_on();
	return true;
}

/***
**	关闭实际的音频输出设备
***/
static bool audio_output_device_close(void)
{
	audio_speak_disable();

	ak_ao_close(audio_output_device);
	audio_output_ch = AUDIO_CHANNEL_RESERVED;
	audio_output_rate = AK_AUDIO_SAMPLE_RATE_16000;

	audio_output_device = -1;
	printf("[AMP] audio_output_device_close: 关闭设备，AMP已禁用\n");
	return true;
}

/***
**	设置16BIT的PCM音量，转换一次的最大size不能超过PCM_SIZE_MAX
**	src:pcm源数据
**	size:pcm数据的长度,比如80将音量设置为之前的80%
**	设置音量-百分比
***/
#if 1
#include "math.h"
static bool audio_volume_cover(unsigned char *src, int size, int volume)
{
	static int prev_volume = 0;
	float multiplier = pow((10), (float)(volume - 96) / 20);
	if (prev_volume != volume)
	{
		prev_volume = volume;
		// printf("%s===============>>%d - %lf\n",__func__,volume,multiplier);
	}

	unsigned char dst[PCM_SIZE_MAX] = {0};

	if (size > PCM_SIZE_MAX)
	{
		return false;
	}

	memset(dst, 0, PCM_SIZE_MAX);
	for (int i = 0; i < size; i += 2)
	{
		short src_data = (src[i + 1] << 8) | (src[i] & 0xFF);

		src_data = src_data * multiplier;

		if (src_data > 32767)
		{
			printf("src_data :%d\n", src_data);
			src_data = 32767;
		}
		else if (src_data < -32768)
		{
			printf("src_data :%d\n", src_data);
			src_data = -32768;
		}

		dst[i] = src_data & 0xFF;
		dst[i + 1] = (src_data >> 8) & 0xFF;
	}

	memcpy(src, dst, size);
	return true;
}
#else

static bool audio_volume_cover(unsigned char *src, int size, int volume)
{
	int i = 0;
	unsigned char dst[PCM_SIZE_MAX] = {0};

	if (size > PCM_SIZE_MAX)
	{
		return false;
	}

	if (volume == 100)
	{
		return true;
	}

	if (volume == 0)
	{
		memset(src, 0, size);
		return true;
	}

	memset(dst, 0, PCM_SIZE_MAX);

	float bar = volume * 1.0 / 100;

	for (i = 0; i < size; i += 2)
	{
		short src_data = (src[i + 1] << 8) | (src[i] & 0xFF);

		src_data = src_data * bar;

		if (src_data > 32767)
		{
			src_data = 32767;
		}
		else if (src_data < -32768)
		{
			src_data = -32768;
		}

		dst[i] = src_data & 0xFF;
		dst[i + 1] = (src_data >> 8) & 0xFF;
	}

	memcpy(src, dst, size);
	return true;
}
#endif
static void ao_cond_timewati(int ms)
{
	pthread_mutex_lock(&audio_output_mutex);
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_nsec += ms * 1000 * 1000; // 将等待时间转换为纳秒

	// 处理纳秒溢出
	if (timeout.tv_nsec >= 1000000000)
	{
		timeout.tv_sec += 1;
		timeout.tv_nsec -= 1000000000;
	}

	pthread_cond_timedwait(&cond, &audio_output_mutex, &timeout);
	pthread_mutex_unlock(&audio_output_mutex);
}
static void *audio_output_task(void *arg)
{
	stream_data audio_data;
	int play_len = 0;
	int data_empty_ms = 0;
	while (1)
	{
		pthread_mutex_lock(&audio_output_mutex);
		if (msgrcv(audio_output_queue_head, (void *)&audio_data, sizeof(stream_data), sizeof(stream_info), IPC_NOWAIT) > 0)
		{
			// printf("==========[%s][%d] ==========\n", __func__, __LINE__);
			// printf("==========[%s][%d] ====is_audio_output_enable %d======\n", __func__, __LINE__, is_audio_output_enable);
			// printf("==========[%s][%d] ====audio_output_device %d======\n", __func__, __LINE__, audio_output_device);
			// printf("==========[%s][%d] ====audio_output_volume %d======\n", __func__, __LINE__, audio_output_volume);
			pthread_mutex_unlock(&audio_output_mutex);
			if (is_audio_output_enable == true)
			{
				if ((audio_output_device != -1) && (audio_output_volume != 0))
				{
					// if (audio_output_volume != 100)
					// {
					// 	// int gain = 0, vol = 0;
					// 	// ak_ao_get_gain(get_ao_hand(), &gain);
					// 	// ak_ao_get_volume(get_ao_hand(), &vol);
					// 	// printf("==========================>>>> ak增益:[%d] ak音量:[%d] audio_output_volume:[%d] \n", gain, vol, audio_output_volume);

					data_empty_ms = 0;
					// printf("[AMP] 收到音频数据(size=%d)，调用amp_turn_on\n", audio_data.info.size);
					amp_turn_on();
					audio_volume_cover(audio_data.info.data, audio_data.info.size, audio_output_volume);
					// }

					int send_len = 0, data_len = audio_data.info.size /* ,remain_len = get_ao_buf_remain_len() */;
					// printf("GET_AO_BUF_REMAIN_LEN:%d,%d\n",get_ao_buf_remain_len(),data_len);
					// unsigned long long x = os_get_ms();
					// static unsigned long long y ;
					while (data_len > 0)
					{

						if (data_len > 4096)
						{

							ak_ao_send_frame(audio_output_device, &(audio_data.info.data[send_len]), 4096, &play_len);
							// printf("AAAAAAAAAAAAAAAA:%lld,%d,%d\n",os_get_ms() - x,remain_len,get_ao_buf_remain_len());
							send_len += 4096;
							data_len -= 4096;
						}
						else
						{
							// x = os_get_ms();
							ak_ao_send_frame(audio_output_device, &(audio_data.info.data[send_len]), data_len, &play_len);
							// printf("ak_ao_send_frame:%lld,%lld,%d,%d\n",os_get_ms() - x,os_get_ms() - y,remain_len,get_ao_buf_remain_len());
							// y = os_get_ms();
							break;
						}
					}
					// ak_ao_send_frame(audio_output_device, audio_data.info.data, audio_data.info.size, &play_len);
				}
			}
			ak_mem_free(audio_data.info.data);
			continue;
		}
		else if ((is_audio_output_enable == false) && (audio_output_device != -1))
		{
			printf("===111=================關閉音頻輸出設備================>>>\n\n");
			audio_output_device_close();
		}
		else if ((is_audio_output_enable == true) && (audio_output_device == -1))
		{
			printf("===222=================打開音頻輸出設備================>>>\n\n");
			ak_sleep_ms(1000);
			audio_output_device_open();
		}
		else if (amp_status_get() && data_empty_ms++ > 500)
		{
			// printf("[AMP] 无音频数据超时(%dms)，调用amp_turn_off\n", data_empty_ms);
			amp_turn_off();
		}
		else
		{
			pthread_mutex_unlock(&audio_output_mutex);
			ao_cond_timewati(10);
			continue;
		}

		pthread_mutex_unlock(&audio_output_mutex);

		// usleep(1);
	}

	return NULL;
}

int get_ao_buf_remain_len(void)
{
	struct ak_dev_buf_status status;
	ak_ao_get_buf_status(audio_output_device, &status);
	return status.buf_remain_len;
}

/***
**  date:2022/04/19
**  author:刘炼
**  初始化音频输出设备
***/
bool audio_output_device_init(void)
{
	/***** 只能初始化一次 *****/
	static bool inited = false;
	if (inited == true)
	{
		return false;
	}

	inited = true;

	audio_output_queue_create();
	pthread_mutex_init(&audio_output_mutex, NULL);
	pthread_create(&audio_output_pthread_id, NULL, audio_output_task, NULL);
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  开启音频输出设备
**  返回false：打开失败
**  CH:音频通道（单通道/双通道）
**  rate:采样率设置
**  音频位深安凯平台只能16BIT
***/
bool audio_output_open(enum ak_audio_channel_type ch, enum ak_audio_sample_rate rate)
{
	pthread_mutex_lock(&audio_output_mutex);
	if (is_audio_output_enable == true)
	{
		if ((ch == audio_output_ch) && (rate == audio_output_rate))
		{
			pthread_mutex_unlock(&audio_output_mutex);
			return true;
		}
		// audio_output_device_close();
	}
	audio_output_ch = ch;
	audio_output_rate = rate;

	is_audio_output_enable = true;
	pthread_mutex_unlock(&audio_output_mutex);
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  关闭音频输出设备
***/
bool audio_output_close(void)
{
	pthread_mutex_lock(&audio_output_mutex);
	if (is_audio_output_enable == false)
	{
		pthread_mutex_unlock(&audio_output_mutex);
		return false;
	}
	is_audio_output_enable = false;
	pthread_mutex_unlock(&audio_output_mutex);
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼4*1024
**  写入音频数据到输出设备
***/
bool audio_output_write(unsigned char *data, int size)
{
	pthread_mutex_lock(&audio_output_mutex);
	if (is_audio_output_enable == false)
	{
		pthread_mutex_unlock(&audio_output_mutex);
		return false;
	}
	stream_data audio_data;
	audio_data.info.data = ak_mem_alloc(MODULE_ID_AO, size);
	memcpy(audio_data.info.data, data, size);
	audio_data.info.size = size;
	audio_data.type = sizeof(stream_info);
	msgsnd(audio_output_queue_head, &audio_data, sizeof(stream_info), 0);
	pthread_mutex_unlock(&audio_output_mutex);
	pthread_cond_signal(&cond);

	return true;
}

void audio_ao_buffer_clear(void)
{
	ak_ao_clear_frame_buffer(audio_output_device);
}
/***
**  date:2022/04/19
**  author:刘炼
**  设置输出音量,音量范围（0-100）
***/
bool audio_output_volume_set(int volume)
{
	if (volume == 0)
	{
		volume = 1;
	}
	else if (volume == 100)
	{
		volume = 96;
	}
	audio_output_volume = volume;
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  获得输出音量,音量范围（0-100）
***/
int audio_output_volume_get(void)
{
	return audio_output_volume;
}

static bool ao_speaker_status = false;
bool amp_status_get(void)
{
	return ao_speaker_status;
}

void amp_turn_off(void)
{
	if ((audio_output_device != -1) && (ao_speaker_status))
	{
		audio_speak_disable();
		ao_speaker_status = false;
		printf(" - AMP TURN OFF - \n\r");
	}
}

void amp_turn_on(void)
{
	if ((audio_output_device != -1) && (ao_speaker_status == false))
	{
		audio_speak_enable();
		ao_speaker_status = true;
		printf(" - AMP TURN ON - \n\r");
	}
}

bool ao_play_finish(void)
{
	if (audio_output_device != -1)
	{
		struct ak_dev_buf_status status;
		int ret = ak_ao_get_buf_status(audio_output_device, &status);
		if (ret == AK_SUCCESS)
		{
			// printf("status.buf_remain_len :%d\n",status.buf_remain_len);
			if (status.buf_remain_len == 0)
			{
				return true;
			}
		}
	}
	return false;
}

static bool ao_howling_status_flag = false;
void ao_howling_suppress_open(void)
{
	return;
	if ((audio_output_device != -1) && (ao_howling_status_flag == false))
	{

		ao_howling_status_flag = true;
		ak_ao_enable_hs(audio_output_device, true);
	}
};

void ao_howling_suppress_close(void)
{
	return;
	if (audio_output_device != -1)
	{
		ak_ao_enable_hs(audio_output_device, false);
		ao_howling_status_flag = false;
	}
};

void ao_howling_suppress_pause(void)
{
	return;
	if (audio_output_device != -1)
	{
		ak_ao_enable_hs(audio_output_device, false);
		ao_howling_status_flag = true;
	}
}

int get_ao_hand(void)
{
	return audio_output_device;
}

bool curr_audio_param_is_tuya = false;
void audio_output_device_param_switch(bool param_type)
{
	printf("****************************%s,%d*****************************\n", __func__, param_type);
	audio_speak_disable();
	if (param_type == false)
	{
		curr_audio_param_is_tuya = false;
		struct ak_audio_nr_attr user_ao_nr_attr = {USER_AO_NOISE_SUP_DB, 0, ENABLE};
		struct ak_audio_aslc_attr user_ao_aslc_attr = {USER_AO_LIMIT, USER_AO_DB, ENABLE};
		struct ak_audio_eq_attr user_ao_eq_attr = {
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
		ak_ao_set_nr_attr(audio_output_device, &user_ao_nr_attr);
		ak_ao_set_aslc_attr(audio_output_device, &user_ao_aslc_attr);
		ak_ao_set_eq_attr(audio_output_device, &user_ao_eq_attr);
		ak_ao_enable_eq(audio_output_device, 1);
		ak_ao_set_gain(audio_output_device, USER_AO_GAIN);
		ak_ao_set_volume(audio_output_device, USER_AO_VOLUME);
		audio_speak_enable();
		return;
	}

	curr_audio_param_is_tuya = true;
	struct ak_audio_nr_attr user_ao_nr_attr = {TUYA_USER_AO_NOISE_SUP_DB, 0, ENABLE};
	struct ak_audio_aslc_attr user_ao_aslc_attr = {TUYA_USER_AO_LIMIT, TUYA_USER_AO_DB, ENABLE};
	struct ak_audio_eq_attr user_ao_eq_attr = {
		0,
		10,
		{60, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{307, 716, 716, 716, 716, 716, 716, 716, 716, 716},
		{TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
	ak_ao_set_nr_attr(audio_output_device, &user_ao_nr_attr);
	ak_ao_set_aslc_attr(audio_output_device, &user_ao_aslc_attr);
	ak_ao_set_eq_attr(audio_output_device, &user_ao_eq_attr);
	ak_ao_enable_eq(audio_output_device, 1);
	ak_ao_set_gain(audio_output_device, TUYA_USER_AO_GAIN);
	ak_ao_set_volume(audio_output_device, TUYA_USER_AO_VOLUME);
	amp_turn_on();
}