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
#include "audio_output.h"
#include "ak_mem.h"
#include "sys/time.h"
#include "audio_input.h"
#include "g711_table.h"

#define AUDIO_FRAME_MAX (2 * 1024)

#define AUDIO_QUEUE_PACKAGE_MAX 64

#define AUDIO_PACKAGE_SIZE_MAX 1510 // 10*1024//1510

static int network_audio_send_eth_id = 0;

static const char audio_start_code[4] = {0x00, 0x00, 0x01, 0xfc};

struct sockaddr_ll *network_get_send_addres(void);
char *nework_get_package_head(unsigned int type);

static bool network_audio_send_thread_run = false;
static bool networK_audio_send_task_run = false;
static bool network_audio_send_ready = false;
static int audio_package_send_fd = -1;
static bool network_audio_send_socket_open(void)
{
	if (audio_package_send_fd != -1)
	{
		return false;
	}

	printf("==========>>> audio send socket %0x <<<==========\n", network_audio_send_eth_id);
	if ((audio_package_send_fd = socket(PF_PACKET, SOCK_RAW, htons(network_audio_send_eth_id))) < 0)
	{
		printf("create socket error raw_socket_receive_fd\n");
		return false;
	}

	return true;
}

static bool network_audio_send_socket_close(void)
{
	if (audio_package_send_fd == -1)
	{
		return false;
	}

	close(audio_package_send_fd);
	audio_package_send_fd = -1;

	return true;
}

static void network_send_package_audio(char *data, int size)
{
	static unsigned char *buffer = NULL;
	static int buffer_size = 0;
	int send_size = 0;
	int remain_size = size;

	/***** 分配音频帧内存 *****/
	if ((buffer == NULL) || ((buffer_size - 77) < size))
	{
		if (buffer != NULL)
		{
			ak_mem_free(buffer);
		}
		buffer_size = size + 77;
		buffer = ak_mem_alloc(MODULE_ID_AI, buffer_size);
	}

	/***** 获取时间戳 *****/
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long pts = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	/***** 帧序号 *****/
	static unsigned long frame_index = 0;
	frame_index++;

	/***** 帧类型 PCM*****/
	char frame_type = 0;
	// printf(" #####################################>>>AUDIO \n");
	while (remain_size > 0)
	{
		memcpy(&buffer[0], nework_get_package_head(network_audio_send_eth_id), 60);
		if (send_size == 0)
		{
			memcpy(&buffer[60], audio_start_code, 4);
			buffer[64] = (remain_size >> 24) & 0xFF;
			buffer[65] = (remain_size >> 16) & 0xFF;
			buffer[66] = (remain_size >> 8) & 0xFF;
			buffer[67] = remain_size & 0xFF;

			buffer[68] = (pts >> 24) & 0xFF;
			buffer[69] = (pts >> 16) & 0xFF;
			buffer[70] = (pts >> 8) & 0xFF;
			buffer[71] = pts & 0xFF;

			buffer[72] = (frame_index >> 24) & 0xFF;
			buffer[73] = (frame_index >> 16) & 0xFF;
			buffer[74] = (frame_index >> 8) & 0xFF;
			buffer[75] = frame_index & 0xFF;
			buffer[76] = frame_type;

			if (remain_size > (AUDIO_PACKAGE_SIZE_MAX - 77))
			{
				memcpy(&buffer[77], &data[send_size], AUDIO_PACKAGE_SIZE_MAX - 77);
				if (sendto(audio_package_send_fd, buffer, AUDIO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)network_get_send_addres(), sizeof(struct sockaddr_ll)) < 0)
				{
					perror(" send to fail \n");
				}
				remain_size -= (AUDIO_PACKAGE_SIZE_MAX - 77);
				send_size += (AUDIO_PACKAGE_SIZE_MAX - 77);
			}
			else
			{
				memcpy(&buffer[77], &data[send_size], remain_size);
				if (sendto(audio_package_send_fd, buffer, remain_size + 77, 0, (struct sockaddr *)network_get_send_addres(), sizeof(struct sockaddr_ll)) < 0)
				{
					perror(" send to fail \n");
				}
				break;
			}
		}

		if (remain_size > (AUDIO_PACKAGE_SIZE_MAX - 60))
		{
			memcpy(&buffer[60], &data[send_size], AUDIO_PACKAGE_SIZE_MAX - 60);
			if (sendto(audio_package_send_fd, buffer, AUDIO_PACKAGE_SIZE_MAX, 0, (struct sockaddr *)network_get_send_addres(), sizeof(struct sockaddr_ll)) < 0)
			{
				perror(" send to fail \n");
			}
			remain_size -= (AUDIO_PACKAGE_SIZE_MAX - 60);
			send_size += (AUDIO_PACKAGE_SIZE_MAX - 60);
		}
		else
		{
			memcpy(&buffer[60], &data[send_size], remain_size);
			if (sendto(audio_package_send_fd, buffer, remain_size + 60, 0, (struct sockaddr *)network_get_send_addres(), sizeof(struct sockaddr_ll)) < 0)
			{
				perror(" send to fail \n");
			}
			break;
		}
	}
}

static void *network_audio_send_package_task(void *arg)
{
	network_audio_send_thread_run = true;
	pcm16_alaw_tableinit();
	network_audio_send_socket_open();

	unsigned char *audio_data = NULL;
	int audio_size = 0;
	char *alaw_buffer = NULL;
	while (networK_audio_send_task_run == true)
	{
		if (audio_input_read(&audio_data, &audio_size) == true)
		{
			if (network_audio_send_ready == true)
			{

				int alaw_buffer_size = audio_size / 2;
				alaw_buffer = ak_mem_alloc(MODULE_ID_APP, alaw_buffer_size);
				if (alaw_buffer == NULL)
				{
					printf("Error while allocating memory for alaw_buffer.\n");
					continue;
				}

				pcm16_to_alaw(audio_size, (const char *)audio_data, alaw_buffer);

				network_send_package_audio(alaw_buffer, alaw_buffer_size);

				ak_mem_free(alaw_buffer);
				alaw_buffer = NULL;
			}
			ak_mem_free(audio_data);
		}

		ak_sleep_ms(1);
	}

	network_audio_send_socket_close();

	audio_input_close();

	network_audio_send_thread_run = false;

	printf("===========<<< network audio send finish >>>===========\n");
	ak_thread_exit();
	return NULL;
}

static bool network_audio_send_wait_thread_quit(void)
{
	int timeout = 300;
	while (timeout--)
	{
		if (network_audio_send_thread_run == false)
		{
			return true;
		}
		ak_sleep_ms(10);
	}
	return false;
}

bool network_audio_send_package_open(unsigned long id)
{
	if (networK_audio_send_task_run == true)
	{
		return false;
	}

	if (network_audio_send_wait_thread_quit() == false)
	{
		return false;
	}

	network_audio_send_eth_id = id;
	networK_audio_send_task_run = true;
	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, network_audio_send_package_task, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, -1);
	ak_thread_detach(thread_id);

	return true;
}

bool network_audio_send_package_close(void)
{
	if (networK_audio_send_task_run == false)
	{
		return false;
	}
	networK_audio_send_task_run = false;
	return true;
}

bool is_network_audio_send_package_open(void)
{
	return network_audio_send_ready;
}

void network_audio_send_package_start(void)
{
	printf("==========>>> audio send package start <<<==========\n");
	network_audio_send_ready = true;
	audio_input_aec_control(true);
}

void network_audio_send_package_stop(void)
{
	printf("==========>>> audio send package stop <<<==========\n");
	network_audio_send_ready = false;
	audio_input_aec_control(false);
}

static int network_audio_receive_eth_id = 0;
static bool network_audio_receive_thread_run = false;
static bool networK_audio_receive_task_run = false;
static int audio_package_receive_fd = -1;
static bool network_audio_receive_socket_open(void)
{
	if (audio_package_receive_fd != -1)
	{
		return false;
	}

	printf("==========>>> audios receive socket %0x <<<==========\n", network_audio_receive_eth_id);
	if ((audio_package_receive_fd = socket(PF_PACKET, SOCK_RAW, htons(network_audio_receive_eth_id))) < 0)
	{
		printf("create socket error raw_socket_receive_fd\n");
		return false;
	}

	int recv_buf_size = 10 * 1024;
	if (setsockopt(audio_package_receive_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(int)) < 0)
	{
		printf("setsockopt SO_RCVBUF\n");
		return false;
	}
	return true;
}

static bool network_audio_receive_socket_close(void)
{
	if (audio_package_receive_fd == -1)
	{
		return false;
	}

	close(audio_package_receive_fd);
	audio_package_receive_fd = -1;

	return true;
}

static bool network_audio_receive_connected = false;
static void *network_audio_receive_package_task(void *arg)
{
	fd_set readfds;
	struct timeval timeout;

	network_audio_receive_thread_run = true;

	network_audio_receive_socket_open();

	alaw_pcm16_tableinit();

	char *receive_frame_buffer = malloc(AUDIO_FRAME_MAX);
	// unsigned long long receive_frame_pts = 0;
	// unsigned long long receive_video_index = 0;
	unsigned int receive_frame_size = 0;
	unsigned int receive_frame_count = 0;
	bool receive_frame_start = false;

	int connected_count = 0;

	char *pcm_buffer = NULL;

	char *buf_ptr = NULL;
	char buffer[AUDIO_PACKAGE_SIZE_MAX] = {0};
	unsigned int skip_frame_count = 2;
	// int fd  = -1;
/****** 变量说明：由于tuya下发的音频帧大小为320个字节，直接播放会出现断续现象，先用buffer存起来，达到溢出后在送入声卡播放******/
#define PCM_BUFFER_MAX 2048 // 1048 * 2
	unsigned char alaw_buffer[PCM_BUFFER_MAX] = {0};
	bzero(alaw_buffer, sizeof(alaw_buffer));
	int alaw_size = 0;

	bool flag = false;
	while (networK_audio_receive_task_run == true)
	{
		if (flag != message_status.open_message)
		{
			flag = message_status.open_message;
			printf("%s  open_message====================>>>\n\r", __func__);
		}
		// if (message_status.open_message)
		// {
		// 	continue;
		// }

		FD_ZERO(&readfds);
		/*将所要检测端socket句柄加入到集合中*/
		FD_SET(audio_package_receive_fd, &readfds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 5000;
		/*设置select等待的最大时间 检测集合read中的句柄是否有可读信息*/
		int ret_select = select(audio_package_receive_fd + 1, &readfds, NULL, NULL, &timeout);
		// printf("message_status.open_message===========================>>>  : %d    ret_select>>:%d\n\r",message_status.open_message,ret_select);
		if (ret_select > 0 && !message_status.open_message)
		{
			/*如果这个被监视端句柄真的变为可读了*/
			if (FD_ISSET(audio_package_receive_fd, &readfds))
			{
				int ret = recvfrom(audio_package_receive_fd, buffer, sizeof(buffer), 0, NULL, NULL);
				// printf("audio receive size : %d\n\r",ret);
				if (ret > 60)
				{
					connected_count = 0;
					network_audio_receive_connected = true;
					buf_ptr = &buffer[60];
					ret -= 60;
					while (ret > 0)
					{

						if (memcmp(buf_ptr, audio_start_code, 4) == 0)
						{
							// if(receive_frame_buffer != NULL)
							//{
							// free(receive_frame_buffer);
							// receive_frame_buffer = NULL;
							//}

							receive_frame_count = 0;

							receive_frame_size = (buf_ptr[4] << 24) | (buf_ptr[5] << 16) | (buf_ptr[6] << 8) | buf_ptr[7];
							// receive_frame_pts =  (buf_ptr[8] << 24) | (buf_ptr[9] << 16) | (buf_ptr[10] << 8) | buf_ptr[11];
							// receive_video_index =  (buf_ptr[12] << 24) | (buf_ptr[13] << 16) | (buf_ptr[14] << 8) | buf_ptr[15];
							// printf("===========================>>> receive : %d \n\r",receive_frame_size);
							// frame_type = buf_ptr[16];
							//	receive_frame_key_frame  =buf_ptr[16]?true:false;
							buf_ptr += 17;
							ret -= 17;
							if (ret < 0)
							{
								receive_frame_start = false;
							}
							else
							{
								// receive_frame_buffer = (char*)malloc(receive_frame_size);
								receive_frame_start = true;
							}
						}

						if (receive_frame_start == true) /*&&(receive_frame_buffer != NULL))*/
						{
							if ((receive_frame_count + ret) <= receive_frame_size)
							{
								memcpy(&receive_frame_buffer[receive_frame_count], buf_ptr, ret);
								receive_frame_count += ret;
								ret = 0;
								if (receive_frame_count == receive_frame_size)
								{
									receive_frame_start = false;

									/***** 达到buffer溢出送入声卡 *****/

									if ((alaw_size + receive_frame_size) >= PCM_BUFFER_MAX)
									{
										bool is_audio_play_ing(void);
										if (skip_frame_count) // 丟掉前两BUFFER
										{
											// printf("skip_frame_count ====>>%d\n\r",skip_frame_count);
											skip_frame_count--;
										}
										else if (is_audio_play_ing() == false)
										{
											int pcm_buffer_size = alaw_size * 2;
											pcm_buffer = malloc(pcm_buffer_size);
											if (pcm_buffer == NULL)
											{
												printf("Error while allocating memory for write buffer.\n");
												ak_sleep_ms(1);
												continue;
											}

											alaw_to_pcm16(alaw_size, (const char *)alaw_buffer, pcm_buffer);
											if (get_ao_buf_remain_len() < PCM_BUFFER_MAX * 8)
											{
												if (audio_output_write((unsigned char *)pcm_buffer, pcm_buffer_size) == false)
													audio_output_open(AUDIO_CHANNEL_MONO, AK_AUDIO_SAMPLE_RATE_16000);
											}

											free(pcm_buffer);
											pcm_buffer = NULL;
										}

										alaw_size = 0;
									}
									memcpy(&alaw_buffer[alaw_size], receive_frame_buffer, receive_frame_size);
									alaw_size += receive_frame_size;
									// printf("receive_frame_size :%d\n\r",receive_frame_size);

									// add_audio_decode_data((unsigned char*)receive_frame_buffer,receive_frame_size);
									// free(receive_frame_buffer);
									// receive_frame_buffer = NULL;
								}
							}
							else
							{
								ret = 0;
							}
						}
						else
						{
							printf("audio unknow data:ret = %d\n", ret);
							ret = -1;
						}
					}
				}
			}
		}
		else if ((connected_count++) > 200 || message_status.open_message)
		{
			connected_count = 0;
			if (network_audio_receive_connected == true)
			{
				network_audio_receive_connected = false;
				ao_howling_suppress_close();
				memset(alaw_buffer, 0, sizeof(alaw_buffer));
				skip_frame_count = 2;
				// close(fd);
				// fd = -1;
				// system("sync");
				printf("===========<<< network audio receive stop >>>===========\n");
			}
		}
		ak_sleep_ms(1);
	}

	if (receive_frame_buffer != NULL)
	{
		free(receive_frame_buffer);
	}
	network_audio_receive_socket_close();

	audio_output_close();

	network_audio_receive_thread_run = false;

	printf("===========<<< network audio receive finish >>>===========\n");
	ak_thread_exit();
	return NULL;
}

static bool network_audio_receive_wait_thread_quit(void)
{
	int timeout = 300;
	while (timeout--)
	{
		if (network_audio_receive_thread_run == false)
		{
			return true;
		}
		ak_sleep_ms(10);
	}
	return false;
}

bool network_audio_receive_package_open(unsigned long id)
{
	if (networK_audio_receive_task_run == true)
	{
		return false;
	}

	if (network_audio_receive_wait_thread_quit() == false)
	{
		return false;
	}
	network_audio_receive_eth_id = id;
	networK_audio_receive_task_run = true;
	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, network_audio_receive_package_task, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, -1);
	ak_thread_detach(thread_id);

	return true;
}

bool network_audio_receive_package_close(void)
{
	if (networK_audio_receive_task_run == false)
	{
		return false;
	}
	networK_audio_receive_task_run = false;
	return true;
}

bool is_network_audio_receive_connected(void)
{
	return network_audio_receive_connected;
}
