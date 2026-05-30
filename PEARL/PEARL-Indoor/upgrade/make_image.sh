#!/bin/sh
#在应用层只需修改app分区即可

#创建一个用于保存升级文件的目录
create_platform()
{
	if [ -d "platform" ]; then
                echo "remove platform dir"
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
}

#把 升级脚本 和 进度条显示程序 也放进去打包压缩
images_compress()
{
        cp -f scripts/update.sh                    platform/
        cp -f upgrade_progress/upgrade_progress    platform/

        rm -rf SAT_ANYKAOS*
        rm -rf ../SAT_ANYKAOS*
        cd platform/
        tar -zcvf $SAT_OS_CHECK *
        mv $SAT_OS_CHECK ../../
        cd ../
}


build_timestamp=$(date "+%m%d%H%M")

SAT_OS_CHECK=SAT_ANYKAOS_$build_timestamp

create_platform

make_squashfs_images

images_compress


