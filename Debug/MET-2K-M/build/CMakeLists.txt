cmake_minimum_required(VERSION 3.5.4)

# 设置编译器，此处为ARM平台，如果你安装了编译链就用名称，没安装请用绝对路径
SET(PROJECT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../)
# SET(CMAKE_C_COMPILER "arm-anykav500-linux-uclibcgnueabi-gcc")
# SET(CMAKE_CXX_COMPILER "arm-anykav500-linux-uclibcgnueabi-g++")
SET(link_project ipc_camera)
project(${link_project})

add_definitions(-DIPC_MODEL="${IPC_MODEL}")
add_definitions(-D_GNU_SOURCE)#隐式声明函数‘strptime’

if(RELEASE_VERSION)
add_definitions(-DRELEASE_VERSION)
endif()

if(CARD_ENABLE)
add_definitions(-DCARD_ENABLE)
endif()

if(ATS_ENABLE)
add_definitions(-DATS_ENABLE)
endif()

if(ITS_ENABLE)
add_definitions(-DITS_ENABLE)
endif()

if(KEYPAD_ENABLE)
add_definitions(-DKEYPAD_ENABLE)
endif()

if(FINGERPRINT_ENABLE)
add_definitions(-DFINGERPRINT_ENABLE)
endif()

add_subdirectory(../cmake cmake)


SET(daemon1_project net_camera)
SET(daemon2_project net_indoor)
SET(DAEMON_PATH ${PROJECT_PATH}daemon)
project(${daemon1_project})
project(${daemon2_project})
add_executable(${daemon1_project} ${DAEMON_PATH}/net_camera.c ${DAEMON_PATH}/GpioControl.c ${DAEMON_PATH}/upgrade_app.c)
add_executable(${daemon2_project} ${DAEMON_PATH}/net_indoor.c)
set_target_properties(${daemon1_project} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_PATH}/upgrade/daemon  # 设置运行时输出路径
)
set_target_properties(${daemon2_project} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_PATH}  # 设置运行时输出路径
)