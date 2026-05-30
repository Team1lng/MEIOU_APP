//
// Created by michael on 2022/4/9.
//

#ifndef NET_CAMERA_V330_AK_DRV_GPIO_H
#define NET_CAMERA_V330_AK_DRV_GPIO_H
#define FILE_PATH_MAX 64

#define NODE_DIRECTION      "direction"
#define NODE_VALUE          "value"
#define NODE_DRIVE          "drive"
#define NODE_PULL_POLARITY  "pull_polarity"
#define NODE_PULL_ENABLE    "pull_enable"
#define NODE_INPUT_ENABLE   "input_enable"
#define NODE_SLEW_RATE      "slew_rate"

#define DIRECTION_IN        "in"
#define DIRECTION_OUT       "out"

#define VALUE_HIGH          "1"
#define VALUE_LOW           "0"

#define DRIVE_0 "0"
#define DRIVE_1 "1"
#define DRIVE_2 "2"
#define DRIVE_3 "3"

#define PULL_UP     "pullup"
#define PULL_DOWN   "pull_polarity"

#define PULL_ENABLE     "1"
#define PULL_DISABLE    "0"

#define INPUT_ENABLE    "1"
#define INPUT_DISABLE   "0"

#define SLEW_FAST   "fast"
#define SLEW_SLOW   "slow"


int ak_drv_gpio_open(int gpio);

int ak_drv_gpio_close(int gpio);

int ak_drv_gpio_read(int gpio, char *node, char *result, int count);

int ak_drv_gpio_write(int gpio, char *node, char *value, int count);

#endif //NET_CAMERA_V330_AK_DRV_GPIO_H
