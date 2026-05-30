#include "network_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <sys/ioctl.h>
#include <bits/ioctls.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include "ak_thread.h"
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "ak_common.h"
#include "video_input.h"
#include "ak_mem.h"

#define VIDEO_QUEUE_PACKAGE_MAX 64

#define VIDEO_PACKAGE_SIZE_MAX (32 * 1024) // 1500 //10*1024//1510 32

static int network_video_send_eth_id = 0;

static const char video_start_code[4] = {0x00, 0x00, 0x01, 0xfc};

#define VIDEO_IP_ADDRES "255.255.255.255" //"192.168.37.1"//

extern struct sockaddr_ll *network_get_send_addres(void);
extern char *nework_get_package_head(unsigned int type);
extern bool video_decode_push(char, unsigned char *data, int len);
extern bool video_record_data_push(char, unsigned char *data, int len, bool is_video);

// char frame_type; //0:h264,1mjpeg

static bool network_video_send_thread_run = false;
static bool networK_video_send_task_run = false;
static bool network_video_send_ready = false;
static bool network_frame_request = false;
static int video_package_send_fd = -1;
static bool network_video_send_socket_open(void)
{
	if (video_package_send_fd != -1)
	{
		return false;
	}

	printf("==========>>> video send socket %04x <<<==========\n", network_video_send_eth_id);
#if 0
	if((video_package_send_fd = socket(PF_PACKET, SOCK_RAW, htons(network_video_send_eth_id))) < 0)
	{
		printf("create socket error raw_socket_receive_fd\n");
		return false;
	}
#endif

	if ((video_package_send_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("create socket error raw_socket_receive_fd\n");
		return false;
	}

	struct ifreq interface;
	memset(&interface, 0x00, sizeof(interface));
	snprintf(interface.ifr_name, IFNAMSIZ, "%s", NETWORK_NAME);
	if (setsockopt(video_package_send_fd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface, sizeof(interface)) < 0)
	{
		perror("eth1-client:SO_BINDTODEVICEfailed");
		return false;
	}

	int on = 1;
	setsockopt(video_package_send_fd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &on, sizeof(on));

	return true;
}

static bool network_video_send_socket_close(void)
{
	if (video_package_send_fd == -1)
	{
		return false;
	}

	close(video_package_send_fd);
	video_package_send_fd = -1;

	return true;
}

static void network_send_package_video(char *data, int size, long ch,struct sockaddr_in *addr, int len)
{
	static unsigned char *buffer = NULL;
	// static int buffer_size = 0;
	int send_size = 0;
	int remain_size = size;

	/***** 分配音频帧内存 *****/
	if (buffer == NULL) 
	{
		// buffer_size = VIDEO_PACKAGE_SIZE_MAX;
		buffer = ak_mem_alloc(MODULE_ID_AKV_VENC, VIDEO_PACKAGE_SIZE_MAX);
	}

	/***** 获取时间戳 *****/
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long pts = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	/***** 帧序号 *****/
	static unsigned long frame_index = 0;
	frame_index++;
		

	/***** 帧类型 H264:0*****/
	char frame_type = /* (char)ch */ 0;
	// printf("send   ==========================>>> index : %ld \n\r", frame_index );
					// printf(" #####################################>>>VIDEO \n");
	while (remain_size > 0)
	{
		if (send_size == 0)
		{
			memcpy(&buffer[0], video_start_code, 4);
			buffer[4] = (remain_size >> 24) & 0xFF;
			buffer[5] = (remain_size >> 16) & 0xFF;
			buffer[6] = (remain_size >> 8) & 0xFF;
			buffer[7] = remain_size & 0xFF;

			buffer[8] = (pts >> 24) & 0xFF;
			buffer[9] = (pts >> 16) & 0xFF;
			buffer[10] = (pts >> 8) & 0xFF;
			buffer[11] = pts & 0xFF;

			buffer[12] = (frame_index >> 24) & 0xFF;
			buffer[13] = (frame_index >> 16) & 0xFF;
			buffer[14] = (frame_index >> 8) & 0xFF;
			buffer[15] = frame_index & 0xFF;

			buffer[16] = frame_type;

			if (remain_size > (VIDEO_PACKAGE_SIZE_MAX - 17))
			{
				memcpy(&buffer[17], data, VIDEO_PACKAGE_SIZE_MAX - 17);

				if (sendto(video_package_send_fd, buffer, VIDEO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)addr, len) < 0)
				{
					perror(" send to fail \n");
				}
				remain_size -= (VIDEO_PACKAGE_SIZE_MAX - 17);
				send_size += (VIDEO_PACKAGE_SIZE_MAX - 17);

			}
			else
			{
				memcpy(&buffer[17], &data[send_size], remain_size);
				if (sendto(video_package_send_fd, buffer, remain_size + 17, 0, (struct sockaddr *)addr, len) < 0)
				{
					perror(" send to fail \n");
				}
				break;
			}
		}
		usleep(1000);
		if (remain_size > (VIDEO_PACKAGE_SIZE_MAX - 60))
		{
			// memcpy(&buffer[0], &data[send_size], VIDEO_PACKAGE_SIZE_MAX);
			if (sendto(video_package_send_fd, &data[send_size], VIDEO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)addr, len) < 0)
			{
				perror(" send to fail \n");
			}
			remain_size -= (VIDEO_PACKAGE_SIZE_MAX);
			send_size += (VIDEO_PACKAGE_SIZE_MAX);
		}
		else
		{
			// memcpy(&buffer[0], &data[send_size], remain_size);
			if (sendto(video_package_send_fd, &data[send_size], remain_size, 0, (struct sockaddr *)addr, len) < 0)
			{
				perror(" send to fail \n");
			}
			break;
		}
	}
}

static void *network_video_send_package_task(void *arg)
{
	network_video_send_thread_run = true;

	network_video_send_socket_open();

	// video_input_open();

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(VIDEO_IP_ADDRES);
	serv_addr.sin_port = htons(network_video_send_eth_id);
	int addr_len = sizeof(struct sockaddr_in);

	unsigned char *video_data = NULL;
	int video_size = 0;
	char ch = 0;
	while (networK_video_send_task_run == true)
	{
		if (video_input_read(&video_data, &video_size,&ch) == true)
		{
			if (network_video_send_ready == true)
			{
				// int cont = ((video_data[4] & 0x1f)  == 5) ? 1 : 3 ;
				// for(int i = 0;i < cont;i ++)
				// {
					network_send_package_video((char *)video_data, video_size, ch, &serv_addr, addr_len);
					// if(video_size > 320*1024)
						// printf("network_video_receive_package_task  video_size =================>>>%d\n\r",video_size);
	// unsigned long long os_get_ms(void);
	// static unsigned long long frist_count_ms = 0;
	// static int count = 0; 
	// count ++;
	// if(count  == 100)
	// {
	// 	count = 0;
	// 	// printf("push end ms =================>>>%lld\n\r",os_get_ms());
	// 	printf("network_video_receive_package_task  ms =================>>>%lld\n\r",(os_get_ms() - frist_count_ms)/100);
	// 	fflush(stdout);
	// 	frist_count_ms = os_get_ms();
	// }
				// }
			}
			ak_mem_free(video_data);
		}
		// else if(network_video_send_ready)
		// {
		// 	printf("video_input_read fail !!!!................................................\n");
		// }

		// ak_sleep_ms(1);
	}

	video_input_close();

	network_video_send_socket_close();
	network_video_send_thread_run = false;

	printf("===========<<< network video send finish >>>===========\n");
	ak_thread_exit();
	return NULL;
}

static bool network_video_send_wait_thread_quit(void)
{
	int timeout = 300;
	while (timeout--)
	{
		if (network_video_send_thread_run == false)
		{
			return true;
		}
		ak_sleep_ms(10);
	}
	return false;
}

bool network_video_send_package_open(unsigned long id)
{
	if (networK_video_send_task_run == true)
	{
		return false;
	}

	if (network_video_send_wait_thread_quit() == false)
	{
		return false;
	}

	network_video_send_ready = false;
	network_video_send_eth_id = id;
	networK_video_send_task_run = true;
	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, network_video_send_package_task, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, -1);
	ak_thread_detach(thread_id);

	return true;
}

bool network_video_send_package_close(void)
{
	if (networK_video_send_task_run == false)
	{
		return false;
	}
	networK_video_send_task_run = false;
	return true;
}

bool is_network_video_send_package_open(void)
{
	return network_video_send_ready;
}

bool is_network_video_i_frame_request(void)
{
	return network_frame_request;
}

void set_network_i_frame_request_param(bool param)
{
	network_frame_request = param;
}

void network_video_send_package_start(void)
{
	printf("==========>>> video send package start <<<==========\n");
	network_video_send_ready = true;
	video_input_open();
	set_network_i_frame_request_param(true);
}

void network_video_send_package_stop(void)
{
	printf("==========>>> video send package stop <<<==========\n");
	network_video_send_ready = false;	
	video_input_close();
}

static int network_video_receive_eth_id = 0;
static bool network_video_receive_thread_run = false;
static bool networK_video_receive_task_run = false;
static int video_package_receive_fd = -1;
static bool network_video_receive_socket_open(void)
{
	if (video_package_receive_fd != -1)
	{
		return false;
	}

	printf("==========>>> video receive socket %0x <<<==========\n", network_video_receive_eth_id);
	if ((video_package_receive_fd = socket(PF_PACKET, SOCK_RAW, htons(network_video_receive_eth_id))) < 0)
	{
		printf("create socket error raw_socket_receive_fd\n");
		return false;
	}

	int recv_buf_size = 100 * 1024;
	if (setsockopt(video_package_receive_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(int)) < 0)
	{
		printf("setsockopt SO_RCVBUF\n");
		return false;
	}
	return true;
}

static bool network_video_receive_socket_close(void)
{
	if (video_package_receive_fd == -1)
	{
		return false;
	}

	close(video_package_receive_fd);
	video_package_receive_fd = -1;

	return true;
}

static void *network_video_receive_package_task(void *arg)
{
	fd_set readfds;
	struct timeval timeout;

	network_video_receive_thread_run = true;

	network_video_receive_socket_open();

	char *receive_frame_buffer = NULL;
	// unsigned long long receive_frame_pts = 0;
	// unsigned long long receive_video_index = 0;
	unsigned int receive_frame_size = 0;
	unsigned int receive_frame_count = 0;
	bool receive_frame_start = false;

	//	char frame_type = 0;

	char *buf_ptr = NULL;
	char buffer[VIDEO_PACKAGE_SIZE_MAX] = {0};
	//	int file_fd = open("/mnt/read.h264",O_CREAT|O_WRONLY);
	while (networK_video_receive_task_run == true)
	{
		FD_ZERO(&readfds);
		/*将所要检测端socket句柄加入到集合中*/
		FD_SET(video_package_receive_fd, &readfds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 5000;
		/*设置select等待的最大时间 检测集合read中的句柄是否有可读信息*/
		int ret_select = select(video_package_receive_fd + 1, &readfds, NULL, NULL, &timeout);
		if (ret_select > 0)
		{
			/*如果这个被监视端句柄真的变为可读了*/
			if (FD_ISSET(video_package_receive_fd, &readfds))
			{
				int ret = recvfrom(video_package_receive_fd, buffer, sizeof(buffer), 0, NULL, NULL);
				if (ret > 60)
				{
					// write(file_fd,buffer,ret);
					buf_ptr = &buffer[60];
					ret -= 60;
					while (ret > 0)
					{

						if (memcmp(buf_ptr, video_start_code, 4) == 0)
						{
							if (receive_frame_buffer != NULL)
							{
								free(receive_frame_buffer);
								receive_frame_buffer = NULL;
							}

							receive_frame_count = 0;

							receive_frame_size = (buf_ptr[4] << 24) | (buf_ptr[5] << 16) | (buf_ptr[6] << 8) | buf_ptr[7];
							// receive_frame_pts =  (buf_ptr[8] << 24) | (buf_ptr[9] << 16) | (buf_ptr[10] << 8) | buf_ptr[11];
							// receive_video_index =  (buf_ptr[12] << 24) | (buf_ptr[13] << 16) | (buf_ptr[14] << 8) | buf_ptr[15];
							// printf("===========================>>> receive : %llu \n\r",receive_video_index);
							//	frame_type = buf_ptr[16];
							//	receive_frame_key_frame  =buf_ptr[16]?true:false;
							buf_ptr += 17;
							ret -= 17;
							if (ret <= 0)
							{
								receive_frame_start = false;
							}
							else
							{
								receive_frame_buffer = (char *)malloc(receive_frame_size);
								receive_frame_start = true;
							}
						}

						if ((receive_frame_start == true) && (receive_frame_buffer != NULL))
						{
							if ((receive_frame_count + ret) <= receive_frame_size)
							{
								memcpy(&receive_frame_buffer[receive_frame_count], buf_ptr, ret);
								receive_frame_count += ret;
								ret = 0;
								if (receive_frame_count == receive_frame_size)
								{
									receive_frame_start = false;
									// video_decode_push(frame_type,(unsigned char*)receive_frame_buffer, receive_frame_size);
									// video_record_data_push(frame_type,(unsigned char*)receive_frame_buffer,receive_frame_size,true);
									// printf("===========================>>> receive : %llu \n\r",receive_video_index);
									free(receive_frame_buffer);
									receive_frame_buffer = NULL;

									// printf("write finish\n");
									// close(file_fd);
									// while(1);
								}
							}
							else
							{
								ret = 0;
							}
						}
						else
						{
							printf("video unknow data:ret = %d\n", ret);
							ret = -1;
						}
					}
				}
			}
		}
	}
	if (receive_frame_buffer != NULL)
	{
		free(receive_frame_buffer);
	}
	network_video_receive_socket_close();
	network_video_receive_thread_run = false;

	printf("===========<<< network video receive finish >>>===========\n");
	ak_thread_exit();
	return NULL;
}

static bool network_video_receive_wait_thread_quit(void)
{
	int timeout = 300;
	while (timeout--)
	{
		if (network_video_receive_thread_run == false)
		{
			return true;
		}
		ak_sleep_ms(10);
	}
	return false;
}

bool network_video_receive_package_open(unsigned long id)
{
	if (networK_video_receive_task_run == true)
	{
		return false;
	}

	if (network_video_receive_wait_thread_quit() == false)
	{
		return false;
	}

	network_video_receive_eth_id = id;
	networK_video_receive_task_run = true;
	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, network_video_receive_package_task, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, -1);
	ak_thread_detach(thread_id);
	return true;
}

bool network_video_receive_package_close(void)
{
	if (networK_video_receive_task_run == false)
	{
		return false;
	}
	networK_video_receive_task_run = false;
	return true;
}
