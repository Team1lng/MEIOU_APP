#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "gpio_base.h"

/*
**** 检测GPIO是否已经导出
*/
static bool is_gpio_export(const int pin)
{
	char gpio_path[128] =
		{
			0};
	sprintf(gpio_path, "/sys/class/gpio/gpio%d", pin);

	if (access(gpio_path, F_OK) == 0)
	{
		return true;
	}

	return false;
}

/*
*** 导出GPIO
*/
bool gpio_export(const int pin)
{
	/***** 1.检测是否已经配置GPIO *****/
	if (is_gpio_export(pin) == true)
	{
		return true;
	}

	/***** 2.打开gpio export句柄 *****/
	int export_fd = open("/sys/class/gpio/export", O_WRONLY);

	if (export_fd < 0)
	{
		printf("export gpio%d failed \n", pin);
		return false;
	}

	/***** 3.写入pin到export句柄 *****/
	char export_buf[32] =
		{
			0};
	int len = sprintf(export_buf, "%d", pin);

	if (write(export_fd, export_buf, len) < 0)
	{
		close(export_fd);
		printf("write export gpio%d failed \n", pin);
		return false;
	}

	close(export_fd);
	return true;
}

/*
*** 设置GPIO输入/输出
*/
bool gpio_direction_set(const int pin, GPIO_DIR dir)
{
	if (gpio_export(pin) == false)
	{
		printf("export gpio%d faild \n", pin);
		return false;
	}

	char direction_buf[128] =
		{
			0};
	sprintf(direction_buf, "/sys/class/gpio/gpio%d/direction", pin);
	int direction_fd = open(direction_buf, O_WRONLY);

	if (direction_fd < 0)
	{
		printf("gpio%d direction setting failed \n", pin);
		return false;
	}

	char *direction[] = {[GPIO_DIR_IN] = "in", [GPIO_DIR_OUT] = "out", [GPIO_DIR_HIGH] = "high", [GPIO_DIR_LOW] = "low"};

	if (direction[dir])
	{
		memset(direction_buf, 0, sizeof(direction_buf));
		int len = sprintf(direction_buf, "%s", direction[dir]);

		if (write(direction_fd, direction_buf, len) < 0)
		{
			printf("diection write gpio%d %s faild \n", pin, direction_buf);
			close(direction_fd);
			return false;
		}
	}
	else
	{
		close(direction_fd);
		return false;
	}
	// printf("diection write gpio%d %s succeed!!! \n", pin, direction[dir]);
	close(direction_fd);
	return true;
}

/*
*** 设置gpio 内置上拉使能
*/
bool gpio_pull_enable(const int pin, bool enable)
{
	if (gpio_export(pin) == false)
	{
		printf("export gpio%d faild \n", pin);
		return false;
	}

	char gpio_pull_buf[128] =
		{
			0};
	sprintf(gpio_pull_buf, "/sys/class/gpio/gpio%d/pull_enable", pin);
	int gpio_pull_fd = open(gpio_pull_buf, O_WRONLY);

	if (gpio_pull_fd < 0)
	{
		printf("gpio%d pull_enable setting failed \n", pin);
		return false;
	}

	memset(gpio_pull_buf, 0, sizeof(gpio_pull_buf));
	int len = sprintf(gpio_pull_buf, "%s", enable ? "1" : "0");

	if (write(gpio_pull_fd, gpio_pull_buf, len) < 0)
	{
		printf("gpio pull write gpio%d %s faild \n", pin, gpio_pull_buf);
		close(gpio_pull_fd);
		return false;
	}

	close(gpio_pull_fd);
	return true;
}

/*
*** 设置IO口电平
*** 在设置IO电平之前,需要调用 gpio_direction_set
*/
bool gpio_level_set(const int pin, GPIO_LEVEL level)
{
	char gpio_buf[64] =
		{
			0};
	sprintf(gpio_buf, "/sys/class/gpio/gpio%d/value", pin);
	int gpio_fd = open(gpio_buf, O_WRONLY);

	if (gpio_fd < 0)
	{
		printf("gpio%d set value open failed \n", pin);
		return false;
	}

	if (write(gpio_fd, (level == GPIO_LEVEL_LOW) ? "0" : "1", 1) < 0)
	{
		printf("gpio%d write %d failed \n", pin, level == GPIO_LEVEL_LOW ? 0 : 1);
		close(gpio_fd);
		return false;
	}

	close(gpio_fd);
	return true;
}

/*
*** 读取IO电平
*** 在读取IO电平之前,需要调用 gpio_direction_set
*/
bool gpio_level_read(const int pin, GPIO_LEVEL *level)
{
	char gpio_buf[64] =
		{
			0};
	sprintf(gpio_buf, "/sys/class/gpio/gpio%d/value", pin);
	int gpio_fd = open(gpio_buf, O_RDONLY);

	if (gpio_fd < 0)
	{
		printf("gpio%d set value open failed \n", pin);
		return false;
	}

	memset(gpio_buf, 0, sizeof(gpio_buf));
	if (read(gpio_fd, gpio_buf, 1) < 0)
	{
		printf("gpio%d read failed \n", pin);
		close(gpio_fd);
		return false;
	}

	close(gpio_fd);

	*level = (gpio_buf[0] == '0') ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
	return true;
}

bool gpio_set(const int pin, GPIO_LEVEL level)
{
	if (gpio_export(pin) == false)
	{
		return false;
	}

	if (gpio_direction_set(pin, GPIO_DIR_OUT) == false)
	{
		return false;
	}

	if (gpio_level_set(pin, level) == false)
	{
		return false;
	}

	return true;
}

bool gpio_read(const int pin, GPIO_LEVEL *level)
{
	if (gpio_export(pin) == false)
	{
		return false;
	}

	if (gpio_direction_set(pin, GPIO_DIR_IN) == false)
	{
		return false;
	}

	if (level == NULL || gpio_level_read(pin, level) == false)
	{
		printf("gpio%d read level fail \n\r", pin);
		return false;
	}
	return true;
}
