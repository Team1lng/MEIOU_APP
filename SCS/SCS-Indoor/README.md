<!--
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-02-19 08:42:02
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-23 11:07:24
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
1.删除SCS刷卡管理

## 20240911
1.SE版本关闭wifi掉线检测
2.添加网络设置tuya解绑功能

## 20240919
1.优化wifi掉线处理

## 20240928
1.修复wifi连接失败问题。原因是不同线程进行wifi操作，可能导致操作失败，因此在wifi设置的相关界面，暂时关闭对wifi掉线的检测 2.修复wifi配置文件丢失问题，将删除配置文件命令去掉，该用tmp目录下文件拷贝覆盖

## 20241015
1.修改待机息屏时屏幕未完全刷黑问题，在息屏时清除所有的对象，其次wifi图标目前显示被背光状态影响，因此在进入home界面时优先打开背光，再进行控件创建工作，保证wifi图标能及时刷新
2.优化wpa_supplicant -Dnl80211 -i wlan0连接wifi操作，将命令写入字符缓冲后再由system调用

## 20241101
1.用户数据处理线程添加看门狗

## 20241111
1.修改tuya监控场景，tuya监控两分钟超时自动退出监控状态，各个标志位清除
2.OTA升级不杀死应用层，避免升级过程被看门狗重启导致升级失败

## 20250116
1.修复SD卡分区内存不足时，清除内存操作异常，导致循环进入清除操作，占用大量CPU资源，其原因是代码函数SD_card_space_clear在做清除数据类型判断时，未添加else，导致在某个判断成立后，指定删除的类型未退出判断代码，进行下一步的其他类型选择；解决方案：在每次判断选择清除数据的类型后添加else，避免if过后继续执行下一段代码
2.优化待机息屏时，屏幕未完全刷黑屏,关闭息屏时右上提示小图标刷新，进入home界面后优先点亮背光 
3.修复视频播放频繁切换后播放异常，不可重新进入播放状态，播放按键无效果，根源：视频播放线程频繁销毁，导致异常，线程出错；解决：将线程销毁更改成阻塞释放
4.修复关闭移动侦测预览状态下，移动侦测录像时进入移动侦测录制文件列表界面，存在偶尔丢失当前录制文件问题，其根源是不同线程对文件操作函数接口的调用，同时访问操作一个缓存数组，导致冲突，在其数据修改后，因其他线程操作导致旧数据重新覆盖掉新数据，因此丢失当前的录制文件；目前修改方式：在进行扫描SD卡文件时，添加判断是否进行视频录制中，当进行录制则等待录制完毕，并优化相关UI显示逻辑
5.添加wifi设置界面涂鸦手动解绑功能
6.视频解码修改为join阻塞关闭，避免detach分离关闭未及时释放导致下次开启解码异常

## 20250818
1.同步涂鸦重连推送开锁问题

## 20250917
1.同步删除音乐文件进入自定义铃声有概率死机问题