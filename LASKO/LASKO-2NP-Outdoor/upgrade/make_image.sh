#!/bin/sh
###
 # @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @Date: 2024-01-27 09:59:10
 # @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @LastEditTime: 2024-11-04 15:41:55
 # @FilePath: /82225-EPC/upgrade/make_image.sh
 # @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
### 
#在应用层只需修改app分区即可
if [ "$#" -gt 0 ]; then
MODEL=$1
else
MODEL="SAT_CAMERA"
fi

IMAGE=image.tar.gz
build_timestamp=$(date "+%m%d%H%M")
IPC_MODEL=$MODEL-$build_timestamp
#创建一个用于保存升级文件的目录
create_platform()
{
	if [ -d "platform" ]; then
                echo "exist platform dir"
                # rm -f "platform/*"
                # mkdir "platform"
        else
                echo "mkdir platform"
                mkdir "platform"
        fi
}

#用mksquashfs工具将app目录打包成 app.sqsh4 文件系统
make_squashfs_images()
{
        ./tools/mksquashfs ./app platform/app.sqsh4 -noappend -comp xz
        ./tools/mksquashfs ./daemon platform/daemon.sqsh4 -noappend -comp xz
}

#把 升级脚本 和 进度条显示程序 也放进去打包压缩
images_compress()
{
        cp -f scripts/update.sh                    platform/

        rm -rf $MODEL*
        cd platform
        
        tar -zcvf ../cbin.update  * 
        # tar -zcvf ../image/$IMAGE  * 
        cd ../
        
        # sha1sum image/$IMAGE>image/image.sha1
        # tar -zcvf $IPC_MODEL image/
        # rm ../$MODEL* -f
        # cp $IPC_MODEL cbin.update
        # mv $IPC_MODEL ../
}

#更新app目录
update_app_dir()
{
        rm -rf ./app/*
        cp ../ipc_camera ./app
        cp -r ../cbin ./app
        cp -f ../isp_gc2063_mipi_2lane_h3b_101101.conf ./app
        cp -f ../daemon.sh ./app
        touch  ./app/$IPC_MODEL
        cp -f ./daemon/net_camera ./app/cbin
}

#更新SDK app目录
update_sdk_dir()
{
        rm -rf /home/leo/workspace/MEIOU/SDK/AK39EV33X_SDK_V1.04/rootfs/rootfs/app
        cp -rf ./app /home/leo/workspace/MEIOU/SDK/AK39EV33X_SDK_V1.04/rootfs/rootfs/
        rm -rf /home/leo/workspace/MEIOU/SDK/AK39EV33X_SDK_V1.04/rootfs/rootfs/daemon
        cp -rf ./daemon /home/leo/workspace/MEIOU/SDK/AK39EV33X_SDK_V1.04/rootfs/rootfs/
}


update_app_dir

create_platform

make_squashfs_images

images_compress

update_sdk_dir

