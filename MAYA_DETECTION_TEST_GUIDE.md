# Maya 环境检测功能测试指南

## 功能概述

本项目新增了完整的 Maya 环境检测功能，可以在 Windows 系统上自动检测和识别所有已安装的 Maya 版本及其相关信息。

## 如何测试

### 1. 启动应用程序

在 Windows 上编译并运行 YuntuClient.exe

### 2. 打开 Maya 检测对话框

在登录界面找到绿色的 **"🔍 检测 Maya 环境"** 按钮，点击即可打开检测对话框。

### 3. 检测功能

对话框会自动开始检测，您也可以点击以下按钮：
- **开始检测** - 手动开始新的检测
- **刷新** - 重新检测
- **导出报告** - 将检测结果保存为文本文件
- **关闭** - 关闭对话框

## 检测内容详解

### 1. Maya 版本检测

**检测方式：**
- Windows 注册表扫描（`HKEY_LOCAL_MACHINE\\SOFTWARE\\Autodesk\\Maya`）
- 常用安装目录扫描
  - `C:/Program Files/Autodesk/Maya2024`
  - `C:/Program Files/Autodesk/Maya2023`
  - `C:/Program Files (x86)/Autodesk/...`

**显示信息：**
- 📦 软件名称（Maya）
- 🔢 版本号（2024, 2023, 2022 等）
- 📝 完整版本号
- ✅ 有效性检查
- 📁 安装路径
- ⚙️ 可执行文件路径及存在性验证

### 2. 渲染器检测

自动检测以下渲染器插件：

**Arnold (mtoa)**
- 文件位置：`Maya安装目录/plug-ins/mtoa.mll`
- Maya 2017+ 内置渲染器
- 显示版本信息（如果可用）

**V-Ray**
- 文件位置：`Maya安装目录/plug-ins/vrayformaya.mll`
- 第三方高质量渲染器
- 显示安装状态和版本

**Redshift**
- 文件位置：`Maya安装目录/plug-ins/redshift4maya.mll`
- GPU 加速渲染器
- 显示安装状态和版本

### 3. 插件检测

**扫描目录：**
- Maya 安装目录插件：`Maya安装目录/plug-ins/*.mll`
- 用户自定义插件：`Documents/maya/{版本}/plug-ins/*.mll`
- Python 插件：`*.py` 文件

**特殊识别的插件：**
- **Miarmy** - 群集动画系统
- **Yeti** - 高级毛发系统
- **XGen** - Maya 内置毛发工具
- **Bifrost** - 流体模拟系统
- **MASH** - 运动图形工具

所有其他检测到的插件也会列出。

### 4. 统计信息

对话框底部会显示：
- 📊 检测到的渲染器类型总数
- 📦 检测到的插件总数
- 各 Maya 版本的详细统计

## 场景文件分析功能（可选测试）

### 测试文件

项目根目录提供了测试场景文件：`test_scene_sample.ma`

### 分析内容

MayaDetector 可以从场景文件中提取：

**1. Maya 版本信息**
```cpp
QString version = detector.extractMayaVersionFromScene("scene.ma");
// 示例结果: "2024"
```

**2. 使用的渲染器**
```cpp
QString renderer = detector.extractRendererFromScene("scene.ma");
// 可能结果: "Arnold", "V-Ray", "Redshift", "RenderMan", "Maya Software"
```

**3. 素材依赖扫描**
```cpp
QStringList assets = detector.scanSceneAssets("scene.ma");
// 返回所有纹理、IES 文件、缓存文件路径
```

**4. 缺失文件检测**
```cpp
QStringList missing = detector.detectMissingAssets("scene.ma");
// 返回场景引用但本地不存在的文件列表
```

### 测试场景文件包含的资源

`test_scene_sample.ma` 包含以下测试数据：
- Maya 2024 版本标识
- Arnold 渲染器节点（mtoa）
- 3 个纹理文件引用：
  - `C:/Projects/Textures/wood_diffuse.jpg`
  - `C:/Projects/Textures/wood_normal.png`
  - `D:/Assets/Textures/metal_roughness.exr`
- 1 个 IES 光源文件：
  - `C:/Projects/Lights/studio_light.ies`
- 2 个缓存文件：
  - `D:/Cache/character_animation.abc` (Alembic)
  - `C:/Cache/environment.abc` (GPU Cache)

## 导出功能

点击 **"导出报告"** 按钮可以将完整的检测结果保存为文本文件：

**默认文件名格式：**
```
Maya_Detection_Report_20250114_140530.txt
```

**报告内容包括：**
- 检测时间戳
- 所有 Maya 版本的详细信息
- 渲染器列表
- 完整的插件列表（不限制显示数量）
- 统计摘要

## 界面特性

### Fluent Design 风格
- 圆角边框和阴影效果
- 柔和的配色方案
- 平滑的动画过渡
- 响应式布局

### 实时反馈
- ✅ 进度条显示检测进度（0-100%）
- 📝 状态标签实时更新当前操作
- 🔄 自动滚动到最新内容
- 🎯 清晰的图标和分隔符

### 用户体验
- 对话框打开后自动开始检测
- 等宽字体显示便于阅读
- 长列表自动分页显示（前20个）
- 详细的错误提示和建议

## 预期测试结果

### 如果系统已安装 Maya

您应该看到：
```
═══════════════════════════════════════════════════════════════
  检测摘要
═══════════════════════════════════════════════════════════════

🎯 检测到 Maya 版本数: 2

📋 版本列表:
   ✅ Maya 2024 - 2 个渲染器, 15 个插件
   ✅ Maya 2023 - 1 个渲染器, 12 个插件

═══════════════════════════════════════════════════════════════
  Maya 版本 #1
═══════════════════════════════════════════════════════════════

📦 软件名称: Maya
🔢 版本号: 2024
📝 完整版本: 2024
✅ 有效性: 是

📁 安装路径:
   C:/Program Files/Autodesk/Maya2024

⚙️  可执行文件:
   C:/Program Files/Autodesk/Maya2024/bin/maya.exe
   存在: 是

🎨 渲染器 (2 个):
   ✓ Arnold Unknown
   ✓ V-Ray Unknown

🔌 插件 (15 个):
   • Miarmy (群集动画)
   • Yeti (毛发系统)
   • Bifrost (流体)
   • MASH (运动图形)
   ...
```

### 如果系统未安装 Maya

您应该看到：
```
═══════════════════════════════════════════════════════════════
  检测摘要
═══════════════════════════════════════════════════════════════

🎯 检测到 Maya 版本数: 0

═══════════════════════════════════════════════════════════════

❌ 未检测到 Maya 安装

可能的原因:
  1. 系统中未安装 Maya
  2. Maya 安装在非标准路径
  3. 注册表信息缺失

═══════════════════════════════════════════════════════════════
```

## 技术实现

### 核心类

**MayaDetector** (`src/services/MayaDetector.h/cpp`)
- 跨平台设计（支持 Windows/macOS/Linux）
- 信号/槽机制提供实时进度反馈
- 多种检测策略确保全面覆盖

**MayaDetectionDialog** (`src/ui/views/MayaDetectionDialog.h/cpp`)
- 基于 Qt Widgets 的对话框
- 异步检测避免界面卡顿
- 丰富的格式化输出

### 数据结构

**MayaSoftwareInfo**
```cpp
struct MayaSoftwareInfo {
    QString name;              // 软件名称
    QString version;           // 版本号
    QString fullVersion;       // 完整版本号
    QString installPath;       // 安装路径
    QString executablePath;    // 可执行文件路径
    QStringList renderers;     // 渲染器列表
    QStringList plugins;       // 插件列表
    bool isValid;              // 是否有效
};
```

**RendererInfo**
```cpp
struct RendererInfo {
    QString name;              // 渲染器名称
    QString version;           // 版本号
    QString pluginPath;        // 插件路径
    bool isLoaded;             // 是否已加载
};
```

## 日志记录

所有操作都会记录到应用程序日志：
- 检测开始/完成事件
- 找到的 Maya 版本数量
- 导出报告的文件路径
- 错误和警告信息

日志位置：`C:/Users/{用户名}/AppData/Roaming/YunTu/logs/`

## 故障排除

### 问题：检测不到已安装的 Maya

**可能原因：**
1. Maya 安装在非标准路径
2. 注册表项缺失或损坏
3. 权限不足无法访问注册表

**解决方案：**
1. 检查 Maya 是否能正常启动
2. 以管理员权限运行 YuntuClient
3. 查看日志文件获取详细错误信息

### 问题：渲染器显示为 "Unknown"

这是正常的，因为当前版本只检测渲染器插件文件是否存在，暂未实现版本号提取功能。

### 问题：插件数量比实际少

检测器只扫描标准插件目录，如果您的插件安装在自定义位置，可能不会被检测到。

## 下一步扩展

未来可以添加的功能：
- [ ] 检测更多渲染器（Octane, Corona, Maxwell）
- [ ] 提取渲染器精确版本号
- [ ] 检测 Python 脚本和工具
- [ ] 检测 Maya 配置和偏好设置
- [ ] 支持其他 3D 软件（3ds Max, Blender, C4D, Houdini）
- [ ] 生成 HTML 格式的详细报告
- [ ] 自动检测场景文件依赖并打包

## 相关文件

- `src/services/MayaDetector.h` - 检测器接口定义
- `src/services/MayaDetector.cpp` - 检测器实现（约 530 行）
- `src/ui/views/MayaDetectionDialog.h` - 对话框接口
- `src/ui/views/MayaDetectionDialog.cpp` - 对话框实现（约 400 行）
- `src/ui/views/LoginWindow.h/cpp` - 登录窗口（添加了检测按钮）
- `test_scene_sample.ma` - Maya 测试场景文件

## 反馈和问题

如果在测试过程中发现任何问题或有改进建议，请通过以下方式反馈：
- GitHub Issues
- 项目内部沟通渠道

---

**版本**: v1.0.0
**更新日期**: 2025-01-14
**测试状态**: ✅ 待 Windows 环境测试
