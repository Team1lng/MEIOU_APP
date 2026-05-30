#include"audio_play.h"
#include"queue.h"
#include"ak_thread.h"
#include"stdio.h"
#include"ak_common.h"
#include"string.h"
#include"app_common.h"
#include"network_common.h"
#include"fcntl.h"
#include"audio_output.h"
#include"mad.h"
#include"ak_mem.h"

#include<unistd.h>

#define PCM_READ_LEN 1024 * 2
#define AUDIO_READ_MAX 128*1024

#define MP3_TYPE ".mp3"
#define PCM_TYPE ".pcm"

#define DOORBELL_ETC_PATH	"/etc/config/cbin/"
#define DOORBELL_APP_PATH	"/app/cbin/"
#if 0
#define DOORBELL_UNLOCK "/etc/config/cbin/unlock_16k.mp3"
#define DOORBELL_MESSAGE_CH "/etc/config/cbin/liuyan_ch.mp3"
#define DOORBELL_MESSAGE_EN "/etc/config/cbin/liuyan_en.mp3"
#define DOORBELL_MESSAGE_GER "/etc/config/cbin/liuyan_ger.mp3"
#define DOORBELL_MESSAGE_HEB "/etc/config/cbin/liuyan_heb.mp3"
#define DOORBELL_MESSAGE_POL "/etc/config/cbin/liuyan_pol.mp3"
#define DOORBELL_MESSAGE_POR "/etc/config/cbin/liuyan_por.mp3"
#define DOORBELL_MESSAGE_SPA "/etc/config/cbin/liuyan_spa.mp3"
#define DOORBELL_MESSAGE_FRE "/etc/config/cbin/liuyan_fre.mp3"
#define DOORBELL_MESSAGE_JAP "/etc/config/cbin/liuyan_jap.mp3"
#define DOORBELL_UNLOCK_CH "/etc/config/cbin/lock_ch.mp3"
#define DOORBELL_UNLOCK_EN "/etc/config/cbin/lock_en.mp3"
#define DOORBELL_UNLOCK_GER "/etc/config/cbin/lock_ger.mp3"
#define DOORBELL_UNLOCK_HEB "/etc/config/cbin/lock_heb.mp3"
#define DOORBELL_UNLOCK_POL "/etc/config/cbin/lock_pol.mp3"
#define	DOORBELL_UNLOCK_POR "/etc/config/cbin/lock_por.mp3"
#define	DOORBELL_UNLOCK_SPA "/etc/config/cbin/lock_spa.mp3"
#define	DOORBELL_UNLOCK_FRE "/etc/config/cbin/lock_fre.mp3"
#define	DOORBELL_UNLOCK_JAP "/etc/config/cbin/lock_jap.mp3"
#define DOORBELL_CALL_BUSY "/etc/config/cbin/knock.mp3"
#define DOORBELL_KEYBOARD_BI1 "/etc/config/cbin/bi1.mp3"
#define DOORBELL_KEYBOARD_BI2 "/etc/config/cbin/bi2.mp3"
#define DOORBELL_KEYBOARD_BI3 "/etc/config/cbin/bi3.mp3"
#define DOORBELL_KEYBOARD_BI4 "/etc/config/cbin/bi4.mp3"
#define DOORBELL_KEYBOARD_LONG_BI "/etc/config/cbin/long_bi.mp3"
#define DOORBELL_KEYBOARD_DIO "/etc/config/cbin/dio.mp3"
#else
#define DOORBELL_UNLOCK "unlock_16k"
#define DOORBELL_MESSAGE_CH "liuyan_ch"
#define DOORBELL_MESSAGE_EN "liuyan_en"
#define DOORBELL_MESSAGE_GER "liuyan_ger"
#define DOORBELL_MESSAGE_HEB "liuyan_heb"
#define DOORBELL_MESSAGE_POL "liuyan_pol"
#define DOORBELL_MESSAGE_POR "liuyan_por"
#define DOORBELL_MESSAGE_SPA "liuyan_spa"
#define DOORBELL_MESSAGE_FRE "liuyan_fre"
#define DOORBELL_MESSAGE_JAP "liuyan_jap"
#define DOORBELL_MESSAGE_ITA "liuyan_ita"
#define DOORBELL_UNLOCK_CH "lock_ch"
#define DOORBELL_UNLOCK_EN "lock_en"
#define DOORBELL_UNLOCK_GER "lock_ger"
#define DOORBELL_UNLOCK_HEB "lock_heb"
#define DOORBELL_UNLOCK_POL "lock_pol"
#define	DOORBELL_UNLOCK_POR "lock_por"
#define	DOORBELL_UNLOCK_SPA "lock_spa"
#define	DOORBELL_UNLOCK_FRE "lock_fre"
#define	DOORBELL_UNLOCK_JAP "lock_jap"
#define	DOORBELL_UNLOCK_ITA "lock_ita"
#define DOORBELL_CALL_BUSY "knock"
#define DOORBELL_KEYBOARD_BI1 "bi1"
#define DOORBELL_KEYBOARD_BI2 "bi2"
#define DOORBELL_KEYBOARD_BI3 "bi3"
#define DOORBELL_KEYBOARD_BI4 "bi4"
#define DOORBELL_KEYBOARD_LONG_BI "long_bi"
#define DOORBELL_KEYBOARD_DIO "dio"
#endif

static  char *ring_group[] = {
	DOORBELL_UNLOCK,
	DOORBELL_MESSAGE_EN,
	DOORBELL_MESSAGE_CH,
	DOORBELL_MESSAGE_GER,
	DOORBELL_MESSAGE_HEB,
	DOORBELL_MESSAGE_POL,
	DOORBELL_MESSAGE_POR,
	DOORBELL_MESSAGE_SPA,
	DOORBELL_MESSAGE_FRE,
	DOORBELL_MESSAGE_JAP,
	DOORBELL_MESSAGE_ITA,
	DOORBELL_UNLOCK_EN,
	DOORBELL_UNLOCK_CH,
	DOORBELL_UNLOCK_GER,
	DOORBELL_UNLOCK_HEB,
	DOORBELL_UNLOCK_POL,
	DOORBELL_UNLOCK_POR,
	DOORBELL_UNLOCK_SPA,
	DOORBELL_UNLOCK_FRE,
	DOORBELL_UNLOCK_JAP,
	DOORBELL_UNLOCK_ITA,
	DOORBELL_CALL_BUSY,
	DOORBELL_KEYBOARD_BI1,
	DOORBELL_KEYBOARD_BI2,
	DOORBELL_KEYBOARD_BI3,
	DOORBELL_KEYBOARD_BI4,
	DOORBELL_KEYBOARD_LONG_BI,
	DOORBELL_KEYBOARD_DIO};


#define AUDIO_PLAY_QUEUE_MAX 8

typedef struct
{
	FILE* fp;

	FILE* temp_fp;

	bool ao_open_init;

	int vol;

	bool send_net;

	unsigned char *start;

	unsigned long length;

	int cache_len;
	
	unsigned char cache[1152*3];
} mp3_mad;

typedef struct
{
	void *prev;
	void *next;

	audio_info msg;
} audio_play_queue;
static audio_play_queue audio_play_buffer[AUDIO_PLAY_QUEUE_MAX];
static queue_s audio_play_queue_free;
static queue_s audio_play_queue_head;
static ak_mutex_t audio_play_queue_free_mutex;
static ak_mutex_t audio_play_queue_head_mutex;

static bool audio_play_status = false;
static bool audio_play_stop = false;
static bool audio_play_puase = false;
int music_cur_played_frame_num = 0;//当前已播放的帧数

static audio_play_queue *audio_play_queue_node_new(void)
{
	audio_play_queue *node = NULL;
	ak_thread_mutex_lock(&audio_play_queue_free_mutex);
	if (queue_empty(&audio_play_queue_free) == 0)
	{
		node = (audio_play_queue *)queue_delete_next(&audio_play_queue_free);
	}
	ak_thread_mutex_unlock(&audio_play_queue_free_mutex);
	return node;
}

static void audio_play_queue_node_del(audio_play_queue *node)
{
	if (node != NULL)
	{
		ak_thread_mutex_lock(&audio_play_queue_free_mutex);
		node->msg.end = NULL;
		node->msg.start = NULL;
		queue_insert((queue_s *)node, &audio_play_queue_free);
		ak_thread_mutex_unlock(&audio_play_queue_free_mutex);
	}
}

static audio_play_queue *audio_play_queue_empty(void)
{
	if (queue_empty(&audio_play_queue_head))
	{
		return NULL;
	}

	audio_play_queue *node = NULL;
	ak_thread_mutex_lock(&audio_play_queue_head_mutex);
	while (!queue_empty(&audio_play_queue_head))
	{
		if (node != NULL)
		{
			audio_play_queue_node_del(node);
		}
		node = (audio_play_queue *)queue_delete_next(&audio_play_queue_head);
	}
	ak_thread_mutex_unlock(&audio_play_queue_head_mutex);
	return node;
}

static void audio_play_finish_wait(void)
{
	// printf("%s =====================>>>start\n\r",__func__);
	unsigned int timeout = 100;
	unsigned int count = 0;
	while (audio_play_stop == false || --timeout > 1)
	{
		count++;
		if (count > 10)
		{
			count = 0;
			if (ao_play_finish())
			{
				// printf("audio_output_buffer_get total :%d            free:%d\n\r ",total,free);
				break;
			}
		}
		ak_sleep_ms(1);
	}
	// printf("%s =====================>>>end\n\r",__func__);
}

static bool audio_play_pcm(const audio_info *info)
{
	if (info->start != NULL)
	{
		// ring_play_event_push((unsigned long)info->start, 1);
	}

	{
		FILE* fp = fopen(info->file_path, "r");
		if (fp == NULL)
		{
			return false;
		}
		unsigned char data[PCM_READ_LEN] = {0};
		int data_len = 0;

		audio_output_volume_set(info->volume);

		while ((data_len = fread(data, sizeof(char),PCM_READ_LEN,fp)) > 0)
		{
			audio_output_write(data, data_len);
            ak_sleep_ms(1);
		}
		fclose(fp);
	}
	audio_play_finish_wait();
	return true;
}

static enum mad_flow mp3_input(void *data, struct mad_stream *stream)
{
	while (audio_play_puase)
	{
		if (audio_play_stop == true)
		{
			audio_play_puase = false;
			return MAD_FLOW_STOP;
		}
		ak_sleep_ms(1);
	}
	mp3_mad *buffer = data;
	if(stream->next_frame != NULL)
	{
		buffer->length = &buffer->start[buffer->length] - stream->next_frame;
		memmove(buffer->start,stream->next_frame,buffer->length);
	}
	int	read_len =fread(&buffer->start[buffer->length],sizeof(char),AUDIO_READ_MAX - buffer->length,buffer->fp);
	if (read_len  <= 0)
	{
		return MAD_FLOW_STOP;
	}
	buffer->length += read_len;
	mad_stream_buffer(stream, buffer->start, buffer->length);
	return MAD_FLOW_CONTINUE;
}

static inline signed int mp3_scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}
static enum mad_flow mp3_output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
	while (audio_play_puase)
	{
		if (audio_play_stop == true)
		{
			audio_play_puase = false;
			return MAD_FLOW_STOP;
		}
		ak_sleep_ms(1);
	}
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	mp3_mad *buffer = data;
	if (audio_play_stop == true)
	{
		return MAD_FLOW_STOP;
	}
	pcm->channels = 1;
	nchannels = pcm->channels;
	nsamples = pcm->length;
	// printf("->duration.fraction:%ld\n",header->duration.fraction);
	left_ch = pcm->samples[0];
	right_ch = pcm->samples[1];

	// if(buffer->ao_open_init == false)
	// {
	// 	audio_output_open(pcm->channels, pcm->samplerate);
	// 	buffer->ao_open_init  = true;
	// }
	// #define MP3_OUTPUT_FRAME_SIZE (1152*3)//4096//1180//

	while (nsamples--)
	{
		signed int sample;

		/* output sample(s) in 16-bit signed little-endian PCM */
		if (audio_play_stop == true)
		{
			return MAD_FLOW_STOP;
		}

		sample = mp3_scale(*left_ch++);
		buffer->cache[buffer->cache_len++] = (sample >> 0) & 0xff;
		buffer->cache[buffer->cache_len++] = (sample >> 8) & 0xff;

		if (nchannels == 2)
		{
			sample = mp3_scale(*right_ch++);
			buffer->cache[buffer->cache_len++] = (sample >> 0) & 0xff;
			buffer->cache[buffer->cache_len++] = (sample >> 8) & 0xff;
		}

		/* output sample(s) in 16-bit signed little-endian PCM */
		if(audio_play_stop == true)
		{
			return MAD_FLOW_STOP;
		}
	}

	if (buffer->cache_len > (sizeof(buffer->cache)/3*2))
	{
		music_cur_played_frame_num++;
		while (get_ao_buf_remain_len() > 1152*8)
		{
			ak_sleep_ms(10);
		}
		
		// if(buffer->cache_len >)
		// memcpy(&buffer->cache[buffer->cache_len],buffer->cache,buffer->cache_len);
		// buffer->cache_len += buffer->cache_len;
		// if(3072 > buffer->cache_len)
		audio_output_write((unsigned char *)buffer->cache, buffer->cache_len);
		buffer->cache_len = 0;
		#ifdef RECORD_PCM
		if(fb)
		{
			fwrite(buffer->cache,buffer->cache_len, 1, fb );
		}
		#endif
	}


	if (audio_play_stop == true)
	{
		return MAD_FLOW_STOP;
	}
	return MAD_FLOW_CONTINUE;
}

static enum mad_flow mp3_error(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
	mp3_mad *buffer = data;
	if (buffer->cache_len > 0)
	{
		while (get_ao_buf_remain_len() > 1152*8)
		{
			ak_sleep_ms(10);
		}
		audio_output_write((unsigned char *)buffer->cache, buffer->cache_len);
		buffer->cache_len = 0;
	}
	return MAD_FLOW_CONTINUE;
}

static void audio_play_mp3(const audio_info *info)
{
	mp3_mad mp3_buffer;
	struct mad_decoder decodec;
	
	ak_ao_cancel(get_ao_hand());
	ak_ao_restart(get_ao_hand());

	audio_output_volume_set(info->volume);
	mp3_buffer.ao_open_init = false;
	mp3_buffer.vol = info->volume;

	music_cur_played_frame_num = 0;
	printf("%s========================>%s-%d\n",__func__,info->file_path,info->volume);
	// unsigned char buffer[AUDIO_READ_MAX];
	unsigned char *buffer = ak_mem_alloc(MODULE_ID_AO,AUDIO_READ_MAX);

	{
		mp3_buffer.fp = fopen(info->file_path, "r");
		if (mp3_buffer.fp == NULL)
		{
			ak_mem_free(buffer);
			return;
		}
		mp3_buffer.start = buffer;
		mp3_buffer.length = 0;
		mp3_buffer.cache_len = 0;
	}

	// mp3_buffer.temp_fd = open("/tmp/indoor_audio.pcm",O_WRONLY|O_CREAT);
	// if(mp3_buffer.temp_fd < 0)
	// {
	// 	printf("write open tmp/indoor_audio.pcm fail\n");
	// }
	// else{
	// 	printf("write open tmp/audio.pcm O_WRONLY|O_CREAT succeed :%d\n",mp3_buffer.temp_fd);
	// }
	#ifdef RECORD_PCM
	if(fb == NULL)
	{
		char file[32] = {0};
		sprintf(file,"/mnt/nfs/%d.pcm",audio_output_volume_get());
		fb = fopen(file,"w");
		if(fb == NULL)
		{
			printf("open %s//////////\n",file);
		}
	}
	#endif

	mad_decoder_init(&decodec, &mp3_buffer, mp3_input, 0, 0, mp3_output, mp3_error, 0);
	if (info->start != NULL)
	{
		// ring_play_event_push((unsigned long)info->start, 1);
	}

	mad_decoder_run(&decodec, MAD_DECODER_MODE_SYNC);

	audio_play_finish_wait();
	mad_decoder_finish(&decodec);
	music_cur_played_frame_num = 0;
	// close(mp3_buffer.temp_fd);
	// system("sync");

	#ifdef RECORD_PCM
	if(fb)
	{
		fclose(fb);
		fb = NULL;		
	}
	#endif

	ak_mem_free(buffer);
	if (mp3_buffer.fp != NULL)
	{
		fclose(mp3_buffer.fp);
	}

}

static void audio_play_decode_start(const audio_info *info)
{
	// printf("%s==============>>\n",__func__);
	if (info->type == AK_AUDIO_TYPE_PCM)
	{
		audio_play_pcm(info);
	    // printf("%s==============>>%s\n",__func__,info->file_path);
	}
	else
	{
		audio_play_mp3(info);
	    // printf("%s==============>>%s\n",__func__,info->file_path);
	}
}

static void *audio_play_task(void *arg)
{
	audio_play_queue *node = NULL;
	while (1)
	{
		if (node != NULL)
		{
			audio_play_queue_node_del(node);
		}
		node = audio_play_queue_empty();

		if (node != NULL)
		{
			audio_play_stop = false;

			audio_play_status = true;
			audio_play_decode_start(&node->msg);
			if (node->msg.end != NULL)
			{
				// ring_play_event_push((unsigned long)node->msg.end, 2);
			}
			audio_play_status = false;
		}
		else
		{
			ak_sleep_ms(10);
		}
	}

	ak_thread_exit();
	return NULL;
}
static bool audio_play_run = false;
static bool audio_file_path = false;
bool audio_play_init(void)
{
	#ifdef  APP_ATS_OPEN
	return false;
	#endif
	if(audio_play_run)
		return false;

	if (get_mtd_num() > 8) {
		printf("The /app directory exists.\n");
		audio_file_path = true;
	} else {
		printf("The /app directory does not exist.\n");
		audio_file_path = false;
	}

	audio_play_run = true;
    // system("rm /etc/config/cbin/*.pcm -rf");
	queue_initialize(&audio_play_queue_free);
	queue_initialize(&audio_play_queue_head);
	ak_thread_mutex_init(&audio_play_queue_free_mutex, NULL);
	ak_thread_mutex_init(&audio_play_queue_head_mutex, NULL);
	// extern int ak_ats_start(unsigned int port);
	// ak_ats_start(8012);
	for (int i = 0; i < AUDIO_PLAY_QUEUE_MAX; i++)
	{
		queue_insert((queue_s *)&audio_play_buffer[i], &audio_play_queue_free);
	}

	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, audio_play_task, NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);
	return true;
}


bool audio_play(audio_info *info)
{
	audio_play_stop = true;
	audio_play_queue *node = audio_play_queue_node_new();
	if (node == NULL)
	{
		printf("audio play queue full \n");
		return false;
	}

	memcpy(&(node->msg), info, sizeof(audio_info));
	ak_thread_mutex_lock(&audio_play_queue_head_mutex);
	queue_insert((queue_s *)node, &audio_play_queue_head);
	ak_thread_mutex_unlock(&audio_play_queue_head_mutex);
	return true;
}

void play_doorbell(int index,int volume)
{
	printf("audio_play file:index:%s%s   volume:%d,audio_play_run:%d,net_talk_state_get():%d\n",audio_file_path ? DOORBELL_APP_PATH:DOORBELL_ETC_PATH,ring_group[index],volume,audio_play_run,net_talk_state_get());
	if(audio_play_run ==  false/*  ||  net_talk_state_get() */)
		return;
    
	enum ak_audio_type type;

	static char file_path[128] = {0};
	sprintf(file_path,"%s%s%s",audio_file_path ? DOORBELL_APP_PATH:DOORBELL_ETC_PATH,ring_group[index],PCM_TYPE);
	if(access(file_path,F_OK) != 0)
	{
		memset(file_path,0,sizeof(file_path));
		sprintf(file_path,"%s%s%s",audio_file_path ? DOORBELL_APP_PATH:DOORBELL_ETC_PATH,ring_group[index],MP3_TYPE);
		if(access(file_path,F_OK) != 0)
		{
			return;
		}
		else
		{
			type = AK_AUDIO_TYPE_MP3;
		}
	}
	else
	{
		type = AK_AUDIO_TYPE_PCM;
	}


	int ring_total = sizeof(ring_group)/sizeof(char*);

    if(index < ring_total)
    {
        audio_info info = 
        {
            .file_path =file_path,
            .ch = AUDIO_CHANNEL_MONO,
            .rate = AK_AUDIO_SAMPLE_RATE_16000,
            .type = type,
            .volume = volume*5 + 46,
            .start = NULL,
            .end = NULL
        };
        audio_play(&info);        
    }
}

bool is_audio_play_ing(void)
{
	return audio_play_status;
}


bool audio_play_stop_set(void)
{
	printf("%s================%d\n",__func__,__LINE__);
	audio_play_queue *node;
	while ((node = audio_play_queue_empty()))
	{
		audio_play_queue_node_del(node);
	}
	while (audio_play_status == true)
	{
		audio_play_stop = true;
		ak_sleep_ms(1);
	}
	// printf("%s================%d\n",__func__,__LINE__);
	return true;
}
