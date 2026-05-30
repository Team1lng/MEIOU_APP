


if(${RELEASE_VERSION})
SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -Os -s  -Wno-unused-function  -Wall -std=gnu11    ")# -Wno-implicit-function-declaration
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Os -s  -Wno-unused-function -Wall -std=c++1y  ") #   
else()
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS}  -g -Wno-unused-function  -Wall -std=gnu11    ")# -Wno-implicit-function-declaration
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS}  -g -Wno-unused-function -Wall -std=c++1y  ") #   
endif()

# 定义函数，用于递归添加头文件搜索目录
function(include_sub_directories_recursively root_dir )
    if (IS_DIRECTORY ${root_dir})               # 当前路径是一个目录吗，是的话就加入到包含目录
        include_directories(${root_dir})
    endif()

    file(GLOB ALL_SUB RELATIVE ${root_dir} ${root_dir}/*) # 获得当前目录下的所有文件，让如ALL_SUB列表中
    foreach(sub ${ALL_SUB})
        if (IS_DIRECTORY ${root_dir}/${sub})
            include_sub_directories_recursively(${root_dir}/${sub}) # 对子目录递归调用，包含
        endif()
    endforeach()
endfunction()

include_sub_directories_recursively(${PROJECT_PATH}/share)
include_sub_directories_recursively(${PROJECT_PATH}/include)
include_sub_directories_recursively(${PROJECT_PATH}/common)


# 匹配common目录下的文件
file(GLOB COMMON_SRCS
    ${PROJECT_PATH}/common/*/*.c
)
# 匹配src目录下的文件
file(GLOB SRC_SRCS
    ${PROJECT_PATH}/src/*.c
)
#于程序中也弱函数，因此源文件的编译条件需要先编译弱函数，
#因此DIR_SRCS存储路径的目录需要按顺序
#file会将目录按照字符大小排序存储到变量中，因此为了按照制定顺序需要将file函数挨个存储到个别变量
#再通过list函数将所有路径按顺序存储至DIR_SRCS中
list(APPEND DIR_SRCS  ${SRC_SRCS} ${TEST_SRCS} ${COMMON_SRCS})


add_executable(${link_project} ${DIR_SRCS})

# 设置输出路径为上一级目录
set_target_properties(${link_project} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_PATH}  # 设置运行时输出路径
)

SET(ANYKA_PATH ${PROJECT_PATH}/lib/anyka3918/)

if(${ATS_ENABLE})
target_link_libraries(${link_project} ${ANYKA_PATH}libakaudiocodec.so)
target_link_libraries(${link_project} ${ANYKA_PATH}libapp_ats.so)
target_link_libraries(${link_project} ${ANYKA_PATH}libmpi_aenc.so)
target_link_libraries(${link_project} ${ANYKA_PATH}libmpi_adec.so)
endif()

target_link_libraries(${link_project} -Wl,--start-group

	${VAR}

	${ANYKA_PATH}libplat_vi.so
	${ANYKA_PATH}libplat_mem.so
	${ANYKA_PATH}libplat_common.so
	${ANYKA_PATH}libplat_osal.so
	${ANYKA_PATH}libplat_thread.so
	${ANYKA_PATH}libplat_isp_sdk.so
	${ANYKA_PATH}libmpi_venc.so
	${ANYKA_PATH}libmpi_svp.so
	${ANYKA_PATH}libak_object_filter.so
	${ANYKA_PATH}libakv_cnn_dt.so
	${ANYKA_PATH}libakv_cnn_fas.so
	${ANYKA_PATH}libakv_cnn_re.so
	${ANYKA_PATH}libakv_encode.so
	${ANYKA_PATH}libplat_log.so
	${ANYKA_PATH}libplat_vpss.so
	${ANYKA_PATH}libplat_dbg.so

	${ANYKA_PATH}libplat_ao.so
	${ANYKA_PATH}libplat_ai.so
	${ANYKA_PATH}libplat_vqe.so
	${ANYKA_PATH}libakaudiofilter.so
	
	${ANYKA_PATH}libapp_md.so
	${ANYKA_PATH}libak_mrd.so
	${ANYKA_PATH}libplat_timer.so
	


	${ANYKA_PATH}libapp_video.so
	${ANYKA_PATH}libapp_its.so
	${PROJECT_PATH}/lib/g711/libg711.a
	${PROJECT_PATH}/lib/mad/libmad.a

	m
	pthread
	dl

	-Wl,--end-group)
