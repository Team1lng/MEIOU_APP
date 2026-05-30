#include "video_input.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <memory.h>
#include <unistd.h>
#include <stdbool.h>

#include "audio_output.h"

#include "ak_common.h"
#include "ak_venc.h"
#include "ak_vi.h"
#include "ak_venc.h"
#include "ak_mem.h"
#include "ak_vpss.h"
#include "ak_common_video.h"

#include "fcntl.h"

#include "ak_ps_ir.h"

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int video_encode_main_device = -1;
static int video_encode_sub_device = -1;
static int *video_encode_curr_device = &video_encode_main_device;

#define VIDEO_CAPTURE_DEV VIDEO_DEV0
#define VIDEO_CAPTURE_MAIN_CH VIDEO_CHN0
#define VIDEO_CAPTURE_SUB_CH VIDEO_CHN1
#define VIDEO_CAPTURE_CURR_CH (video_encode_curr_device == &video_encode_main_device ? VIDEO_CAPTURE_MAIN_CH : VIDEO_CAPTURE_SUB_CH)

#define VIDEO_PIXEL_MAIN_WIDTH 1920	 // 1024
#define VIDEO_PIXEL_MIAN_HEIGHT 1080 // 600

#define VIDEO_PIXEL_SUB_WIDTH 1280 // 1024
#define VIDEO_PIXEL_SUB_HEIGHT 720 // 600

#define VIDEO_SNESOR_MODLE_KO "/usr/modules/sensor_gc2063.ko" //"/usr/modules/sensor_f22_f23_f28_f35_f37.ko"//sensor_gc2063.ko"
#define VIDEO_ISP_MODLE_KO "/usr/modules/ak_isp.ko"

static pthread_mutex_t video_input_mutex;
static pthread_t video_input_pthread_id;

/***
** 视频输出使能判断标志
***/
static bool is_video_input_enable = false;

/***
**	暂停采集
***/
static bool is_video_input_pause = false;

/***
**	音频输出队列创建
***/
static int video_input_queue_head = -1;

/***
**	采集队列计数器
***/
#define VIDEO_INPUT_QUEUE_MAX 10
static int video_input_queue_count = 0;

void video_encode_ch_change(bool sub_ch)
{
	pthread_mutex_lock(&video_input_mutex);
	if (sub_ch && video_encode_curr_device != &video_encode_sub_device)
	{
		printf("[VIDEO CH CHANGE] : > > > SUB_CH\n\r");
		video_encode_curr_device = &video_encode_sub_device;
	}
	else if (!sub_ch && video_encode_curr_device != &video_encode_main_device)
	{
		printf("[VIDEO CH CHANGE] : > > > MAIN_CH\n\r");
		video_encode_curr_device = &video_encode_main_device;
	}
	pthread_mutex_unlock(&video_input_mutex);
}

static void video_input_queue_create(void)
{
	key_t key = ftok("/tmp", 03);
	video_input_queue_head = msgget(key, 0666 | IPC_CREAT);
	stream_data data;
	while (msgrcv(video_input_queue_head, (void *)&data, sizeof(stream_data), 0, IPC_NOWAIT) > 0)
		;
}

static bool video_input_capture_open(void)
{
	/*****打开VI	 OPEN 设备*****/
	if (ak_vi_open(VIDEO_CAPTURE_DEV) != 0)
	{
		printf("vi device %d open failled \n", VIDEO_CAPTURE_DEV);
		return false;
	}

	char conf_path[128];
	if (access("/etc/config/isp_gc2063_mipi_2lane_h3b_101101.conf", F_OK) == 0)
		sprintf(conf_path, "%s", "/etc/config/isp_gc2063_mipi_2lane_h3b_101101.conf");
	else if (access("/app/isp_gc2063_mipi_2lane_h3b_101101.conf", F_OK) == 0)
		sprintf(conf_path, "%s", "/app/isp_gc2063_mipi_2lane_h3b_101101.conf");
	/***** 加载isp 配置文件 *****/								  /// etc/isp_f37_mipi_1lane_h3b.conf
	if (ak_vi_load_sensor_cfg(VIDEO_CAPTURE_DEV, conf_path) != 0) // isp_gc2063_mipi_2lane_h3b isp_f37_mipi_1lane_h3b      2022-10-11 : h3b_gc2063_mipi2lane_sample_11_20220715.conf
	{
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}

	/***** 获取sensor 的分辨率 *****/
	RECTANGLE_S res;
	if (ak_vi_get_sensor_resolution(VIDEO_CAPTURE_DEV, &res) != 0)
	{
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}

	printf("get dev res w:[%d] h:[%d] \n", res.width, res.height);

	/***** 设置sensor 支持最大的分辨率 *****/
	VI_DEV_ATTR dev_attr;
	memset(&dev_attr, 0, sizeof(VI_DEV_ATTR));
	dev_attr.dev_id = VIDEO_CAPTURE_DEV;
	dev_attr.crop.left = 0;
	dev_attr.crop.top = 0;
	dev_attr.crop.width = res.width;
	dev_attr.crop.height = res.height;

	dev_attr.max_width = res.width;
	dev_attr.max_height = res.height;
	dev_attr.sub_max_width = VIDEO_PIXEL_SUB_WIDTH;	  // 320;//640;
	dev_attr.sub_max_height = VIDEO_PIXEL_SUB_HEIGHT; // 240;//360;
	if (ak_vi_set_dev_attr(VIDEO_CAPTURE_DEV, &dev_attr) != 0)
	{
		printf("vi device set attribute faild \n");
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}

	/***** 设置主通道属性 *****/
	VI_CHN_ATTR main_chn_attr =
		{
			0};
	memset(&main_chn_attr, 0, sizeof(VI_CHN_ATTR));
	main_chn_attr.chn_id = VIDEO_CAPTURE_MAIN_CH;
	main_chn_attr.res.width = VIDEO_PIXEL_MAIN_WIDTH;
	main_chn_attr.res.height = VIDEO_PIXEL_MIAN_HEIGHT;
	main_chn_attr.frame_depth = 3;

	/***** 禁止帧率控制 *****/
	main_chn_attr.frame_rate = 0;

	if (ak_vi_set_chn_attr(main_chn_attr.chn_id, &main_chn_attr) != 0)
	{
		printf("vi device set main channel attribute failed \n");
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}

	/***** 设置子通道属性 *****/
	VI_CHN_ATTR sub_chn_attr =
		{
			0};
	memset(&sub_chn_attr, 0, sizeof(VI_CHN_ATTR));
	sub_chn_attr.chn_id = VIDEO_CAPTURE_SUB_CH;
	sub_chn_attr.res.width = dev_attr.sub_max_width;
	sub_chn_attr.res.height = dev_attr.sub_max_height;
	sub_chn_attr.frame_depth = 2;

	/***** 禁止帧率控制 *****/
	sub_chn_attr.frame_rate = 0;

	if (ak_vi_set_chn_attr(sub_chn_attr.chn_id, &sub_chn_attr) != 0)
	{
		printf("vi device set sub channel attribute failed \n");
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}

	/***** 使能视频采集设备 *****/
	if (ak_vi_enable_dev(VIDEO_CAPTURE_DEV) != 0)
	{
		printf("vi device enable failed \n");
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}

	/***** 使能主通道 *****/
	if (ak_vi_enable_chn(main_chn_attr.chn_id) != 0)
	{
		printf("vi enable mian channel failed \n");
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}

#if 1
	/***** 使能子通道 *****/
	if (ak_vi_enable_chn(sub_chn_attr.chn_id) != 0)
	{
		printf("vi enable sub channel failed \n");
		ak_vi_close(VIDEO_CAPTURE_DEV);
		return false;
	}
#endif
	return true;
}

static bool video_input_encode_open(int *handle_id, int w, int h)
{
	/***** 设置编码参数 *****/
	struct venc_param ve_param;

	/***** 根据参数设置编码分辨率 *****/
	ve_param.width = w;
	ve_param.height = h;

	/***** 暂时强制设置25fps *****/
	ve_param.fps = 25;
	// 设置为1024 / 2048
	ve_param.goplen = ve_param.fps * 2;
	ve_param.target_kbps = &video_encode_main_device == handle_id ? 1024 * 2 : 512; //* 4096 */2048/* 512 */; // 6144//;
	// 2048;			     // 1024k bps
	ve_param.max_kbps = &video_encode_main_device == handle_id ? 4096 : 1024; ///* 6144 */4096/* 1024 */;
	// 8192;
	//  4096; // max 2048kbps
	/***
	 * CBR 是一种恒定比特率
	 * VBR 是一种动态比特率
	 **/
	ve_param.br_mode = BR_MODE_CBR;									  // BR_MODE_VBR; 				//br mode
	ve_param.minqp = 25;											  // qp set
	ve_param.maxqp = 50;											  // qp max value
	ve_param.initqp = /* (ve_param.minqp + ve_param.maxqp) / 2 */ 50; // qp value
	ve_param.jpeg_qlevel = JPEG_QLEVEL_DEFAULT;						  // jpeg qlevel
	ve_param.chroma_mode = CHROMA_4_2_0;							  // chroma mode
	ve_param.max_picture_size = 0;									  // 0 means default
	/***
	 *	默认编码质量为30[10,50]
	 **/
	ve_param.enc_level = 20;	 // 30;						//enc level
	ve_param.smart_mode = 0;	 // smart mode set
	ve_param.smart_goplen = 100; // smart mode value
	/***
	 *	smart_quality 默认为50
	 *
	 **/
	ve_param.smart_quality = 50;	 // quality
	ve_param.smart_static_value = 0; // value

	/***** 设置编码类型 *****/
	ve_param.enc_out_type = H264_ENC_TYPE;
	ve_param.profile = PROFILE_MAIN;

	int ret = ak_venc_open(&ve_param, handle_id);

	ak_venc_request_idr(*handle_id);

	return ret ? false : true;
}

static void video_input_device_open(void)
{
	video_input_capture_open();

	video_input_encode_open(&video_encode_main_device, VIDEO_PIXEL_MAIN_WIDTH, VIDEO_PIXEL_MIAN_HEIGHT);
	video_input_encode_open(&video_encode_sub_device, VIDEO_PIXEL_SUB_WIDTH, VIDEO_PIXEL_SUB_HEIGHT);
	ak_vi_change_chn_fps(VIDEO_CAPTURE_MAIN_CH, 25);
	ak_vi_change_chn_fps(VIDEO_CAPTURE_SUB_CH, 25);

	ak_vpss_effect_set(VIDEO_CAPTURE_DEV, VPSS_EFFECT_BRIGHTNESS, -5); // 亮度
	ak_vpss_effect_set(VIDEO_CAPTURE_DEV, VPSS_EFFECT_CONTRAST, 5);	   // 对比度

	// int value;
	// ak_vpss_effect_get(VIDEO_CAPTURE_DEV, VPSS_EFFECT_CONTRAST, &value);
	// printf("==================================================>>> 对比度 : [%d] \n", value);

	// ak_vpss_effect_get(VIDEO_CAPTURE_DEV, VPSS_EFFECT_BRIGHTNESS, &value);
	// printf("==================================================>>> 亮度 : [%d] \n", value);

	// ak_vpss_effect_set(VIDEO_CAPTURE_DEV, VPSS_EFFECT_SATURATION, 12);//饱和
	// ak_vpss_effect_set(VIDEO_CAPTURE_DEV, VPSS_EFFECT_HUE, 15);//色调

	// ak_vpss_effect_set(VIDEO_CAPTURE_DEV, VPSS_EFFECT_SHARP, -40);//锐度
	// ak_vpss_effect_set(VIDEO_CAPTURE_DEV, VPSS_EFFECT_WDR, 0);//宽动态
	// 调风格会出现浮点异常 死机
	// ak_vpss_effect_set(VIDEO_CAPTURE_DEV, VPSS_STYLE_ID, 1);//风格
}

#if 0
static bool video_input_capture_close(void)
{
	ak_vi_disable_chn(VIDEO_CAPTURE_MAIN_CH);
	ak_vi_disable_chn(VIDEO_CAPTURE_SUB_CH);
	ak_vi_disable_dev(VIDEO_CAPTURE_DEV);
	ak_vi_close(VIDEO_CAPTURE_DEV);
	return true;
}

static bool video_input_encode_close(void)
{
	if (video_encode_main_device == -1 || video_encode_sub_device == -1)
	{
		return false;
	}

	ak_venc_close(video_encode_main_device);
	video_encode_main_device = -1;
	ak_venc_close(video_encode_sub_device);
	video_encode_sub_device = -1;
	return true;
}

#endif

#if 0
static void video_input_device_close(void)
{
	video_input_encode_close();
	video_input_capture_close();
}
#endif
static void *video_input_task(void *arg)
{
	extern bool is_network_video_i_frame_request(void);
	extern void set_network_i_frame_request_param(bool param);
	video_input_device_open();
	struct video_input_frame vi_frame; // {0};
	struct video_stream stream;
	stream_data vdeo_data;
	char curr_vi_ch = VIDEO_CAPTURE_CURR_CH;
	enum video_frame_type swicth_ch_first_frame = FRAME_TYPE_P;
	// int avi_size = 0;

	// extern unsigned long long os_get_ms(void);
	// unsigned long long x = os_get_ms();
	while (1)
	{
		pthread_mutex_lock(&video_input_mutex);
		// if ((is_video_input_enable == false) && ((video_encode_main_device != -1) || (video_encode_sub_device != -1)))
		// {
		// 	printf("VIDEO_INPUT_DEVICE_CLOSE ================>\n\r");
		// 	video_input_device_close();
		// }
		// else if ((is_video_input_enable == true) && ((video_encode_main_device == -1) || (video_encode_sub_device == -1)))
		// {
		// 	printf("VIDEO_INPUT_DEVICE_OPEN ================>\n\r");
		// 	video_input_device_open();
		// }

		/*
		static int aaa = 0;
		if(aaa == 0){
			extern void motion_init(void);
			motion_init();
			aaa = 1;
		}
		*/
		//	printf("==============>>>%d\n\n",*video_encode_curr_device);
		if (is_video_input_enable && (*video_encode_curr_device != -1))
		{

			if (is_network_video_i_frame_request())
			{
				set_network_i_frame_request_param(false);
				//	usleep(40 * 1000);
				ak_venc_request_idr(*video_encode_curr_device);
			}

			if ((is_video_input_pause == false) && (video_input_queue_count < VIDEO_INPUT_QUEUE_MAX))
			{
				// printf("==============>>>%d\n\n==============》》》%d\n\n",is_video_input_pause,video_input_queue_count);
				memset(&vi_frame, 0, sizeof(struct video_input_frame));

				// x = os_get_ms();
				if (ak_vi_get_frame(VIDEO_CAPTURE_CURR_CH, &vi_frame) == 0)
				{
					if (curr_vi_ch != VIDEO_CAPTURE_CURR_CH)
					{
						curr_vi_ch = VIDEO_CAPTURE_CURR_CH;
						swicth_ch_first_frame = false;
					}
					// printf("ak_vi_get_frame 1  ms:%llu\n",os_get_ms() - x);
					// x = os_get_ms();
					memset(&stream, 0, sizeof(struct video_stream));
					if (ak_venc_encode_frame(*video_encode_curr_device, vi_frame.vi_frame.data, vi_frame.vi_frame.len, NULL, &stream) == 0)
					{

						// printf("ak_venc_encode_frame 1  ms:%lld\n",os_get_ms() - x);
						// x = os_get_ms();
						if (swicth_ch_first_frame != FRAME_TYPE_I)
						{
							swicth_ch_first_frame = stream.frame_type;
						}

						if (swicth_ch_first_frame == FRAME_TYPE_I)
						{
							vdeo_data.info.data = ak_mem_alloc(MODULE_ID_VI, stream.len);
							memcpy(vdeo_data.info.data, stream.data, stream.len);
							vdeo_data.info.size = stream.len;
							vdeo_data.info.ch = VIDEO_CAPTURE_CURR_CH;
							vdeo_data.type = sizeof(stream_info);
							msgsnd(video_input_queue_head, &vdeo_data, sizeof(stream_info), 0);
							video_input_queue_count++;
							pthread_cond_signal(&cond);
						}
						// printf("msgsnd 1  ms:%lld\n\n\n",os_get_ms() - x);

						// static int fd = -1;
						// char is_i_fream = ((stream.data[4] & 0x1f)  == 5 || (stream.data[4] & 0x1f)  == 7 || (stream.data[4] & 0x1f)  == 8) ? 1 : 0 ;
						// if(fd == -1 && is_i_fream && video_input_queue_count < 100000 && motion_detect_result_get())
						// {
						// 	printf("open 2M_1080.h264 file!!!\n\r");
						// 	fd = open("/tmp/2M_1080.h264",O_CREAT | O_WRONLY);
						// }

						// if(((stream.len + avi_size) <  1024*1024 *3) && (fd != -1))
						// {VIDEO_CAPTURE_CURR_CH
						// 	close(fd);
						// 	system("sync");
						// 	fd = -2;
						// }
						ak_venc_release_stream(*video_encode_curr_device, &stream);
					}
					else
					{
						// printf("ak_venc_encode_frame fail !!!!................................................\n");
					}
					ak_vi_release_frame(VIDEO_CAPTURE_CURR_CH, &vi_frame);
				}
				else
				{
					printf("ak_vi_get_frame fail !!!!................................................\n");
				}
			}
		}
		pthread_mutex_unlock(&video_input_mutex);
		usleep(10 * 1000);
	}
	return NULL;
}

/***
**  date:2022/04/19
**  author:刘炼
**  初始化视频采集设备
***/
bool video_input_device_init(void)
{
	/***** 只能初始化一次 *****/
	static bool inited = false;
	if (inited == true)
	{
		return false;
	}

	inited = true;

	video_input_queue_create();

	system("insmod " VIDEO_ISP_MODLE_KO);
	system("insmod " VIDEO_SNESOR_MODLE_KO " SENSOR_I2C_ADDR=0x37");
	pthread_mutex_init(&video_input_mutex, NULL);
	pthread_create(&video_input_pthread_id, NULL, video_input_task, NULL);
	return true;
}

/***
**	date:2022/04/19
**	author:刘炼
**	打开视频采集设备
***/
bool video_input_open(void)
{
	pthread_mutex_lock(&video_input_mutex);
	if (is_video_input_enable == true)
	{
		pthread_mutex_unlock(&video_input_mutex);
		return true;
	}

	is_video_input_pause = false;
	is_video_input_enable = true;
	pthread_mutex_unlock(&video_input_mutex);
	return true;
}

/***
**  date:2022/04/19
**  author:刘炼
**  关闭视频采集设备
***/
bool video_input_close(void)
{
	pthread_mutex_lock(&video_input_mutex);
	if (is_video_input_enable == false)
	{
		pthread_mutex_unlock(&video_input_mutex);
		return false;
	}
	is_video_input_enable = false;
	pthread_mutex_unlock(&video_input_mutex);
	return true;
}

static void vi_cond_timewati(int ms)
{
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_nsec += ms * 1000 * 1000; // 将等待时间转换为纳秒

	// 处理纳秒溢出
	if (timeout.tv_nsec >= 1000000000)
	{
		timeout.tv_sec += 1;
		timeout.tv_nsec -= 1000000000;
	}

	pthread_cond_timedwait(&cond, &video_input_mutex, &timeout);
}
/***
**  date:2022/04/19
**  author:刘炼
**  采集视频数据
***/
bool video_input_read(unsigned char **data, int *size, char *ch)
{
	bool reslut = false;

	pthread_mutex_lock(&video_input_mutex);
	vi_cond_timewati(10);

	if (is_video_input_enable == false)
	{
		pthread_mutex_unlock(&video_input_mutex);
		return false;
	}

	stream_data video_data;
	if (msgrcv(video_input_queue_head, (void *)&video_data, sizeof(stream_data), sizeof(stream_info), IPC_NOWAIT) > 0)
	{
		*data = video_data.info.data;
		*size = video_data.info.size;
		*ch = video_data.info.ch;
		video_input_queue_count--;
		reslut = true;
	}
	pthread_mutex_unlock(&video_input_mutex);
	return reslut;
}

/***
**	date:2022/04/19
**	author:刘炼
**	视频采集暂停/开始
***/
bool video_input_capture_pause(bool yeno)
{
	is_video_input_pause = yeno;
	return true;
}
