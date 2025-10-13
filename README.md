# 盛世云图 Windows 客户端

基于 Qt 6 的云渲染平台 Windows 桌面客户端。

## 功能特性

### 已实现功能（v1.0.0）

#### 核心功能
- ✅ **Maya 环境检测**
  - 自动扫描系统中安装的 Maya 版本
  - 支持从 Windows 注册表扫描
  - 解析 Maya 场景文件（.ma/.mb）
  - 识别渲染器类型（Arnold、V-Ray、Redshift 等）
  - 检测缺失资源

#### 网络层
- ✅ **HTTP 客户端** - 基于 QNetworkAccessManager，自动 JWT Token 管理
- ✅ **WebSocket 客户端** - 实时通信，自动重连，心跳检测
- ✅ **文件上传器** - 分块上传（5MB/块），并行上传，断点续传，MD5 校验

#### 数据模型
- ✅ **用户模型** (User) - 用户信息、会员等级、余额管理
- ✅ **任务模型** (Task) - 8种任务状态，完整的渲染参数，时间跟踪
- ✅ **渲染配置模型** (RenderConfig) - 5种质量预设，多渲染器支持

#### 业务管理器
- ✅ **认证管理器** (AuthManager) - 登录/注册/登出，自动 Token 刷新
- ✅ **任务管理器** (TaskManager) - 任务 CRUD，本地缓存，生命周期管理
- ✅ **用户管理器** (UserManager) - 用户信息、余额、会员管理

#### UI 系统（Fluent Design）
- ✅ **ThemeManager** - 亮色/暗色主题，完整 QSS 样式表
- ✅ **UI 组件** - FluentButton, FluentLineEdit, FluentCard, FluentDialog, TaskItemWidget
- ✅ **主要视图**
  - LoginWindow - 登录窗口（无边框设计，主题切换）
  - MainWindow - 主窗口（自定义标题栏，侧边导航）
  - TaskDetailDialog - 任务详情对话框（标签页，实时进度）
  - CreateTaskDialog - 新建任务对话框（场景检测，参数配置）

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

## 构建说明

### 前置要求

- **Windows 10/11** (64-bit)
- **CMake 3.16+**
- **Qt 6.5.3** 或更高版本
- **Visual Studio 2019/2022** (MSVC 编译器)

### 方法1：GitHub Actions 自动构建（推荐）

推送代码到 GitHub 后，Actions 会自动触发构建：

1. 推送代码：
```bash
git add .
git commit -m "Update YuntuClient"
git push origin main
```

2. 查看构建进度：
   - 访问仓库的 Actions 页面
   - 等待构建完成（约 8-10 分钟）

3. 下载构建产物：
   - 点击成功的工作流
   - 在 Artifacts 部分下载 `YuntuClient-Windows-x64`
   - 解压即可使用

### 方法2：本地构建

#### 1. 安装依赖

下载并安装 Qt 6.5.3：
- 访问 https://www.qt.io/download-qt-installer
- 选择 MSVC 2019 64-bit 组件
- 确保安装 Qt WebSockets 模块

#### 2. 配置项目

```powershell
# 创建构建目录
mkdir build
cd build

# 配置 CMake（替换 Qt 路径）
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2019_64"
```

#### 3. 编译

```powershell
# 构建 Release 版本
cmake --build . --config Release

# 可执行文件位于
# build\Release\YuntuClient.exe
```

#### 4. 部署

手动复制 Qt DLLs 到 Release 目录：
```powershell
cd Release

# 复制核心 DLL
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Core.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Gui.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Widgets.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Network.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6WebSockets.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Sql.dll .

# 复制平台插件
mkdir platforms
copy C:\Qt\6.5.3\msvc2019_64\plugins\platforms\qwindows.dll platforms\
```

## 设计风格

采用 **Microsoft Fluent Design** 设计语言：
- 圆角矩形（4-12px 圆角）
- 柔和阴影效果
- 悬停/焦点动画（200ms 过渡）
- 亮色/暗色主题切换
- 现代配色方案

## 项目结构

```
YuntuClient/
├── .github/
│   └── workflows/
│       ├── build-windows.yml      # 测试程序构建
│       └── build-windows-gui.yml  # GUI 程序构建
├── CMakeLists.txt              # GUI 构建配置
├── CMakeLists_Test.txt         # 测试构建配置
├── README.md                   # 项目说明
│
└── src/
    ├── main.cpp                # GUI 主程序入口
    ├── test_main.cpp           # 测试程序入口
    │
    ├── core/                   # 核心模块
    │   ├── Application.h/cpp   # 应用程序单例
    │   ├── Config.h/cpp        # 配置管理
    │   └── Logger.h/cpp        # 日志系统
    │
    ├── network/                # 网络层
    │   ├── HttpClient.h/cpp    # HTTP 客户端
    │   ├── WebSocketClient.h/cpp  # WebSocket 客户端
    │   ├── ApiService.h/cpp    # API 服务封装
    │   └── FileUploader.h/cpp  # 文件上传服务
    │
    ├── models/                 # 数据模型
    │   ├── User.h/cpp          # 用户模型
    │   ├── Task.h/cpp          # 任务模型
    │   └── RenderConfig.h/cpp  # 渲染配置模型
    │
    ├── managers/               # 业务管理器
    │   ├── AuthManager.h/cpp   # 认证管理 ⭐
    │   ├── TaskManager.h/cpp   # 任务管理 ⭐
    │   └── UserManager.h/cpp   # 用户管理 ⭐
    │
    ├── services/               # 业务服务
    │   └── MayaDetector.h/cpp  # Maya 环境检测 ⭐重点
    │
    └── ui/                     # UI 界面
        ├── ThemeManager.h/cpp  # 主题管理 ⭐
        │
        ├── components/         # Fluent Design 组件
        │   ├── FluentButton.h/cpp       # Fluent 按钮
        │   ├── FluentLineEdit.h/cpp     # Fluent 输入框
        │   ├── FluentCard.h/cpp         # Fluent 卡片
        │   ├── FluentDialog.h/cpp       # Fluent 对话框
        │   └── TaskItemWidget.h/cpp     # 任务列表项
        │
        └── views/              # 主要视图
            ├── LoginWindow.h/cpp        # 登录窗口 ⭐
            ├── MainWindow.h/cpp         # 主窗口 ⭐
            ├── TaskDetailDialog.h/cpp   # 任务详情对话框 ⭐
            └── CreateTaskDialog.h/cpp   # 新建任务对话框 ⭐
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
# YuntuClient
