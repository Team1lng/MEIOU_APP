<!--
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-02-19 08:42:02
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-01 16:55:58
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

## 20240929
1.优化wifi连接卡顿问题，具体原因：
    1.调用system时未后台执行，导致需要等待命令结束才会回到主进程中，导致卡顿
    2.多线程操作wpa_cli冲突，造成wpa_cli -i wlan0 status获取不到返回，从而阻塞主线程，导致卡机；在获取返回时添加超时退出解决
2.优化wifi配置文件wpa_supplicant丢失问题，etc/config目录下的wpa_supplicant文件只做拷贝覆盖，不直接删除，间接避免删除后拷贝新的配置文件失败，导致文件丢失从而wifi无法搜索
3.优化待机黑屏未完全刷新问题
4.优化wifi模块掉线检测，禁止wifi操作界面检测，防止wcli操作接口冲突


## 20241101
1.添加看门狗
2.优化升级门口机过程中呼叫后重新进入高级设置卡死
3.优化rtsp流切换线程，更改成阻塞释放

## 20250102
1.修复SD卡分区内存不足时，清除内存操作异常，导致循环进入清除操作，占用大量CPU资源，其原因是代码函数SD_card_space_clear在做清除数据类型判断时，未添加else，导致在某个判断成立后，指定删除的类型未退出判断代码，进行下一步的其他类型选择；解决方案：在每次判断选择清除数据的类型后添加else，避免if过后继续执行下一段代码

## 20250109
1.修复视频播放频繁切换后播放异常，不可重新进入播放状态，播放按键无效果，根源：视频播放线程频繁销毁，导致异常，线程出错；解决：将线程销毁更改成阻塞释放
2.优化待机息屏时，屏幕未完全刷黑屏,关闭息屏时右上提示小图标刷新，进入home界面后优先点亮背光 
3.移植看门狗至用户数据处理线程
4.修复关闭移动侦测预览状态下，移动侦测录像时进入移动侦测录制文件列表界面，存在偶尔丢失当前录制文件问题，其根源是不同线程对文件操作函数接口的调用，同时访问操作一个缓存数组，导致冲突，在其数据修改后，因其他线程操作导致旧数据重新覆盖掉新数据，因此丢失当前的录制文件；目前修改方式：在进行扫描SD卡文件时，添加判断是否进行视频录制中，当进行录制则等待录制完毕，并优化相关UI显示逻辑
5.添加每日凌晨1点左右重启功能

## 20250115
1.OTA升级应关闭看门狗
2.添加wifi设置界面涂鸦手动解绑功能
3.视频解码修改为join阻塞关闭，避免detach分离关闭未及时释放导致下次开启解码异常

## xxxxxx
1.优化wifi重加载
2.清除涂鸦二维码缓存

## 20250528
1.添加氛围灯的检测和控制功能

## 20250704
1.修改默认语言以色列语为英语

## 20250818
1.门口机呼叫上报涂鸦，会改变锁的状态（打开，实际上没有开锁），然后涂鸦重连时会上报一次锁的的状态，所以会有推送开锁现象（可能是网络不好引起涂鸦重新连接）,现在门口机呼叫上报时，不改变锁的状态（只有开锁才会上报给涂鸦

## 20250912
1.修改删除媒体文件后，进入自定义铃声概率性死机问题

## 20251010
1.同步tuya_dp_235控制氛围灯功能，保存用户配置。
2.待视频稳定后进行抓拍
3.修复删除媒体文件后，进入自定义铃声概率性死机问题

## 20251216
1.更换pid wewbamvu27rye4au-------------> qbtwyrrdixvftn75