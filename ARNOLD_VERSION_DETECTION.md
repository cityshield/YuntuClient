# Arnold 版本检测功能说明

## 概述

本文档详细说明了 YuntuClient 中 Arnold 渲染器版本检测的实现方式和支持的检测方法。

## 版本检测策略

Arnold 版本检测采用多层次策略，按优先级依次尝试以下方法：

### 方法 1: 从插件路径提取版本 🔍

**支持的路径格式：**
```
✓ C:/Program Files/Autodesk/Arnold/maya2024/plug-ins/mtoa.mll
✓ C:/solidangle/mtoadeploy/2024/plug-ins/mtoa.mll
✓ C:/Arnold-5.3.0.1/plug-ins/mtoa.mll
✓ D:/mtoa-5.2.1.0/mtoa.mll
```

**提取的正则表达式模式：**
- `mtoa[_-]?(\\d+\\.\\d+\\.\\d+\\.\\d+)` - 匹配 mtoa-5.3.0.1
- `mtoa[_-]?(\\d+\\.\\d+\\.\\d+)` - 匹配 mtoa-5.3.0
- `arnold[/_-](\\d+\\.\\d+\\.\\d+\\.\\d+)` - 匹配 Arnold-5.3.0.1
- `arnold[/_-](\\d+\\.\\d+\\.\\d+)` - 匹配 Arnold-5.3.0

### 方法 2: 从版本文件读取 📄

**搜索策略：**
1. 使用专门的 `searchArnoldVersionFiles()` 函数
2. 按优先级搜索多个目录层级
3. 搜索已知的 Arnold 版本信息文件

**搜索的目录：**
1. 插件所在目录（plug-ins/）
2. 插件父目录（Arnold 安装根目录）
3. 插件祖父目录

**搜索的文件名（简化策略）：**
```
include/mtoa/utils/Version.h       # MtoA版本头文件（优先级最高）
include/arnold/ai_version.h        # Arnold核心版本头文件（备用）
```

**特殊文件处理：**

**mtoa.mod 文件：**
`mtoa.mod` 文件通常不直接包含版本号，而是包含 Arnold 安装路径。代码会：
1. 解析模块文件中的 Arnold 路径（如：`C:\Program Files\Autodesk\Arnold\maya2022`）
2. 直接检查该路径下的两个关键头文件：
   - `include/mtoa/utils/Version.h`（优先）
   - `include/arnold/ai_version.h`（备用）

**MtoA 版本头文件 (include/mtoa/utils/Version.h)：**
解析 C++ 头文件中的版本定义：
```cpp
#define MTOA_ARCH_VERSION_NUM 5
#define MTOA_MAJOR_VERSION_NUM 0
#define MTOA_MINOR_VERSION_NUM 0
#define MTOA_FIX_VERSION "2"
```
组合为版本号：`5.0.0.2`

**Arnold 核心版本头文件 (include/arnold/ai_version.h)：**
解析 Arnold 核心版本定义：
```cpp
#define AI_VERSION_ARCH_NUM    7
#define AI_VERSION_MAJOR_NUM   0
#define AI_VERSION_MINOR_NUM   0
#define AI_VERSION_FIX         "1"
```
组合为版本号：`7.0.0.1`

**支持的版本格式：**
```
# Arnold for Maya 详细格式
MtoA 5.0.0.2 - 1c3d7bb2 (fix-5.0.0) - Nov 24 2021 01:02:04
Arnold Core 7.0.0.1
Arnold for Maya 5.0.0.2

# 通用格式
arnold: 5.3.0.1
arnold 5.3.0.1
mtoa: 5.2.1
mtoa 5.2.1
version: 5.3.0.1
version 5.3.0.1
5.3.0.1          (纯版本号)
```

### 方法 3: 从模块文件 (.mod) 提取 ⭐ **最重要**

Arnold 的版本信息通常存储在 Maya 的模块系统中。

**搜索的模块目录：**

Windows:
```
用户级：
- ~/Documents/maya/{版本}/modules/
- ~/Documents/maya/modules/

系统级（Arnold 常在这里）：
- C:/ProgramData/Autodesk/ApplicationPlugins/
- C:/Program Files/Common Files/Autodesk Shared/Modules/maya/{版本}/
- C:/Program Files/Common Files/Autodesk Shared/Modules/maya/

插件附近：
- 插件目录/../
- 插件目录/../modules/
- 插件目录/../Contents/
- 插件目录/../Contents/modules/
```

**支持的模块文件格式：**

格式1 - 标准格式（最常见）：
```
+ MAYAVERSION:2024 mtoa 5.3.0.1 C:/Program Files/Autodesk/Arnold/maya2024
```

格式2 - 简化格式：
```
+ mtoa 5.2.1 ../
```

格式3 - 带平台信息：
```
+ PLATFORM:win64 MAYAVERSION:2024 mtoa 5.3.0.1 path
```

格式4 - [r] 格式：
```
[r] mtoa 5.3.0.1
```

格式5 - VERSION= 格式：
```
VERSION=5.3.0.1
VERSION: 5.3.0.1
```

格式6 - 描述性格式：
```
mtoa version: 5.3.0.1
mtoa version 5.3.0.1
```

**提取的正则表达式模式：**
```regex
\\+[^\\n]*?mtoa\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)    # + ... mtoa 5.3.0.1 ...
\\+[^\\n]*?mtoa\\s+(\\d+\\.\\d+\\.\\d+)          # + ... mtoa 5.3.0 ...
\\[r\\]\\s*mtoa\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)   # [r] mtoa 5.3.0.1
\\[r\\]\\s*mtoa\\s+(\\d+\\.\\d+\\.\\d+)          # [r] mtoa 5.3.0
VERSION\\s*[=:]\\s*(\\d+\\.\\d+\\.\\d+\\.\\d+)   # VERSION=5.3.0.1
VERSION\\s*[=:]\\s*(\\d+\\.\\d+\\.\\d+)          # VERSION=5.3.0
mtoa.*?version[:\\s]+(\\d+\\.\\d+\\.\\d+\\.\\d+) # mtoa version: 5.3.0.1
mtoa.*?version[:\\s]+(\\d+\\.\\d+\\.\\d+)        # mtoa version: 5.3.0
```

### 方法 4: 从文件属性读取 (Windows) 🪟

在 Windows 系统上，读取插件文件的元数据：
- 文件大小
- 最后修改时间
- 文件版本信息（如果可用）

### 方法 5: 根据 Maya 版本推断 🔮 **备选方案**

当以上所有方法都失败时，根据 Maya 版本推断 Arnold 版本。

**Maya 与 Arnold 版本对应关系：**

| Maya 版本 | Arnold 版本 | 说明 |
|----------|------------|------|
| 2025 | 5.4.x | Arnold 5.4 for Maya 2025 |
| 2024 | 5.3.x | Arnold 5.3 for Maya 2024 |
| 2023 | 5.2.x | Arnold 5.2 for Maya 2023 |
| 2022 | 4.2.x | Arnold 4.2 for Maya 2022 |
| 2020 | 4.0.x | Arnold 4.0 for Maya 2020 |
| 2019 | 3.3.x | Arnold 3.3 for Maya 2019 |
| 2018 | 3.1.x | Arnold 3.1 for Maya 2018 |
| 2017 | 2.0.x | Arnold 2.0 for Maya 2017 |
| 2016 | 1.4.x | Arnold 1.4 for Maya 2016 |

⚠️ **注意：** 这只是基于官方推荐版本的估算值，用户可能安装了不同版本的 Arnold。

## 版本显示格式

版本信息将以以下格式显示：

- **精确版本：** `5.3.0.1`、`5.2.1.0`、`4.2.5.3`
- **三位版本：** `5.3.0`、`5.2.1`
- **推断版本：** `5.3.x`、`5.2.x`（带 .x 后缀表示推断）
- **未知版本：** `Unknown`（仅在所有方法都失败时显示）

## 调试信息

版本提取过程会输出详细的调试日志：

```
========== 开始提取 Arnold 版本信息 ==========
插件路径: C:/Program Files/Autodesk/Maya2024/plug-ins/mtoa.mll
Maya 版本: 2024
插件文件存在: true
  检查模块文件: C:/ProgramData/Autodesk/ApplicationPlugins/MtoA/mtoa.mod
  ✓ 从模块文件提取到版本: 5.3.0.1 （模式: \+[^\n]*?mtoa\s+(\...）
========== Arnold 版本提取完成 ==========
```

## 常见问题

### Q: 为什么版本显示为 "Unknown"?

**可能的原因：**
1. Arnold 插件未正确安装
2. 模块文件 (.mod) 不存在或格式不标准
3. 插件目录中缺少版本文件
4. Maya 版本不在已知映射表中

**解决方法：**
1. 检查 Arnold 是否正确安装
2. 查看调试日志了解失败原因
3. 确认 Maya Plug-in Manager 中 mtoa 已加载
4. 检查模块目录：`C:/ProgramData/Autodesk/ApplicationPlugins/`

### Q: 版本显示为 "5.3.x" 是什么意思?

这表示版本是根据 Maya 版本推断出来的，不是从实际文件中读取的精确版本。实际的 Arnold 版本可能是 5.3.0、5.3.0.1、5.3.1 等。

### Q: 如何获取准确的版本号?

最准确的方法是确保 Arnold 的模块文件 (.mod) 存在且格式正确。检查以下位置：
```
C:/ProgramData/Autodesk/ApplicationPlugins/MtoA/
```

### Q: 支持哪些操作系统?

- ✅ Windows (完整支持)
- ✅ macOS (支持，路径有所不同)
- ✅ Linux (支持，路径有所不同)

## 技术实现

版本提取功能在 `MayaDetector::extractArnoldVersion()` 函数中实现，位于：
```
src/services/MayaDetector.cpp (行 1656-1868)
src/services/MayaDetector.h (行 231-237)
```

该函数被以下检测点调用：
- Maya 内置 Arnold 检测
- 独立安装 Arnold 检测
- 环境变量 Arnold 检测
- 模块文件 Arnold 检测
- 暴力搜索 Arnold 检测
- Plug-in Manager 读取的 Arnold

## 更新历史

- **2024-10-17**: 增强版本检测逻辑
  - 添加多种路径模式匹配
  - 支持多种模块文件格式
  - 增加多层目录搜索
  - 更新 Maya-Arnold 版本映射表
  - 添加详细的调试日志
  - **新增**: 支持 Arnold for Maya 详细版本格式
  - **新增**: 专门的版本文本解析器 `parseArnoldVersionText()`
  - **新增**: 支持 "MtoA 5.0.0.2 - 1c3d7bb2 (fix-5.0.0)" 格式
  - **新增**: 支持 "Arnold Core 7.0.0.1" 格式
  - **新增**: 专门的版本文件搜索函数 `searchArnoldVersionFiles()`
  - **新增**: 基于实际文件位置的搜索策略
  - **新增**: 专门的 `mtoa.mod` 文件解析函数 `extractVersionFromMtoaMod()`
  - **新增**: 支持从 `mtoa.mod` 路径中提取 Arnold 安装位置并搜索版本文件
  - **新增**: MtoA 版本头文件解析函数 `extractVersionFromMtoaHeader()`
  - **新增**: Arnold 核心版本头文件解析函数 `extractVersionFromArnoldHeader()`
  - **新增**: 支持从 C++ 头文件中提取版本定义（如 `MTOA_ARCH_VERSION_NUM`）
  - **新增**: 简化搜索策略，只关注两个关键头文件
  - **优化**: 移除多余的文件搜索，提高检测效率

- **初始版本**: 基础版本检测
  - 简单的路径匹配
  - 基础模块文件解析

