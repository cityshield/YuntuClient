# GitHub Actions 构建指南 - macOS 版本

**适用于：** 在 macOS 上开发，需要构建 Windows 可执行文件

## 🎯 方案优势

✅ 在 Mac 上开发，自动构建 Windows exe
✅ 无需安装 Windows 虚拟机或双系统
✅ 无需配置复杂的交叉编译环境
✅ 自动化：每次提交代码自动构建
✅ 可以同时构建 Windows、macOS、Linux 版本

---

## 📋 前置准备

### 1. 检查 Git 是否已安装

打开终端（Terminal），运行：

```bash
git --version
```

如果显示版本号（如 `git version 2.x.x`），说明已安装 ✓

如果未安装，运行：
```bash
xcode-select --install
```

### 2. GitHub 账号

确保你已经有 GitHub 账号并登录。

---

## 🚀 快速上传到 GitHub（5 分钟）

### 步骤 1: 在 GitHub 创建新仓库

1. 访问 https://github.com/new
2. 填写信息：
   - **Repository name**: `YuntuClient`
   - **Description**: `盛世云图客户端 - Maya云渲染工具`
   - 选择 **Public** 或 **Private**（都可以）
   - **不要勾选** "Add a README file"
3. 点击 **"Create repository"**

创建后，GitHub 会显示一个页面，**先不要关闭**，后面会用到。

---

### 步骤 2: 在 Mac 上传项目

打开**终端**（Terminal），执行以下命令：

```bash
# 1. 进入项目目录
cd /Users/pretty/Documents/Workspace/YuntuClient

# 2. 初始化 Git 仓库
git init

# 3. 添加所有文件
git add .

# 4. 创建第一次提交
git commit -m "Initial commit: YuntuClient with GitHub Actions"

# 5. 添加远程仓库（替换下面的 YOUR_USERNAME 为你的 GitHub 用户名）
git remote add origin https://github.com/YOUR_USERNAME/YuntuClient.git

# 6. 创建并切换到 main 分支
git branch -M main

# 7. 推送到 GitHub
git push -u origin main
```

**说明：**
- 第 5 步的 URL 在 GitHub 创建仓库后的页面上可以找到
- 如果提示输入用户名和密码，输入你的 GitHub 账号信息
- **推荐使用 Personal Access Token 代替密码**（见下方说明）

---

### 🔑 配置 GitHub Personal Access Token（推荐）

GitHub 已不再支持密码推送，需要使用 Token：

1. 访问 https://github.com/settings/tokens
2. 点击 **"Generate new token"** → **"Generate new token (classic)"**
3. 设置：
   - **Note**: `YuntuClient Development`
   - **Expiration**: `90 days`（或更长）
   - **Scopes**: 勾选 `repo`（全部）
4. 点击 **"Generate token"**
5. **复制生成的 token**（只显示一次！）

推送时，使用 token 代替密码：
- Username: 你的 GitHub 用户名
- Password: 粘贴刚才复制的 token

---

## ⏳ 步骤 3: 等待自动构建（5-10 分钟）

推送完成后：

1. 访问你的 GitHub 仓库页面：
   ```
   https://github.com/YOUR_USERNAME/YuntuClient
   ```

2. 点击顶部的 **"Actions"** 标签

3. 你会看到一个正在运行的工作流：
   ```
   🟡 Initial commit: YuntuClient with GitHub Actions
   ```

4. 点击它查看实时构建日志

5. 等待约 5-10 分钟，构建完成后会变成：
   ```
   ✅ Initial commit: YuntuClient with GitHub Actions
   ```

---

## 📥 步骤 4: 下载编译好的 Windows EXE

构建成功后：

1. 在 Actions 页面，点击成功的构建（绿色勾号 ✅）

2. 滚动到页面底部，找到 **"Artifacts"** 区域

3. 点击下载：
   ```
   YuntuClient-Test-Windows-x64
   ```

4. 下载的是一个 ZIP 文件：
   ```
   YuntuClient_Test_Windows_x64.zip
   ```

5. **解压后包含**：
   - `YuntuClient_Test.exe` - 测试程序
   - `Qt6Core.dll`
   - `Qt6Network.dll`
   - `Qt6WebSockets.dll`
   - `Qt6Sql.dll`

6. **在 Windows 电脑上**运行 `YuntuClient_Test.exe`

---

## 🔄 后续开发流程

每次修改代码后：

```bash
# 1. 查看修改的文件
git status

# 2. 添加修改
git add .

# 3. 提交修改（写清楚改了什么）
git commit -m "描述你的修改内容"

# 4. 推送到 GitHub
git push

# 5. GitHub Actions 会自动触发新的构建
```

然后去 Actions 页面下载最新构建的 exe。

---

## 🎨 使用 GitHub Desktop（图形界面，更简单）

如果不习惯命令行，可以使用 GitHub Desktop：

### 1. 下载安装

https://desktop.github.com/

### 2. 登录 GitHub 账号

打开 GitHub Desktop，登录你的账号。

### 3. 添加本地仓库

1. **File** → **Add Local Repository**
2. 选择项目文件夹：`/Users/pretty/Documents/Workspace/YuntuClient`
3. 如果提示 "not a Git repository"，点击 **"Create a repository"**

### 4. 发布到 GitHub

1. 左下角输入 Commit 信息：`Initial commit: YuntuClient`
2. 点击 **"Commit to main"**
3. 点击顶部的 **"Publish repository"**
4. 选择仓库名称和公开/私有
5. 点击 **"Publish Repository"**

完成！后续修改只需要：
1. 在左侧勾选要提交的文件
2. 输入 Commit 信息
3. 点击 **"Commit to main"**
4. 点击 **"Push origin"**

---

## 🏷️ 创建正式发布版本

当你完成一个重要功能，想创建正式版本：

```bash
# 1. 创建标签
git tag -a v1.0.0 -m "首个测试版本"

# 2. 推送标签
git push origin v1.0.0
```

GitHub Actions 会自动：
1. 构建 Windows exe
2. 创建 GitHub Release
3. 自动附加编译好的文件

然后任何人都可以从 **Releases** 页面下载：
```
https://github.com/YOUR_USERNAME/YuntuClient/releases
```

---

## 📊 查看构建状态

### 实时查看构建日志

1. Actions 页面 → 点击正在运行的构建
2. 点击左侧的 **"build-windows"**
3. 展开每个步骤查看详细日志

### 常见构建阶段

- **Checkout code** - 下载代码
- **Install Qt** - 安装 Qt 6.8.3（第一次较慢，后续会缓存）
- **Setup MSVC** - 配置 Visual Studio 编译器
- **Configure CMake** - 配置项目
- **Build** - 编译（最耗时）
- **Package artifacts** - 打包
- **Upload artifacts** - 上传可下载文件

---

## 🛠️ 手动触发构建

如果想在不提交代码的情况下重新构建：

1. Actions 页面
2. 点击左侧 **"Build Windows Test"**
3. 点击右侧 **"Run workflow"** 下拉菜单
4. 选择分支（main）
5. 点击绿色的 **"Run workflow"** 按钮

---

## ❓ 常见问题

### Q1: 推送时提示 "Permission denied"

**解决**：使用 Personal Access Token 代替密码（见上方说明）

### Q2: 构建失败怎么办？

1. 点击失败的构建查看日志
2. 查找红色错误信息
3. 常见问题：
   - **CMake 错误**：检查 `CMakeLists_Test.txt` 语法
   - **编译错误**：检查 C++ 代码语法
   - **Qt 模块缺失**：修改 `.github/workflows/build-windows.yml`

### Q3: 如何同时构建 macOS 版本？

创建 `.github/workflows/build-macos.yml`：

```yaml
name: Build macOS Test
on: [push, pull_request, workflow_dispatch]
jobs:
  build-macos:
    runs-on: macos-latest
    # ... 类似配置，使用 macOS 环境
```

### Q4: 构建时间太长？

- 第一次：10-15 分钟（需要下载 Qt）
- 后续：5-8 分钟（使用缓存）
- 可以在 workflow 中启用更多缓存优化

### Q5: Actions 有使用限制吗？

- **Public 仓库**：✅ 无限制免费
- **Private 仓库**：每月 2000 分钟免费（足够个人项目使用）

---

## 🎯 完整工作流示例

```bash
# === 开发新功能 ===
cd /Users/pretty/Documents/Workspace/YuntuClient

# 修改代码...
# 例如：编辑 src/services/MayaDetector.cpp

# 查看修改
git status
git diff

# 提交修改
git add .
git commit -m "优化 Maya 检测算法，支持 Maya 2025"

# 推送到 GitHub
git push

# === 访问 GitHub Actions ===
# 打开浏览器：https://github.com/YOUR_USERNAME/YuntuClient/actions
# 等待构建完成（5-10 分钟）

# === 下载测试 ===
# 在 Actions 页面下载 Artifacts
# 在 Windows 电脑上测试
```

---

## 📚 额外资源

- **GitHub Actions 文档**: https://docs.github.com/actions
- **Git 教程**: https://git-scm.com/book/zh/v2
- **GitHub Desktop 指南**: https://docs.github.com/desktop
- **项目文档**: README.md

---

## 🎉 总结

使用 GitHub Actions，你可以：

✅ 在 Mac 上开发
✅ 自动构建 Windows exe
✅ 无需配置复杂环境
✅ 版本控制 + 自动化构建 = 一步到位
✅ 团队协作更方便

**现在就开始吧！整个流程不超过 10 分钟！** 🚀

有任何问题随时问我！
