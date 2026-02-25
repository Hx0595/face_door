
## 核心代码文件说明

```text
face_door_system/
├── face_data/              # 人脸数据目录
│   ├── user_1/             # 用户1的人脸样本目录
│   │   ├── 001.jpg         # 采集的人脸样本图片
│   │   └── ...
│   ├── user_2/             # 用户2的人脸样本目录
│   └── face_model.yml      # 训练好的LBPH人脸识别模型文件
│
├── include/
│   ├── config.h            # 全局配置项（路径、阈值、引脚等常量定义）
│   ├── door_core.h         # 门禁核心业务逻辑接口（开门/报警联动声明）
│   ├── face_collect.h      # 人脸采集工具接口（样本采集函数声明）
│   ├── face_tool.h         # 人脸处理工具接口（预处理、裁剪等工具函数）
│   ├── face_train.h         # 人脸模型训练接口（LBPH模型训练/保存声明）
│   ├── gpio_control.h      # GPIO硬件控制接口（libgpiod）
│   ├── log_util.h          # 日志工具接口（异步日志声明）
│   └── safe_queue.h        # 线程安全队列（实现多线程通信）
│
├── src/
│   ├── door_core.cpp       # 门禁核心业务实现（线程调度、逻辑联动）
│   ├── face_collect.cpp    # 人脸采集工具实现（样本采集、保存）
│   ├── face_tool.cpp       # 人脸预处理实现（灰度、裁剪）
│   ├── face_train.cpp      # 人脸模型训练实现（LBPH训练、模型保存/加载）
│   ├── gpio_control.cpp    # GPIO底层实现（控制继电器/蜂鸣器）
│   ├── log_util.cpp        # 异步日志实现（日志队列、终端/文件输出）
│   └── main.cpp            # 项目入口（初始化、线程启停、资源释放）
│
└── CMakeLists.txt          # 编译配置（依赖libgpiod、OpenCV，多文件编译管理））
