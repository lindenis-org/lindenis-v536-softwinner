### 目录说明
    下面展示初步确定的应用程序代码文件及目录结构，在开发过程中需根据实际情况进行调整

```
.
├── apps // 应用程序
│   └── ipc
│       ├── config // 应用配置文件
│       ├── README.md
│       ├── res // 应用资源文件
│       │   ├── audio // 声音，开机音乐，按键音，提示音
│       │   ├── fonts // GUI字体文件
│       │   ├── images // GUI图片文件
│       │   └── languages // 语言包文件
│       └── source // 应用源代码
│           ├── bll_presenter // 业务逻辑层代码
│           │   ├── ipc_mode.cpp // ipc工作模式
│           │   ├── ipc_mode.h
│           │   ├── mosaic_mode.cpp // 拼接模式
│           │   ├── mosaic_mode.h
│           │   ├── remote // 网络平台适配层
│           │   │   ├── cloud
│           │   │   │   ├── 360
│           │   │   │   ├── include
│           │   │   │   ├── qq
│           │   │   │   └── tutk
│           │   │   └── onvif
│           │   ├── video_call.cpp // 双向视频模式
│           │   └── video_call.h
│           ├── common // 公共头文件定义
│           │   ├── app_def.h // 定义产品特性
│           │   ├── app_log.h // log头文件
│           │   ├── app_platform.h // 定义平台特性
│           │   └── utils // 实用工具头文件
│           │       └── glib_header.h
│           ├── device_model // 设备抽象层代码
│           │   ├── conf // 动态配置文件解析
│           │   │   ├── conf_parser.cpp
│           │   │   └── conf_parser.h
│           │   ├── database.cpp // 数据库
│           │   ├── database.h
│           │   ├── display.cpp // 显示管理
│           │   ├── display.h
│           │   ├── media // 多媒体相关
│           │   │   ├── camera.cpp
│           │   │   ├── camera.h
│           │   │   ├── media_file_manager.cpp // 媒体文件管理
│           │   │   ├── media_file_manager.h
│           │   │   ├── player.cpp
│           │   │   ├── player.h
│           │   │   ├── recorder.cpp
│           │   │   └── recorder.h
│           │   ├── rtsp.h
│           │   ├── storage_manager.cpp // 存储管理
│           │   ├── storage_manager.h
│           │   └── system // 系统相关(HAL)
│           │       ├── event_manager.cpp // 事件管理
│           │       ├── event_manager.h
│           │       ├── led.cpp // led控制接口
│           │       ├── led.h
│           │       ├── net
│           │       │   ├── ethernet_controller.cpp // 以太网控制
│           │       │   ├── ethernet_controller.h
│           │       │   ├── net_manager.cpp // 网络管理
│           │       │   ├── net_manager.h
│           │       │   ├── softap_controller.cpp // softap热点控制
│           │       │   ├── softap_controller.h
│           │       │   ├── wifi_connector.cpp // wifi连接管理
│           │       │   └── wifi_connector.h
│           │       ├── power_manager.cpp // 电源管理
│           │       ├── power_manager.h
│           │       ├── pwm.cpp // pwm控制
│           │       ├── pwm.h
│           │       ├── rtc.cpp // rtc时钟控制
│           │       └── rtc.h
│           └── uilayer_view // UI层代码
│               ├── gui
│               │   └── minigui // minigui代码, 参考touch cdr ui2.0 代码结构
│               └── web // web 代码
├── libs // 与应用关系密切的库, 不对外发布的库, 如gui库，私有网络库等
├── include // 上述库头文件
└── README.md
```
