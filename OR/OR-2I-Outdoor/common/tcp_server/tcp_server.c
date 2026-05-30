#include <pthread.h>
#include "tcp_socket.h"
#include"tcp_network_cmd.h"
#include"ak_thread.h"



static tcp_callback client_close_callback = NULL;

void tcp_client_close_register(tcp_callback callback)
{
    client_close_callback = callback;
}

bool network_short_send_common(tcp_device send_to_devce,unsigned char cmd,unsigned char arg1,unsigned char arg2,unsigned int fd)
{
	unsigned char code[SHORT_PACK_LEN];
	code[0] = SHORT_PACK_START;
	code[1] = LOCAL_DEVICE;
	code[2] = send_to_devce;
	code[3] = cmd;
	code[4] = arg1;
	code[5] = arg2;
	code[6] = (code[1] + code[2] + code[3] + code[4] + code[5])&0xFF;
	code[7] = PACK_END;
    int send_len = tcp_send(fd, code,SHORT_PACK_LEN);
    if (send_len <= 0)
    {
        printf("tcp_send cmd 0x%x error!\n",cmd);
        return false;
    }

        printf("tcp_send cmd 0x%x succeed!\n",cmd);
    return true;
}

bool network_long_package_data(tcp_device send_to_device, unsigned char cmd,unsigned char *code,unsigned char arg,unsigned int fd)
{
    unsigned char data[LONG_PACK_LEN] = {0};
    memcpy(&data[6],code,LONG_PACK_LEN - 7);
    data[0] = LONG_PACK_START;
    data[1] = LOCAL_DEVICE;
    data[2] = send_to_device;
	data[3] = cmd;
    data[4] = arg;
	data[LONG_PACK_LEN -1 ] = PACK_END;
    int send_len = tcp_send(fd, data,LONG_PACK_LEN);
    if (send_len <= 0)
    {
        printf("tcp_send cmd 0x%x error!\n",cmd);
        return false;
    }
        printf("tcp_send cmd 0x%x succeed!\n",cmd);
    return true;
}

static net_common_event_info *net_common_event = NULL;
static unsigned int net_event_len = 0;
void tcp_event_group_register(net_common_event_info *event_group,unsigned int size)
{
    net_common_event = event_group;
    net_event_len = size;
}

static bool net_common_event_process(tcp_device device,unsigned char cmd ,unsigned char arg1,unsigned char arg2,char *data,unsigned int fd)
{
	// printf("net_common_event_process:%d\n",__LINE__);

    if(net_common_event == NULL)
    {
        return false;
    }

	int i = 0;
	for(i = 0; i < net_event_len ; i++)
	{
		if(net_common_event[i].cmd == cmd)
		{
			//device_info_display(device_info);
			printf("receive dev :%d ,cmd :%s\n",device,net_common_event[i].str);
			net_common_event[i].proc(device,cmd,arg1,arg2,data,fd);
			break;
		}
	}
	return true;
}

bool net_common_code_check_valid(char* buffer,int ret)
{
	/*先判断起始码和结束码是否一致*/
	if(((buffer[0] != SHORT_PACK_START)&&(buffer[0] != LONG_PACK_START))||
		((buffer[ret -1] != PACK_END)&&(buffer[ret -1] != PACK_END)))
	{
		return false;
	}

	/*判断设备是否正确*/

	if((buffer[2] != LOCAL_DEVICE)&&(buffer[2] != TCP_DEVICE_ALL))
	{
		return false;
	}

	// if((buffer[1] == device_info.device))//必须屏蔽，否则无法识别冲突
	// {
	// 	return false;
	// }

	return true;
}

typedef struct 
{
    int tcp_fd;
    ak_pthread_t tid;
}process_client_t;

static process_client_t cli_data_proce_thread_tid[MAX_CONNECT_NUM];

static  int server_fd = -1;
static bool tcp_server_run = false;

static void *process_client_data(void *arg)
{
    // int client_fd = *(int*)arg;
    process_client_t *client_t = (process_client_t*)arg;
    printf("%s=================>>%d:%d\n",__func__,__LINE__,client_t->tcp_fd);
    while (tcp_server_run)
    {
        
        char buf[LONG_PACK_LEN] = {0};
        #if 0
        int recv_len = tcp_blocking_recv(client_fd, buf, sizeof(buf));
        #else
        int recv_len = tcp_nonblocking_recv(client_t->tcp_fd, buf, sizeof(buf),0,5000);
        #endif
        
        if (recv_len == 0 || !tcp_server_run)
        {
            if(client_close_callback != NULL)
                client_close_callback(client_t->tcp_fd);

            printf("Client_t %d Exit Connection!!!!\n",client_t->tcp_fd);
            tcp_close(client_t->tcp_fd);
            client_t->tid = -1;


            ak_thread_exit();
            return NULL;
        }
        // printf("recv_len ============>>%d\n",recv_len);
        if(net_common_code_check_valid(buf,recv_len))
        {
            printf("CLIENT_FD = %d\n", client_t->tcp_fd);
            if((buf[0] == SHORT_PACK_START))
                net_common_event_process(buf[1],buf[3],buf[4],buf[5],NULL,client_t->tcp_fd);
            else if((buf[0] == LONG_PACK_START))
                net_common_event_process(buf[1],buf[3],buf[4],buf[5],buf,client_t->tcp_fd);
        }
        usleep(10 * 1000);
    }

    if(client_close_callback != NULL)
        client_close_callback(client_t->tcp_fd);

    tcp_close(client_t->tcp_fd);
    client_t->tid = -1;
    ak_thread_exit();
    return NULL;
}


static int fine_cli_proce_empty(void)
{
    int index = 0;
    for(;index < MAX_CONNECT_NUM;index ++)
    {
        if(cli_data_proce_thread_tid[index].tid == -1)
        {
            return index;
        }
    }
    return -1;
} 

void *tcp_server_accept_detect(void*arg)
{
    printf("%s=================>>%d:%d\n",__func__,__LINE__,tcp_server_run);
    unsigned int cli_data_proce_index = 0;
    memset(cli_data_proce_thread_tid,-1,sizeof(cli_data_proce_thread_tid));
    while(tcp_server_run)
    {
        int new_fd = tcp_accept(server_fd);

        // printf("%s=================>>%d\n",__func__,__LINE__);
        // 创建客户端数据处理线程
        if(new_fd > 0 && (cli_data_proce_index = fine_cli_proce_empty()) != -1)
        {
            cli_data_proce_thread_tid[cli_data_proce_index].tcp_fd = new_fd;
            printf("%s=================>>>>>>%d:%d\n",__func__,new_fd,cli_data_proce_index);
            ak_thread_create(&(cli_data_proce_thread_tid[cli_data_proce_index].tid), process_client_data, (void*)&(cli_data_proce_thread_tid[cli_data_proce_index]), 20*1024, -1);      
        }

        usleep(1000*10);
    } 
    
    for(int i = 0 ;i < MAX_CONNECT_NUM ;i ++)
    {
        if(cli_data_proce_thread_tid[i].tid != -1)
        {
            ak_thread_join(cli_data_proce_thread_tid[i].tid);
        }
        usleep(1000);
    }

    tcp_close(server_fd);
    server_fd = -1;
    ak_thread_exit();
    return NULL;
}


void tcp_server_init(void)
{
    if(tcp_server_run)
    {
        return ;
    }

    if(server_fd == -1)
    {
        server_fd= tcp_init(NULL, 4321);
        printf("==================tcp server==================\n");
    }
    tcp_server_run = true;
	ak_pthread_t thread_id;
    ak_thread_create(&thread_id, tcp_server_accept_detect, NULL,500*1024, -1);
	ak_thread_detach(thread_id);
}


void tcp_server_close(void)
{
    if(server_fd != -1 && tcp_server_run)
    {
        tcp_server_run = false;
    }
}