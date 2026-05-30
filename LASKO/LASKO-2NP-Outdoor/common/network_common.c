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
#include "ak_common.h"
#include "app_common.h"
#include "audio_output.h"
#include "audio_input.h"
#include "outdoor_unlock.h"
#include "card_manage.h"
#include <math.h>

#define NET_COMMON_CMD_START 0XAA
#define NET_COMMON_CMD_END 0X55

#define ETH_P_CMD 0xFFFF

#define SOFTWARE_VERSION 613

static int cmd_receive_fd = -1;
static int cmd_send_fd = -1;
static ak_mutex_t network_device_mutex;

static bool net_online_device[DEVICE_TOTAL] = {false};
static bool net_talk_state = false;
int network_tlak_device = DEVICE_UNKONW;

/* 心跳超时检测：记录每个 indoor 设备最后收包时间（ms） */
#define HEARTBEAT_TIMEOUT_MS  10000   /* 10 秒未收到任何包 → 判离线 */
static unsigned long long net_heart_time[DEVICE_TOTAL] = {0};

extern void audio_input_aec_control(bool enable);

bool net_talk_state_get(void)
{
	return net_talk_state;
}

static network_device network_local_device = DEVICE_UNKONW;
void network_local_device_set(network_device device)
{
	network_local_device = device;
}
network_device network_local_device_get(void)
{
	return network_local_device;
}

static struct sockaddr_ll nework_local_sockaddr_ll;
struct sockaddr_ll *network_get_send_addres(void)
{
	return &nework_local_sockaddr_ll;
}
static bool network_cmd_socket_init(void)
{ // AF_INET  //PF_PACKET             //SOCK_DGRAM  SOCK_RAW
	if ((cmd_receive_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_CMD))) < 0)
	{
		printf("create socket error raw_socket_receive_fd\n");
		return false;
	}

	int recv_buf_size = 512;
	if (setsockopt(cmd_receive_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(int)) < 0)
	{
		printf("setsockopt SO_RCVBUF\n");
		return false;
	}

	if ((cmd_send_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_CMD))) < 0)
	{
		printf("create socket error raw_socket_send_aduio_fd\n");
		return false;
	}

	struct ifreq req;
	int fd = socket(PF_INET, SOCK_DGRAM, 0);
	strcpy(req.ifr_name, NETWORK_NAME);
	ioctl(fd, SIOCGIFINDEX, &req);
	close(fd);

	memset(&nework_local_sockaddr_ll, 0, sizeof(nework_local_sockaddr_ll));
	/*网卡eth0的index，非常重要，系统把数据往哪张网卡上发，就靠这个标识*/
	nework_local_sockaddr_ll.sll_ifindex = req.ifr_ifindex;
#if 0
	/*标识包的类型为发出去的包*/
	nework_local_sockaddr_ll.sll_pkttype   = PACKET_OUTGOING;
	/*目标MAC地址长度为6*/
	nework_local_sockaddr_ll.sll_halen     = 6;    
	/*填写目标MAC地址*/
	nework_local_sockaddr_ll.sll_addr[0]   = 0xFF;
	nework_local_sockaddr_ll.sll_addr[1]   = 0xFF;
	nework_local_sockaddr_ll.sll_addr[2]   = 0xFF;
	nework_local_sockaddr_ll.sll_addr[3]   = 0xFF;
	nework_local_sockaddr_ll.sll_addr[4]   = 0xFF;
	nework_local_sockaddr_ll.sll_addr[5]   = 0xFF;
#endif
	return true;
}

static void network_device_init(network_device device)
{
	printf("=============>>local device:%d \n", device);
	network_local_device = device;
	if ((network_local_device == DEVICE_INDOOR_ID1) ||
		(network_local_device == DEVICE_INDOOR_ID2) ||
		(network_local_device == DEVICE_INDOOR_ID3) ||
		(network_local_device == DEVICE_INDOOR_ID4))
	{
		system("1 >/proc/sys/net/ipv4/ip_forward");

		system("ip link add name " NETWORK_NAME " type bridge");

		system("ip link set " NETWORK_NAME " up");

		system("ip link set dev eth0 master " NETWORK_NAME);

		system("ip link set dev eth1 master " NETWORK_NAME);

		system("ifconfig eth0 0.0.0.0 promisc");

		system("ifconfig eth1 0.0.0.0 promisc");

		char buffer[128] = {0};
		sprintf(buffer, "ip addr add dev %s %s/24", NETWORK_NAME, "192.168.37.1");
		printf("%s \n", buffer);
		system(buffer);
	}
	else
	{
		char buffer[128] = {0};
		sprintf(buffer, "ifconfig eth0 %s netmask 255.255.255.0", network_local_device == DEVICE_OUTDOOR_1 ? "192.168.37.7" : "192.168.37.8");
		system(buffer);
	}
	system("route add -net 224.0.0.0 netmask 224.0.0.0 " NETWORK_NAME);

	network_cmd_socket_init();

	memset(net_online_device, 0, sizeof(network_local_device));
	net_online_device[network_local_device] = true;
}

static bool net_common_code_check_valid(const char *buffer)
{
	/*先判断起始码和结束码是否一致*/
	if ((buffer[0] != NET_COMMON_CMD_START) || (buffer[COMMON_CMD_LEN - 1] != NET_COMMON_CMD_END))
	{
		return false;
	}

	/*判断校验和是否正确*/
	unsigned char num = (buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5]) & 0xFF;
	if (num != buffer[COMMON_CMD_LEN - 2])
	{
		return false;
	}

	/*判断设备是否正确*/
	if ((buffer[2] != network_local_device) && (buffer[2] != DEVICE_ALL))
	{
		return false;
	}

	/*判断设备是否冲突*/
	if (buffer[2] == buffer[1])
	{
		; // printf("device id repeat ID%d\n",buffer[2]);
	}
	return true;
}

static bool net_common_code_check_valid_upgrade(const char *buffer)
{
	/*先判断起始码和结束码是否一致*/
	if ((buffer[0] != NET_COMMON_CMD_START) || ((buffer[UPGRADE_PACK_LEN + 12] != NET_COMMON_CMD_END) && (buffer[UPGRADE_DATA_LEN + 12] != NET_COMMON_CMD_END)))
	{

		return false;
	}
	/*判断设备是否正确*/
	if ((buffer[2] != network_local_device) && (buffer[2] != DEVICE_ALL))
	{

		return false;
	}

	/*判断设备是否冲突*/
	if (buffer[2] == buffer[1])
	{
		printf("======>>>>>>func =:%s line = :%d\n", __func__, __LINE__);
	}
	return true;
}

static bool net_common_code_check_valid_card_data(const char *buffer)
{
	/*先判断起始码和结束码是否一致*/
	if ((buffer[0] != NET_COMMON_CMD_START) || (buffer[CARD_DATA_TOTAL_SIZE + 6] != NET_COMMON_CMD_END))
	{
		return false;
	}
	/*判断设备是否正确*/
	if ((buffer[2] != network_local_device) && (buffer[2] != DEVICE_ALL))
	{
		return false;
	}

	/*判断设备是否冲突*/
	if (buffer[2] == buffer[1])
	{
		printf("======>>>>>>func =:%s line = :%d\n", __func__, __LINE__);
	}
	return true;
}

typedef struct
{
	char *str;
	unsigned char cmd;
	void (*proc)(network_device device, unsigned char arg1, unsigned char arg2);
	bool log_open;
} net_common_event_info;

#if 0
static void net_common_deivce_repeat_func(network_device device,unsigned char arg1,unsigned char arg2)
{
#if 0
	if(arg1 == 1)
	{
		/*收到查询id状态*/
		network_cmd_data data;
		data.device = device;
		data.cmd = NET_COMMON_CMD_ID_REPEAT;
		data.arg1 = 2;
		if(device == network_local_device)
		{
			data.arg2 = 0x80|network_local_device;
		}
		else
		{
			data.arg2  = network_local_device;
		}
		network_send_cmd_data(&data);
	}
	else if(arg1 == 2)
	{
		/*收到查询状态的结果*/
		if(arg2&0x80)
		{
			printf("receive event:device id repeat ID%d\n",arg2&0x0F);
			extern bool device_id_repeat_push(char arg1);
			device_id_repeat_push(arg2&0x0F);
		}
		else
		{
			printf("online device:%d \n",arg2);
			net_online_device[arg1] = true;

		}
	}
#else
	if(arg1 == 1)
	{
		/*收到查询id状态*/
		network_cmd_data data;
		data.device = device;
		data.cmd = NET_COMMON_CMD_ID_REPEAT;
		data.arg1 = 2;
		if(device == network_local_device)
		{
			data.arg2 = 0x80|network_local_device;
		}
		else
		{
			data.arg2  = network_local_device;
		}
		network_send_cmd_data(&data);
	}
#endif
}
#endif
static void net_common_motion_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	// door_light_control(arg1,CALL_LIGHT);
}

static void net_common_outdoor_call_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	// door_light_control(arg1,CALL_LIGHT);
}

static void net_common_outdoor_talk_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	printf("%s==================%d\n", __func__, arg1);
	// if(is_network_video_send_package_open() == false)
	// {
	// 	return ;
	// }

	message_status.open_message = false;
	if (arg1 == (network_local_device_get() == DEVICE_OUTDOOR_1 ? 1 : 2))
	{
		door_light_control(true, TALKING_LIGHT);
		net_talk_state = true;
		if (arg1)
		{ // 通话
			stop_doorbell_ring();

			// audio_input_aec_control(true);
			ao_howling_suppress_open();
			// audio_output_volume_set(50);

			// 开call机键灯
			//  call_light_ctrl(arg2 == 1 ? KEY1_LED : arg2 + 4);
			//  start_call_light_ctrl_ms_set(0); //不灭call机键灯
			//  network_audio_send_package_start();
		}
		else
		{
			// start_call_light_ctrl_ms_set(1);
			ao_howling_suppress_close();
			// audio_output_volume_set(50);
		}
	}
	// extern bool curr_audio_param_is_tuya;
	// if(curr_audio_param_is_tuya != (bool)arg2)
	// {
	// 	printf("======================================>>> 參數切換 \n");
	// 	ak_ao_cancel(get_ao_hand());
	// 	ak_ao_restart(get_ao_hand());
	// 	audio_output_device_param_switch((bool)arg2);
	// 	audio_input_device_param_switch((bool)arg2);
	// }
}

static void net_common_outdoor_hang_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	// printf("====outdoor_hang===arg1 == %d====arg2 ==%d>>>\n\n", arg1, arg2);
	net_talk_state = false;
}

static void net_common_interphone_call_func(network_device device, unsigned char arg1, unsigned char arg2)
{
}

/* 
autoor: zio
date: 2025/12/18 修改波兰语开锁留言播报，波兰语lock和gate有单独语音播报
date：2025/12/23 设置新增五种欧洲语言的开锁播报为英语
*/
static void net_common_outdoor_unlock_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	printf("====outdoor_unlock===arg1 == %d====langage ==%d>>>\n\n", arg1, ((arg2 & 0x3C) >> 2));
	start_unlock(arg1, arg2 & 0x03);

	if (arg2 & 0x80)
	{
 	int lang_index = ((arg2 & 0x3C) >> 2);
    int lock_type = arg2 & 0x03;  // 0: lock1, 1: lock2
	printf("====outdoor_unlock===langage ==%d lock_type ==%d>>>\n\n", lang_index, lock_type);
	if (lang_index > 9 && lang_index <= 14) 
	{
    	play_doorbell(RING_INDEX_UNLOCK_EH, 7); 
    }
    else if (lang_index == 4 && lock_type == 2) 
	{  
		play_doorbell(RING_INDEX_UNLOCK_SPA + lang_index, 7);
    } 
	else 
	{
		play_doorbell(RING_INDEX_UNLOCK_EH + lang_index, 7);
    }
	}
	// if(net_talk_state == false)
	// if (arg2 & 0x80)
	// 	play_doorbell(RING_INDEX_UNLOCK_EH + ((arg2 & 0x3C) >> 2), 7);
}

static void net_common_outdoor_light_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	printf("====outdoor_light===arg1 == %d====>>>\n\n", arg1);
	door_light_control(arg1, INDOOR_LIGHT);
}

static int network_stream_count = 0;
static void net_common_stream_status_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	extern void set_ai_gain(int gain);
	/* arg1 第一位 视频稳定标志    ||     第二位 留言开启标志 */
	if ((device != DEVICE_INDOOR_ID1) && (device != DEVICE_INDOOR_ID2) && (device != DEVICE_INDOOR_ID3) && (device != DEVICE_INDOOR_ID4) && (device != DEVICE_INDOOR_ID5) && (device != DEVICE_INDOOR_ID6))
	{
		return;
	}

	void tuya_audio_in_vol_switch(bool tuya);
	// printf("====net_common_stream_status_func===arg1 == %d==arg2 == %d==>>>\n\n", arg1,arg2);
	network_stream_count = 0;
	if ((arg1 & 0x01) == 0) // arg1 bit[0]视频不稳定
	{
		extern void set_network_i_frame_request_param(bool param);
		set_network_i_frame_request_param(true); // 请求发送I帧
	}

	if ((arg1 & 0x02)) // arg1 bit[1]留言标志
	{
		if (net_talk_state == false)
		{
			struct ak_timeval tv;
			ak_get_ostime(&tv);
			message_status.message_recv_t = tv.sec;
			message_status.open_message = true;
			message_status.message_language = arg1 >> 2;
		}
	}
	else if (arg1 & 0x08) // 切换涂鸦通道后，只有门口机关闭视频时，通道才会再做切换操作
	{
		void video_encode_ch_change(bool sub_ch);
		video_encode_ch_change(true);

		net_talk_state = true;
	}

	tuya_audio_in_vol_switch(!(arg1 & 0x02) && (arg1 & 0x08) ? true : false);
	// printf("ARG1 :%d    video_encode_ch :%d\n\r",arg1,arg1 & 0x08);
	if (is_network_video_send_package_open() == false)
	{
		door_light_control(0, AUTO_LIGHT);
		door_light_control(1, TALKING_LIGHT);
		network_video_send_package_start();

		extern bool ao_play_finish(void);
	}

	if (is_network_audio_send_package_open() == false)
	{
		network_audio_send_package_start();
	}
	int volume = arg2 * 3 + 66;
	// printf("talk volume:%d\n",volume);
	if (net_talk_state /*  && !get_dead_lock_state() && !get_alarm_lock_state() */)
	{
		set_ai_gain(-1);
		if (audio_output_volume_get() != volume)
			audio_output_volume_set(volume);
	}
	else
	{
		set_ai_gain(5);
	}
}

#define UPGRADE_FILE "/tmp/cbin.update"

#define UPGRADE_FILE_PATH "/etc/config/cbin"
#define APP_FILE_PATH "/etc/config/cbin/*"
#define UPGRADE_TMP_PATH "/tmp/cbin/*"

static int sum = 0;

static void upgrade_file(network_device device, int arg1, int arg2, char *buf)
{
	FILE *fp = fopen(UPGRADE_FILE, "a+");

	if (fp == NULL)
	{
		printf("open file error\n");
		return;
	}
	int write_len = fwrite(buf, sizeof(char), arg2, fp);

	if ((++sum != arg1) || (write_len != arg2))
	{
		printf("@@@receive sum =:%d  write_len =:%d\n@@@packbage_idnex =:%d packbage_idnex =%d\n", sum, write_len, arg1, arg2);

		network_cmd_data data;
		data.device = device;
		data.cmd = NET_COMON_CMD_UPGRADE_OUTDOOR;
		data.arg1 = 3; // 升级失败
		data.arg2 = 1;
		network_send_cmd_data(&data);

		sum = 0;

		fclose(fp);
	}
	else
	{
		printf("###recive sum =:%d  write_len =:%d\n###packbage_idnex =:%d packbage_idnex =%d\n", sum, write_len, arg1, arg2);

		fclose(fp);
	}
}

static void net_common_device_upgrade_func(network_device device, int arg1, int arg2, char *buf)
{
	if (arg2 != 0)
	{ // 收包写入

		if (arg1 == 1)
		{

			printf("upgrade app version time %s  %s\n\r", __DATE__, __TIME__);
			printf("touch " UPGRADE_FILE "\n\r");
			sum = 0;
			system("rm -rf " UPGRADE_FILE);
			system("touch " UPGRADE_FILE);
			system("chmod 777 " UPGRADE_FILE);
		}

		upgrade_file(device, arg1, arg2, buf);
	}
}
// arg1 == 1检查设备是否在线 arg1 == 2 arg2 == 1 结束发送 arg2 == 2 升级取消
static void net_common_outdoordevice_upgrade_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	if (arg1 == 1)
	{ // 检查是否在线
		printf("check outdoor online !!!\n");
		network_cmd_data data;

		data.cmd = NET_COMON_CMD_UPGRADE_OUTDOOR;
		data.arg1 = 1;				  // 在线
		data.arg2 = UPGRADE_BIG_DATA; // 每个网络包的数据包个数
		data.device = device;
		network_send_cmd_data(&data);
	}
	if (arg1 == 2)
	{
		if (arg2 == 1)
		{ // 升级结束

			printf("outdoor upgrade over!!!\n");
			network_cmd_data data;
			data.device = device;
			data.cmd = NET_COMON_CMD_UPGRADE_OUTDOOR;
			data.arg1 = 2; // 升级结束
			data.arg2 = 1; // 升级结束
			network_send_cmd_data(&data);

			play_doorbell(RING_INDEX_UNLOCK, 7);

			ak_sleep_ms(2000);

			system("tar -zxvf" UPGRADE_FILE " -C /tmp"); //

			int ret = 0;
			if (get_mtd_num() > 8)
			{
				printf("THE /APP DIRECTORY EXISTS.\n");
				system("/tmp/update.sh &"); //
			}
			else if (ret != -1)
			{
				printf("The /app directory does not exist.\n");
				system("rm -rf " APP_FILE_PATH);						   //
				system("\\cp -f " UPGRADE_TMP_PATH " " UPGRADE_FILE_PATH); //

				system("rm -rf " UPGRADE_FILE); //

				system("reboot"); // 重启更新
			}
			else
			{
				system("rm -rf /tmp/*");
			}
		}
		else if (arg2 == 2)
		{ // 升级取消
			printf("outdoor upgrade cancle!!!\n");
			system("rm -rf " UPGRADE_FILE);
		}
	}
}

static void net_common_motion_sensitivity_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	bool motion_detect_sensitivity_set(int sensitivity);
	motion_detect_sensitivity_set(arg1);
}

static void net_common_card_data_sync_func(network_device device, unsigned char arg1, unsigned char arg2, char *buf)
{
	card_data_sync_handler(arg1, buf);
}

static void net_common_add_card_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	network_card_event_receive(arg1);
}

static void net_common_outdoor_reset_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	extern bool user_data_reset(void);
	user_data_reset();
}

static void net_common_def_unlock_time_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	// printf("%s =====================>>>%d,%d,%d\n", __func__, device, arg1, arg2);
	if (device == DEVICE_OUTDOOR_1 || device == DEVICE_OUTDOOR_2)
		return;
	// printf("%s =====================>>>%d,%d,%d\n", __func__, device, arg1, arg2);
	user_data_get()->lock_unlock_time = arg1;
	user_data_get()->gate_unlock_time = arg2;
	void Default_unlock_time_packet(void);
	Default_unlock_time_packet();
	user_data_save();
}

static void net_common_def_exit_button_func(network_device device, unsigned char arg1, unsigned char arg2)
{
	if (device == DEVICE_OUTDOOR_1 || device == DEVICE_OUTDOOR_2)
		return;
	// printf("%s =====================>>>%d,%d,%d\n", __func__, device, arg1, arg2);
	user_data_get()->exit_button_lock = arg1;
	user_data_get()->exit_button_gate1 = arg2;
	void Default_exit_button_packet(void);
	Default_exit_button_packet();
	user_data_save();
}

static net_common_event_info net_common_event[] =
	{
		// {"device id repeat",		NET_COMMON_CMD_ID_REPEAT,		net_common_deivce_repeat_func,			false},
		{"out door call", NET_COMMON_CMD_OUTDOOR_CALL, net_common_outdoor_call_func, false},
		{"indoor and outdoor talk", NET_COMMON_CMD_OUTDOOR_TALK, net_common_outdoor_talk_func, false},
		{"indoor and outdoor hang", NET_COMMON_CMD_OUTDOOR_HANG, net_common_outdoor_hang_func, false},
		{"interphone call ", NET_COMMON_CMD_INTERCOM_CALL, net_common_interphone_call_func, false},
		{"outdoor unlock", NET_COMMON_CMD_UNLOCK, net_common_outdoor_unlock_func, false},
		{"outdoor light", NET_COMMON_CMD_LIGHT, net_common_outdoor_light_func, false},
		{"stream status empty", NET_COMMON_CMD_STREAM_STATUS, net_common_stream_status_func, false},
		{"motion", NET_COMMON_CMD_MOTION, net_common_motion_func, false},
		{"device is upgrade", NET_COMON_CMD_UPGRADE_OUTDOOR, net_common_outdoordevice_upgrade_func, false},
		{"motion detect", NET_COMON_CMD_MOTION_SENSITIVITY, net_common_motion_sensitivity_func, false},
		{"add card", NET_COMMON_CMD_ADD_CARDS, net_common_add_card_func, false},
		{"outdoor reset", NET_COMON_CMD_OUTDOOR_RESET, net_common_outdoor_reset_func, false},
		{"Default unlock-time packet", NET_COMMON_CMD_DEF_UNLOCK_TIME, net_common_def_unlock_time_func, false},
		{"Default exit-button lock packet", NET_COMMON_CMD_EXIT_BUTTON_TIME, net_common_def_exit_button_func, false},
};

static bool net_common_event_process(network_device device, unsigned char cmd, unsigned char arg1, unsigned char arg2)
{
	int size = sizeof(net_common_event) / sizeof(net_common_event_info);
	for (int i = 0; i < size; i++)
	{
		if (net_common_event[i].cmd == cmd)
		{
			if (net_common_event[i].log_open == true)
			{
				printf("receive cmd %s \n", net_common_event[i].str);
			}
			net_common_event[i].proc(device, arg1, arg2);
			break;
		}
	}
	return true;
}

static bool net_common_event_process_card_data_sync(char *buf)
{
	if (buf[3] == NET_COMMON_CMD_CARD_DATA_SYNC)
	{
		network_device device = buf[1];
		unsigned char arg1 = buf[4];
		unsigned char arg2 = buf[5];
		char buffer[480];
		memcpy(buffer, &buf[6], 480);
		net_common_card_data_sync_func(device, arg1, arg2, buffer);
		return true;
	}
	return false;
}

static bool net_common_event_process_upgrade(char *buf)
{
	if (buf[3] == NET_COMON_CMD_UPGRADE_OUTDOOR)
	{
		printf("=======>>>>>receive cmd upgrade begin   version %s  %s <<<<<<======\n", __DATE__, __TIME__);
		network_device device = buf[1];
		int arg1 = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];

		int arg2 = buf[8] << 24 | buf[9] << 16 | buf[10] << 8 | buf[11];
		char buffer[UPGRADE_PACK_LEN];
		memcpy(&buffer, &buf[12], UPGRADE_PACK_LEN);
		net_common_device_upgrade_func(device, arg1, arg2, buffer);

		return true;
	}

	return false;
}

static void *network_cmd_receive_task(void *arg)
{
	fd_set readfds; //定义一个监听集合
	struct timeval timeout;
	struct ak_timeval tv_cur;
	struct ak_timeval heatpack_t;
	char buffer[UPGRADE_PACK_LEN + 73] = {0};
	struct tm Fetch_compile_time();
	struct tm fetch_compile_t = Fetch_compile_time();
	while (1)
	{
		FD_ZERO(&readfds); //清空
		FD_SET(cmd_receive_fd, &readfds); //把cmd_receive_fd加入监听集合

		timeout.tv_sec = 0;
		timeout.tv_usec = 5000;
		int ret_select = select(cmd_receive_fd + 1, &readfds, NULL, NULL, &timeout);
		if (ret_select > 0)
		{
			if (FD_ISSET(cmd_receive_fd, &readfds)) // 判断这个fd是否有数据
			{
				int read_len = recvfrom(cmd_receive_fd, buffer, sizeof(buffer), 0, NULL, NULL);
				if ((read_len == 68) && (buffer[12] == ((ETH_P_CMD >> 8) & 0xFF)) && (buffer[13] == (ETH_P_CMD & 0xFF)) && (net_common_code_check_valid(&buffer[60])))
				{
					/* 更新发送方心跳时间戳 */
					network_device send_dev = (network_device)buffer[61];
					if (send_dev > DEVICE_UNKONW && send_dev < DEVICE_TOTAL)
					{
						net_online_device[send_dev] = true;
						net_heart_time[send_dev] = get_sys_ms();
					}
					net_common_event_process((network_device)buffer[61], buffer[63], buffer[64], buffer[65]);
				}
				else if (((read_len == UPGRADE_PACK_LEN + 73) || (read_len == UPGRADE_DATA_LEN + 73)) && (buffer[12] == ((ETH_P_CMD >> 8) & 0xFF)) && (buffer[13] == (ETH_P_CMD & 0xFF)) && (net_common_code_check_valid_upgrade(&buffer[60])))
				{
					// printf("=====>>>> net_common_event_process upgrade!!!<<<<=====\n");
					net_common_event_process_upgrade(&buffer[60]);
				}
				else if ((read_len == (67 + CARD_DATA_TOTAL_SIZE)) && (buffer[12] == ((ETH_P_CMD >> 8) & 0xFF)) && (buffer[13] == (ETH_P_CMD & 0xFF)) && (net_common_code_check_valid_card_data(&buffer[60])))
				{
					// printf("=====>>>> 数据同步 <<<<=====\n");
					net_common_event_process_card_data_sync(&buffer[60]);
				}
				else
				{
					// printf("=====>>>> other!read_len = %d!!!<<<<=====\n",read_len);
				}
			}
		}

		ak_get_ostime(&tv_cur);
		if (ak_diff_ms_time(&tv_cur, &heatpack_t) > 1000)
		{
			static int time_3s = 0;
			heatpack_t = tv_cur;
			network_stream_count = (is_network_video_send_package_open() == true) ? network_stream_count + 1 : 0;

			if (network_stream_count > 5)
			{
				// if(is_network_video_send_package_open() == true)
				// {
				ao_howling_suppress_close();
				net_talk_state = false;
				door_light_control(1, AUTO_LIGHT);
				door_light_control(0, TALKING_LIGHT);
				void video_encode_ch_change(bool sub_ch);
				video_encode_ch_change(false);
				network_video_send_package_stop();
				// }

				if (is_network_audio_send_package_open() == true)
				{
					network_audio_send_package_stop();
				}
				network_stream_count = 0;
			}

			/* ---- 心跳超时检测：超过 HEARTBEAT_TIMEOUT_MS 未收包 → 判该 indoor 离线 ---- */
			unsigned long long now_ms = get_sys_ms();
			bool any_indoor_online = false;
			for (network_device dev = DEVICE_INDOOR_ID1; dev <= DEVICE_INDOOR_ID6; dev++)
			{
				if (net_online_device[dev])
				{
					if ((net_heart_time[dev] != 0) && (now_ms - net_heart_time[dev] > HEARTBEAT_TIMEOUT_MS))
					{
						net_online_device[dev] = false;
						printf("[heartbeat] Device[%d] timeout, offline\n", dev);
					}
					else
					{
						any_indoor_online = true;
					}
				}
			}
			/* 全部 indoor 离线 → 立即重启 */
			if (!any_indoor_online)
			{
				/* 至少有一台设备曾经上线过才触发重启，避免开机即重启 */
				bool ever_online = false;
				for (int i = DEVICE_INDOOR_ID1; i <= DEVICE_INDOOR_ID6; i++)
				{
					if (net_heart_time[i] != 0) { ever_online = true; break; }
				}
				if (ever_online)
				{
					printf("[heartbeat] All indoor devices offline, rebooting...\n");
					system("reboot");
				}
			}
			/* ---- 心跳超时检测结束 ---- */

			if (++time_3s < 3)
			{
				extern bool motion_detect_result_get(void);
				/*每各一秒查询ID状态*/
				network_cmd_data data;
				data.cmd = NET_COMMON_CMD_STREAM_STATUS;
				data.arg1 = ((fetch_compile_t.tm_mon + 1) << 4) | ((0 << 2) | motion_detect_result_get()) | (net_talk_state << 1); // 通知移动侦测结果
				data.arg2 = fetch_compile_t.tm_mday % 100;
				data.device = DEVICE_ALL;
				network_send_cmd_data(&data);
				// printf("SEND MOTION DETECT SIGNAL .........arg1 :%d arg2:%d\n\r",data.arg1,data.arg2);
			}
			else
			{
				time_3s = 0;

				void Compile_time_packet(struct tm t);
				void Default_unlock_time_packet(void);
				void Default_exit_button_packet(void);
				static char switch_packet = 0;
				switch_packet++;
				switch (switch_packet)
				{
				case 1:
					Compile_time_packet(fetch_compile_t);
					break;
				case 2:
					Default_exit_button_packet();
					break;
				case 3:
					Default_unlock_time_packet();
					switch_packet = 0;
					break;

				default:
					break;
				}
			}
		}
		// ak_sleep_ms(1);
	}

	ak_thread_exit();
	return NULL;
}

#include <time.h>
struct tm Fetch_compile_time(void)
{
	// 获取当前源文件的编译日期和时间
	const char *s_date = __DATE__;
	const char *s_time = __TIME__;

	// 解析日期时间字符串，获取对应的时间结构体
	struct tm t;
	strptime(s_date, "%b %d %Y", &t);
	strptime(s_time, "%H:%M:%S", &t);

	// printf("This program was compiled at %d.%d.%d\n", 1900+t.tm_year,t.tm_mon+1,t.tm_mday);
	return t;
}

void Default_unlock_time_packet(void)
{
	// printf("Default_unlock_time_packet lock:%d.gate:%d\n",user_data_get()->lock_unlock_time,user_data_get()->gate_unlock_time);

	network_cmd_data data;
	data.cmd = NET_COMMON_CMD_DEF_UNLOCK_TIME;
	data.arg1 = user_data_get()->lock_unlock_time;
	data.arg2 = user_data_get()->gate_unlock_time;
	data.device = DEVICE_ALL;
	network_send_cmd_data(&data);
	// printf("Compile_time_packet 0x%x.0x%x\n",arg2,arg1);
	return;
}

void Default_exit_button_packet(void)
{
	network_cmd_data data;
	data.cmd = NET_COMMON_CMD_EXIT_BUTTON_TIME;
	data.arg1 = user_data_get()->exit_button_lock;
	data.arg2 = user_data_get()->exit_button_gate1;
	data.device = DEVICE_ALL;
	network_send_cmd_data(&data);
	// printf("Default_exit_button_packet 0x%x.0x%x\n", data.arg2, data.arg1);
	return;
}

void Compile_time_packet(struct tm t)
{
	// printf("This program was compiled at %d.%d.%d\n", 1900+t.tm_year,t.tm_mon+1,t.tm_mday);
	char arg1 = (char)(((t.tm_mon + 1) * 100 + t.tm_mday) & 0xFF);
	char arg2 = (char)(((t.tm_year - 100) << 3) | (((t.tm_mon + 1) * 100 + t.tm_mday) >> 8));
	network_cmd_data data;
	data.cmd = NET_COMMON_CMD_COMPILE_TIME;
	data.arg1 = arg1;
	data.arg2 = arg2;
	data.device = DEVICE_ALL;
	network_send_cmd_data(&data);
	// printf("Compile_time_packet 0x%x.0x%x\n",arg2,arg1);
	return;
}

bool network_init(network_device device)
{
	ak_thread_mutex_init(&network_device_mutex, NULL);
	network_device_init(device);
	ak_pthread_t thread_id;
	ak_thread_create(&thread_id, network_cmd_receive_task, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, -1);
	ak_thread_detach(thread_id);
	return true;
}

char *nework_get_package_head(int type)
{
	static char *mac_local = NULL;
	if (mac_local == NULL)
	{
		struct ifreq req;
		int fd = socket(PF_INET, SOCK_DGRAM, 0);
		strcpy(req.ifr_name, NETWORK_NAME);
		ioctl(fd, SIOCGIFHWADDR, &req);
		close(fd);

		mac_local = malloc(60);
		memset(mac_local, 0, 60);
#if 1
		// 设置目的网卡地址
		mac_local[0] = 0x01;
		mac_local[1] = 0x01;
		mac_local[2] = 0x01;
		mac_local[3] = 0x01;
		mac_local[4] = 0x01;
		mac_local[5] = 0x01;
#endif
		memcpy(&mac_local[6], req.ifr_hwaddr.sa_data, 6);
		// mac_local[12] = 0x88;
	}
	mac_local[12] = type / 256;
	mac_local[13] = type % 256;
	return mac_local;
}

static void network_cmd_code_get(network_cmd_data *src, char *dst)
{
	network_cmd_data *device = (network_cmd_data *)src;

	dst[0] = NET_COMMON_CMD_START;
	dst[1] = network_local_device;
	dst[2] = device->device;
	dst[3] = device->cmd;
	dst[4] = device->arg1;
	dst[5] = device->arg2;
	dst[6] = (dst[1] + dst[2] + dst[3] + dst[4] + dst[5]) & 0xFF;
	dst[7] = NET_COMMON_CMD_END;
}

bool network_send_cmd_data(network_cmd_data *data)
{
	char package[68];
	memcpy(package, nework_get_package_head(ETH_P_CMD), 60);
	network_cmd_code_get(data, &package[60]);

	pthread_mutex_lock(&network_device_mutex);
	if (sendto(cmd_send_fd, package, sizeof(package), 0, (struct sockaddr *)&nework_local_sockaddr_ll, sizeof(nework_local_sockaddr_ll)) < 0)
	{
		perror("sendto fail ");
	}
	ak_thread_mutex_unlock(&network_device_mutex);
	ak_sleep_ms(1);
	return true;
}

//**********************************************************
static void network_addcardcmd_code_get(char *src, char *dst)
{
	dst[0] = NET_COMMON_CMD_START;
	dst[1] = network_local_device;
	dst[2] = DEVICE_INDOOR_ID1;
	dst[3] = NET_COMMON_CMD_ADD_CARDS;
	memcpy(&dst[4], src, USER_CARD_NUMBER_SIZE);
	dst[4 + USER_CARD_NUMBER_SIZE] = NET_COMMON_CMD_END;
}

bool network_sendaddcard_cmd_data(char *card_num)
{
	char package[65 + USER_CARD_NUMBER_SIZE];
	memcpy(package, nework_get_package_head(ETH_P_CMD), 60);
	network_addcardcmd_code_get(card_num, &package[60]);

	pthread_mutex_lock(&network_device_mutex);
	if (sendto(cmd_send_fd, package, sizeof(package), 0, (struct sockaddr *)&nework_local_sockaddr_ll, sizeof(nework_local_sockaddr_ll)) < 0)
	{
		perror("sendto fail ");
	}
	printf("=====================================>>> 已发送:[%02x %02x %02x %02x] \n", card_num[0], card_num[1], card_num[2], card_num[3]);
	ak_thread_mutex_unlock(&network_device_mutex);
	ak_sleep_ms(1);
	return true;
}
//**********************************************************

int network_common_socket_eth_p_audio_get(int slave_id)
{
#define SOCKET_ETH_P_BASE_AUDIO 0X2600
	int mstart_id = 0;
	switch (network_local_device)
	{
	case DEVICE_INDOOR_ID1:
		mstart_id = network_get_id_indoor_id1(network_local_device);
		break;
	case DEVICE_INDOOR_ID2:
		mstart_id = network_get_id_indoor_id2(network_local_device);
		break;
	case DEVICE_INDOOR_ID3:
		mstart_id = network_get_id_indoor_id3(network_local_device);
		break;
	case DEVICE_INDOOR_ID4:
		mstart_id = network_get_id_indoor_id4(network_local_device);
		break;
	case DEVICE_INDOOR_ID5:
		mstart_id = network_get_id_indoor_id5(network_local_device);
		break;
	case DEVICE_INDOOR_ID6:
		mstart_id = network_get_id_indoor_id6(network_local_device);
		break;
	case DEVICE_OUTDOOR_1:
		mstart_id = network_get_id_outdoor1(network_local_device);
		break;
	case DEVICE_OUTDOOR_2:
		mstart_id = network_get_id_outdoor2(network_local_device);
		break;
	case DEVICE_CCTV_1:
		mstart_id = network_get_id_cctv1(network_local_device);
		break;
	case DEVICE_CCTV_2:
		mstart_id = network_get_id_cctv2(network_local_device);
		break;
	default:
		break;
	}
	// printf("%s,0x%x,0x%x\n", __func__, mstart_id, slave_id);
	int vol = 0;
	if (mstart_id < slave_id)
	{
		vol = (slave_id << 4) | mstart_id;
	}
	else
	{
		vol = (mstart_id << 4) | slave_id;
	}
	return SOCKET_ETH_P_BASE_AUDIO | vol;
}

int network_common_socket_eth_p_video_get(int slave_id)
{
#define SOCKET_ETH_P_BASE_VIDEO 0X1600
	return SOCKET_ETH_P_BASE_VIDEO | slave_id;
}

int network_common_socket_eth_p_get(char type, int slave_id)
{
	/*
	 *type 1:代表音频，0:代表视频
	 */
	if (type == 1)
	{
		return network_common_socket_eth_p_audio_get(slave_id);
	}
	else if (type == 0)
	{
		return network_common_socket_eth_p_video_get(slave_id);
	}
	return 0;
}
