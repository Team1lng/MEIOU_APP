###
 # @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @Date: 2023-08-08 15:59:34
 # @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 # @LastEditTime: 2024-02-20 14:08:06
 # @FilePath: /outdoor_pro/build.sh
 # @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
### 
cmake ./
make -j8

cp net_camera ./cbin/
cp  cbin/*   upgrade/app/cbin/ -f
tar -czf cbin.update cbin
cd upgrade/
./make_image.sh

rm -rf ../../../../SDK/AK39EV33X_SDK_V1.04/rootfs/rootfs/app/cbin
cp -rf ../cbin ../../../../SDK/AK39EV33X_SDK_V1.04/rootfs/rootfs/app/


