# 阿里云 OSS 日志上传配置指南

## 📋 功能说明

本客户端会自动将日志文件上传到阿里云 OSS，方便远程调试和问题排查。

## 🔧 配置步骤

### 1. 创建配置文件

在编译后的可执行文件目录下创建 `.env` 文件（可以复制 `.env.example`）：

```bash
# 在 build 目录或可执行文件目录下
cp .env.example .env
```

### 2. 填写 OSS 配置

编辑 `.env` 文件，填入真实的阿里云 OSS 配置：

```env
# 阿里云 OSS 配置
OSS_ACCESS_KEY_ID=你的AccessKeyID
OSS_ACCESS_KEY_SECRET=你的AccessKeySecret
OSS_BUCKET_NAME=你的Bucket名称
OSS_ENDPOINT=oss-cn-hangzhou.aliyuncs.com
```

**示例：**
```env
OSS_ACCESS_KEY_ID=LTAI5tXXXXXXXXXXXXXX
OSS_ACCESS_KEY_SECRET=your_secret_key_here
OSS_BUCKET_NAME=your-bucket-name
OSS_ENDPOINT=oss-cn-hangzhou.aliyuncs.com
```

### 3. 运行程序

启动程序后，会自动：
1. 读取 `.env` 文件中的 OSS 配置
2. 延迟 3 秒后开始上传日志
3. 将日志上传到 OSS 的 `logs/YYYY-MM-DD/` 目录

## 📝 日志路径规则

上传到 OSS 的路径格式：
```
logs/2025-10-17/2025-10-17.log
logs/2025-10-17/maya_detection.log
```

## 🔍 调试日志

可以在应用程序日志中查看上传状态：

```
[2025-10-17 15:30:00] [INFO] 已从 .env 文件加载 OSS 配置
[2025-10-17 15:30:03] [INFO] 开始上传日志文件到 OSS
[2025-10-17 15:30:03] [DEBUG] ========== 开始上传日志到 OSS ==========
[2025-10-17 15:30:03] [DEBUG] 总共 1 个文件
[2025-10-17 15:30:03] [DEBUG] 准备上传到 OSS: https://oss-ssyt.oss-cn-hangzhou.aliyuncs.com/logs/2025-10-17/2025-10-17.log
[2025-10-17 15:30:04] [DEBUG] 日志上传成功
[2025-10-17 15:30:04] [INFO] 所有日志上传完成
```

## ⚠️ 注意事项

1. **不要将 `.env` 文件提交到 Git**
   - `.env` 已添加到 `.gitignore`
   - 仅提交 `.env.example` 作为模板

2. **确保 OSS Bucket 权限正确**
   - 需要有上传（PUT Object）权限
   - 建议创建专门用于日志的 Bucket

3. **文件位置**
   - `.env` 文件需要放在可执行文件所在目录
   - 如果是 Debug 版本，放在 `build/Debug/` 下
   - 如果是 Release 版本，放在 `build/Release/` 下

4. **Windows 用户**
   - 编译后需要将 `.env` 文件复制到 exe 文件同目录

## 🚀 实现细节

### OSS 签名算法

使用 HMAC-SHA1 签名：
```
Signature = base64(hmac-sha1(AccessKeySecret,
    VERB + "\n"
    + Content-MD5 + "\n"
    + Content-Type + "\n"
    + Date + "\n"
    + CanonicalizedOSSHeaders
    + CanonicalizedResource))
```

### 上传流程

1. 应用启动时加载 `.env` 配置
2. 3 秒后触发日志上传
3. 使用 `QNetworkAccessManager` 发送 PUT 请求
4. 添加正确的签名和 Authorization 头
5. 上传成功或失败都会记录日志

## 📁 相关文件

- `src/core/Application.cpp` - 加载 .env 配置
- `src/core/Config.h/cpp` - OSS 配置管理
- `src/services/LogUploader.h/cpp` - 日志上传实现
- `.env.example` - 配置文件模板
- `.env` - 实际配置（不提交到 git）

## 🔗 相关资源

- [阿里云 OSS 文档](https://help.aliyun.com/product/31815.html)
- [OSS API 签名](https://help.aliyun.com/document_detail/31951.html)
