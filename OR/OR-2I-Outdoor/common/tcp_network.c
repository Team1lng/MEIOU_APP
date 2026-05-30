#include"tcp_network_cmd.h"
#include"string.h"
#include"card_manage.h"
// #include"fingerprint.h"
#include"app_common.h"
#include"user_data.h"

// #define TCP_NETWORK_MANAGEMENT
// #define PASSWORD_MODE
#ifdef TCP_NETWORK_MANAGEMENT


#define FAIL_RET 9
#define CONTINUE_RET 0
#define SUCCEE_RET 1
#define SATUTS_SWICTH 2

static int curr_fingerprint_fd = -1;
static int curr_card_fd = -1;
static int curr_password_fd = -1;

#ifdef FINGERPRINT_MODE
static void net_send_finger_data(unsigned char arg1,unsigned fd)
{
    unsigned char card_data[1280];
    memset(card_data,0,sizeof(card_data));
    card_data[0] = get_fingerprint_table_info()->Number_fingerprints;
    memcpy(&card_data[1],(char *)(get_fingerprint_table_info()->fingerpriint_table),sizeof(get_fingerprint_table_info()->fingerpriint_table));
    network_long_package_data(LOCAL_DEVICE,NET_COMMON_CMD_GET_FINGER,card_data,arg1,fd);
}
#endif

#ifdef CARD_MODE
static void net_send_card_data(unsigned char arg1,unsigned fd)
{
    int i = 0;
    unsigned char buffer[USER_DEFINE_CARD_MAX][sizeof(card_data)];
    unsigned char data[1280];
    memset(data,0,sizeof(data));
    data[0] =  (uint8_t)(user_card_data_get()->card_number);
    for(;i<USER_DEFINE_CARD_MAX;i++)
    {
        memcpy(&buffer[i],(char *)&(user_card_data_get()->info[i]),sizeof(card_data));
    }
    int num = 0;
    for(i = 0;i<USER_DEFINE_CARD_MAX;i++)
    {
        if(buffer[i][0] != 0)
        {
            num++;
            printf("[%x][%x][%x][%x][%x][%x]================", buffer[i][0], buffer[i][1], buffer[i][2], buffer[i][3], buffer[i][4],buffer[i][5]);
            printf("[%x][%x][%x][%x][%x][%x]\n", user_card_data_get()->info[i].lock_type,user_card_data_get()->info[i].card[0],user_card_data_get()->info[i].card[1],
                        user_card_data_get()->info[i].card[2],user_card_data_get()->info[i].card[3],user_card_data_get()->info[i].card[4]);            
        }
        if(num == user_card_data_get()->card_number)
        {
            break;
        }
    }
    memcpy(&data[1],(char *)(buffer),sizeof(buffer));
    network_long_package_data(LOCAL_DEVICE,NET_COMMON_CMD_GET_CARD,data,arg1,fd);
}
#endif
#ifdef PASSWORD_MODE
static void net_send_password_data(unsigned char arg1,unsigned fd)
{
    int i = 0;
    unsigned char buffer[USER_CODE_GROUP][sizeof(user_code_info)];
    unsigned char data[1280];
    memset(data,0,sizeof(data));
    for(;i<USER_CODE_GROUP;i++)
    {
        memcpy(&buffer[i],(char *)&(user_data_get()->public_code[i]),sizeof(user_code_info));
        if(buffer[i][0] != 0)
        {
            data[0]++;
        }
    }
    int num = 0;
    for(i = 0;i<USER_CODE_GROUP;i++)
    {
        if(buffer[i][0] != 0)
        {
            num++;
            printf("[%x][%x][%x][%x][%x][%x]========%d========", buffer[i][0], buffer[i][1], buffer[i][2], buffer[i][3], buffer[i][4],buffer[i][5],data[0]);
            printf("[%x][%x][%x][%x][%x][%x]\n", user_data_get()->public_code[i].type,user_data_get()->public_code[i].password[0],user_data_get()->public_code[i].password[1],
                        user_data_get()->public_code[i].password[2],user_data_get()->public_code[i].password[3],user_data_get()->public_code[i].password[4]);            
        }
    }
    memcpy(&data[1],(char *)(buffer),sizeof(buffer));
    network_long_package_data(LOCAL_DEVICE,NET_COMMON_CMD_GET_PASSW,data,arg1,fd);
}
// #endif
static int net_send_access_denied(unsigned char arg1,unsigned char fd)
{
    network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_ACCESS_DENIED,arg1,0,fd);
    return 0;
}
#endif

/* ***********************************指纹管理命令处理********************************** */
#ifdef FINGERPRINT_MODE
/* ***********************************指纹管理命令处理********************************** */
static int add_finger_finish_backfunc(uint8_t arg1,uint8_t fd)
{
    if(arg1 == SUCCEE_RET)
    {
        network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_ADD_FINGER,arg1,0,fd);
        play_doorbell(RING_INDEX_KEYBOARD_BI2,7);
        // net_send_finger_data(arg1,fd);
    }
    else if(arg1 == FAIL_RET)
    {
        network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_ADD_FINGER,arg1,0,fd);
        play_doorbell(RING_INDEX_KEYBOARD_BI4,7);
    }
    else if(arg1 == SATUTS_SWICTH)
    {
        finger_event_node_register(0,0,STANDBY_STATUS,NULL,fd);
        network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_ADD_FINGER,arg1,0,fd);
        // play_doorbell(RING_INDEX_KEYBOARD_BI4,7);
    }
    return 0;
}

static int del_finger_finish_backfunc(uint8_t arg1,uint8_t fd)
{
    if(arg1 == SUCCEE_RET)
    {
        finger_event_node_register(0,0,STANDBY_STATUS,NULL,fd);
        network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_DEL_FINGER,arg1,0,fd);
        play_doorbell(RING_INDEX_KEYBOARD_BI2,7);
        // net_send_finger_data(arg1,fd);
    }
    else if(arg1 == FAIL_RET)
    {
        network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_DEL_FINGER,arg1,0,fd);
        play_doorbell(RING_INDEX_KEYBOARD_BI4,7);
    }
    else if(arg1 == SATUTS_SWICTH)
    {
        finger_event_node_register(0,0,STANDBY_STATUS,NULL,fd);
        network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_DEL_FINGER,arg1,0,fd);
    }
    return 0;
}

static void net_common_ack_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    
}
static void net_common_add_finger_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    // printf("%s===============================>>>%d\n",__func__,__LINE__);
    finger_event_node_register(arg1,arg2,ADD_FINGER_STATUS,add_finger_finish_backfunc,fd);
}
static void net_common_exit_finger_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    printf("%s===============================>>>%d\n",__func__,__LINE__);
    finger_event_node_register(0,0,STANDBY_STATUS,NULL,fd);

    play_doorbell(RING_INDEX_KEYBOARD_BI3,7);
}
static void net_common_del_finger_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    printf("%s===============================>>>%d:%d\n",__func__,arg1,arg2);
    finger_event_node_register(arg1,arg2,DEL_FINGER_STATUS,del_finger_finish_backfunc,fd);
}
static void net_common_verify_finger_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    // printf("%s===============================>>>%d\n",__func__,__LINE__);
    network_short_send_common(LOCAL_DEVICE,NET_COMMON_CMD_VERIFY_FINGER,0,0,fd);
}
static void net_common_get_finger_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    printf("curr_fingerprint_fd :%d   fd:%d\n",curr_fingerprint_fd,fd);
    if(curr_fingerprint_fd == -1 ||  curr_fingerprint_fd == fd)
    {
        curr_fingerprint_fd = fd;
        net_send_finger_data(arg1,fd);
    }
    else
        net_send_access_denied(arg1,fd);
}
static void net_common_set_finger_permission_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    set_fingerprint_permission(arg1,arg2);
}
#endif
/* *************************************************************************************** */




/* ***********************************卡片管理命令处理********************************** */
#ifdef CARD_MODE
static void tcp_network_cmd_add_card_success_event_register(unsigned int fd)
{
    // sock_fd = fd;
}
bool tcp_network_cmd_add_card_result_send(bool result)
{
    printf("tcp_network_cmd_add_card_result_send:%d\n",result);
    if(/* sock_fd */curr_card_fd < 0){
        return false;
    }else{
        return network_short_send_common(LOCAL_DEVICE, NET_COMMON_CMD_ADD_CARD, result ? SUCCEE_RET : FAIL_RET, 0, /* sock_fd */curr_card_fd);
    }
}
bool tcp_network_cmd_exit_add_card_send(void)
{
    if(/* sock_fd */curr_card_fd < 0){
        return false;
    }else{
        return network_short_send_common(LOCAL_DEVICE, NET_COMMON_CMD_EXIT_CARD, 0, 0, /* sock_fd */curr_card_fd);
    }
}

static void net_common_add_card_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(curr_card_fd != fd)
        return;

    card_event_trigger(CARD_EVENT_START_ADD_CARD,0x00,0x00);
    tcp_network_cmd_add_card_success_event_register(fd);
    printf("%s===============================>>>%d\n", __func__, __LINE__);
}
static void net_common_exit_card_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(curr_card_fd != fd)
        return;

    card_event_trigger(CARD_EVENT_STOP_ADD_CARD,0x00,0x00);
    tcp_network_cmd_add_card_success_event_register(-1);
    printf("%s===============================>>>%d\n", __func__, __LINE__);
}
static void net_common_del_card_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(card_data_delete(arg1))
    {
        play_doorbell(RING_INDEX_KEYBOARD_BI2,7);
    }
    else
    {
        play_doorbell(RING_INDEX_KEYBOARD_BI3,7);
    }
        
}
static void net_common_verify_card_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    // printf("%s===============================>>>%d\n",__func__,__LINE__);
}
static void net_common_get_card_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(curr_card_fd == -1 ||  curr_card_fd == fd)
    {
        curr_card_fd = fd;
        net_send_card_data(arg1, fd);
    }
    else
        net_send_access_denied(arg1, fd);

}
static void net_common_set_card_permission_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    set_card_permission(arg1, arg2);
}
#endif
/* *************************************************************************************** */

/* ***********************************密码管理命令处理********************************** */
#ifdef PASSWORD_MODE
bool tcp_network_cmd_add_password_result_send(bool result)
{
    printf("tcp_network_cmd_add_password_result_send:%d\n",result);
    if(/* sock_fd */curr_password_fd < 0){
        return false;
    }else{
        return network_short_send_common(LOCAL_DEVICE, NET_COMMON_CMD_ADD_PASSW, result ? SUCCEE_RET : FAIL_RET, 0, /* sock_fd */curr_password_fd);
    }
}

static void net_common_add_password_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(curr_password_fd != fd)
        return;

    /* 这里乘10是因为密码只需要四位，乘10凑够五位 */
    int password = ((int)(arg1<<8)|arg2)*10;
    user_code_add(password);
    net_send_password_data(arg1, fd);
    printf("%s===============================>>>%d\n", __func__, __LINE__);
}
static void net_common_del_password_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(curr_password_fd != fd)
        return;
    user_code_del(arg1);
    play_doorbell(RING_INDEX_KEYBOARD_BI2,7);
    printf("%s===============================>>>%d\n", __func__, arg1);
        
}
static void net_common_get_password_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(curr_password_fd == -1 ||  curr_password_fd == fd)
    {
        curr_password_fd = fd;
        net_send_password_data(arg1, fd);
    }
    else
        net_send_access_denied(arg1, fd);
    printf("%s===============================>>>%d\n", __func__, __LINE__);
}
static void net_common_set_password_permission_func(tcp_device device,unsigned char cmd,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
    if(curr_password_fd != fd)
        return;
    user_code_permission_set(arg1, arg2);
    printf("%s===============================>>>%d\n", __func__, __LINE__);
}
#endif
/* *************************************************************************************** */

static net_common_event_info net_common_event[]  = {

#ifdef FINGERPRINT_MODE
	{"add finger status", NET_COMMON_CMD_ADD_FINGER,net_common_add_finger_func},

	{"exit finger status", NET_COMMON_CMD_EXIT_FINGER,net_common_exit_finger_func},
	
	{"del finger status", NET_COMMON_CMD_DEL_FINGER,net_common_del_finger_func},

	{"verify finger status", NET_COMMON_CMD_VERIFY_FINGER,net_common_verify_finger_func},

	{"get finger info", NET_COMMON_CMD_GET_FINGER,net_common_get_finger_func},

	{"set finger permission", NET_COMMON_CMD_SET_FINGER_PERMISSION,net_common_set_finger_permission_func},
#endif
#ifdef CARD_MODE
	{"add card status", NET_COMMON_CMD_ADD_CARD,net_common_add_card_func},

	{"exit card status", NET_COMMON_CMD_EXIT_CARD,net_common_exit_card_func},
	
	{"del card status", NET_COMMON_CMD_DEL_CARD,net_common_del_card_func},

	{"verify card status", NET_COMMON_CMD_VERIFY_CARD,net_common_verify_card_func},

	{"get card info", NET_COMMON_CMD_GET_CARD,net_common_get_card_func},

	{"set card permission", NET_COMMON_CMD_SET_CARD_PERMISSION,net_common_set_card_permission_func},
#endif
#ifdef PASSWORD_MODE
	{"add passwrod status", NET_COMMON_CMD_ADD_PASSW,net_common_add_password_func},
	
	{"del passwrod status", NET_COMMON_CMD_DEL_PASSW,net_common_del_password_func},

	{"get passwrod info", NET_COMMON_CMD_GET_PASSW,net_common_get_password_func},

	{"set passwrod permission", NET_COMMON_CMD_SET_PASSW_PERMISSION,net_common_set_password_permission_func},
#endif
};

void tcp_client_close_callback(unsigned int fd)
{
    if(curr_fingerprint_fd == fd)
    {
        printf("tcp_client_close_callback  =====>>\n");
        curr_fingerprint_fd = -1;
    }
    else if(curr_card_fd == fd)
    {
        curr_card_fd = -1;
    }
    else if(curr_password_fd == fd)
    {
        curr_password_fd = -1;
    }
    
}

void tcp_netwrok_init(void)
{
    tcp_server_init();
    tcp_client_close_register(tcp_client_close_callback);
    tcp_event_group_register(net_common_event,sizeof(net_common_event)/sizeof(net_common_event_info));
}

#endif