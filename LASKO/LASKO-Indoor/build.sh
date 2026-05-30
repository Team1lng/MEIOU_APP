###
 # @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @Date: 2023-07-04 10:33:17
 # @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @LastEditTime: 2024-04-27 14:10:26
 # @FilePath: /two-wire-indoor/build.sh
 # @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
### 

CC="$(pwd)/compilat_tool/arm-anykav500-linux-uclibcgnueabi/bin/arm-anykav500-linux-uclibcgnueabi-gcc"
CXX="$(pwd)/compilat_tool/arm-anykav500-linux-uclibcgnueabi/bin/arm-anykav500-linux-uclibcgnueabi-g++"

#选择编译对象
# SelectCompiledObject()
# {
#     echo "What is your Compile Object?"
#     select Object in "OR" "ANNA" "LASKOMEX"; 
#         do echo "You have selected $Object"
#         break
#     done

#     case $Object in
#     "OR")
#     IPC_APP_PID=tiqjsu35askqmgh3
#     cp upgrade/logo/meiou.rgb upgrade/platform/anyka_logo.rgb -f
#     ;;
#     "ANNA")
#     #hlf: 8寸PID:pfznk2mw3kdumbbg, 10寸:e51zk70a4ojs4yqc
#     IPC_APP_PID=pfznk2mw3kdumbbg
#     cp upgrade/logo/meiou.rgb upgrade/platform/anyka_logo.rgb -f
#     ;;

#     "LASKOMEX")
#     IPC_APP_PID=k65begkkmxqdwgxa
#     cp upgrade/logo/laskomex.rgb upgrade/platform/anyka_logo.rgb -f
#     ;;
#     esac
# }

# SelectCompiledObject

clear
rm CMakeFiles/ -r
rm CMakeCache.txt -f
cmake ./ -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DIPC_APP_PID="$IPC_APP_PID"
make clean
make -j16

cp ./src/layout/resource/rom.bin ./
cp -f ./ANYKA37E.BIN ./upgrade/app/
cp -f ./rom.bin ./upgrade/app/
cp -f ./language.xls ./upgrade/app/

# # 拷贝到SDK
# rm -rf ../../../MEIOU/SDK/AK37E_SDK_V1.05/rootfs/rootfs/app/app
# cp -rf ./upgrade/app ../../../MEIOU/SDK/AK37E_SDK_V1.05/rootfs/rootfs/app/
# cp -f ./upgrade/platform/anyka_logo.rgb ../../../MEIOU/SDK/AK37E_SDK_V1.05/image/anyka_logo.rgb

cd ./upgrade
./make_image.sh
cd -

echo "Done!!!"