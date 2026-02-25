cmake_minimum_required(VERSION 3.10)

project(face_door)

#C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#查找系统中的OpenCV库
find_package(OpenCV REQUIRED) 
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "未找到OpenCV库")
endif()
message(STATUS " 已找到OpenCV,版本::${OpenCV_VERSION}")
message(STATUS " OpenCV头文件路径:${OpenCV_INCLUDE_DIRS}")
message(STATUS " OpenCV库文件列表:${OpenCV_LIBS}")
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

#查找gpiod库
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)
include_directories(${GPIOD_INCLUDE_DIRS})
message(STATUS " gpiod头文件路径:${GPIOD_INCLUDE_DIRS}")
message(STATUS " gpiod库文件列表:${GPIOD_LIBRARIES}")

#门禁主程序
add_executable(face_door
    src/main.cpp            # 主程序入口
    src/door_core.cpp       # 门禁核心逻辑（人脸验证、开门控制）
    src/log_util.cpp        # 日志工具
    src/gpio_control.cpp    # GPIO控制
)
target_link_libraries(face_door
    ${OpenCV_LIBS}    #OpenCV核心库(人脸检测/识别依赖）
    pthread           #Linux线程库
    #wiringPi         #树莓派GPIO控制库
    ${GPIOD_LIBRARIES}# 链接gpiod库
    atomic            #原子操作库
)

#人脸采集
add_executable(face_collect
    src/face_collect.cpp # 人脸采集逻辑（从摄像头采集人脸图片，用于训练）
    src/face_tool.cpp
)
target_link_libraries(face_collect
    ${OpenCV_LIBS}
)

#模型训练
add_executable(face_train
    src/face_train.cpp   # 模型训练逻辑（用采集的人脸数据训练识别模型）
    src/face_tool.cpp
)
target_link_libraries(face_train
    PRIVATE
    ${OpenCV_LIBS}
    atomic
)