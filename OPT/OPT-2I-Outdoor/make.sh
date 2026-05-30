###
 # @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @Date: 2024-04-09 08:59:00
 # @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @LastEditTime: 2024-05-14 08:23:25
 # @FilePath: /2CD-ME/make.sh
 # @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
### 

OPTION=2CD-ME2I
ENABLE_ATS=0
ENABLE_CARD=1
ENABLE_KEYPAD=0
CC="$(pwd)/compilat_tool/arm-anykav500-linux-uclibcgnueabi/bin/arm-anykav500-linux-uclibcgnueabi-gcc"
CXX="$(pwd)/compilat_tool/arm-anykav500-linux-uclibcgnueabi/bin/arm-anykav500-linux-uclibcgnueabi-g++"

#编译
compile_cmake()
{
    rm -rf CMakeFiles
    rm -f cmake_install.cmake
    rm -f CMakeCache.txt
    cmake -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DIPC_MODEL="$OPTION"  -DCARD_ENABLE=${ENABLE_CARD}  -DKEYPAD_ENABLE=${ENABLE_KEYPAD} .
    
    make clean
    make -j8
}

#制作升级文件
make_upgrade_package()
{   
    cd upgrade/
    ./make_image.sh $OPTION
    cd -
    
}


clear

compile_cmake

make_upgrade_package
