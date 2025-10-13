# Windows 测试指南

## 前置准备

### 1. 安装必需工具

#### Qt 6
1. 下载 Qt 在线安装器
   - 访问: https://www.qt.io/download-qt-installer
   - 下载 `qt-unified-windows-x64-online.exe`

2. 安装 Qt 6.5.3
   - 选择组件时，勾选：
     - Qt 6.5.3
     - MSVC 2019 64-bit
     - Qt WebSockets
     - Qt Network
   - 安装路径示例: `C:\Qt\6.5.3`

#### CMake
1. 下载 CMake
   - 访问: https://cmake.org/download/
   - 下载 `cmake-3.27.x-windows-x86_64.msi`
   - 安装时勾选 "Add CMake to system PATH"

#### Visual Studio 2019/2022
1. 下载 Visual Studio Community（免费）
   - 访问: https://visualstudio.microsoft.com/downloads/

2. 安装时选择工作负载：
   - "使用 C++ 的桌面开发"

### 2. 环境变量设置

打开 PowerShell 或 CMD，设置环境变量：

```cmd
# 设置 Qt 路径（根据实际安装路径修改）
set Qt6_DIR=C:\Qt\6.5.3\msvc2019_64
set PATH=%Qt6_DIR%\bin;%PATH%
```

或者永久设置（系统环境变量）：
1. 右键"此电脑" -> "属性"
2. "高级系统设置" -> "环境变量"
3. 在"系统变量"中添加：
   - 变量名: `Qt6_DIR`
   - 变量值: `C:\Qt\6.5.3\msvc2019_64`
4. 编辑 `PATH`，添加: `%Qt6_DIR%\bin`

---

## 编译测试程序

### 方法一：使用命令行编译

1. **打开 Visual Studio Developer Command Prompt**
   - 开始菜单搜索 "Developer Command Prompt for VS 2019/2022"
   - 或使用 PowerShell 并运行：
     ```cmd
     "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
     ```

2. **进入项目目录**
   ```cmd
   cd C:\path\to\YuntuClient
   ```

3. **创建构建目录**
   ```cmd
   mkdir build-test
   cd build-test
   ```

4. **配置 CMake**
   ```cmd
   cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64
   ```

   注意：如果使用 VS 2019，改为：
   ```cmd
   cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64
   ```

5. **编译**
   ```cmd
   cmake --build . --config Release
   ```

6. **运行测试程序**
   ```cmd
   Release\YuntuClient_Test.exe
   ```

### 方法二：使用 Qt Creator（推荐）

1. **打开 Qt Creator**
   - 位置: `C:\Qt\Tools\QtCreator\bin\qtcreator.exe`

2. **打开项目**
   - File -> Open File or Project
   - 选择 `YuntuClient/CMakeLists_Test.txt`

3. **配置 Kit**
   - 选择 "Desktop Qt 6.5.3 MSVC2019 64bit"
   - 点击 "Configure Project"

4. **构建**
   - 点击左下角"锤子"图标（Build）
   - 或按 `Ctrl + B`

5. **运行**
   - 点击左下角"播放"图标（Run）
   - 或按 `Ctrl + R`

### 方法三：使用批处理脚本（最简单）

创建 `build_test.bat` 文件：

```bat
@echo off
echo ========================================
echo   盛世云图客户端 - 测试程序构建脚本
echo ========================================

REM 设置 Qt 路径（根据实际情况修改）
set Qt6_DIR=C:\Qt\6.5.3\msvc2019_64
set PATH=%Qt6_DIR%\bin;%PATH%

REM 检查 Qt 环境
if not exist "%Qt6_DIR%" (
    echo 错误: Qt6 未找到，请检查路径
    pause
    exit /b 1
)

REM 创建构建目录
if exist build-test rmdir /s /q build-test
mkdir build-test
cd build-test

REM 配置 CMake（使用测试的 CMakeLists）
echo.
echo 配置 CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=%Qt6_DIR% -DCMAKE_PROJECT_INCLUDE=CMakeLists_Test.txt

REM 编译
echo.
echo 开始编译...
cmake --build . --config Release

REM 运行
if exist Release\YuntuClient_Test.exe (
    echo.
    echo ========================================
    echo   编译完成！启动测试程序...
    echo ========================================
    echo.
    Release\YuntuClient_Test.exe
) else (
    echo.
    echo 编译失败！
    pause
)

cd ..
```

然后双击运行 `build_test.bat`

---

## 运行测试

### 交互模式

直接运行程序：
```cmd
YuntuClient_Test.exe
```

会显示菜单：
```
========================================
  盛世云图客户端 - 功能测试
========================================
可用测试项:
  1. Maya 环境检测
  2. 配置管理
  3. 日志系统
  4. HTTP 客户端（需要后端）
  5. WebSocket 客户端（需要后端）
  0. 退出

选择测试项 (0-5):
```

### 命令行模式

```cmd
# 测试 Maya 检测
YuntuClient_Test.exe --maya

# 测试配置管理
YuntuClient_Test.exe --config

# 测试日志系统
YuntuClient_Test.exe --log

# 测试 HTTP（需要后端服务器）
YuntuClient_Test.exe --http

# 测试 WebSocket（需要后端服务器）
YuntuClient_Test.exe --ws

# 运行所有测试
YuntuClient_Test.exe --all
```

---

## 测试说明

### 1. Maya 环境检测测试

**测试内容：**
- 自动扫描系统中安装的 Maya 版本
- 检测渲染器（Arnold、V-Ray、Redshift）
- 识别已安装的插件

**预期结果：**
- 如果系统安装了 Maya，会显示：
  ```
  找到 2 个 Maya 版本

  ========== Maya 1 ==========
  软件名称: Maya
  版本号: 2024
  安装路径: C:/Program Files/Autodesk/Maya2024
  可执行文件: C:/Program Files/Autodesk/Maya2024/bin/maya.exe
  有效性: 是

  支持的渲染器:
    - Arnold Unknown

  已安装的插件 (15个):
    - mtoa
    - xgen
    - ...
  ```

- 如果未安装 Maya：
  ```
  找到 0 个 Maya 版本
  未检测到 Maya 安装
  ```

### 2. 配置管理测试

**测试内容：**
- 读取和显示配置信息
- 修改配置并保存

**预期结果：**
```
API 地址: https://api.yuntu.com
WebSocket 地址: wss://api.yuntu.com/ws
下载路径: C:/Users/xxx/Downloads/yuntu
自动下载: 启用
通知: 启用
...
```

配置文件位置：
```
C:\Users\{用户名}\AppData\Roaming\YunTu\YuntuClient.ini
```

### 3. 日志系统测试

**测试内容：**
- 输出不同级别的日志
- 验证日志写入文件

**预期结果：**
- 控制台显示彩色日志
- 日志文件位置：
  ```
  C:\Users\{用户名}\AppData\Roaming\YunTu\logs\2025-01-13.log
  ```

### 4. HTTP 客户端测试

**注意：** 需要后端服务器运行

**测试内容：**
- 发送 HTTP GET 请求

**预期结果：**
- 如果后端运行：
  ```
  ✓ HTTP GET 成功:
    响应: {...}
  ```

- 如果后端未运行（正常）：
  ```
  ✗ HTTP GET 失败:
    状态码: -1
    错误: Connection refused
    提示: 这是正常的，如果后端服务器未运行
  ```

### 5. WebSocket 客户端测试

**注意：** 需要后端服务器运行

**测试内容：**
- 连接到 WebSocket 服务器

**预期结果：**
- 如果后端运行：
  ```
  ✓ WebSocket 连接成功
  ```

- 如果后端未运行（正常）：
  ```
  ✗ WebSocket 错误: Connection refused
    提示: 这是正常的，如果后端服务器未运行
  ```

---

## 常见问题

### 1. 找不到 Qt6Core.dll

**错误信息：**
```
无法启动此程序，因为计算机中丢失 Qt6Core.dll
```

**解决方案：**
- 将 Qt 的 bin 目录添加到 PATH：
  ```cmd
  set PATH=C:\Qt\6.5.3\msvc2019_64\bin;%PATH%
  ```
- 或者将以下 DLL 复制到程序目录：
  - Qt6Core.dll
  - Qt6Network.dll
  - Qt6WebSockets.dll

### 2. CMake 找不到 Qt

**错误信息：**
```
Could not find a package configuration file provided by "Qt6"
```

**解决方案：**
- 设置 CMAKE_PREFIX_PATH：
  ```cmd
  cmake -DCMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64 ..
  ```

### 3. MSVC 编译器未找到

**错误信息：**
```
No CMAKE_CXX_COMPILER could be found
```

**解决方案：**
- 使用 Visual Studio Developer Command Prompt
- 或安装 Visual Studio

### 4. Maya 检测不到

**可能原因：**
- Maya 未安装
- Maya 安装在非标准位置

**解决方案：**
- 确认 Maya 已安装
- 检查安装路径是否在以下位置：
  - `C:\Program Files\Autodesk\Maya*`
  - `C:\Program Files (x86)\Autodesk\Maya*`

---

## 下一步

测试完成后，可以：

1. **查看日志文件**
   ```
   C:\Users\{用户名}\AppData\Roaming\YunTu\logs\
   ```

2. **修改配置**
   ```
   C:\Users\{用户名}\AppData\Roaming\YunTu\YuntuClient.ini
   ```

3. **继续开发**
   - 实现数据模型
   - 实现 UI 界面
   - 实现完整功能

---

## 联系支持

如果遇到问题：
1. 查看日志文件
2. 检查环境配置
3. 参考 README.md

**快速测试命令（一键测试所有功能）：**
```cmd
YuntuClient_Test.exe --all
```
