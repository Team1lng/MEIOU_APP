//
// Created by michael on 2022/4/9.
//
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include "ak_drv_gpio.h"
#include "ak_common.h"
#include "ak_log.h"



int ak_drv_gpio_open(int gpio) {
    int fd = -1;
    int ret = 0;
    char value[FILE_PATH_MAX] = {0};
    char path[FILE_PATH_MAX] = {0};

    memset(path, 0, FILE_PATH_MAX);
    sprintf(path, "/sys/class/gpio/gpio%d", gpio);

    if (access(path, F_OK)) {
        memset(path, 0, FILE_PATH_MAX);
        sprintf(path, "/sys/class/gpio/export");
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            ak_print_error(MODULE_ID_APP,"Open gpio export fail\n");
            return -1;
        }
        memset(value, 0, FILE_PATH_MAX);
        sprintf(value, "%d\n", gpio);
        ret = write(fd, value, strlen(value));
        if (ret < 0) {
            close(fd);
            ak_print_error(MODULE_ID_APP, "create gpio%d fail %d\n", gpio, ret);
            return -1;
        }
        close(fd);
        //ak_print_normal("open gpio%d successfully\n", gpio);
    } else {
        ak_print_normal(MODULE_ID_APP, "gpio%d already open\n", gpio);
    }

    return 0;
}

int ak_drv_gpio_close(int gpio) {
    int fd = -1;
    int ret = 0;
    char value[FILE_PATH_MAX] = {0};
    char path[FILE_PATH_MAX] = {0};

    memset(path, 0, FILE_PATH_MAX);
    sprintf(path, "/sys/class/gpio/gpio%d", gpio);

    if (!access(path, F_OK)) {
        memset(path, 0, FILE_PATH_MAX);
        sprintf(path, "/sys/class/gpio/unexport");
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            ak_print_error(MODULE_ID_APP, "Open gpio unexport fail\n");
            return -1;
        }
        memset(value, 0, FILE_PATH_MAX);
        sprintf(value, "%d\n", gpio);
        ret = write(fd, value, strlen(value));
        if (ret < 0) {
            close(fd);
            ak_print_error(MODULE_ID_APP, "remove gpio%d fail %d\n", gpio, ret);
            return -1;
        }
        close(fd);
    }

    //ak_print_normal("close gpio%d successfully\n", gpio);
    return 0;
}

int ak_drv_gpio_read(int gpio, char *node, char *result, int count) {
    int fd = -1;
    int ret = 0;
    char path[FILE_PATH_MAX] = {0};

    memset(path, 0, FILE_PATH_MAX);
    sprintf(path, "/sys/class/gpio/gpio%d/%s", gpio, node);
    //ak_print_normal("trying to open %s\n", path);

    if (!access(path, F_OK)) {
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            ak_print_error(MODULE_ID_APP, "open %s: %s\n", path, strerror(errno));
            return -1;
        }
        ret = read(fd, result, count);
        if (ret < 0) {
            close(fd);
            ak_print_error(MODULE_ID_APP, "read %s: %s\n", path, strerror(errno));
            return -1;
        }
        close(fd);
    }

    //ak_print_normal("close %s successfully\n", path);
    return 0;
}

int ak_drv_gpio_write(int gpio, char *node, char *value, int count) {
    int fd = -1;
    int ret = 0;
    char path[FILE_PATH_MAX] = {0};

    memset(path, 0, FILE_PATH_MAX);
    sprintf(path, "/sys/class/gpio/gpio%d/%s", gpio, node);
    //ak_print_normal("trying to open %s\n", path);

    if (!access(path, F_OK)) {
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            ak_print_error(MODULE_ID_APP, "open %s:%s\n", path, strerror(errno));
            return -1;
        }
        ret = write(fd, value, strlen(value));
        if (ret < 0) {
            close(fd);
            ak_print_error(MODULE_ID_APP, "write %s:%s %s\n", path, value, strerror(errno));
            return -1;
        }
        close(fd);
    }

    //ak_print_normal("close %s successfully\n", path);
    return 0;
}