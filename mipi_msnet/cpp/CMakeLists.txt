#设置CMake的最低版本要求
cmake_minimum_required(VERSION 3.10)
#设置项目名称
project(msnet_cpp)

#设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#设置C++扩展
set(CMAKE_CXX_EXTENSIONS ON)

#设置编译类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "build type: ${CMAKE_BUILD_TYPE}")
#设置编译选项
set(CMAKE_CXX_FLAGS_DEBUG " -Wall -Werror -g -O0 ")
set(CMAKE_C_FLAGS_DEBUG " -Wall -Werror -g -O0 ")
set(CMAKE_CXX_FLAGS_RELEASE " -Wall -Werror -O3 ")
set(CMAKE_C_FLAGS_RELEASE " -Wall -Werror -O3 ")

# libdnn.so 依赖设置
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wl,-unresolved-symbols=ignore-in-shared-libs")

#添加外部依赖opencv包
find_package(OpenCV REQUIRED)

#设置RDK BPU 路径
set(DNN_PATH "/usr/include/dnn") # DNN 头文件路径
set(DNN_LIB_PATH "/usr/lib")  # DNN 库文件路径
#添加头文件和库文件路径
set(INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR})

#添加BPU_detect类编译
add_library(bpu_detect STATIC BPU_Detect.cpp)
add_library(mipi STATIC Mipi_capture.cpp)

#包含头文件路径
include_directories(bpu_detect PUBLIC
    ${DNN_PATH}
    ${OpenCV_INCLUDE_DIRS}
    ${INCLUDE_PATH}
)

include_directories(mipi PUBLIC
    ${DNN_PATH}
    ${OpenCV_INCLUDE_DIRS}
    ${INCLUDE_PATH}
)

#添加库文件路径
link_directories(
    ${DNN_LIB_PATH}
    ${LIB_PATH}
)

#添加源文件
add_executable(main 
    main.cpp
)

#添加链接依赖库
target_link_libraries(main 
    bpu_detect #BPU_detect类
    mipi # Mipi_capture类
    ${OpenCV_LIBS} #opencv库
    dnn   #rdk_bpu库
    pthread #线程库
    rt #实时库
    dl #动态链接库
    spcdev #spcdev库    
)

#安装目标
install(TARGETS main
    RUNTIME DESTINATION bin
)
