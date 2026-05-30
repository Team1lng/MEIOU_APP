#! /bin/sh

driver_reload(){
    killall wpa_supplicant
    ps | pgrep -f 'udhcpc.*wlan0.*' | xargs kill    # 清除所有udhcpc wlan0 相关线程, 由于该项目有多网卡, 所以需要精确定位wlan0

    if [ -f "/usr/modules/hi3881.ko" ]; then  
        rmmod hi3881
        rmmod ak_mci
        rmmod cfg80211

        sleep 1

        modprobe cfg80211.ko
        insmod /usr/modules/ak_mci.ko
        insmod /usr/modules/hi3881.ko
    fi

    if [ -f "/usr/modules/wifi_soc.ko" ]; then  
        rmmod wifi_soc
        rmmod plat_soc
        rmmod ak_mci
        sleep 1
        insmod /usr/modules/ak_mci.ko
        insmod /usr/modules/plat_soc.ko
        insmod /usr/modules/wifi_soc.ko
    fi

    sleep 1

    wpa_supplicant -Dnl80211 -i wlan0 -c /etc/config/wpa_supplicant.conf -B

    sleep 1

    udhcpc -i wlan0

}

driver_reload &     # 需要放在后台运行, 防止客户端调用时造成堵塞



