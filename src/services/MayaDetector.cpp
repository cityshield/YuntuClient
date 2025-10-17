#include "MayaDetector.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QStringConverter>
#include <QMap>
#include <QDirIterator>

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

    qDebug() << "常规方法找到" << mayaPaths.size() << "个可能的 Maya 路径，开始验证...";
    emit detectProgress(30, QString("找到 %1 个可能的 Maya 安装路径，正在验证...").arg(mayaPaths.size()));

    // 验证每个路径
    int currentProgress = 30;
    int step = mayaPaths.isEmpty() ? 0 : (50 / mayaPaths.size());

    for (const QString &path : mayaPaths) {
        qDebug() << "验证路径:" << path;
        if (isValidMayaInstall(path)) {
            MayaSoftwareInfo info = detectMayaAtPath(path);
            if (info.isValid) {
                results.append(info);
                qDebug() << "✓ 检测到有效 Maya:" << info.version << "安装路径:" << info.installPath;
            } else {
                qDebug() << "✗ 路径无效（版本号或 maya.exe 不存在）:" << path;
            }
        } else {
            qDebug() << "✗ 路径验证失败（maya.exe 不存在）:" << path;
        }

        currentProgress += step;
        emit detectProgress(currentProgress, QString("验证: %1").arg(path));
    }

    // 如果常规方法没有找到任何有效的 Maya，启动暴力搜索
    if (results.isEmpty()) {
        qDebug() << "========================================";
        qDebug() << "常规方法未找到有效 Maya，启动暴力搜索...";
        qDebug() << "========================================";
        emit detectProgress(80, "启动全盘搜索 Maya...");

        QStringList bruteForcePaths = bruteForceSearchMaya();
        qDebug() << "暴力搜索找到" << bruteForcePaths.size() << "个 Maya 路径";

        for (const QString &path : bruteForcePaths) {
            if (isValidMayaInstall(path)) {
                MayaSoftwareInfo info = detectMayaAtPath(path);
                if (info.isValid) {
                    results.append(info);
                    qDebug() << "✓ 暴力搜索检测到 Maya:" << info.version << "安装路径:" << info.installPath;
                }
            }
        }
    }

    emit detectProgress(100, "Maya 检测完成");
    qDebug() << "最终检测到" << results.size() << "个有效 Maya 安装";
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

    qDebug() << "========== 开始检测渲染器 ==========";

#ifdef Q_OS_WIN
    // 优先从 pluginPrefs.mel 读取渲染器插件信息
    QMap<QString, QString> pluginPrefsMap = readPluginPrefs(mayaInfo.version);

    // 检查 Arnold (mtoa)
    if (pluginPrefsMap.contains("mtoa")) {
        RendererInfo arnold;
        arnold.name = "Arnold";
        arnold.pluginPath = pluginPrefsMap["mtoa"];
        arnold.isLoaded = true;
        arnold.version = "Unknown";

        // 尝试找到实际的 .mll 文件
        QString mllPath = arnold.pluginPath + "/mtoa.mll";
        if (QFile::exists(mllPath)) {
            arnold.pluginPath = mllPath;
            qDebug() << "  ✓ Arnold (从 Plug-in Manager):" << mllPath;
        } else {
            qDebug() << "  ✓ Arnold (从 Plug-in Manager，路径:" << arnold.pluginPath << ")";
        }

        renderers.append(arnold);
    }

    // 检查 V-Ray
    for (const QString &pluginName : pluginPrefsMap.keys()) {
        if (pluginName.contains("vray", Qt::CaseInsensitive)) {
            RendererInfo vray;
            vray.name = "V-Ray";
            vray.pluginPath = pluginPrefsMap[pluginName];
            vray.isLoaded = true;
            vray.version = "Unknown";

            qDebug() << "  ✓ V-Ray (从 Plug-in Manager):" << vray.pluginPath;
            renderers.append(vray);
            break;
        }
    }

    // 检查 Redshift
    for (const QString &pluginName : pluginPrefsMap.keys()) {
        if (pluginName.contains("redshift", Qt::CaseInsensitive)) {
            RendererInfo redshift;
            redshift.name = "Redshift";
            redshift.pluginPath = pluginPrefsMap[pluginName];
            redshift.isLoaded = true;
            redshift.version = "Unknown";

            qDebug() << "  ✓ Redshift (从 Plug-in Manager):" << redshift.pluginPath;
            renderers.append(redshift);
            break;
        }
    }
#endif

    // 备用检测方法：在 Maya 安装目录中查找（如果 pluginPrefs.mel 中没有找到）
    bool hasArnold = false, hasVRay = false, hasRedshift = false;
    for (const RendererInfo &r : renderers) {
        if (r.name == "Arnold") hasArnold = true;
        if (r.name == "V-Ray") hasVRay = true;
        if (r.name == "Redshift") hasRedshift = true;
    }

    if (!hasArnold) {
        RendererInfo arnold = detectArnold(mayaInfo.installPath);
        if (!arnold.name.isEmpty()) {
            qDebug() << "  ✓ Arnold (从 Maya 目录扫描):" << arnold.pluginPath;
            renderers.append(arnold);
        }
    }

    if (!hasVRay) {
        RendererInfo vray = detectVRay(mayaInfo.installPath);
        if (!vray.name.isEmpty()) {
            qDebug() << "  ✓ V-Ray (从 Maya 目录扫描):" << vray.pluginPath;
            renderers.append(vray);
        }
    }

    if (!hasRedshift) {
        RendererInfo redshift = detectRedshift(mayaInfo.installPath);
        if (!redshift.name.isEmpty()) {
            qDebug() << "  ✓ Redshift (从 Maya 目录扫描):" << redshift.pluginPath;
            renderers.append(redshift);
        }
    }

    qDebug() << "========== 渲染器检测完成，共找到" << renderers.size() << "个 ==========\n";

    return renderers;
}

QStringList MayaDetector::detectPlugins(const MayaSoftwareInfo &mayaInfo)
{
    QStringList plugins;

#ifdef Q_OS_WIN
    // 优先策略：直接从 pluginPrefs.mel 读取已加载的插件（Maya Plug-in Manager 信息）
    qDebug() << "========== 开始从 Plug-in Manager 读取插件 ==========";
    QMap<QString, QString> pluginPrefsMap = readPluginPrefs(mayaInfo.version);

    if (!pluginPrefsMap.isEmpty()) {
        qDebug() << "从 pluginPrefs.mel 找到" << pluginPrefsMap.size() << "个已注册插件";

        for (auto it = pluginPrefsMap.constBegin(); it != pluginPrefsMap.constEnd(); ++it) {
            QString pluginName = it.key();
            QString pluginPath = it.value();

            // 检查插件文件是否存在
            QString fullPluginPath;

            // 如果有明确路径，先尝试
            if (!pluginPath.isEmpty()) {
                if (QFile::exists(pluginPath + "/" + pluginName + ".mll")) {
                    fullPluginPath = pluginPath + "/" + pluginName + ".mll";
                } else if (QFile::exists(pluginPath + "/" + pluginName + ".py")) {
                    fullPluginPath = pluginPath + "/" + pluginName + ".py";
                } else if (QFile::exists(pluginPath + "/" + pluginName + ".bundle")) {
                    fullPluginPath = pluginPath + "/" + pluginName + ".bundle";
                }
            }

            // 如果没有路径或路径中找不到，搜索常见位置
            if (fullPluginPath.isEmpty()) {
                QStringList searchPaths;

                // Maya 安装目录
                searchPaths << mayaInfo.installPath + "/plug-ins";
                searchPaths << mayaInfo.installPath + "/bin/plug-ins";

                // 用户插件目录
                searchPaths << QDir::homePath() + "/Documents/maya/" + mayaInfo.version + "/plug-ins";

                // 获取所有可用驱动器并搜索第三方插件
#ifdef Q_OS_WIN
                QFileInfoList drives = QDir::drives();
                for (const QFileInfo &drive : drives) {
                    QString driveLetter = drive.absolutePath();  // 例如 "C:/", "D:/"

                    // Arnold 可能的安装位置
                    searchPaths << driveLetter + "Program Files/Autodesk/Arnold/maya" + mayaInfo.version + "/plug-ins";
                    searchPaths << driveLetter + "Program Files (x86)/Autodesk/Arnold/maya" + mayaInfo.version + "/plug-ins";
                    searchPaths << driveLetter + "solidangle/mtoadeploy/" + mayaInfo.version + "/plug-ins";

                    // V-Ray 可能的安装位置
                    searchPaths << driveLetter + "Program Files/Chaos Group/V-Ray/Maya " + mayaInfo.version + "/plug-ins";

                    // Redshift 可能的安装位置
                    searchPaths << driveLetter + "ProgramData/Redshift/Plugins/Maya/" + mayaInfo.version;

                    // Yeti 可能的安装位置
                    searchPaths << driveLetter + "Program Files/Peregrine Labs/Yeti-v" + "*" + "/plug-ins";
                }
#else
                // 非 Windows 系统，保留原来的硬编码路径
                searchPaths << "/usr/autodesk/Arnold/maya" + mayaInfo.version + "/plug-ins";
                searchPaths << "/opt/solidangle/mtoa/" + mayaInfo.version;
#endif

                for (const QString &searchPath : searchPaths) {
                    if (QFile::exists(searchPath + "/" + pluginName + ".mll")) {
                        fullPluginPath = searchPath + "/" + pluginName + ".mll";
                        qDebug() << "    在搜索路径中找到:" << fullPluginPath;
                        break;
                    } else if (QFile::exists(searchPath + "/" + pluginName + ".py")) {
                        fullPluginPath = searchPath + "/" + pluginName + ".py";
                        qDebug() << "    在搜索路径中找到:" << fullPluginPath;
                        break;
                    }
                }
            }

            // 格式化插件名称
            QString formattedName = pluginName;
            if (pluginName.contains("mtoa", Qt::CaseInsensitive)) {
                formattedName = "Arnold (mtoa)";
            } else if (pluginName.contains("vray", Qt::CaseInsensitive)) {
                formattedName = "V-Ray";
            } else if (pluginName.contains("redshift", Qt::CaseInsensitive)) {
                formattedName = "Redshift";
            } else if (pluginName.contains("miarmy", Qt::CaseInsensitive)) {
                formattedName = "Miarmy (群集动画)";
            } else if (pluginName.contains("yeti", Qt::CaseInsensitive)) {
                formattedName = "Yeti (毛发系统)";
            } else if (pluginName.contains("xgen", Qt::CaseInsensitive)) {
                formattedName = "XGen (毛发)";
            } else if (pluginName.contains("bifrost", Qt::CaseInsensitive)) {
                formattedName = "Bifrost (流体)";
            } else if (pluginName.contains("mash", Qt::CaseInsensitive)) {
                formattedName = "MASH (运动图形)";
            }

            if (!fullPluginPath.isEmpty()) {
                qDebug() << "  ✓" << formattedName << "(" << pluginPath << ")";
                plugins << formattedName + " [已加载]";
            } else {
                // 如果所有方法都找不到，使用暴力搜索作为最后手段
                qDebug() << "  ?" << pluginName << "未在常规路径找到，尝试暴力搜索...";

#ifdef Q_OS_WIN
                QStringList bruteSearchResults = bruteForceSearchPlugin(pluginName + ".mll", mayaInfo.version);
                if (!bruteSearchResults.isEmpty()) {
                    fullPluginPath = bruteSearchResults.first();  // 使用第一个匹配（优先级最高）
                    qDebug() << "  ✓✓✓ 通过暴力搜索找到:" << fullPluginPath;
                    plugins << formattedName + " [暴力搜索找到]";
                } else {
                    qDebug() << "  ✗ 暴力搜索也未找到" << pluginName;
                    plugins << formattedName + " [已注册，但文件未找到]";
                }
#else
                plugins << formattedName + " [已注册]";
#endif
            }
        }
    } else {
        qDebug() << "pluginPrefs.mel 为空或不存在，使用备用检测方法";
    }

    qDebug() << "========== Plug-in Manager 检测完成 ==========\n";
#endif

    // 备用策略：扫描插件目录（用于未在 pluginPrefs.mel 中注册的插件）
    QStringList pluginDirs;
    pluginDirs << mayaInfo.installPath + "/plug-ins";
    pluginDirs << mayaInfo.installPath + "/bin/plug-ins";

#ifdef Q_OS_WIN
    QString userPlugins = QDir::homePath() + "/Documents/maya/" + mayaInfo.version + "/plug-ins";
    pluginDirs << userPlugins;

    QStringList envPaths = readMayaEnvPaths(mayaInfo.version);
    pluginDirs.append(envPaths);

    QStringList modulePaths = readModulePaths(mayaInfo.version);
    pluginDirs.append(modulePaths);

    QStringList registryPaths = scanThirdPartyPluginRegistry(mayaInfo.version);
    pluginDirs.append(registryPaths);

#elif defined(Q_OS_MAC)
    QString userPlugins = QDir::homePath() + "/Library/Preferences/Autodesk/maya/" + mayaInfo.version + "/plug-ins";
    pluginDirs << userPlugins;
#endif

    pluginDirs.removeDuplicates();
    qDebug() << "扫描额外插件目录:" << pluginDirs;

    for (const QString &dir : pluginDirs) {
        QDir pluginDir(dir);
        if (!pluginDir.exists()) continue;

        QStringList filters;
#ifdef Q_OS_WIN
        filters << "*.mll" << "*.py";
#elif defined(Q_OS_MAC)
        filters << "*.bundle" << "*.py";
#elif defined(Q_OS_LINUX)
        filters << "*.so" << "*.py";
#endif

        QFileInfoList files = pluginDir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo &file : files) {
            QString pluginName = file.baseName();

            // 跳过已经从 pluginPrefs 添加的
            bool alreadyAdded = false;
            for (const QString &existing : plugins) {
                if (existing.contains(pluginName, Qt::CaseInsensitive)) {
                    alreadyAdded = true;
                    break;
                }
            }

            if (!alreadyAdded) {
                QString formattedName = pluginName;
                if (pluginName.contains("miarmy", Qt::CaseInsensitive)) {
                    formattedName = "Miarmy (群集动画)";
                } else if (pluginName.contains("yeti", Qt::CaseInsensitive)) {
                    formattedName = "Yeti (毛发系统)";
                } else if (pluginName.contains("xgen", Qt::CaseInsensitive)) {
                    formattedName = "XGen (毛发)";
                } else if (pluginName.contains("bifrost", Qt::CaseInsensitive)) {
                    formattedName = "Bifrost (流体)";
                } else if (pluginName.contains("mash", Qt::CaseInsensitive)) {
                    formattedName = "MASH (运动图形)";
                } else if (pluginName.contains("mtoa", Qt::CaseInsensitive)) {
                    formattedName = "Arnold (mtoa)";
                }

                plugins << formattedName + " [扫描发现]";
                qDebug() << "  发现额外插件:" << formattedName;
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
    // 扫描所有驱动器的常用路径
    QFileInfoList drives = QDir::drives();
    qDebug() << "检测到的驱动器:" << drives;

    for (const QFileInfo &drive : drives) {
        QString driveLetter = drive.absolutePath();  // 例如 "C:/", "D:/"

        QStringList basePaths;
        basePaths << driveLetter + "Program Files/Autodesk";
        basePaths << driveLetter + "Program Files (x86)/Autodesk";
        basePaths << driveLetter + "Autodesk";  // 有些用户可能直接装在根目录

        for (const QString &basePath : basePaths) {
            QDir dir(basePath);
            if (!dir.exists()) continue;

            QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &subdir : subdirs) {
                if (subdir.startsWith("Maya", Qt::CaseInsensitive)) {
                    QString mayaPath = dir.absoluteFilePath(subdir);
                    qDebug() << "  在" << driveLetter << "发现 Maya:" << mayaPath;
                    paths.append(mayaPath);
                }
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
    // pluginPrefs.mel 文件可能的位置
    QStringList possiblePaths;

    // 标准路径
    possiblePaths << QDir::homePath() + "/Documents/maya/" + mayaVersion + "/prefs/pluginPrefs.mel";

    // 备用路径（某些系统配置）
    possiblePaths << QDir::homePath() + "/My Documents/maya/" + mayaVersion + "/prefs/pluginPrefs.mel";
    possiblePaths << "C:/Users/" + qgetenv("USERNAME") + "/Documents/maya/" + mayaVersion + "/prefs/pluginPrefs.mel";

    qDebug() << "尝试查找 pluginPrefs.mel for Maya" << mayaVersion;
    qDebug() << "  用户主目录:" << QDir::homePath();

    QString prefsPath;
    for (const QString &path : possiblePaths) {
        qDebug() << "  检查路径:" << path << "存在:" << QFile::exists(path);
        if (QFile::exists(path)) {
            prefsPath = path;
            qDebug() << "    ✓ 找到 pluginPrefs.mel:" << prefsPath;
            break;
        }
    }

    if (prefsPath.isEmpty()) {
        qDebug() << "  ✗ 未找到 pluginPrefs.mel 文件";
        qDebug() << "  提示：请检查以下目录:";
        qDebug() << "    1." << QDir::homePath() + "/Documents/maya/" + mayaVersion + "/prefs/";
        qDebug() << "    2. 或者 Maya 是否曾经运行过（pluginPrefs.mel 在首次运行 Maya 时创建）";
        return pluginMap;
    }

    QFile file(prefsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        qDebug() << "  pluginPrefs.mel 文件大小:" << content.size() << "字节";

        int count = 0;

        // 格式1: evalDeferred("autoLoadPlugin(\"\", \"mtoa\", \"mtoa\")");
        //  这是 Maya 2022+ 的格式，没有路径信息
        QRegularExpression reEvalDeferred("autoLoadPlugin\\([^,]*,\\s*\"([^\"]+)\"");
        QRegularExpressionMatchIterator it = reEvalDeferred.globalMatch(content);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString pluginName = match.captured(1);

            if (!pluginMap.contains(pluginName)) {
                qDebug() << "  [evalDeferred格式] 找到:" << pluginName << "(需要搜索路径)";
                pluginMap[pluginName] = "";  // 空路径，稍后搜索
                count++;
            }
        }

        // 格式2: pluginInfo -edit -pluginPath "C:/path/to/plug-ins" "mtoa";
        //  这是较旧的格式，包含路径信息
        QRegularExpression reWithPath("pluginInfo.*?-pluginPath\\s+\"([^\"]+)\".*?\"([^\"]+)\"");
        it = reWithPath.globalMatch(content);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString pluginPath = match.captured(1);
            QString pluginName = match.captured(2);

            qDebug() << "  [pluginInfo格式] 找到:" << pluginName << "->" << pluginPath;
            pluginMap[pluginName] = pluginPath;
            count++;
        }

        qDebug() << "  共解析到" << count << "个插件配置";
    } else {
        qDebug() << "  ✗ 无法打开文件:" << prefsPath;
        qDebug() << "  错误:" << file.errorString();
    }
#endif

    return pluginMap;
}

QStringList MayaDetector::readModulePaths(const QString &mayaVersion)
{
    QStringList paths;

#ifdef Q_OS_WIN
    qDebug() << "========== 开始扫描 Maya 模块系统 ==========";

    // Maya 模块路径（优先级从高到低）
    QStringList moduleDirs;

    // 1. 用户级模块（最高优先级）
    moduleDirs << QDir::homePath() + "/Documents/maya/" + mayaVersion + "/modules";
    moduleDirs << QDir::homePath() + "/Documents/maya/modules";

    // 2. 系统级模块（关键！Arnold 通常在这里注册）
    moduleDirs << "C:/ProgramData/Autodesk/ApplicationPlugins";  // 重要！
    moduleDirs << "C:/Program Files/Common Files/Autodesk Shared/Modules/maya/" + mayaVersion;
    moduleDirs << "C:/Program Files/Common Files/Autodesk Shared/Modules/maya";

    // 3. 扫描所有驱动器的 ProgramData
    QFileInfoList drives = QDir::drives();
    for (const QFileInfo &drive : drives) {
        QString driveLetter = drive.absolutePath();
        moduleDirs << driveLetter + "ProgramData/Autodesk/ApplicationPlugins";
        moduleDirs << driveLetter + "Program Files/Common Files/Autodesk Shared/Modules/maya/" + mayaVersion;
    }

    // 去重
    moduleDirs.removeDuplicates();

    for (const QString &moduleDir : moduleDirs) {
        QDir dir(moduleDir);
        if (!dir.exists()) {
            qDebug() << "  跳过不存在的目录:" << moduleDir;
            continue;
        }

        qDebug() << "✓ 扫描模块目录:" << moduleDir;

        // 在 ApplicationPlugins 中，插件通常在子文件夹中
        // 例如: C:/ProgramData/Autodesk/ApplicationPlugins/MtoA/Contents/
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir : subdirs) {
            QString subdirPath = dir.absoluteFilePath(subdir);

            // 查找 .mod 文件（在子目录或 Contents 目录中）
            QStringList modSearchPaths;
            modSearchPaths << subdirPath;
            modSearchPaths << subdirPath + "/Contents";
            modSearchPaths << subdirPath + "/Contents/modules";

            for (const QString &modSearchPath : modSearchPaths) {
                QDir modDir(modSearchPath);
                if (!modDir.exists()) continue;

                QFileInfoList modFiles = modDir.entryInfoList(QStringList() << "*.mod" << "*.xml", QDir::Files);
                for (const QFileInfo &modFile : modFiles) {
                    qDebug() << "    找到模块文件:" << modFile.fileName();

                    QFile file(modFile.absoluteFilePath());
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QTextStream in(&file);
                        while (!in.atEnd()) {
                            QString line = in.readLine().trimmed();

                            // 跳过注释和空行
                            if (line.isEmpty() || line.startsWith("#")) {
                                continue;
                            }

                            // 解析模块定义（多种格式）
                            // 格式1: + MAYAVERSION:2022 mtoa 5.1.0 C:/Program Files/Autodesk/Arnold/maya2022
                            // 格式2: + mtoa 5.1.0 ../
                            QString modulePath;

                            if (line.startsWith("+")) {
                                QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                                if (parts.size() >= 2) {
                                    // 最后一个部分通常是路径
                                    QString lastPart = parts.last();

                                    // 处理相对路径
                                    if (lastPart == "../" || lastPart == "..") {
                                        modulePath = QFileInfo(modFile.absoluteFilePath()).dir().absolutePath();
                                        QDir parentDir(modulePath);
                                        parentDir.cdUp();
                                        modulePath = parentDir.absolutePath();
                                    } else if (lastPart.startsWith("./")) {
                                        modulePath = QFileInfo(modFile.absoluteFilePath()).dir().absolutePath() + "/" + lastPart.mid(2);
                                    } else if (QDir(lastPart).exists() || QDir(modSearchPath + "/" + lastPart).exists()) {
                                        modulePath = QDir(lastPart).exists() ? lastPart : modSearchPath + "/" + lastPart;
                                    }

                                    if (!modulePath.isEmpty() && QDir(modulePath).exists()) {
                                        qDebug() << "      解析到模块路径:" << modulePath;

                                        // 添加 plug-ins 子目录
                                        QStringList pluginSubDirs;
                                        pluginSubDirs << "/plug-ins"
                                                     << "/bin/plug-ins"
                                                     << "";

                                        for (const QString &pluginSubDir : pluginSubDirs) {
                                            QString pluginPath = modulePath + pluginSubDir;
                                            if (QDir(pluginPath).exists()) {
                                                qDebug() << "        ✓ 插件目录:" << pluginPath;
                                                paths.append(pluginPath);
                                            }
                                        }

                                        // 也添加根目录
                                        paths.append(modulePath);
                                    }
                                }
                            }
                        }
                        file.close();
                    }
                }
            }
        }
    }

    qDebug() << "========== 模块扫描完成，共找到" << paths.size() << "个路径 ==========\n";
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

QStringList MayaDetector::bruteForceSearchPlugin(const QString &pluginFileName, const QString &mayaVersion)
{
    QStringList foundPaths;

#ifdef Q_OS_WIN
    qDebug() << "========== 开始暴力搜索插件:" << pluginFileName << "==========";

    // 获取所有可用驱动器
    QFileInfoList drives = QDir::drives();
    qDebug() << "可用驱动器:" << drives.size() << "个";

    for (const QFileInfo &drive : drives) {
        QString driveLetter = drive.absolutePath();  // 例如 "C:/", "D:/"
        qDebug() << "\n正在搜索驱动器:" << driveLetter;

        // 定义常见的搜索路径（优先级从高到低）
        QStringList searchPaths;

        // 1. Program Files 下的 Autodesk 目录
        searchPaths << driveLetter + "Program Files/Autodesk";
        searchPaths << driveLetter + "Program Files (x86)/Autodesk";

        // 2. Arnold 的标准安装位置
        searchPaths << driveLetter + "Program Files/Autodesk/Arnold";
        searchPaths << driveLetter + "solidangle";

        // 3. Yeti 的标准安装位置
        searchPaths << driveLetter + "Program Files/Peregrine Labs";
        searchPaths << driveLetter + "Peregrine Labs";

        // 4. V-Ray 位置
        searchPaths << driveLetter + "Program Files/Chaos Group";

        // 5. Redshift 位置
        searchPaths << driveLetter + "ProgramData/Redshift";

        // 6. 通用插件位置
        searchPaths << driveLetter + "ProgramData/Autodesk";

        // 递归搜索每个路径
        for (const QString &basePath : searchPaths) {
            QDir baseDir(basePath);
            if (!baseDir.exists()) {
                continue;
            }

            qDebug() << "  搜索基础路径:" << basePath;

            // 使用 QDirIterator 递归搜索（限制深度避免太慢）
            QDirIterator it(basePath,
                           QStringList() << pluginFileName,
                           QDir::Files,
                           QDirIterator::Subdirectories);

            while (it.hasNext()) {
                QString foundPath = it.next();
                qDebug() << "    ✓✓✓ 找到!" << foundPath;

                // 检查是否与 Maya 版本匹配
                if (foundPath.contains(mayaVersion, Qt::CaseInsensitive) ||
                    foundPath.contains("maya" + mayaVersion, Qt::CaseInsensitive)) {
                    qDebug() << "      [版本匹配] 添加到结果列表（优先级高）";
                    foundPaths.prepend(foundPath);  // 版本匹配的放在前面
                } else {
                    qDebug() << "      添加到结果列表";
                    foundPaths.append(foundPath);
                }
            }
        }
    }

    if (foundPaths.isEmpty()) {
        qDebug() << "========== 未找到" << pluginFileName << "==========\n";
    } else {
        qDebug() << "========== 找到" << foundPaths.size() << "个" << pluginFileName << "文件 ==========";
        for (const QString &path : foundPaths) {
            qDebug() << "  -" << path;
        }
        qDebug() << "==========\n";
    }
#endif

    return foundPaths;
}

QStringList MayaDetector::bruteForceSearchMaya()
{
    QStringList mayaPaths;

#ifdef Q_OS_WIN
    qDebug() << "========== 开始暴力搜索 Maya 安装 ==========";

    // 获取所有可用驱动器
    QFileInfoList drives = QDir::drives();
    qDebug() << "可用驱动器:" << drives.size() << "个";

    for (const QFileInfo &drive : drives) {
        QString driveLetter = drive.absolutePath();  // 例如 "C:/", "D:/"
        qDebug() << "\n正在搜索驱动器:" << driveLetter;

        // 定义要搜索的基础路径
        QStringList searchBasePaths;
        searchBasePaths << driveLetter + "Program Files";
        searchBasePaths << driveLetter + "Program Files (x86)";
        searchBasePaths << driveLetter;  // 有些用户直接安装在根目录

        for (const QString &basePath : searchBasePaths) {
            QDir baseDir(basePath);
            if (!baseDir.exists()) {
                continue;
            }

            qDebug() << "  搜索基础路径:" << basePath;

            // 使用 QDirIterator 递归搜索 maya.exe
            // 限制深度为3层避免搜索太深太慢
            // 例如: D:/Program Files/Autodesk/Maya2022/bin/maya.exe (深度4层)
            QDirIterator it(basePath,
                           QStringList() << "maya.exe",
                           QDir::Files,
                           QDirIterator::Subdirectories);

            while (it.hasNext()) {
                QString mayaExePath = it.next();
                qDebug() << "    ✓✓✓ 找到 maya.exe:" << mayaExePath;

                // 从 maya.exe 路径推导出安装路径
                // maya.exe 在 {安装路径}/bin/maya.exe
                QFileInfo fileInfo(mayaExePath);
                QDir binDir = fileInfo.dir();  // bin 目录
                if (binDir.dirName().toLower() == "bin") {
                    binDir.cdUp();  // 回到安装目录
                    QString mayaInstallPath = binDir.absolutePath();

                    // 验证这是一个合法的 Maya 安装目录（应该包含版本号）
                    if (mayaInstallPath.contains(QRegularExpression("Maya\\d{4}", QRegularExpression::CaseInsensitiveOption))) {
                        qDebug() << "      [有效 Maya 安装] 添加:" << mayaInstallPath;
                        mayaPaths.append(mayaInstallPath);
                    } else {
                        qDebug() << "      [跳过] 路径不包含版本号:" << mayaInstallPath;
                    }
                }
            }
        }
    }

    mayaPaths.removeDuplicates();

    if (mayaPaths.isEmpty()) {
        qDebug() << "========== 暴力搜索未找到 Maya ==========\n";
    } else {
        qDebug() << "========== 暴力搜索找到" << mayaPaths.size() << "个 Maya 安装 ==========";
        for (const QString &path : mayaPaths) {
            qDebug() << "  -" << path;
        }
        qDebug() << "==========\n";
    }
#endif

    return mayaPaths;
}
