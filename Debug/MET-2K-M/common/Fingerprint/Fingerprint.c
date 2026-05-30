/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-04 15:08:22
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-05 11:10:09
 * @FilePath: /project_3/common/Fingerprint/Fingerprint.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "Fingerprint.h"
#include "UartControl.h"
#include "GpioControl.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>
static int UartFd = -1;

static bool finger_data_package_read(FingerDataAck *receive_buffer)
{

    int UartBufferSize(int fd);
    int ret = UartBufferSize(UartFd);
    if (ret <= 0)
        return false;

    usleep(10000);
    memset(receive_buffer, 0, sizeof(FingerDataAck));
    int pack_format_len = sizeof(FingerHaed);
    int uart_read_len = 0;
    if ((uart_read_len = UartRead(UartFd, (char *)&(receive_buffer->format), pack_format_len)) == pack_format_len) // 读取包头至包长度的数据
    {
        if (PACKAGE_HEADER == (uint16_t)((receive_buffer->format.header[0] << 8) | receive_buffer->format.header[1]))
        {
            for (int i = 0; i < 4; i++)
            {
                if (receive_buffer->format.addr[i] != (/* (DEVICE_ADDR >> (i * 8)) &  */ 0xFF))
                    return false;
            }

            if (receive_buffer->format.type != DATA_PACKAGE && receive_buffer->format.type != ACK_PACKAGE && receive_buffer->format.type != END_PACKAGE)
                return false;
            uint16_t pack_len = (((uint16_t)(receive_buffer->format.len[0]) << 8) | (uint16_t)(receive_buffer->format.len[1] & 0xFF));

            if ((UartRead(UartFd, receive_buffer->format.type != ACK_PACKAGE ? (char *)receive_buffer->data_check : (char *)&(receive_buffer->affirm), pack_len)) != pack_len)
                return false;

            uint32_t check_sum = receive_buffer->format.type + pack_len + receive_buffer->affirm;

            uint16_t data_len = receive_buffer->format.type == ACK_PACKAGE ? pack_len - AFFIRM_CODE_LEN : pack_len;

            for (int i = 0; i < data_len - CHECK_CODE_LEN; i++)
            {
                check_sum += receive_buffer->data_check[i];
            }

            // printf("[ %x ][ %x ][ %x ]",check_sum,receive_buffer->data_check[data_len-2], receive_buffer->data_check[data_len-1]);
            if (check_sum == (((uint16_t)(receive_buffer->data_check[data_len - 2]) << 8) | (uint16_t)(receive_buffer->data_check[data_len - 1] & 0xFF)))
            {
                receive_buffer->check_sum[0] = receive_buffer->data_check[data_len - 2];
                receive_buffer->check_sum[1] = receive_buffer->data_check[data_len - 1];

                printf("RECEIVE DATA:");
                printf("[ %2x ]", receive_buffer->format.type);
                printf("[ %2x ][ %2x ]", receive_buffer->format.len[0], receive_buffer->format.len[1]);
                printf("[ %2x ]", receive_buffer->affirm);
                for (int i = 0; i < data_len - CHECK_CODE_LEN; i++)
                {
                    printf("[ %2x ]", receive_buffer->data_check[i]);
                }
                printf("[ %2x ][ %2x ]", receive_buffer->data_check[data_len - 2], receive_buffer->data_check[data_len - 1]);
                printf("\n");
                printf("=============================================\n");
                return true;
            }
            else
            {
                printf("RECEIVE DATA:");
                printf("[ %2x ]", receive_buffer->format.type);
                printf("[ %2x ][ %2x ]", receive_buffer->format.len[0], receive_buffer->format.len[1]);
                printf("[ %2x ]", receive_buffer->affirm);
                for (int i = 0; i < data_len - CHECK_CODE_LEN; i++)
                {
                    printf("[ %2x ]", receive_buffer->data_check[i]);
                }
                printf("[ %2x ][ %2x ]", receive_buffer->data_check[data_len - 2], receive_buffer->data_check[data_len - 1]);
                printf("\n");
                printf("=============================================\n");
            }
        }
    }
    else if (uart_read_len > 0)
    {
        printf("RECEIVE DATA ERROR LEN: %d ", uart_read_len);
        printf("[ %2x ][ %2x ]", receive_buffer->format.header[0], receive_buffer->format.header[1]);
        printf("[ %2x ][ %2x ][ %2x ][ %2x ]\n", receive_buffer->format.addr[0], receive_buffer->format.addr[1], receive_buffer->format.addr[2], receive_buffer->format.addr[3]);
        printf("\n");
        // printf("=============================================\n");
    }
    return false;
}

static void finger_uart_send_data(uint8_t package_type, uint8_t cmd, uint8_t *data, uint16_t data_size)
{

    uint16_t data_len = FINGER_BASE_DATA_LEN + data_size;
    uint8_t buffer[data_len];
    uint32_t check_sum = 0;
    buffer[0] = (PACKAGE_HEADER >> 8) & 0xFF;
    buffer[1] = PACKAGE_HEADER & 0xFF;
    buffer[2] = (DEVICE_ADDR >> 24) & 0xFF;
    buffer[3] = (DEVICE_ADDR >> 16) & 0xFF;
    buffer[4] = (DEVICE_ADDR >> 8) & 0xFF;
    buffer[5] = DEVICE_ADDR & 0xFF;
    buffer[6] = package_type;
    buffer[7] = ((data_size + 3) >> 8) & 0xFF;
    buffer[8] = (data_size + 3) & 0xFF;
    buffer[9] = cmd;
    check_sum = package_type + (data_size + 3) + cmd;
    for (int i = 0; i < data_size; i++)
    {
        buffer[10 + i] = data[i];
        check_sum += data[i];
    }
    buffer[data_len - 2] = (check_sum >> 8) & 0xFF;
    buffer[data_len - 1] = check_sum & 0xFF;
    UartWrite(UartFd, (char *)buffer, data_len);

    printf("\n\rsend data :");
    for (int i = 0; i < data_len; i++)
    {
        printf("[ %x ]", buffer[i]);
    }
    printf("\n");
}

bool empty_template_cmd(void)
{
    finger_uart_send_data(CMD_PACKAGE, CODE_EMPTY, NULL, 0);
    FingerDataAck receive_buffer;
    while (1)
    {
        if (finger_data_package_read(&receive_buffer))
        {
            if (receive_buffer.affirm == 0x00)
            {

                printf("清空指紋成功\n");

                return true;
            }
            else
            {
                break;
            }
        }
    }
    printf("清空指紋失败\n");
    return false;
}

static void *DrvFingerprintThread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    printf("empty_template_cmd:%s\n", empty_template_cmd() ? "succee" : "fail");
    while (1)
    {
        /* code */
    }
    return NULL;
}
/**
 * @description: 指纹驱动初始化
 * @return {*}
 */
int DrvFingerprintInit(void)
{
    UartFd = UartOpen("ttySAK1", 57600, 8, 1, 'n');
    if (UartFd < 0)
    {
        printf("open ttySAK0 faild \n");
        usleep(1000 * 1000);
        return false;
    }
    GpioOpen(55, GPIO_DIR_IN, 1);
    pthread_t Thread;
    pthread_create(&Thread, NULL, DrvFingerprintThread, NULL);
    pthread_detach(Thread);
    return 0;
}