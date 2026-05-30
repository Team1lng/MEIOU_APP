#ifndef _NETWORK_COMMON_H_
#define _NETWORK_COMMON_H_
#include <stdbool.h>
#include "stdlib.h"
#include "sys/time.h"
#include "user_data.h"

#define NETWORK_NAME "eth0"

static inline unsigned long long get_sys_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#define network_get_id_indoor_id1(x) ((x & 0x00) | (0x0))
#define network_get_id_indoor_id2(x) ((x & 0x00) | (0x1))
#define network_get_id_indoor_id3(x) ((x & 0x00) | (0x2))
#define network_get_id_indoor_id4(x) ((x & 0x00) | (0x3))
#define network_get_id_indoor_id5(x) ((x & 0x00) | (0x4))
#define network_get_id_indoor_id6(x) ((x & 0x00) | (0x5))

#define network_get_id_outdoor1(x) ((x & 0x00) | (0x6))
#define network_get_id_outdoor2(x) ((x & 0x00) | (0x7))

#define network_get_id_cctv1(x) ((x & 0x00) | (0x8))
#define network_get_id_cctv2(x) ((x & 0x00) | (0x9))

#define UPGRADE_BIG_DATA 1
#define UPGRADE_DATA_LEN 512

#if UPGRADE_BIG_DATA

#define UPGRADE_PACK_LEN (UPGRADE_DATA_LEN * 2)

#else

#define UPGRADE_PACK_LEN (UPGRADE_DATA_LEN * 1)

#endif

typedef enum
{
	DEVICE_UNKONW,
	DEVICE_INDOOR_ID1 = 1,
	DEVICE_INDOOR_ID2,
	DEVICE_INDOOR_ID3,
	DEVICE_INDOOR_ID4,
	DEVICE_INDOOR_ID5,
	DEVICE_INDOOR_ID6,
	DEVICE_OUTDOOR_1,
	DEVICE_OUTDOOR_2,
	DEVICE_CCTV_1,
	DEVICE_CCTV_2,

	DEVICE_ALL = 0XFF,
	DEVICE_TOTAL
} network_device;

#define COMMON_CMD_LEN 8

/*
 *	arg1: 1:查询ID状态
 *
 *		  2:收到ID状态
 *
 *			   arg2:最高位1表示id，冲突，低4位表示冲突的ID号
 *					最高位0:表示该id存在，但不冲突
 */
#define NET_COMMON_CMD_LIGHT 0x53

#define NET_COMMON_CMD_UNLOCK 0x54

#define NET_COMMON_CMD_ID_REPEAT 0x55

#define NET_COMMON_CMD_OUTDOOR_CALL 0X56
#define NET_COMMON_CMD_OUTDOOR_TALK 0X57
#define NET_COMMON_CMD_OUTDOOR_HANG 0X75

/*
 *	arg1 = 1:呼叫
 *   arg1 = 2:有设备应答呼叫
 *	arg1 = 3:有设备接受通话
 *   arg1 = 4:设备挂断正在通话
 *	arg1 = 5:设备正忙
 */
#define NET_COMMON_CMD_INTERCOM_CALL 0X58

#define NET_COMMON_CMD_STREAM_STATUS 0X59
#define NET_COMMON_CMD_DEVICE_BUSY 0X60
#define NET_COMMON_CMD_MOTION 0X61
#define NET_COMON_CMD_UPGRADE_OUTDOOR 0x62

#define NET_COMON_CMD_MOTION_SENSITIVITY 0x63

#define NET_COMMON_CMD_ADD_CARDS 0x64

#define NET_COMON_CMD_OUTDOOR_RESET 0x65

#define NET_COMMON_CMD_CARD_DATA_SYNC 0x66

#define NET_COMMON_PARAM_RECORD_MESSAGE (1 << 1)

#define NET_COMMON_CMD_COMPILE_TIME 0X70

#define NET_COMMON_CMD_DEF_UNLOCK_TIME 0X71

#define NET_COMMON_CMD_EXIT_BUTTON_TIME 0x72
typedef struct
{
	network_device device;
	char cmd;
	char arg1;
	char arg2;
} network_cmd_data;

typedef struct
{
	network_device device;
	char cmd;
	char arg1;
	char arg2;
	char buf[CARD_DATA_TOTAL_SIZE];
} network_card_data;

#define netwrok_cmd_create(a, b, c, d) { \
	.device = a,                         \
	.cmd = b,                            \
	.arg1 = c,                           \
	.arg2 = d};

typedef struct
{
	bool open_message;
	unsigned long message_recv_t; // 接受到留言指令时间
	char message_language;
} outdoor_message_status; // 门口机留言状态
outdoor_message_status message_status;

bool net_talk_state_get(void);

bool network_init(network_device device);
void network_local_device_set(network_device device);
network_device network_local_device_get(void);

bool network_send_cmd_data(network_cmd_data *package);

bool network_video_send_package_open(unsigned long id);
bool network_video_send_package_push(char type, const char *data, int len);
bool network_video_send_package_close(void);

bool is_network_video_send_package_open(void);
void network_video_send_package_start(void);
void network_video_send_package_stop(void);

bool network_video_receive_package_open(unsigned long id);
bool network_video_receive_package_close(void);

bool network_audio_send_package_open(unsigned long id);
bool network_audio_send_package_close(void);
bool network_audio_send_package_push(char type, const char *data, int len);

bool is_network_audio_send_package_open(void);
void network_audio_send_package_start(void);
void network_audio_send_package_stop(void);

bool network_audio_receive_package_open(unsigned long);
bool network_audio_receive_package_close(void);
bool is_network_audio_receive_connected(void);

int network_common_socket_eth_p_get(char type, int slave_id);

bool network_sendaddcard_cmd_data(char *card_num);

#endif
