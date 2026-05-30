<!--
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-02-19 08:42:02
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-11 08:37:23
 * @FilePath: /two-wire-indoor/README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# two-wire-indoor


#### 介绍
美欧两线室内机

#### 软件架构
软件架构说明


#### 安装教程

1.  xxxx
2.  xxxx
3.  xxxx

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)

#### 修复问题：
1. wifi功能掉线处理
2. cctv涂鸦切换异常，cctv线程未完全关闭导致下次开启失败
3. 自定义铃声切换显示异常
4. 室内机cctv通道切换显示效果异常，cctv分辨率与其他通道不一致且未重启解码导致

#### 20240718
1.降低铃声播放最低音，拉高差值


## 20240929
1.优化wifi连接卡顿问题，具体原因三调用system时未后台执行，导致需要等待命令结束才会回到主进程中，导致卡顿
2.优化wifi配置文件wpa_supplicant丢失问题，etc/config目录下的wpa_supplicant文件只做拷贝覆盖，不直接删除，间接避免删除后拷贝新的配置文件失败，导致文件丢失从而wifi无法搜索
3.wifi切换连接失败问题，原因:wifi连接时状态判断未根据节点状态得来，而是根据一个wifi_connect_flag标志位，将连接成功与否条件更改成wifi_connection_status_sucess函数即可
4.修改待机息屏时屏幕未完全刷黑问题，在息屏时清除所有的对象，其次wifi图标目前显示被背光状态影响，因此在进入home界面时优先打开背光，再进行控件创建工作，保证wifi图标能及时刷新
5.新添加reboot重启前重置LCD复位脚及关闭背光灯

## 20241031
1.添加看门狗
2.修复涂鸦CCTV切换异常，将RTSP流线程设置为阻塞关闭

## 20241204
1.将看门狗移植至用户数据处理线程，避免RTSP流导致主线程阻塞从而导致看门狗重启
2.避免升级过程被看门狗重启导致升级失败，OTA升级关闭看门狗；

## 20241211
3.优化息屏处理，息屏时不删除所有控件，创建一个全屏且黑色的控件进行覆盖

## 20250116
1.修复SD卡分区内存不足时，清除内存操作异常，导致循环进入清除操作，占用大量CPU资源，其原因是代码函数SD_card_space_clear在做清除数据类型判断时，未添加else，导致在某个判断成立后，指定删除的类型未退出判断代码，进行下一步的其他类型选择；解决方案：在每次判断选择清除数据的类型后添加else，避免if过后继续执行下一段代码
2.优化待机息屏时，屏幕未完全刷黑屏,关闭息屏时右上提示小图标刷新，进入home界面后优先点亮背光 
3.修复视频播放频繁切换后播放异常，不可重新进入播放状态，播放按键无效果，根源：视频播放线程频繁销毁，导致异常，线程出错；解决：将线程销毁更改成阻塞释放
4.修复关闭移动侦测预览状态下，移动侦测录像时进入移动侦测录制文件列表界面，存在偶尔丢失当前录制文件问题，其根源是不同线程对文件操作函数接口的调用，同时访问操作一个缓存数组，导致冲突，在其数据修改后，因其他线程操作导致旧数据重新覆盖掉新数据，因此丢失当前的录制文件；目前修改方式：在进行扫描SD卡文件时，添加判断是否进行视频录制中，当进行录制则等待录制完毕，并优化相关UI显示逻辑
5.优化待机黑屏问题，添加新函数接口standby_black_screen，替换待机界面的backlight_open函数调用
6.添加wifi设置界面涂鸦手动解绑功能
7.视频解码修改为join阻塞关闭，避免detach分离关闭未及时释放导致下次开启解码异常
8.视频解码修改为join阻塞关闭，避免detach分离关闭未及时释放导致下次开启解码异常


##20250818
1.同步tuya重连推送开锁问题