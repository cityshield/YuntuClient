# 盛世云图 Windows 客户端

基于 Qt 6 的云渲染平台 Windows 桌面客户端。

## 功能特性

### 第一期功能（当前版本）
- ✅ 用户登录（手机验证码、微信扫码、30天自动登录）
- ✅ Maya 环境自动检测
  - 自动扫描系统中安装的 Maya 版本
  - 检测渲染器（Arnold、V-Ray、Redshift）
  - 识别已安装的插件
  - 场景文件依赖检测
- ✅ 文件上传（分片上传、断点续传）
- ✅ 任务管理
  - 任务列表查看
  - 实时进度追踪
  - 任务控制（暂停/继续/取消）
- ✅ 实时通信（WebSocket）
- ✅ 本地缓存和配置管理

### 后续版本规划
- ⏳ 多软件支持（3ds Max、Blender、C4D、UE）
- ⏳ 子账号管理
- ⏳ 费用管理和充值
- ⏳ AI客服
- ⏳ 自动更新

## 技术栈

- **框架**: Qt 6.5+
- **语言**: C++17
- **构建系统**: CMake 3.16+
- **网络**: Qt Network, Qt WebSockets
- **数据库**: SQLite (本地缓存)
- **JSON**: Qt JSON

## 构建要求

### Windows
- Visual Studio 2019/2022
- Qt 6.5+ (MSVC 2019/2022 64-bit)
- CMake 3.16+

### macOS (开发测试)
- Xcode 13+
- Qt 6.5+
- CMake 3.16+

## 构建步骤

### 1. 安装依赖

**Windows:**
```bash
# 安装 Qt 6 (使用在线安装器或离线包)
# 下载地址: https://www.qt.io/download

# 安装 CMake
# 下载地址: https://cmake.org/download/
```

### 2. 配置环境变量

```bash
# Windows
set Qt6_DIR=C:/Qt/6.5.3/msvc2019_64
set PATH=%Qt6_DIR%/bin;%PATH%

# macOS/Linux
export Qt6_DIR=/path/to/Qt/6.5.3/clang_64
export PATH=$Qt6_DIR/bin:$PATH
```

### 3. 编译项目

```bash
# 创建构建目录
mkdir build
cd build

# 配置 CMake
cmake ..

# 编译
cmake --build . --config Release

# 运行
./YuntuClient  # Linux/macOS
Release/YuntuClient.exe  # Windows
```

### 使用 Qt Creator

1. 打开 Qt Creator
2. 文件 -> 打开文件或项目 -> 选择 `CMakeLists.txt`
3. 配置 Kit (选择合适的编译器和 Qt 版本)
4. 点击"构建"按钮
5. 点击"运行"按钮

## 项目结构

```
YuntuClient/
├── CMakeLists.txt              # CMake 配置文件
├── README.md                   # 项目说明
│
├── src/
│   ├── main.cpp                # 程序入口
│   │
│   ├── core/                   # 核心模块
│   │   ├── Application.h/cpp   # 应用程序单例
│   │   ├── Config.h/cpp        # 配置管理
│   │   └── Logger.h/cpp        # 日志系统
│   │
│   ├── network/                # 网络层
│   │   ├── HttpClient.h/cpp    # HTTP 客户端
│   │   ├── WebSocketClient.h/cpp  # WebSocket 客户端
│   │   ├── ApiService.h/cpp    # API 服务封装
│   │   └── FileUploader.h/cpp  # 文件上传服务
│   │
│   ├── models/                 # 数据模型
│   │   ├── User.h/cpp          # 用户模型
│   │   ├── Task.h/cpp          # 任务模型
│   │   └── RenderConfig.h/cpp  # 渲染配置模型
│   │
│   ├── services/               # 业务服务
│   │   ├── AuthService.h/cpp   # 认证服务
│   │   ├── TaskService.h/cpp   # 任务服务
│   │   ├── FileService.h/cpp   # 文件服务
│   │   ├── MayaDetector.h/cpp  # Maya 环境检测 ⭐重点
│   │   └── UpdateService.h/cpp # 自动更新服务
│   │
│   ├── database/               # 数据库
│   │   └── Database.h/cpp      # SQLite 封装
│   │
│   ├── ui/                     # UI 界面
│   │   ├── MainWindow.h/cpp/ui # 主窗口
│   │   ├── LoginDialog.h/cpp/ui # 登录对话框
│   │   ├── TaskListWidget.h/cpp # 任务列表
│   │   ├── TaskDetailWidget.h/cpp # 任务详情
│   │   ├── AccountWidget.h/cpp # 账号管理
│   │   ├── SettingsDialog.h/cpp # 设置对话框
│   │   └── widgets/            # 自定义控件
│   │       ├── ProgressBar.h/cpp # 进度条
│   │       └── LogViewer.h/cpp # 日志查看器
│   │
│   └── utils/                  # 工具类
│       ├── FileUtil.h/cpp      # 文件工具
│       ├── ProcessUtil.h/cpp   # 进程工具
│       ├── SystemUtil.h/cpp    # 系统工具
│       └── Cryptor.h/cpp       # 加密工具
│
└── resources/                  # 资源文件
    ├── icons/                  # 图标
    ├── qss/                    # 样式表
    ├── translations/           # 多语言
    └── resources.qrc           # Qt 资源文件
```

## Maya 环境检测功能说明

`MayaDetector` 是本项目的核心功能模块之一，用于自动检测和识别用户系统中的 Maya 环境。

### 主要功能

1. **Maya 版本检测**
   - 自动扫描 Windows 注册表
   - 扫描常用安装目录
   - 提取版本号和安装路径

2. **渲染器检测**
   - Arnold (Maya 2017+ 内置)
   - V-Ray
   - Redshift
   - 其他主流渲染器

3. **插件识别**
   - Miarmy (群集动画)
   - Yeti (毛发系统)
   - XGen (毛发)
   - Bifrost (流体)
   - MASH (运动图形)
   - 其他第三方插件

4. **场景文件分析**
   - 从 .ma 或 .mb 文件提取 Maya 版本
   - 识别所使用的渲染器
   - 扫描纹理和素材依赖
   - 检测缺失的文件

### 使用示例

```cpp
#include "services/MayaDetector.h"

MayaDetector detector;

// 连接信号
connect(&detector, &MayaDetector::detectProgress, [](int progress, const QString &message) {
    qDebug() << "进度:" << progress << "%" << message;
});

// 检测所有 Maya 版本
QVector<MayaSoftwareInfo> mayaVersions = detector.detectAllMayaVersions();

for (const MayaSoftwareInfo &info : mayaVersions) {
    qDebug() << "Maya 版本:" << info.version;
    qDebug() << "安装路径:" << info.installPath;
    qDebug() << "渲染器:" << info.renderers;
    qDebug() << "插件:" << info.plugins;
}

// 分析场景文件
QString sceneFile = "C:/projects/myScene.ma";
QString mayaVersion = detector.extractMayaVersionFromScene(sceneFile);
QString renderer = detector.extractRendererFromScene(sceneFile);
QStringList missingAssets = detector.detectMissingAssets(sceneFile);

qDebug() << "场景所需 Maya 版本:" << mayaVersion;
qDebug() << "使用的渲染器:" << renderer;
qDebug() << "缺失的素材:" << missingAssets;
```

## API 接口

后端 API 地址：`https://api.yuntu.com/api/v1`

### 主要接口

- `POST /auth/login` - 用户登录
- `POST /auth/send-code` - 发送验证码
- `GET /auth/me` - 获取当前用户信息
- `GET /tasks` - 获取任务列表
- `POST /tasks` - 创建任务
- `GET /tasks/{id}` - 获取任务详情
- `PUT /tasks/{id}/pause` - 暂停任务
- `POST /files/upload/chunk` - 上传文件分片
- `POST /files/upload/merge` - 合并文件

WebSocket: `wss://api.yuntu.com/ws/{userId}`

## 配置文件

配置文件位置：
- Windows: `C:/Users/{用户名}/AppData/Roaming/YunTu/YuntuClient.ini`
- macOS: `~/Library/Preferences/com.yuntu.YuntuClient.plist`

## 日志文件

日志文件位置：
- Windows: `C:/Users/{用户名}/AppData/Roaming/YunTu/logs/`
- macOS: `~/Library/Application Support/YunTu/logs/`

## 常见问题

### 1. 找不到 Qt 库
确保已正确设置 `Qt6_DIR` 环境变量，或在 CMake 命令中指定：
```bash
cmake -DQt6_DIR=C:/Qt/6.5.3/msvc2019_64/lib/cmake/Qt6 ..
```

### 2. 编译错误：无法打开包含文件
检查 Qt 模块是否完整安装，确保包含以下模块：
- Qt6::Core
- Qt6::Gui
- Qt6::Widgets
- Qt6::Network
- Qt6::Sql
- Qt6::WebSockets

### 3. Maya 检测不到
- 确保 Maya 已正确安装
- 检查 Maya 安装路径是否在常用位置
- 查看日志文件获取详细错误信息

## 开发团队

- 前端开发：Qt/C++ 团队
- 后端开发：Python 团队
- UI/UX 设计：设计团队

## 许可证

Copyright © 2025 盛世云图. All rights reserved.
