# 快速开始 - Windows 测试

## 🚀 最简单的方式（推荐新手）

### 步骤 1: 安装 Qt

1. 下载 Qt 在线安装器：https://www.qt.io/download-qt-installer
2. 安装 Qt 6.5.3 或更高版本（如 6.8.3），选择组件：
   - Qt 6.x.x → MSVC 2019 64-bit
3. 安装到默认路径（如 `C:\Qt\6.5.3` 或 `C:\Qt\6.8.3`）

### 步骤 2: 设置环境变量

右键点击"此电脑" → 属性 → 高级系统设置 → 环境变量

添加系统变量：
- 变量名：`Qt6_DIR`
- 变量值：`C:\Qt\6.8.3\msvc2019_64`（根据你的实际安装路径修改）
  - 如果安装的是 6.5.3：`C:\Qt\6.5.3\msvc2019_64`
  - 如果安装的是 6.8.3：`C:\Qt\6.8.3\msvc2019_64`

### 步骤 3: 编译和运行

1. **打开项目文件夹**
   ```
   将项目复制到 Windows 电脑
   例如: C:\Projects\YuntuClient
   ```

2. **双击运行编译脚本**
   ```
   双击：build_test.bat
   ```

3. **等待编译完成**
   - 如果成功，会自动启动测试程序
   - 如果失败，查看错误信息

4. **选择测试项目**
   ```
   可用测试项:
     1. Maya 环境检测
     2. 配置管理
     3. 日志系统
     4. HTTP 客户端（需要后端）
     5. WebSocket 客户端（需要后端）
     0. 退出

   选择测试项 (0-5):
   ```

---

## 📋 测试项说明

### 1. Maya 环境检测 ⭐ 推荐先测试

**功能：**
- 自动扫描系统中安装的 Maya 版本
- 检测渲染器（Arnold、V-Ray、Redshift）
- 识别已安装的插件

**测试方法：**
1. 在菜单中输入 `1` 并回车
2. 等待扫描完成（约 5-10 秒）

**预期结果：**
- 如果安装了 Maya：显示版本、路径、渲染器、插件列表
- 如果未安装：显示"未检测到 Maya 安装"

**示例输出：**
```
开始检测系统中的 Maya 版本...
[10%] 正在扫描 Maya 安装路径...
[30%] 找到 2 个可能的 Maya 安装路径
[50%] 检测: C:/Program Files/Autodesk/Maya2024
...

检测结果：
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
  - bifrost
  ...
```

### 2. 配置管理

**功能：**
- 显示当前配置
- 测试配置读写

**预期结果：**
```
API 地址: https://api.yuntu.com
WebSocket 地址: wss://api.yuntu.com/ws
下载路径: C:/Users/xxx/Downloads/yuntu
自动下载: 启用
通知: 启用
```

### 3. 日志系统

**功能：**
- 测试日志输出
- 验证日志文件写入

**预期结果：**
- 控制台显示不同级别的日志
- 日志文件自动创建在：
  ```
  C:\Users\{用户名}\AppData\Roaming\YunTu\logs\
  ```

### 4. HTTP 客户端

⚠️ **注意：需要后端服务器运行**

如果后端未运行，会显示连接失败（这是正常的）

### 5. WebSocket 客户端

⚠️ **注意：需要后端服务器运行**

如果后端未运行，会显示连接失败（这是正常的）

---

## 📂 查看测试结果

### 配置文件
```
C:\Users\{用户名}\AppData\Roaming\YunTu\YuntuClient.ini
```

可以用记事本打开查看配置内容。

### 日志文件
```
C:\Users\{用户名}\AppData\Roaming\YunTu\logs\2025-01-13.log
```

包含详细的运行日志。

---

## ❌ 常见问题

### 问题 1: 找不到 Qt6Core.dll

**解决：**
脚本会自动复制 DLL，如果还是报错：
1. 确认 Qt 正确安装
2. 检查 `Qt6_DIR` 环境变量是否正确
3. 重新运行 `build_test.bat`

### 问题 2: CMake 找不到

**解决：**
1. 下载安装 CMake：https://cmake.org/download/
2. 安装时勾选"Add CMake to system PATH"
3. 重启命令行

### 问题 3: Visual Studio 错误

**解决：**
如果使用 VS 2019 而不是 VS 2022：
1. 编辑 `build_test.bat`
2. 找到这一行：
   ```
   cmake .. -G "Visual Studio 17 2022" -A x64 ^
   ```
3. 改为：
   ```
   cmake .. -G "Visual Studio 16 2019" -A x64 ^
   ```

### 问题 4: Maya 检测不到

**可能原因：**
- Maya 未安装
- Maya 安装在非标准位置

**验证方法：**
检查以下路径是否存在 `maya.exe`：
```
C:\Program Files\Autodesk\Maya2024\bin\maya.exe
C:\Program Files\Autodesk\Maya2023\bin\maya.exe
```

---

## 🎯 命令行快捷测试

如果你熟悉命令行，可以快速测试：

```cmd
# 进入构建目录
cd build-test\Release

# 测试 Maya 检测
YuntuClient_Test.exe --maya

# 测试所有功能
YuntuClient_Test.exe --all
```

---

## 📸 测试截图

**成功编译：**
```
========================================
  编译完成！
========================================

可执行文件位置: C:\...\build-test\Release\YuntuClient_Test.exe

运行测试程序...
```

**Maya 检测成功：**
```
找到 2 个 Maya 版本

========== Maya 1 ==========
软件名称: Maya
版本号: 2024
...
```

---

## 💬 反馈测试结果

测试完成后，请反馈：

1. **编译是否成功？**
2. **Maya 检测结果：**
   - 检测到几个版本？
   - 版本号是多少？
   - 检测到哪些渲染器和插件？
3. **有没有报错？**
4. **日志文件是否正常生成？**

这些信息将帮助我们改进软件！

---

## 🚀 下一步

测试完成后，可以：
1. 查看源代码了解实现细节
2. 等待 UI 界面实现
3. 继续测试其他功能

**项目状态：**
- ✅ 核心功能：100%
- ✅ Maya 检测：100%
- ✅ 网络层：100%
- ⏳ UI 界面：0%
- ⏳ 完整流程：30%
