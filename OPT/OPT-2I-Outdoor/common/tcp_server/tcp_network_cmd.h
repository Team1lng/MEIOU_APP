#ifndef _TCP_NETWORK_CMD_H_
#define _TCP_NETWORK_CMD_H_

#include"stdbool.h"
#include"stdio.h"

#define SHORT_PACK_LEN 8
#define LONG_PACK_LEN   1288

#define SHORT_PACK_START 0xAA
#define LONG_PACK_START 0xBB
#define PACK_END 0xCC


#define NET_COMMON_CMD_ACK        0x01
#define NET_COMMON_CMD_ADD_FINGER        0x02
#define NET_COMMON_CMD_EXIT_FINGER        0x07
#define NET_COMMON_CMD_DEL_FINGER        0x03
#define NET_COMMON_CMD_VERIFY_FINGER        0x04
#define NET_COMMON_CMD_GET_FINGER        0x05
#define NET_COMMON_CMD_SET_FINGER_PERMISSION        0x06


#define NET_COMMON_CMD_ADD_CARD        0x10
#define NET_COMMON_CMD_EXIT_CARD        0x15
#define NET_COMMON_CMD_DEL_CARD        0x11
#define NET_COMMON_CMD_VERIFY_CARD        0x12
#define NET_COMMON_CMD_GET_CARD        0x13
#define NET_COMMON_CMD_SET_CARD_PERMISSION	0x14

#define NET_COMMON_CMD_ADD_PASSW        0x20
#define NET_COMMON_CMD_DEL_PASSW        0x21
#define NET_COMMON_CMD_GET_PASSW        0x22
#define NET_COMMON_CMD_SET_PASSW_PERMISSION        0x23

#define NET_COMMON_CMD_ACCESS_DENIED	0x90


#define LOCAL_DEVICE    1
typedef enum
{
	TCP_DEVICE_INDOOR_1 = 1,
	TCP_DEVICE_INDOOR_2,
	TCP_DEVICE_INDOOR_3,
	TCP_DEVICE_INDOOR_4,
	TCP_DEVICE_INDOOR_5,
	TCP_DEVICE_INDOOR_6,
	TCP_DEVICE_OUTDOOR_1,
	TCP_DEVICE_OUTDOOR_2,
	TCP_DEVICE_END,
	TCP_DEVICE_ALL = 0XFF,
	TCP_DEVICE_TOTAL
}tcp_device;

typedef struct
{
	char* str;
	unsigned char cmd;
	void(*proc)(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd);
}net_common_event_info;

typedef void (*tcp_callback)(unsigned int fd);

bool network_short_send_common(tcp_device send_to_devce,unsigned char cmd,unsigned char arg1,unsigned char arg2,unsigned int fd);

bool network_long_package_data(tcp_device send_to_device, unsigned char cmd,unsigned char *code,unsigned char arg,unsigned int fd);

void tcp_event_group_register(net_common_event_info*event_group,unsigned int size);

void tcp_server_init(void);

void tcp_client_close_register(tcp_callback callback);
#endif