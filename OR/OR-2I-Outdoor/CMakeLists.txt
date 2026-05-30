cmake_minimum_required (VERSION 3.5.1)  

# set(CMAKE_C_COMPILER "arm-anykav500-linux-uclibcgnueabi-gcc")
# set(CMAKE_CXX_COMPILER "arm-anykav500-linux-uclibcgnueabi-g++")
#设置编译器，此处为ARM平台，如果你安装了编译链就用名称，没安装请用绝对路径

SET(leo_proj ipc_camera)
project(${leo_proj} C)
#工程名

SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -Os -Wall -std=gnu11 -s")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Os -Wall -std=c++1y -s")

SET(CMAKE_ATS_OPEN OFF)

if(CMAKE_ATS_OPEN)
    add_definitions(-DAPP_ATS_OPEN)
endif()

ENABLE_LANGUAGE(ASM)


add_definitions(-D_GNU_SOURCE)#隐式声明函数‘strptime’

# 门口机机型
add_definitions(-DIPC_MODEL="${IPC_MODEL}")

#刷卡功能
if(CARD_ENABLE)
add_definitions(-DCARD_MODE)	
endif()

#数字按键
if(KEYPAD_ENABLE)
add_definitions(-DNUMERIC_KEY_MODE)	
endif()

#管理功能
add_definitions(-DTCP_NETWORK_MANAGEMENT)	
#密码功能
add_definitions(-DPASSWORD_MODE)	

SET(ASM_OPTIONS "-x assembler-with-cpp")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}")


include_directories(
    ./include/
    ./include/anyka3918
    ./include/g711
    ./include/libmad
    ./share/include
    ./common/
    ./common/tcp_server/
    ./common/peripheral_module/
    ./common/Numeric_key/
    ./src/
    )
    
    

#添加src目录
aux_source_directory(./src/ DIR_SRCS)
aux_source_directory(./common/ DIR_SRCS)
aux_source_directory(./common/tcp_server/ DIR_SRCS)
aux_source_directory(./common/peripheral_module/ DIR_SRCS)
aux_source_directory(./common/Numeric_key/ DIR_SRCS)


add_subdirectory(share)
set(VAR ${VAR} speex)

add_executable(${leo_proj} ${DIR_SRCS})  #链接生成文件${leo_proj} 由DIR_SRC链接。

message("VAR : ${VAR}")
target_link_libraries(${leo_proj} ${VAR})   #设置链接${leo_proj}的 依赖关系 ，依赖 VAR = lvgl_lib lv_drivers


target_link_libraries(${leo_proj} m)
target_link_libraries(${leo_proj} pthread)
target_link_libraries(${leo_proj} dl)

set(ANYKA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/anyka3918/)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_vi.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_mem.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_common.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_osal.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_thread.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_isp_sdk.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libmpi_venc.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libakv_encode.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_log.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_vpss.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_dbg.so)

target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_ao.a)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_ai.a)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_vqe.a)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libakaudiofilter.so)

target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_log.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libapp_md.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libak_mrd.so)
target_link_libraries(${leo_proj} ${CMAKE_CURRENT_SOURCE_DIR}/lib/g711/libg711.a)
target_link_libraries(${leo_proj} ${CMAKE_CURRENT_SOURCE_DIR}/lib/mad/libmad.a)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libapp_its.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_timer.so)


if(CMAKE_ATS_OPEN)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libapp_ats.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libakaudiocodec.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libmpi_aenc.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libmpi_adec.so)
target_link_libraries(${leo_proj} ${ANYKA_PATH}libplat_timer.so)
endif()

SET(daemon1_project net_camera)
SET(daemon2_project net_indoor)
SET(DAEMON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/daemon)
project(${daemon1_project})
project(${daemon2_project})
add_executable(${daemon1_project} ${DAEMON_PATH}/net_camera.c ${DAEMON_PATH}/GpioControl.c ${DAEMON_PATH}/upgrade_app.c)
add_executable(${daemon2_project} ${DAEMON_PATH}/net_indoor.c)
set_target_properties(${daemon1_project} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/upgrade/daemon  # 设置运行时输出路径
)
set_target_properties(${daemon2_project} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}  # 设置运行时输出路径
)