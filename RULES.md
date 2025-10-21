# YuntuClient 开发规范和技术参考

> **重要**: 在修改任何代码之前，请先阅读本文件，确保遵循项目规范和使用正确的技术参考资料。

---

## 📋 目录

- [阿里云 OSS SDK 规范](#阿里云-oss-sdk-规范)
- [Qt 开发规范](#qt-开发规范)
- [代码风格规范](#代码风格规范)
- [网络层规范](#网络层规范)

---

## 🗄️ 阿里云 OSS SDK 规范

### ⚠️ 必读文档

在修改任何 OSS 相关代码前，**必须**参考以下官方文档：

#### 1. 官方技术文档（中国大陆版）

- **断点续传上传**: https://help.aliyun.com/zh/oss/developer-reference/resumable-upload-2
- **分片上传**: https://help.aliyun.com/zh/oss/developer-reference/multipart-upload-9
- **C++ SDK 概览**: https://help.aliyun.com/zh/oss/developer-reference/overview-67
- **C++ SDK 快速入门**: https://help.aliyun.com/zh/oss/developer-reference/getting-started-with-oss-sdk-for-cpp

#### 2. 官方 SDK 源码

- **GitHub 仓库**: https://github.com/aliyun/aliyun-oss-cpp-sdk
- **示例代码**: https://github.com/aliyun/aliyun-oss-cpp-sdk/blob/master/sample/src/object/ObjectSample.cc
- **头文件参考**: https://github.com/aliyun/aliyun-oss-cpp-sdk/tree/master/sdk/include/alibabacloud/oss

### 🔑 关键技术要点

#### ResumableUploadObject（断点续传上传）

**UploadObjectRequest 构造函数参数**：

```cpp
// 方式1: 无 checkpoint（不推荐，无法断点续传）
UploadObjectRequest request(bucketName, objectName, filePath);

// 方式2: 带 checkpoint 目录（推荐）
UploadObjectRequest request(bucketName, objectName, filePath, checkpointDir);

// 方式3: 完整参数（最佳实践）
UploadObjectRequest request(bucketName, objectName, filePath,
                           checkpointDir, partSize, threadNum);
```

**⚠️ 重要注意事项**：

1. **checkpoint 参数必须是目录路径，不是文件路径**
   - ✅ 正确: `"D:\\temp\\checkpoints"` 或 `"/tmp/checkpoints"`
   - ❌ 错误: `"D:\\temp\\checkpoints\\task123.checkpoint"`
   - SDK 会自动在该目录下创建和管理 checkpoint 文件

2. **checkpoint 目录必须提前创建并存在**
   - 使用 `QDir::mkpath()` 创建目录
   - 创建后验证目录是否真的存在

3. **SDK 自动管理 checkpoint 文件生命周期**
   - 上传过程中：SDK 自动创建和更新 checkpoint 文件
   - 上传成功后：SDK 自动删除 checkpoint 文件
   - **不要手动删除 checkpoint 文件**

4. **分片大小限制**
   - 最小值: 100 KB（最后一片除外）
   - 最大值: 5 GB
   - 推荐值: 5-10 MB（根据网络状况调整）

5. **STS 临时凭证**
   - 使用 STS（Security Token Service）临时凭证，不要使用主账号密钥
   - 凭证包含: `accessKeyId`, `accessKeySecret`, `securityToken`
   - 注意凭证过期时间

#### 错误处理

常见错误码：

- `ValidateError`: 参数验证失败（通常是 checkpoint 目录不存在或参数类型错误）
- `RequestTimeout`: 请求超时（调整 `requestTimeoutMs`）
- `SignatureDoesNotMatch`: 签名错误（检查 STS 凭证是否正确）

### 📁 相关文件

修改 OSS 相关功能时，可能涉及以下文件：

- `src/network/OSSUploader.h` - OSS 上传器头文件
- `src/network/OSSUploader.cpp` - OSS 上传器实现
- `src/ui/views/CreateTaskDialog.cpp` - 创建任务对话框（调用 OSS 上传）
- `docs/OSS_SDK_DLL_Guide.md` - OSS SDK DLL 部署指南

### 🚫 常见错误示例

#### ❌ 错误: 传递文件路径而不是目录路径

```cpp
// 错误示例
QString checkpointFile = checkpointDir + "/" + taskId + ".checkpoint";
UploadObjectRequest request(bucket, key, file, checkpointFile.toStdString());
```

#### ✅ 正确: 传递目录路径

```cpp
// 正确示例
UploadObjectRequest request(bucket, key, file, checkpointDir.toStdString());
```

---

## 🎨 Qt 开发规范

### 版本要求

- **Qt 版本**: 6.5.3 或更高
- **编译器**: MSVC 2019/2022 (Windows), Clang (macOS)
- **C++ 标准**: C++17

### 必需模块

```cmake
Qt6::Core
Qt6::Gui
Qt6::Widgets
Qt6::Network
Qt6::Sql
Qt6::WebSockets
```

### 路径处理规范

1. **使用 Qt 路径 API**，不要使用字符串拼接：
   ```cpp
   // ✅ 正确
   QString path = QDir(baseDir).filePath("subdir/file.txt");

   // ❌ 错误
   QString path = baseDir + "/subdir/file.txt";
   ```

2. **跨平台路径分隔符**：
   ```cpp
   QString nativePath = QDir::toNativeSeparators(path);
   ```

3. **创建目录**：
   ```cpp
   QDir dir(dirPath);
   if (!dir.exists()) {
       dir.mkpath(".");  // 创建目录及所有父目录
   }
   ```

### 信号与槽

- 优先使用新式信号槽语法（lambda 或函数指针）
- 避免使用 `SIGNAL()` 和 `SLOT()` 宏

### 内存管理

- UI 组件使用父子关系自动管理内存
- 手动 `new` 的对象要么指定父对象，要么在析构函数中 `delete`

---

## 🎯 代码风格规范

### 命名约定

- **类名**: PascalCase (如 `OSSUploader`, `TaskManager`)
- **函数名**: camelCase (如 `startUpload`, `createTask`)
- **成员变量**: `m_` 前缀 + camelCase (如 `m_isUploading`, `m_fileSize`)
- **常量**: UPPER_SNAKE_CASE (如 `DEFAULT_PART_SIZE`)

### 注释规范

```cpp
/**
 * @brief 函数简要描述
 * @param paramName 参数说明
 * @return 返回值说明
 */
void functionName(int paramName);
```

### 字符和图标规范

**⚠️ 禁止在代码中使用 Emoji 表情符号**

- **原因**: Emoji 可能导致跨平台编译错误、编码问题、源文件损坏
- **替代方案**: 使用纯 ASCII 字符或英文缩写标识

#### ❌ 错误示例：

```cpp
// 错误：使用 Emoji
QString icon = QString::fromUtf8("✅");  // 编译可能失败
QString status = QString::fromUtf8("📤 上传中");  // 跨平台问题
```

#### ✅ 正确示例：

```cpp
// 正确：使用 ASCII 字符或英文缩写
QString icon = QString::fromUtf8("[OK]");  // 使用方括号标识
QString status = QString::fromUtf8("[U] 上传中");  // [U] = Uploading
```

#### 状态图标建议映射：

```cpp
Draft      -> "[D]"   // 草稿
Uploading  -> "[U]"   // 上传中
Pending    -> "[P]"   // 待审核
Queued     -> "[Q]"   // 队列中
Rendering  -> "[R]"   // 渲染中
Paused     -> "[||]"  // 已暂停
Completed  -> "[OK]"  // 已完成
Failed     -> "[X]"   // 失败
Cancelled  -> "[-]"   // 已取消
```

### 日志规范

使用统一的日志系统：

```cpp
Application::instance().logger()->info("ModuleName", "日志信息");
Application::instance().logger()->warning("ModuleName", "警告信息");
Application::instance().logger()->error("ModuleName", "错误信息");
```

### 错误处理

- 使用 Qt 信号发送错误信息
- 关键操作使用 `try-catch` 捕获异常
- 提供用户友好的错误提示

---

## 🌐 网络层规范

### HTTP 客户端 (HttpClient)

- 基于 `QNetworkAccessManager`
- 自动管理 JWT Token
- 自动刷新过期 Token
- 支持请求超时设置

### WebSocket 客户端 (WebSocketClient)

- 实时通信
- 自动重连机制
- 心跳检测

### API 服务 (ApiService)

- 统一的 API 调用接口
- 使用回调函数处理成功/失败
- 自动处理身份验证

### 文件上传

- **小文件 (<100MB)**: 使用 FileUploader（分块上传）
- **大文件 (>=100MB)**: 使用 OSSUploader（OSS 直传）

---

## 📦 构建和部署

### 构建配置

- **开发环境**: CMakeLists.txt
- **测试程序**: CMakeLists_Test.txt

### CI/CD

- **GitHub Actions**: `.github/workflows/`
  - `build-windows-gui.yml` - GUI 程序构建
  - `build-windows.yml` - 测试程序构建

### DLL 依赖

Windows 部署时必需的 DLL：

**Qt DLLs**:
- Qt6Core.dll
- Qt6Gui.dll
- Qt6Widgets.dll
- Qt6Network.dll
- Qt6WebSockets.dll
- Qt6Sql.dll

**OSS SDK DLLs** (如果启用 ENABLE_OSS_SDK):
- alibabacloud-oss-cpp-sdk.dll
- libcurl.dll
- libeay32.dll / libcrypto-1_1-x64.dll
- ssleay32.dll / libssl-1_1-x64.dll
- zlibwapi.dll / zlib1.dll

详见：`docs/OSS_SDK_DLL_Guide.md`

---

## 🔄 版本控制规范

### Git 工作流

- **main 分支**: 稳定版本
- **dev 分支**: 开发版本
- **feature/* 分支**: 新功能开发
- **bugfix/* 分支**: Bug 修复

### Commit 消息格式

```
<type>: <subject>

<body>

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

**Type 类型**:
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具链相关

---

## 📚 其他参考资料

### Qt 文档

- **Qt 6 Documentation**: https://doc.qt.io/qt-6/
- **Qt Network**: https://doc.qt.io/qt-6/qtnetwork-index.html
- **Qt WebSockets**: https://doc.qt.io/qt-6/qtwebsockets-index.html

### C++ 参考

- **C++ Reference**: https://en.cppreference.com/
- **Modern C++ Best Practices**: https://github.com/cpp-best-practices/cppbestpractices

### 项目文档

- `README.md` - 项目概览和构建说明
- `docs/OSS_SDK_DLL_Guide.md` - OSS SDK DLL 部署指南
- `RULES.md` - 本文件

---

## 🔄 更新记录

- **2025-10-21**:
  - 创建初始版本，添加阿里云 OSS SDK 规范
  - 添加字符和图标规范（禁止使用 Emoji）
- 后续更新将在此记录...

---

**最后更新**: 2025-10-21
**维护者**: YuntuClient 开发团队
