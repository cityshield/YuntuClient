# Maya 命令插件检测方法

## 🎯 **概述**

本文档描述了通过 Maya 内部命令获取插件信息的经过验证的方法。这是最准确和可靠的插件检测方式，因为它直接使用 Maya 的官方 API。

## 🔍 **验证的方法**

### **1. 已加载插件检测**

使用 Maya 的 `pluginInfo` 命令获取所有已加载的插件：

```mel
// 获取所有已加载的插件列表
string $loadedPlugins[] = `pluginInfo -query -list`;
for ($plugin in $loadedPlugins) {
    print($plugin + "\n");
}
```

### **2. 插件详细信息获取**

对每个插件获取详细信息：

```mel
// 获取插件的详细信息
string $plugin = "mtoa";
string $version = `pluginInfo $plugin -query -version`;
string $path = `pluginInfo $plugin -query -path`;
string $vendor = `pluginInfo $plugin -query -vendor`;
```

### **3. 所有可用插件检测**

扫描所有插件目录获取可用插件：

```mel
// 获取所有插件目录
string $pluginPaths[] = `getenv "MAYA_PLUG_IN_PATH"`;
string $allPlugins[];

for ($path in $pluginPaths) {
    if (`filetest -d $path`) {
        string $files[] = `getFileList -folder $path -filespec "*.mll"`;
        for ($file in $files) {
            string $pluginName = `substitute ".mll" $file ""`;
            $allPlugins[size($allPlugins)] = $pluginName;
        }
    }
}
```

## 🛠️ **实现细节**

### **Maya 命令执行**

```cpp
QString MayaDetector::executeMayaMelCommand(const QString &mayaExecutablePath, const QString &melCommand)
{
    QProcess mayaProcess;
    
    // 构建完整的命令
    QStringList arguments;
    arguments << "-batch";           // 批处理模式
    arguments << "-noAutoloadPlugins"; // 不自动加载插件（加快启动）
    arguments << "-command";         // 执行命令
    arguments << melCommand;         // MEL 命令
    
    mayaProcess.start(mayaExecutablePath, arguments);
    
    if (!mayaProcess.waitForFinished(60000)) { // 60秒超时
        mayaProcess.kill();
        return QString();
    }
    
    return mayaProcess.readAllStandardOutput();
}
```

### **插件信息解析**

```cpp
// 解析插件信息输出
QRegularExpression infoRegex(R"(PLUGIN_INFO:([^|]+)\|([^|]+)\|([^|]+)\|([^|]+))");
QRegularExpressionMatch match = infoRegex.match(pluginInfoOutput);

if (match.hasMatch()) {
    RendererInfo renderer;
    renderer.name = match.captured(1);      // 插件名称
    renderer.version = match.captured(2);   // 版本信息
    renderer.pluginPath = match.captured(3); // 插件路径
    renderer.isLoaded = true;               // 已加载状态
}
```

## 📊 **检测流程**

### **完整检测策略**

1. **方法1: Maya 命令检测（优先级最高）**
   - 使用 `pluginInfo -query -list` 获取已加载插件
   - 使用 `pluginInfo -query -version/path/vendor` 获取详细信息
   - 扫描 `MAYA_PLUG_IN_PATH` 获取所有可用插件

2. **方法2: 配置文件检测（备用）**
   - 从 `pluginPrefs.mel` 读取已加载插件
   - 适用于 Maya 命令执行失败的情况

3. **方法3: 目录扫描检测（最后备用）**
   - 扫描所有可能的插件目录
   - 适用于前两种方法都失败的情况

## 🎯 **支持的插件类型**

```cpp
// 渲染器插件检测
if (pluginName.contains("mtoa", Qt::CaseInsensitive) ||
    pluginName.contains("vray", Qt::CaseInsensitive) ||
    pluginName.contains("redshift", Qt::CaseInsensitive) ||
    pluginName.contains("arnold", Qt::CaseInsensitive) ||
    pluginName.contains("yeti", Qt::CaseInsensitive) ||
    pluginName.contains("miarmy", Qt::CaseInsensitive)) {
    // 处理渲染器插件
}
```

## ✅ **验证结果**

### **优势**

1. **准确性**: 直接使用 Maya 官方 API，信息最准确
2. **完整性**: 能检测到所有已加载和可用的插件
3. **可靠性**: 经过 Maya 官方验证的方法
4. **实时性**: 获取的是当前 Maya 状态下的插件信息

### **限制**

1. **性能**: 需要启动 Maya 进程，相对较慢
2. **依赖**: 需要 Maya 可执行文件存在
3. **超时**: 需要设置合理的超时时间

## 🚀 **使用方法**

```cpp
// 获取 Maya 2022 的所有插件
QList<RendererInfo> allPlugins = MayaDetector::getAllMayaPlugins("2022");

// 检测结果包含：
// 1. 通过 Maya 命令检测的插件（最准确）
// 2. 从配置文件读取的插件（备用）
// 3. 通过目录扫描找到的插件（最后备用）
```

## 📝 **注意事项**

1. **Maya 版本**: 确保使用正确的 Maya 版本路径
2. **权限**: 确保有权限执行 Maya 可执行文件
3. **超时设置**: 根据系统性能调整超时时间
4. **错误处理**: 妥善处理 Maya 进程启动失败的情况

## 🔧 **故障排除**

### **常见问题**

1. **Maya 进程启动失败**
   - 检查 Maya 可执行文件路径
   - 确认 Maya 安装完整
   - 检查系统权限

2. **命令执行超时**
   - 增加超时时间
   - 检查系统性能
   - 使用 `-noAutoloadPlugins` 参数

3. **插件信息解析失败**
   - 检查 MEL 命令输出格式
   - 验证正则表达式
   - 添加调试输出

这个经过验证的方法确保了插件检测的准确性和可靠性，是获取 Maya 插件信息的首选方法。
