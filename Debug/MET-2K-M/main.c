#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_EVENTS 5
#define GPIO_PATH "/sys/class/gpio/gpio26/value"

int main()
{
    int epoll_fd, gpio_fd;
    struct epoll_event event, events[MAX_EVENTS];

    system("echo both > /sys/class/gpio/gpio26/edge");

    // 打开GPIO值文件
    gpio_fd = open(GPIO_PATH, O_RDONLY | O_NONBLOCK);
    if (gpio_fd < 0)
    {
        perror("Failed to open GPIO value file");
        exit(EXIT_FAILURE);
    }

    // 创建epoll实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        perror("Failed to create epoll instance");
        close(gpio_fd);
        exit(EXIT_FAILURE);
    }

    // 配置epoll事件
    event.events = EPOLLPRI | EPOLLET; // 边缘触发模式
    event.data.fd = gpio_fd;

    // 添加GPIO文件描述符到epoll
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gpio_fd, &event) < 0)
    {
        perror("Failed to add GPIO fd to epoll");
        close(gpio_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // 读取初始值（必须执行以清除初始状态）
    char buf;
    read(gpio_fd, &buf, 1);
    lseek(gpio_fd, 0, SEEK_SET);

    printf("Waiting for GPIO events...\n");

    while (1)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++)
        {
            if (events[i].data.fd == gpio_fd)
            {
                read(gpio_fd, &buf, 1);
                lseek(gpio_fd, 0, SEEK_SET);
                printf("GPIO event detected! Value: %c\n", buf);
            }
        }
    }

    close(gpio_fd);
    close(epoll_fd);
    return 0;
}