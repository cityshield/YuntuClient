# Aliyun OSS C++ SDK DLL 部署指南

## 问题说明

当运行 YuntuClient 时，如果提示缺少 `libcurl.dll` 或其他 DLL，说明 OSS SDK 的第三方依赖库没有正确部署。

## 需要的 DLL 文件

### 1. OSS SDK 主 DLL
- `alibabacloud-oss-cpp-sdk.dll` - OSS C++ SDK 主库

### 2. 第三方依赖 DLL
- `libcurl.dll` - HTTP 客户端库
- `libeay32.dll` - OpenSSL 加密库（legacy）
- `ssleay32.dll` - OpenSSL SSL/TLS 库（legacy）
- `zlibwapi.dll` - Zlib 压缩库

## 解决方案

### 方案 1: 从 GitHub Actions 下载完整构建

1. 前往项目的 GitHub Actions 页面
2. 下载最新的 `YuntuClient-Windows-x64-{timestamp}` artifact
3. 解压后，所有必需的 DLL 都已包含在内

### 方案 2: 手动编译 OSS SDK（本地开发）

如果您在本地编译 YuntuClient，需要手动复制 DLL：

#### 步骤 1: 编译 Aliyun OSS C++ SDK

```powershell
# 克隆 SDK
git clone --depth 1 --branch 1.10.1 https://github.com/aliyun/aliyun-oss-cpp-sdk.git
cd aliyun-oss-cpp-sdk

# 创建构建目录
mkdir build
cd build

# 配置 CMake
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_INSTALL_PREFIX="C:\OSS-SDK" `
  -DBUILD_SHARED_LIBS=ON

# 编译并安装
cmake --build . --config Release --target install
```

#### 步骤 2: 复制 DLL 到应用程序目录

将以下 DLL 复制到 YuntuClient.exe 所在目录：

**从 OSS SDK 安装目录 (`C:\OSS-SDK\bin`)：**
```powershell
Copy-Item "C:\OSS-SDK\bin\*.dll" "path\to\YuntuClient\Release\"
```

**从 OSS SDK 源码第三方库 (`aliyun-oss-cpp-sdk\third_party\lib\x64`)：**
```powershell
Copy-Item "aliyun-oss-cpp-sdk\third_party\lib\x64\libcurl.dll" "path\to\YuntuClient\Release\"
Copy-Item "aliyun-oss-cpp-sdk\third_party\lib\x64\libeay32.dll" "path\to\YuntuClient\Release\"
Copy-Item "aliyun-oss-cpp-sdk\third_party\lib\x64\ssleay32.dll" "path\to\YuntuClient\Release\"
Copy-Item "aliyun-oss-cpp-sdk\third_party\lib\x64\zlibwapi.dll" "path\to\YuntuClient\Release\"
```

### 方案 3: 从 OSS SDK 发布包下载

您也可以直接从阿里云 OSS SDK 的官方发布包中获取 DLL：

1. 访问：https://github.com/aliyun/aliyun-oss-cpp-sdk/releases/tag/1.10.1
2. 下载 Windows 预编译包（如果有）
3. 解压并复制 DLL 文件

## DLL 依赖检查工具

使用 Dependency Walker 或 Dependencies 工具查看应用程序的 DLL 依赖：

- **Dependencies**（推荐）: https://github.com/lucasg/Dependencies
- **Dependency Walker**: http://www.dependencywalker.com/

运行工具并打开 `YuntuClient.exe`，可以看到所有缺失的 DLL。

## 常见问题

### Q1: 为什么 GitHub Actions 构建的版本没有这个问题？

A: GitHub Actions 工作流已配置为自动复制所有必需的 DLL。具体参见 `.github/workflows/build-windows-gui.yml` 的 "Copy Qt DLLs" 步骤。

### Q2: 能否使用静态链接避免 DLL 依赖？

A: 可以，但需要重新编译 OSS SDK 和 Qt（设置 `BUILD_SHARED_LIBS=OFF`），这会显著增加编译时间和最终可执行文件大小。当前推荐使用动态链接。

### Q3: libcurl.dll 从哪里来？

A: libcurl.dll 是 Aliyun OSS C++ SDK 的第三方依赖，随 SDK 源码一起提供在 `third_party/lib/x64` 目录中。

### Q4: 如何验证 DLL 版本兼容性？

A: 确保所有 DLL 都是为 x64 架构编译的，且使用相同的 MSVC 运行时（建议使用 Visual Studio 2022 / MSVC 2019）。

## CI/CD 配置说明

项目的 GitHub Actions 工作流已更新（v3 缓存）：

1. **构建时**：将第三方 DLL 复制到 `oss-sdk-third-party` 目录并缓存
2. **打包时**：从缓存目录复制 DLL 到 Release 目录
3. **缓存键**：`oss-sdk-1.10.1-win-v3-${{ runner.os }}`

这确保了即使使用缓存的 OSS SDK，第三方 DLL 也能正确包含。

## 技术参考

- **Aliyun OSS C++ SDK**: https://github.com/aliyun/aliyun-oss-cpp-sdk
- **官方文档**: https://help.aliyun.com/zh/oss/developer-reference/cpp/
- **OpenSSL**: https://www.openssl.org/
- **libcurl**: https://curl.se/libcurl/
- **zlib**: https://zlib.net/

---

**更新日期**: 2025-10-21
**适用版本**: YuntuClient v1.0+, OSS SDK v1.10.1
