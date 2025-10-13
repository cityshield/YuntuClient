# GitHub Actions 自动构建指南

本指南将帮助你使用 GitHub Actions 自动编译 Windows 可执行文件，**无需配置本地环境**。

## 📋 前置准备

1. **GitHub 账号**（你已经有了 ✓）
2. **Git 客户端**（用于上传代码）

---

## 🚀 步骤 1: 创建 GitHub 仓库

### 方法 A: 通过网页创建

1. 访问 https://github.com
2. 登录你的账号
3. 点击右上角 **"+"** → **"New repository"**
4. 填写信息：
   - **Repository name**: `YuntuClient`（或任意名称）
   - **Description**: `盛世云图客户端 - Maya云渲染工具`
   - **Public** 或 **Private**（都可以，Actions 都能用）
5. **不要勾选** "Initialize this repository with a README"
6. 点击 **"Create repository"**

### 方法 B: 使用 GitHub Desktop（更简单）

1. 下载安装 GitHub Desktop: https://desktop.github.com/
2. 登录你的 GitHub 账号
3. 使用图形界面创建和上传仓库

---

## 📤 步骤 2: 上传项目到 GitHub

### 方法 A: 使用命令行 Git

打开命令行，进入项目目录：

```bash
# 进入项目目录
cd C:\Users\USER\Downloads\YuntuClient

# 初始化 Git 仓库
git init

# 添加所有文件
git add .

# 创建第一个提交
git commit -m "Initial commit: YuntuClient Test"

# 添加远程仓库（替换 YOUR_USERNAME 为你的 GitHub 用户名）
git remote add origin https://github.com/YOUR_USERNAME/YuntuClient.git

# 创建并切换到 main 分支
git branch -M main

# 推送到 GitHub
git push -u origin main
```

### 方法 B: 使用 GitHub Desktop（推荐新手）

1. 打开 GitHub Desktop
2. **File** → **Add Local Repository**
3. 选择 `C:\Users\USER\Downloads\YuntuClient` 文件夹
4. 如果提示 "This directory does not appear to be a Git repository"：
   - 点击 **"Create a repository"**
   - 填写 Name 和 Description
   - 点击 **"Create Repository"**
5. 在左下角输入 Commit 信息：`Initial commit`
6. 点击 **"Commit to main"**
7. 点击顶部的 **"Publish repository"**
8. 选择是否公开，点击 **"Publish Repository"**

---

## ⚙️ 步骤 3: 触发自动构建

### 自动触发

推送代码后，GitHub Actions 会**自动开始构建**：

1. 访问你的 GitHub 仓库页面
2. 点击顶部的 **"Actions"** 标签
3. 你会看到一个正在运行的工作流：**"Build Windows Test"**
4. 点击它查看构建进度

### 手动触发

如果需要手动触发构建：

1. 进入 **Actions** 页面
2. 点击左侧的 **"Build Windows Test"**
3. 点击右侧的 **"Run workflow"** 按钮
4. 选择分支（main）
5. 点击绿色的 **"Run workflow"** 按钮

---

## 📥 步骤 4: 下载编译好的 EXE

### 等待构建完成

构建通常需要 **5-10 分钟**。完成后：

1. 进入 **Actions** 页面
2. 点击最新的成功构建（绿色勾号 ✓）
3. 滚动到页面底部，找到 **"Artifacts"** 区域
4. 点击 **"YuntuClient-Test-Windows-x64"** 下载 ZIP 文件

### 解压并运行

1. 下载的文件名：`YuntuClient_Test_Windows_x64.zip`
2. 解压到任意位置
3. 双击 `YuntuClient_Test.exe` 运行测试程序
4. **所有需要的 DLL 都已经包含在内**，可以直接运行！

---

## 🎯 测试程序使用方法

运行 `YuntuClient_Test.exe` 后：

```
========================================
  盛世云图客户端 - 功能测试
========================================
可用测试项:
  1. Maya 环境检测      ⭐ 推荐先测试
  2. 配置管理
  3. 日志系统
  4. HTTP 客户端（需要后端）
  5. WebSocket 客户端（需要后端）
  0. 退出

选择测试项 (0-5):
```

**推荐先测试项目 1**，查看是否能检测到你系统中的 Maya 安装。

---

## 🔄 更新代码后重新构建

每次你修改代码后：

1. **提交更改**：
   ```bash
   git add .
   git commit -m "描述你的修改"
   git push
   ```

2. **自动构建**：推送后会自动触发新的构建

3. **下载新版本**：从 Actions 页面下载最新的构建结果

---

## 🏷️ 创建正式版本（Release）

如果想创建一个正式版本：

1. **创建标签**：
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. **自动创建 Release**：GitHub Actions 会自动创建 Release，并附带编译好的文件

3. **下载地址**：仓库页面 → **Releases** → 下载对应版本

---

## ❓ 常见问题

### Q1: 构建失败怎么办？

1. 点击失败的构建查看日志
2. 查找红色的错误信息
3. 常见问题：
   - CMake 配置错误：检查 CMakeLists_Test.txt
   - 编译错误：检查源代码语法
   - Qt 模块缺失：修改 `.github/workflows/build-windows.yml` 中的 modules

### Q2: 构建时间太长？

- 正常情况：5-10 分钟
- 第一次构建会更慢（需要下载 Qt）
- 后续构建会使用缓存，更快

### Q3: 能否构建 macOS 或 Linux 版本？

可以！只需要创建对应的 workflow 文件：
- `.github/workflows/build-macos.yml`
- `.github/workflows/build-linux.yml`

### Q4: Actions 有使用限制吗？

- **Public 仓库**：无限制免费
- **Private 仓库**：每月 2000 分钟免费额度

---

## 🎉 优势总结

✅ **无需本地环境**：不需要安装 Qt、CMake、Visual Studio
✅ **自动化构建**：每次推送代码自动编译
✅ **多人协作**：团队成员都能下载最新版本
✅ **跨平台**：可以同时构建 Windows、macOS、Linux 版本
✅ **版本管理**：每个构建都有记录，可追溯

---

## 📚 相关资源

- GitHub Actions 文档: https://docs.github.com/actions
- Qt 官方文档: https://doc.qt.io/
- 项目 README: README.md
- 快速开始: QUICK_START.md

---

## 🆘 需要帮助？

如果遇到问题：

1. 查看 Actions 构建日志
2. 检查错误信息
3. 参考项目文档
4. GitHub Issues 提问

**祝你构建成功！** 🚀
