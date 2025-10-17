#include "MayaDetector.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QStringConverter>
#include <QMap>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <QSettings>
#endif

MayaDetector::MayaDetector(QObject *parent)
    : QObject(parent)
{
}

MayaDetector::~MayaDetector()
{
}

QVector<MayaSoftwareInfo> MayaDetector::detectAllMayaVersions()
{
    QVector<MayaSoftwareInfo> results;
    QStringList mayaPaths;

    emit detectProgress(10, "正在扫描 Maya 安装路径...");

#ifdef Q_OS_WIN
    // Windows: 从注册表读取
    mayaPaths.append(readMayaPathsFromRegistry());
#endif

    // 扫描常用安装目录
    mayaPaths.append(scanCommonInstallPaths());

    // 去重
    mayaPaths.removeDuplicates();

    emit detectProgress(30, QString("找到 %1 个可能的 Maya 安装路径").arg(mayaPaths.size()));

    // 验证每个路径
    int currentProgress = 30;
    int step = 60 / qMax(1, mayaPaths.size());

    for (const QString &path : mayaPaths) {
        if (isValidMayaInstall(path)) {
            MayaSoftwareInfo info = detectMayaAtPath(path);
            if (info.isValid) {
                results.append(info);
                qDebug() << "检测到 Maya:" << info.version << "安装路径:" << info.installPath;
            }
        }

        currentProgress += step;
        emit detectProgress(currentProgress, QString("检测: %1").arg(path));
    }

    emit detectProgress(100, "Maya 检测完成");
    emit detectFinished();

    return results;
}

MayaSoftwareInfo MayaDetector::detectMayaAtPath(const QString &installPath)
{
    MayaSoftwareInfo info;
    info.installPath = installPath;
    info.name = "Maya";

    // 提取版本号
    info.version = extractVersionFromPath(installPath);

    // 获取可执行文件路径
    info.executablePath = getMayaExecutablePath(installPath);

    // 检测渲染器
    QVector<RendererInfo> renderers = detectRenderers(info);
    for (const RendererInfo &renderer : renderers) {
        info.renderers.append(QString("%1 %2").arg(renderer.name).arg(renderer.version));
    }

    // 检测插件
    info.plugins = detectPlugins(info);

    // 验证有效性
    info.isValid = !info.version.isEmpty() && QFile::exists(info.executablePath);

    // 尝试读取完整版本号
    QDir binDir(installPath + "/bin");
    if (binDir.exists()) {
        // 从 maya.exe 的版本信息读取
        // TODO: 使用 GetFileVersionInfo API (Windows)
        info.fullVersion = info.version; // 暂时使用主版本号
    }

    return info;
}

QVector<RendererInfo> MayaDetector::detectRenderers(const MayaSoftwareInfo &mayaInfo)
{
    QVector<RendererInfo> renderers;

    // 检测 Arnold (Maya 2017+ 内置)
    RendererInfo arnold = detectArnold(mayaInfo.installPath);
    if (!arnold.name.isEmpty()) {
        renderers.append(arnold);
    }

    // 检测 V-Ray
    RendererInfo vray = detectVRay(mayaInfo.installPath);
    if (!vray.name.isEmpty()) {
        renderers.append(vray);
    }

    // 检测 Redshift
    RendererInfo redshift = detectRedshift(mayaInfo.installPath);
    if (!redshift.name.isEmpty()) {
        renderers.append(redshift);
    }

    return renderers;
}

QStringList MayaDetector::detectPlugins(const MayaSoftwareInfo &mayaInfo)
{
    QStringList plugins;

    // 1. Maya 内置插件目录
    QStringList pluginDirs;
    pluginDirs << mayaInfo.installPath + "/plug-ins";
    pluginDirs << mayaInfo.installPath + "/bin/plug-ins";

#ifdef Q_OS_WIN
    // 2. Windows 用户插件目录
    QString userPlugins = QDir::homePath() + "/Documents/maya/" + mayaInfo.version + "/plug-ins";
    pluginDirs << userPlugins;

    // 3. 从 Maya.env 读取插件路径
    QStringList envPaths = readMayaEnvPaths(mayaInfo.version);
    pluginDirs.append(envPaths);

    // 4. 从模块路径文件读取
    QStringList modulePaths = readModulePaths(mayaInfo.version);
    pluginDirs.append(modulePaths);

    // 5. 扫描第三方插件注册表
    QStringList registryPaths = scanThirdPartyPluginRegistry(mayaInfo.version);
    pluginDirs.append(registryPaths);

#elif defined(Q_OS_MAC)
    // macOS 用户插件目录
    QString userPlugins = QDir::homePath() + "/Library/Preferences/Autodesk/maya/" + mayaInfo.version + "/plug-ins";
    pluginDirs << userPlugins;
#endif

    // 去重
    pluginDirs.removeDuplicates();
    qDebug() << "检测插件目录:" << pluginDirs;

    for (const QString &dir : pluginDirs) {
        QDir pluginDir(dir);
        qDebug() << "扫描目录:" << dir << "存在:" << pluginDir.exists();
        if (!pluginDir.exists()) continue;

        // 扫描插件文件
        QStringList filters;
#ifdef Q_OS_WIN
        filters << "*.mll" << "*.py";
#elif defined(Q_OS_MAC)
        filters << "*.bundle" << "*.py";
#elif defined(Q_OS_LINUX)
        filters << "*.so" << "*.py";
#endif

        QFileInfoList files = pluginDir.entryInfoList(filters, QDir::Files);
        qDebug() << "  找到" << files.size() << "个文件";
        for (const QFileInfo &file : files) {
            QString pluginName = file.baseName();
            qDebug() << "    插件:" << pluginName;

            // 识别常见插件
            if (pluginName.contains("miarmy", Qt::CaseInsensitive)) {
                plugins << "Miarmy (群集动画)";
            } else if (pluginName.contains("yeti", Qt::CaseInsensitive)) {
                plugins << "Yeti (毛发系统)";
            } else if (pluginName.contains("xgen", Qt::CaseInsensitive)) {
                plugins << "XGen (毛发)";
            } else if (pluginName.contains("bifrost", Qt::CaseInsensitive)) {
                plugins << "Bifrost (流体)";
            } else if (pluginName.contains("mash", Qt::CaseInsensitive)) {
                plugins << "MASH (运动图形)";
            } else {
                // 其他插件
                plugins << pluginName;
            }
        }
    }

    plugins.removeDuplicates();
    return plugins;
}

QString MayaDetector::extractMayaVersionFromScene(const QString &sceneFilePath)
{
    QFileInfo fileInfo(sceneFilePath);
    QString suffix = fileInfo.suffix().toLower();

    QString content;
    if (suffix == "ma") {
        // ASCII 场景文件
        content = readMayaAsciiScene(sceneFilePath);
    } else if (suffix == "mb") {
        // 二进制场景文件
        content = parseMayaBinaryScene(sceneFilePath);
    } else {
        return QString();
    }

    // 从文件头提取版本信息
    // 示例: //Maya 2024 Scene File
    QRegularExpression re("Maya\\s+(\\d{4})");
    QRegularExpressionMatch match = re.match(content);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    return QString();
}

QString MayaDetector::extractRendererFromScene(const QString &sceneFilePath)
{
    QFileInfo fileInfo(sceneFilePath);
    QString suffix = fileInfo.suffix().toLower();

    QString content;
    if (suffix == "ma") {
        content = readMayaAsciiScene(sceneFilePath);
    } else if (suffix == "mb") {
        content = parseMayaBinaryScene(sceneFilePath);
    } else {
        return QString();
    }

    // 检测渲染器
    if (content.contains("mtoa", Qt::CaseInsensitive) ||
        content.contains("aiStandard", Qt::CaseInsensitive)) {
        return "Arnold";
    } else if (content.contains("vray", Qt::CaseInsensitive)) {
        return "V-Ray";
    } else if (content.contains("redshift", Qt::CaseInsensitive)) {
        return "Redshift";
    } else if (content.contains("renderman", Qt::CaseInsensitive)) {
        return "RenderMan";
    }

    return "Maya Software"; // 默认渲染器
}

QStringList MayaDetector::scanSceneAssets(const QString &sceneFilePath)
{
    QStringList assets;

    QString content;
    QFileInfo fileInfo(sceneFilePath);
    if (fileInfo.suffix().toLower() == "ma") {
        content = readMayaAsciiScene(sceneFilePath);
    } else {
        content = parseMayaBinaryScene(sceneFilePath);
    }

    // 提取纹理路径
    // 示例: setAttr ".fileTextureName" -type "string" "D:/textures/wood.jpg";
    QRegularExpression texRe("fileTextureName.*?\"([^\"]+)\"");
    QRegularExpressionMatchIterator it = texRe.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString path = match.captured(1);
        if (!path.isEmpty()) {
            assets.append(path);
        }
    }

    // 提取 IES 文件
    QRegularExpression iesRe("iesProfile.*?\"([^\"]+)\"");
    it = iesRe.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString path = match.captured(1);
        if (!path.isEmpty()) {
            assets.append(path);
        }
    }

    // 提取缓存文件 (Alembic, GPU Cache等)
    QRegularExpression cacheRe("cacheFile.*?\"([^\"]+)\"");
    it = cacheRe.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString path = match.captured(1);
        if (!path.isEmpty()) {
            assets.append(path);
        }
    }

    assets.removeDuplicates();
    return assets;
}

QStringList MayaDetector::detectMissingAssets(const QString &sceneFilePath)
{
    QStringList missingAssets;
    QStringList allAssets = scanSceneAssets(sceneFilePath);

    QDir sceneDir = QFileInfo(sceneFilePath).dir();

    for (const QString &assetPath : allAssets) {
        // 绝对路径
        if (QFile::exists(assetPath)) {
            continue;
        }

        // 相对路径（相对于场景文件）
        QString relativePath = sceneDir.absoluteFilePath(assetPath);
        if (QFile::exists(relativePath)) {
            continue;
        }

        // 文件不存在
        missingAssets.append(assetPath);
    }

    return missingAssets;
}

// ============ 私有方法实现 ============

QStringList MayaDetector::readMayaPathsFromRegistry()
{
    QStringList paths;

#ifdef Q_OS_WIN
    // 读取 Windows 注册表
    QStringList registryKeys;
    registryKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\Autodesk\\Maya";
    registryKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Autodesk\\Maya";

    for (const QString &key : registryKeys) {
        QSettings registry(key, QSettings::NativeFormat);
        QStringList childGroups = registry.childGroups();

        for (const QString &version : childGroups) {
            registry.beginGroup(version);

            QString installPath = registry.value("MAYA_INSTALL_LOCATION").toString();
            if (!installPath.isEmpty()) {
                paths.append(installPath);
            }

            registry.endGroup();
        }
    }
#endif

    return paths;
}

QStringList MayaDetector::scanCommonInstallPaths()
{
    QStringList paths;

#ifdef Q_OS_WIN
    // Windows 常用路径
    QStringList basePaths;
    basePaths << "C:/Program Files/Autodesk";
    basePaths << "C:/Program Files (x86)/Autodesk";

    for (const QString &basePath : basePaths) {
        QDir dir(basePath);
        if (!dir.exists()) continue;

        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir : subdirs) {
            if (subdir.startsWith("Maya", Qt::CaseInsensitive)) {
                paths.append(dir.absoluteFilePath(subdir));
            }
        }
    }

#elif defined(Q_OS_MAC)
    // macOS 常用路径
    paths << "/Applications/Autodesk/maya2024";
    paths << "/Applications/Autodesk/maya2023";
    paths << "/Applications/Autodesk/maya2022";

#elif defined(Q_OS_LINUX)
    // Linux 常用路径
    paths << "/usr/autodesk/maya2024";
    paths << "/usr/autodesk/maya2023";
    paths << "/opt/autodesk/maya2024";
    paths << "/opt/autodesk/maya2023";
#endif

    return paths;
}

QString MayaDetector::extractVersionFromPath(const QString &path)
{
    // 从路径中提取版本号
    // 例如: "C:/Program Files/Autodesk/Maya2024" -> "2024"
    // 也支持: "C:/Program Files/Autodesk/Maya 2024" -> "2024"
    QRegularExpression re("Maya\\s?(\\d{4})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(path);
    if (match.hasMatch()) {
        QString version = match.captured(1);
        qDebug() << "从路径提取版本号:" << path << "->" << version;
        return version;
    }

    qDebug() << "无法从路径提取版本号:" << path;
    return QString();
}

QString MayaDetector::getMayaExecutablePath(const QString &installPath)
{
#ifdef Q_OS_WIN
    return installPath + "/bin/maya.exe";
#elif defined(Q_OS_MAC)
    return installPath + "/Maya.app/Contents/bin/maya";
#elif defined(Q_OS_LINUX)
    return installPath + "/bin/maya";
#endif
}

bool MayaDetector::isValidMayaInstall(const QString &path)
{
    QString exePath = getMayaExecutablePath(path);
    return QFile::exists(exePath);
}

QString MayaDetector::readMayaAsciiScene(const QString &sceneFilePath)
{
    QFile file(sceneFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    // 只读取前10000行（场景文件可能很大）
    QString content;
    int lineCount = 0;
    while (!in.atEnd() && lineCount < 10000) {
        content += in.readLine() + "\n";
        lineCount++;
    }

    file.close();
    return content;
}

QString MayaDetector::parseMayaBinaryScene(const QString &sceneFilePath)
{
    // Maya 二进制文件(.mb) 解析比较复杂
    // 这里简化处理：只读取文件头部信息
    QFile file(sceneFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QByteArray header = file.read(1024); // 读取前1KB
    file.close();

    // 将二进制转为字符串（部分信息是可读的）
    return QString::fromLatin1(header);
}

RendererInfo MayaDetector::detectArnold(const QString &mayaPath)
{
    RendererInfo info;

    // Arnold (mtoa) 可能在多个位置
    QStringList possiblePaths;

#ifdef Q_OS_WIN
    possiblePaths << mayaPath + "/bin/plug-ins/mtoa.mll";
    possiblePaths << mayaPath + "/plug-ins/mtoa.mll";
#elif defined(Q_OS_MAC)
    possiblePaths << mayaPath + "/Maya.app/Contents/plug-ins/mtoa.bundle";
    possiblePaths << mayaPath + "/plug-ins/mtoa.bundle";
#elif defined(Q_OS_LINUX)
    possiblePaths << mayaPath + "/plug-ins/mtoa.so";
#endif

    for (const QString &arnoldPlugin : possiblePaths) {
        qDebug() << "检测 Arnold:" << arnoldPlugin << "存在:" << QFile::exists(arnoldPlugin);
        if (QFile::exists(arnoldPlugin)) {
            info.name = "Arnold";
            info.pluginPath = arnoldPlugin;
            info.isLoaded = true;
            info.version = "Unknown";
            break;
        }
    }

    return info;
}

RendererInfo MayaDetector::detectVRay(const QString &mayaPath)
{
    RendererInfo info;

    // V-Ray 可能在多个位置
    QStringList possiblePaths;

#ifdef Q_OS_WIN
    possiblePaths << mayaPath + "/bin/plug-ins/vrayformaya.mll";
    possiblePaths << mayaPath + "/plug-ins/vrayformaya.mll";
#elif defined(Q_OS_MAC)
    possiblePaths << mayaPath + "/Maya.app/Contents/plug-ins/vrayformaya.bundle";
    possiblePaths << mayaPath + "/plug-ins/vrayformaya.bundle";
#elif defined(Q_OS_LINUX)
    possiblePaths << mayaPath + "/plug-ins/vrayformaya.so";
#endif

    for (const QString &vrayPlugin : possiblePaths) {
        qDebug() << "检测 V-Ray:" << vrayPlugin << "存在:" << QFile::exists(vrayPlugin);
        if (QFile::exists(vrayPlugin)) {
            info.name = "V-Ray";
            info.pluginPath = vrayPlugin;
            info.isLoaded = true;
            info.version = "Unknown";
            break;
        }
    }

    return info;
}

RendererInfo MayaDetector::detectRedshift(const QString &mayaPath)
{
    RendererInfo info;

    // Redshift 可能在多个位置
    QStringList possiblePaths;

#ifdef Q_OS_WIN
    possiblePaths << mayaPath + "/bin/plug-ins/redshift4maya.mll";
    possiblePaths << mayaPath + "/plug-ins/redshift4maya.mll";
#elif defined(Q_OS_MAC)
    possiblePaths << mayaPath + "/Maya.app/Contents/plug-ins/redshift4maya.bundle";
    possiblePaths << mayaPath + "/plug-ins/redshift4maya.bundle";
#elif defined(Q_OS_LINUX)
    possiblePaths << mayaPath + "/plug-ins/redshift4maya.so";
#endif

    for (const QString &redshiftPlugin : possiblePaths) {
        qDebug() << "检测 Redshift:" << redshiftPlugin << "存在:" << QFile::exists(redshiftPlugin);
        if (QFile::exists(redshiftPlugin)) {
            info.name = "Redshift";
            info.pluginPath = redshiftPlugin;
            info.isLoaded = true;
            info.version = "Unknown";
            break;
        }
    }

    return info;
}

// ============ 插件路径检测新方法 ============

QStringList MayaDetector::readMayaEnvPaths(const QString &mayaVersion)
{
    QStringList paths;

#ifdef Q_OS_WIN
    // Maya.env 文件位置
    QString mayaEnvPath = QDir::homePath() + "/Documents/maya/" + mayaVersion + "/Maya.env";

    qDebug() << "读取 Maya.env:" << mayaEnvPath;

    QFile file(mayaEnvPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            // 跳过注释和空行
            if (line.isEmpty() || line.startsWith("#") || line.startsWith("//")) {
                continue;
            }

            // 解析 MAYA_PLUG_IN_PATH 或 PATH
            if (line.contains("MAYA_PLUG_IN_PATH") || line.contains("PATH")) {
                QStringList parts = line.split("=");
                if (parts.size() >= 2) {
                    QString pathStr = parts[1].trimmed();
                    // Windows 路径可能用 ; 分隔
                    QStringList pathList = pathStr.split(";", Qt::SkipEmptyParts);
                    for (const QString &p : pathList) {
                        QString cleanPath = p.trimmed();
                        if (!cleanPath.isEmpty() && QDir(cleanPath).exists()) {
                            qDebug() << "  从 Maya.env 找到路径:" << cleanPath;
                            paths.append(cleanPath);
                        }
                    }
                }
            }
        }
        file.close();
    }
#endif

    return paths;
}

QMap<QString, QString> MayaDetector::readPluginPrefs(const QString &mayaVersion)
{
    QMap<QString, QString> pluginMap;

#ifdef Q_OS_WIN
    // pluginPrefs.mel 文件位置
    QString prefsPath = QDir::homePath() + "/Documents/maya/" + mayaVersion + "/prefs/pluginPrefs.mel";

    qDebug() << "读取 pluginPrefs.mel:" << prefsPath;

    QFile file(prefsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // 解析 pluginInfo 命令
        // 示例: pluginInfo -edit -autoload true -pluginPath "C:/solidangle/mtoadeploy/2022/plug-ins" "mtoa";
        QRegularExpression re("pluginInfo.*?-pluginPath\\s+\"([^\"]+)\".*?\"([^\"]+)\"");
        QRegularExpressionMatchIterator it = re.globalMatch(content);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString pluginPath = match.captured(1);
            QString pluginName = match.captured(2);

            qDebug() << "  从 pluginPrefs 找到:" << pluginName << "->" << pluginPath;
            pluginMap[pluginName] = pluginPath;
        }
    }
#endif

    return pluginMap;
}

QStringList MayaDetector::readModulePaths(const QString &mayaVersion)
{
    QStringList paths;

#ifdef Q_OS_WIN
    // Maya 模块路径
    QStringList moduleDirs;
    moduleDirs << QDir::homePath() + "/Documents/maya/" + mayaVersion + "/modules";
    moduleDirs << QDir::homePath() + "/Documents/maya/modules";
    moduleDirs << "C:/Program Files/Common Files/Autodesk Shared/Modules/maya/" + mayaVersion;

    for (const QString &moduleDir : moduleDirs) {
        QDir dir(moduleDir);
        if (!dir.exists()) continue;

        qDebug() << "扫描模块目录:" << moduleDir;

        // 读取 .mod 文件
        QFileInfoList modFiles = dir.entryInfoList(QStringList() << "*.mod", QDir::Files);
        for (const QFileInfo &modFile : modFiles) {
            QFile file(modFile.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();

                    // 跳过注释和空行
                    if (line.isEmpty() || line.startsWith("#") || line.startsWith("+")) {
                        continue;
                    }

                    // 解析模块定义
                    // 示例: + MAYAVERSION:2022 mtoa 5.1.0 C:/solidangle/mtoadeploy/2022
                    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (parts.size() >= 4) {
                        QString modulePath = parts.last();
                        if (QDir(modulePath).exists()) {
                            // 添加 plug-ins 子目录
                            QString pluginPath = modulePath + "/plug-ins";
                            if (QDir(pluginPath).exists()) {
                                qDebug() << "  从模块文件找到:" << pluginPath;
                                paths.append(pluginPath);
                            }
                            // 也添加根目录（可能直接包含插件）
                            paths.append(modulePath);
                        }
                    }
                }
                file.close();
            }
        }
    }
#endif

    return paths;
}

QStringList MayaDetector::scanThirdPartyPluginRegistry(const QString &mayaVersion)
{
    QStringList paths;

#ifdef Q_OS_WIN
    qDebug() << "扫描第三方插件注册表 for Maya" << mayaVersion;

    // 定义要扫描的插件基础注册表路径
    QStringList baseKeys;

    // Arnold 可能的注册表位置
    // 机器级注册表
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\Autodesk\\Arnold";
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Autodesk\\Arnold";
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\SolidAngle\\Arnold";
    // 用户级注册表 (关键!)
    baseKeys << "HKEY_CURRENT_USER\\Software\\MtoA" + mayaVersion;  // 直接匹配版本
    baseKeys << "HKEY_CURRENT_USER\\Software\\Autodesk\\Arnold";
    baseKeys << "HKEY_CURRENT_USER\\Software\\SolidAngle\\Arnold";

    // V-Ray 可能的注册表位置
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\Chaos Group\\V-Ray";
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Chaos Group\\V-Ray";
    baseKeys << "HKEY_CURRENT_USER\\Software\\Chaos Group\\V-Ray";

    // Redshift 可能的注册表位置
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\Redshift";
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Redshift";
    baseKeys << "HKEY_CURRENT_USER\\Software\\Redshift";

    // Yeti 可能的注册表位置
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\Peregrine Labs\\Yeti";
    baseKeys << "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Peregrine Labs\\Yeti";
    baseKeys << "HKEY_CURRENT_USER\\Software\\Peregrine Labs\\Yeti";

    // 扫描每个基础路径下的子键
    for (const QString &baseKey : baseKeys) {
        QSettings baseRegistry(baseKey, QSettings::NativeFormat);
        QStringList allKeys = baseRegistry.allKeys();
        QStringList subKeys = baseRegistry.childGroups();

        qDebug() << "  扫描基础键:" << baseKey;
        qDebug() << "    所有键:" << allKeys;
        qDebug() << "    子键:" << subKeys;

        // 先尝试读取基础键本身的值 (针对 HKEY_CURRENT_USER\Software\MtoA2022 这种情况)
        QStringList valueNames;
        valueNames << "INSTALL_DIR" << "InstallDir" << "INSTALL_PATH" << "Path"
                   << "PluginPath" << "Location" << "MTOA_INSTALL_DIR" << "";

        for (const QString &valueName : valueNames) {
            QString installPath = baseRegistry.value(valueName).toString();
            if (!installPath.isEmpty()) {
                qDebug() << "    读取到键值 [" << valueName << "]:" << installPath;

                if (QDir(installPath).exists()) {
                    qDebug() << "      路径存在，添加:" << installPath;
                    paths.append(installPath);

                    // 添加常见的插件子目录
                    QStringList subDirs;
                    subDirs << "/plug-ins"
                            << "/bin/plug-ins"
                            << "/maya" + mayaVersion + "/plug-ins"
                            << "/maya" + mayaVersion
                            << "/scripts"
                            << "";

                    for (const QString &subDir : subDirs) {
                        QString pluginPath = installPath + subDir;
                        if (QDir(pluginPath).exists()) {
                            qDebug() << "        子目录存在:" << pluginPath;
                            paths.append(pluginPath);
                        }
                    }
                }
            }
        }

        // 检查是否有与当前 Maya 版本匹配的子键
        for (const QString &subKey : subKeys) {
            // 检查子键名称是否包含 Maya 版本号
            // 例如: "Maya2022", "Maya 2022 for x64", "Maya2018" 等
            if (subKey.contains(mayaVersion, Qt::CaseInsensitive) ||
                subKey.contains("Maya" + mayaVersion, Qt::CaseInsensitive)) {

                QString fullKey = baseKey + "\\" + subKey;
                qDebug() << "    找到匹配的子键:" << fullKey;

                QSettings registry(fullKey, QSettings::NativeFormat);

                for (const QString &valueName : valueNames) {
                    QString installPath = registry.value(valueName).toString();
                    if (!installPath.isEmpty() && QDir(installPath).exists()) {
                        qDebug() << "      从子键找到路径:" << installPath;
                        paths.append(installPath);

                        // 添加常见的插件子目录
                        QStringList subDirs;
                        subDirs << "/plug-ins"
                                << "/bin/plug-ins"
                                << "/maya" + mayaVersion + "/plug-ins"
                                << "/maya" + mayaVersion
                                << "";

                        for (const QString &subDir : subDirs) {
                            QString pluginPath = installPath + subDir;
                            if (QDir(pluginPath).exists()) {
                                qDebug() << "        子目录:" << pluginPath;
                                paths.append(pluginPath);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
#endif

    return paths;
}
