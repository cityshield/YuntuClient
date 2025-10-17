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
    
    // 使用完整的插件检测方法获取所有渲染器
    QList<RendererInfo> allPlugins = getAllMayaPlugins(mayaInfo.version);
    for (const RendererInfo &plugin : allPlugins) {
        renderers.append(plugin);
    }
    
    qDebug() << "通过完整检测找到" << renderers.size() << "个渲染器插件";
    
    // 如果完整检测没有找到任何插件，则使用原有的检测方法作为备用
    if (renderers.isEmpty()) {
        qDebug() << "完整检测未找到插件，使用备用检测方法...";

#ifdef Q_OS_WIN
    // 优先从 pluginPrefs.mel 读取渲染器插件信息
    QMap<QString, QString> pluginPrefsMap = readPluginPrefs(mayaInfo.version);

    // 检查 Arnold (mtoa)
    if (pluginPrefsMap.contains("mtoa")) {
        RendererInfo arnold;
        arnold.name = "Arnold (Plug-in Manager)";
        arnold.pluginPath = pluginPrefsMap["mtoa"];
        arnold.isLoaded = true;
        arnold.version = "Unknown";  // 先设置默认值，后面会更新

        qDebug() << "从 pluginPrefs.mel 找到 Arnold 配置，路径:" << arnold.pluginPath;

        // 如果路径为空，说明是 Maya 2022+ 的格式，需要搜索实际文件
        if (arnold.pluginPath.isEmpty()) {
            qDebug() << "  Arnold 路径为空，尝试搜索实际插件文件...";

        // 尝试找到实际的插件文件
        QStringList arnoldExts;
#ifdef Q_OS_WIN
        arnoldExts << ".mll" << ".dll";
#elif defined(Q_OS_MAC)
        arnoldExts << ".bundle";
#elif defined(Q_OS_LINUX)
        arnoldExts << ".so";
#endif

            // 在多个可能的位置搜索
            QStringList searchPaths;
            searchPaths << mayaInfo.installPath + "/bin/plug-ins";
            searchPaths << mayaInfo.installPath + "/plug-ins";
            searchPaths << mayaInfo.installPath + "/bin/plug-ins/arnold";
            
            // 添加独立安装的Arnold路径
            QString mayaVersion = extractVersionFromPath(mayaInfo.installPath);
            searchPaths << "C:/Program Files/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins";
            searchPaths << "C:/Program Files (x86)/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins";
            searchPaths << "C:/solidangle/mtoadeploy/" + mayaVersion + "/plug-ins";
            searchPaths << "C:/Program Files/solidangle/mtoadeploy/" + mayaVersion + "/plug-ins";

            bool found = false;
            for (const QString &searchPath : searchPaths) {
                for (const QString &ext : arnoldExts) {
                    QString pluginFile = searchPath + "/mtoa" + ext;
                    if (QFile::exists(pluginFile)) {
                        arnold.pluginPath = pluginFile;
                        qDebug() << "  ✓ Arnold (从 Plug-in Manager 搜索):" << pluginFile;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            
            if (!found) {
                qDebug() << "  ✗ 在搜索路径中未找到 Arnold 插件文件";
            } else {
                // 提取版本信息
                arnold.version = extractArnoldVersion(arnold.pluginPath, mayaInfo.version);
                qDebug() << "  Arnold 版本:" << arnold.version;
            }
        } else {
            // 路径不为空，验证文件是否存在
            QStringList arnoldExts;
#ifdef Q_OS_WIN
            arnoldExts << ".mll" << ".dll";
#elif defined(Q_OS_MAC)
            arnoldExts << ".bundle";
#elif defined(Q_OS_LINUX)
            arnoldExts << ".so";
#endif

            bool found = false;
        for (const QString &ext : arnoldExts) {
            QString pluginFile = arnold.pluginPath + "/mtoa" + ext;
            if (QFile::exists(pluginFile)) {
                arnold.pluginPath = pluginFile;
                    qDebug() << "  ✓ Arnold (从 Plug-in Manager 路径):" << pluginFile;
                    found = true;
                break;
            }
        }

            if (!found) {
                qDebug() << "  ⚠ Arnold 配置路径存在但插件文件未找到:" << arnold.pluginPath;
            } else {
                // 提取版本信息
                arnold.version = extractArnoldVersion(arnold.pluginPath, mayaInfo.version);
                qDebug() << "  Arnold 版本:" << arnold.version;
            }
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
    } // 备用检测方法的结束

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

            // 根据平台确定插件扩展名
            QStringList pluginExtensions;
#ifdef Q_OS_WIN
            pluginExtensions << ".mll" << ".dll" << ".py";
#elif defined(Q_OS_MAC)
            pluginExtensions << ".bundle" << ".py";
#elif defined(Q_OS_LINUX)
            pluginExtensions << ".so" << ".py";
#endif

            // 如果有明确路径，先尝试
            if (!pluginPath.isEmpty()) {
                for (const QString &ext : pluginExtensions) {
                    QString testPath = pluginPath + "/" + pluginName + ext;
                    if (QFile::exists(testPath)) {
                        fullPluginPath = testPath;
                        break;
                    }
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
                    for (const QString &ext : pluginExtensions) {
                        QString testPath = searchPath + "/" + pluginName + ext;
                        if (QFile::exists(testPath)) {
                            fullPluginPath = testPath;
                            qDebug() << "    在搜索路径中找到:" << fullPluginPath;
                            break;
                        }
                    }
                    if (!fullPluginPath.isEmpty()) {
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

                // 尝试所有可能的扩展名进行暴力搜索
                bool foundByBruteForce = false;
                for (const QString &ext : pluginExtensions) {
                    QStringList bruteSearchResults = bruteForceSearchPlugin(pluginName + ext, mayaInfo.version);
                    if (!bruteSearchResults.isEmpty()) {
                        fullPluginPath = bruteSearchResults.first();
                        qDebug() << "  ✓✓✓ 通过暴力搜索找到:" << fullPluginPath;
                        plugins << formattedName + " [暴力搜索找到]";
                        foundByBruteForce = true;
                        break;
                    }
                }

                if (!foundByBruteForce) {
                    qDebug() << "  ✗ 暴力搜索也未找到" << pluginName;
                    plugins << formattedName + " [已注册，但文件未找到]";
                }
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
        filters << "*.mll" << "*.dll" << "*.py";
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
    QString mayaVersion = extractVersionFromPath(mayaPath);

    qDebug() << "========== 开始检测 Arnold 渲染器 ==========";
    qDebug() << "Maya 路径:" << mayaPath;
    qDebug() << "Maya 版本:" << mayaVersion;

    // 1. 首先检查 Maya 内置的 Arnold 插件
    QStringList mayaBuiltinPaths;
#ifdef Q_OS_WIN
    mayaBuiltinPaths << mayaPath + "/bin/plug-ins/mtoa.mll";
    mayaBuiltinPaths << mayaPath + "/bin/plug-ins/mtoa.dll";
    mayaBuiltinPaths << mayaPath + "/plug-ins/mtoa.mll";
    mayaBuiltinPaths << mayaPath + "/plug-ins/mtoa.dll";
    mayaBuiltinPaths << mayaPath + "/bin/plug-ins/arnold/mtoa.mll";
    mayaBuiltinPaths << mayaPath + "/bin/plug-ins/arnold/mtoa.dll";
#elif defined(Q_OS_MAC)
    mayaBuiltinPaths << mayaPath + "/Maya.app/Contents/plug-ins/mtoa.bundle";
    mayaBuiltinPaths << mayaPath + "/plug-ins/mtoa.bundle";
    mayaBuiltinPaths << mayaPath + "/Maya.app/Contents/plug-ins/arnold/mtoa.bundle";
#elif defined(Q_OS_LINUX)
    mayaBuiltinPaths << mayaPath + "/plug-ins/mtoa.so";
    mayaBuiltinPaths << mayaPath + "/plug-ins/arnold/mtoa.so";
#endif

    for (const QString &arnoldPlugin : mayaBuiltinPaths) {
        qDebug() << "检测 Maya 内置 Arnold:" << arnoldPlugin << "存在:" << QFile::exists(arnoldPlugin);
        if (QFile::exists(arnoldPlugin)) {
            info.name = "Arnold (Maya 内置)";
            info.pluginPath = arnoldPlugin;
            info.isLoaded = true;
            info.version = extractArnoldVersion(arnoldPlugin, mayaVersion);
            qDebug() << "  ✓ 找到 Maya 内置 Arnold:" << arnoldPlugin;
            qDebug() << "  版本:" << info.version;
            return info;
        }
    }

    // 2. 检查独立安装的 Arnold 渲染器
    QStringList independentArnoldPaths;
#ifdef Q_OS_WIN
    // 常见的 Arnold 独立安装路径
    QStringList driveLetters = {"C:/", "D:/", "E:/", "F:/"};
    for (const QString &drive : driveLetters) {
        // Autodesk Arnold 官方安装路径
        independentArnoldPaths << drive + "Program Files/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins/mtoa.mll";
        independentArnoldPaths << drive + "Program Files/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins/mtoa.dll";
        independentArnoldPaths << drive + "Program Files (x86)/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins/mtoa.mll";
        independentArnoldPaths << drive + "Program Files (x86)/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins/mtoa.dll";
        
        // Solid Angle Arnold 安装路径
        independentArnoldPaths << drive + "solidangle/mtoadeploy/" + mayaVersion + "/plug-ins/mtoa.mll";
        independentArnoldPaths << drive + "solidangle/mtoadeploy/" + mayaVersion + "/plug-ins/mtoa.dll";
        independentArnoldPaths << drive + "Program Files/solidangle/mtoadeploy/" + mayaVersion + "/plug-ins/mtoa.mll";
        independentArnoldPaths << drive + "Program Files/solidangle/mtoadeploy/" + mayaVersion + "/plug-ins/mtoa.dll";
        
        // 其他可能的路径
        independentArnoldPaths << drive + "Arnold/maya" + mayaVersion + "/plug-ins/mtoa.mll";
        independentArnoldPaths << drive + "Arnold/maya" + mayaVersion + "/plug-ins/mtoa.dll";
    }
#elif defined(Q_OS_MAC)
    independentArnoldPaths << "/Applications/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins/mtoa.bundle";
    independentArnoldPaths << "/opt/solidangle/mtoa/" + mayaVersion + "/plug-ins/mtoa.bundle";
    independentArnoldPaths << "/usr/local/arnold/maya" + mayaVersion + "/plug-ins/mtoa.bundle";
#elif defined(Q_OS_LINUX)
    independentArnoldPaths << "/opt/autodesk/arnold/maya" + mayaVersion + "/plug-ins/mtoa.so";
    independentArnoldPaths << "/opt/solidangle/mtoa/" + mayaVersion + "/plug-ins/mtoa.so";
    independentArnoldPaths << "/usr/local/arnold/maya" + mayaVersion + "/plug-ins/mtoa.so";
#endif

    for (const QString &arnoldPlugin : independentArnoldPaths) {
        qDebug() << "检测独立 Arnold:" << arnoldPlugin << "存在:" << QFile::exists(arnoldPlugin);
        if (QFile::exists(arnoldPlugin)) {
            info.name = "Arnold (独立安装)";
            info.pluginPath = arnoldPlugin;
            info.isLoaded = true;
            info.version = extractArnoldVersion(arnoldPlugin, mayaVersion);
            qDebug() << "  ✓ 找到独立 Arnold:" << arnoldPlugin;
            qDebug() << "  版本:" << info.version;
            return info;
        }
    }

    // 3. 检查环境变量中的路径
    QStringList envPaths;
#ifdef Q_OS_WIN
    QString mayaModulePath = qgetenv("MAYA_MODULE_PATH");
    if (!mayaModulePath.isEmpty()) {
        QStringList modulePaths = mayaModulePath.split(";");
        for (const QString &modulePath : modulePaths) {
            if (modulePath.contains("arnold", Qt::CaseInsensitive) || 
                modulePath.contains("mtoa", Qt::CaseInsensitive)) {
                envPaths << modulePath + "/mtoa.mll";
                envPaths << modulePath + "/mtoa.dll";
                envPaths << modulePath + "/plug-ins/mtoa.mll";
                envPaths << modulePath + "/plug-ins/mtoa.dll";
            }
        }
    }
    
    QString arnoldPath = qgetenv("ARNOLD_ROOT");
    if (!arnoldPath.isEmpty()) {
        envPaths << arnoldPath + "/maya" + mayaVersion + "/plug-ins/mtoa.mll";
        envPaths << arnoldPath + "/maya" + mayaVersion + "/plug-ins/mtoa.dll";
    }
#endif

    for (const QString &arnoldPlugin : envPaths) {
        qDebug() << "检测环境变量 Arnold:" << arnoldPlugin << "存在:" << QFile::exists(arnoldPlugin);
        if (QFile::exists(arnoldPlugin)) {
            info.name = "Arnold (环境变量)";
            info.pluginPath = arnoldPlugin;
            info.isLoaded = true;
            info.version = extractArnoldVersion(arnoldPlugin, mayaVersion);
            qDebug() << "  ✓ 找到环境变量 Arnold:" << arnoldPlugin;
            qDebug() << "  版本:" << info.version;
            return info;
        }
    }

    // 4. 检查用户文档目录中的模块文件
    QStringList userModulePaths;
#ifdef Q_OS_WIN
    QString userDocs = QDir::homePath() + "/Documents/maya/" + mayaVersion + "/modules";
    userModulePaths << userDocs;
    userModulePaths << QDir::homePath() + "/My Documents/maya/" + mayaVersion + "/modules";
#elif defined(Q_OS_MAC)
    userModulePaths << QDir::homePath() + "/Library/Preferences/Autodesk/maya/" + mayaVersion + "/modules";
#elif defined(Q_OS_LINUX)
    userModulePaths << QDir::homePath() + "/maya/" + mayaVersion + "/modules";
#endif

    for (const QString &modulePath : userModulePaths) {
        if (QDir(modulePath).exists()) {
            QDir dir(modulePath);
            QStringList filters;
            filters << "*.mod" << "mtoa.mod" << "*arnold*.mod";
            QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
            
            for (const QFileInfo &file : files) {
                qDebug() << "检查模块文件:" << file.absoluteFilePath();
                // 读取模块文件内容，提取路径
                QFile moduleFile(file.absoluteFilePath());
                if (moduleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&moduleFile);
                    QString content = in.readAll();
                    moduleFile.close();
                    
                    // 解析模块文件中的路径
                    QRegularExpression pathRegex("\\+\\s+\\w+\\s+\\d+\\.\\d+\\.\\d+\\s+(.+)");
                    QRegularExpressionMatch match = pathRegex.match(content);
                    if (match.hasMatch()) {
                        QString arnoldRoot = match.captured(1).trimmed();
                        QStringList testPaths;
                        testPaths << arnoldRoot + "/plug-ins/mtoa.mll";
                        testPaths << arnoldRoot + "/plug-ins/mtoa.dll";
                        testPaths << arnoldRoot + "/mtoa.mll";
                        testPaths << arnoldRoot + "/mtoa.dll";
                        
                        for (const QString &testPath : testPaths) {
                            qDebug() << "检测模块文件 Arnold:" << testPath << "存在:" << QFile::exists(testPath);
                            if (QFile::exists(testPath)) {
                                info.name = "Arnold (模块文件)";
                                info.pluginPath = testPath;
                                info.isLoaded = true;
                                info.version = extractArnoldVersion(testPath, mayaVersion);
                                qDebug() << "  ✓ 找到模块文件 Arnold:" << testPath;
                                qDebug() << "  版本:" << info.version;
                                return info;
                            }
                        }
                    }
                }
            }
        }
    }

    // 5. 最后尝试暴力搜索（如果前面都没找到）
    qDebug() << "尝试暴力搜索 Arnold 插件...";
    QStringList bruteForceResults = bruteForceSearchPlugin("mtoa.mll", mayaVersion);
    if (bruteForceResults.isEmpty()) {
        bruteForceResults = bruteForceSearchPlugin("mtoa.dll", mayaVersion);
    }
    
    if (!bruteForceResults.isEmpty()) {
        info.name = "Arnold (暴力搜索)";
        info.pluginPath = bruteForceResults.first();
        info.isLoaded = true;
        info.version = extractArnoldVersion(info.pluginPath, mayaVersion);
        qDebug() << "  ✓ 暴力搜索找到 Arnold:" << info.pluginPath;
        qDebug() << "  版本:" << info.version;
        return info;
    }

    qDebug() << "  ✗ 未找到 Arnold 插件";
    qDebug() << "========== Arnold 检测完成 ==========";
    return info;
}

RendererInfo MayaDetector::detectVRay(const QString &mayaPath)
{
    RendererInfo info;

    // V-Ray 可能在多个位置
    QStringList possiblePaths;

#ifdef Q_OS_WIN
    possiblePaths << mayaPath + "/bin/plug-ins/vrayformaya.mll";
    possiblePaths << mayaPath + "/bin/plug-ins/vrayformaya.dll";
    possiblePaths << mayaPath + "/plug-ins/vrayformaya.mll";
    possiblePaths << mayaPath + "/plug-ins/vrayformaya.dll";
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
    possiblePaths << mayaPath + "/bin/plug-ins/redshift4maya.dll";
    possiblePaths << mayaPath + "/plug-ins/redshift4maya.mll";
    possiblePaths << mayaPath + "/plug-ins/redshift4maya.dll";
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

        // 2. Arnold 的标准安装位置（更全面的路径）
        searchPaths << driveLetter + "Program Files/Autodesk/Arnold";
        searchPaths << driveLetter + "Program Files (x86)/Autodesk/Arnold";
        searchPaths << driveLetter + "solidangle";
        searchPaths << driveLetter + "Program Files/solidangle";
        searchPaths << driveLetter + "Program Files (x86)/solidangle";
        searchPaths << driveLetter + "Arnold";
        searchPaths << driveLetter + "Program Files/Arnold";
        searchPaths << driveLetter + "Program Files (x86)/Arnold";

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

            // 对于Arnold插件，使用更智能的搜索策略
            if (pluginFileName.contains("mtoa", Qt::CaseInsensitive)) {
                // Arnold插件特殊处理：优先搜索特定目录结构
                QStringList arnoldSpecificPaths;
                arnoldSpecificPaths << basePath + "/maya" + mayaVersion + "/plug-ins";
                arnoldSpecificPaths << basePath + "/mtoadeploy/" + mayaVersion + "/plug-ins";
                arnoldSpecificPaths << basePath + "/plug-ins";
                arnoldSpecificPaths << basePath + "/bin/plug-ins";
                
                for (const QString &arnoldPath : arnoldSpecificPaths) {
                    QDir arnoldDir(arnoldPath);
                    if (arnoldDir.exists()) {
                        QStringList filters;
                        filters << "mtoa.mll" << "mtoa.dll";
                        QFileInfoList files = arnoldDir.entryInfoList(filters, QDir::Files);
                        
                        for (const QFileInfo &file : files) {
                            QString foundPath = file.absoluteFilePath();
                            qDebug() << "    ✓✓✓ Arnold专用搜索找到!" << foundPath;
                            foundPaths.prepend(foundPath);  // Arnold专用搜索的结果优先级最高
                        }
                    }
                }
            }

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

QString MayaDetector::extractArnoldVersion(const QString &pluginPath, const QString &mayaVersion)
{
    qDebug() << "========== 开始提取 Arnold 版本信息 ==========";
    qDebug() << "插件路径:" << pluginPath;
    qDebug() << "Maya 版本:" << mayaVersion;
    qDebug() << "插件文件存在:" << QFile::exists(pluginPath);

    // 方法1: 从插件路径中提取版本信息
    // 例如: C:/Program Files/Autodesk/Arnold/maya2024/plug-ins/mtoa.mll
    // 或: C:/solidangle/mtoadeploy/2024/plug-ins/mtoa.mll
    // 或: C:/Arnold-5.3.0.1/plug-ins/mtoa.mll
    QStringList pathPatterns;
    pathPatterns << "mtoa[_-]?(\\d+\\.\\d+\\.\\d+\\.\\d+)";  // 4位版本号
    pathPatterns << "mtoa[_-]?(\\d+\\.\\d+\\.\\d+)";        // 3位版本号
    pathPatterns << "arnold[/_-](\\d+\\.\\d+\\.\\d+\\.\\d+)"; // Arnold-x.x.x.x
    pathPatterns << "arnold[/_-](\\d+\\.\\d+\\.\\d+)";       // Arnold-x.x.x
    
    for (const QString &pattern : pathPatterns) {
        QRegularExpression versionInPath(pattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = versionInPath.match(pluginPath);
        if (match.hasMatch()) {
            QString version = match.captured(1);
            qDebug() << "  ✓ 从路径提取到版本:" << version;
            return version;
        }
    }

    // 方法2: 搜索Arnold版本信息文件
    QStringList versionFiles = searchArnoldVersionFiles(pluginPath);
    
    for (const QString &versionFilePath : versionFiles) {
        qDebug() << "读取版本文件:" << versionFilePath;
        
        // 只处理两个关键的头文件
        if (versionFilePath.endsWith("include/mtoa/utils/Version.h", Qt::CaseInsensitive)) {
            QString headerVersion = extractVersionFromMtoaHeader(versionFilePath);
            if (headerVersion != "Unknown") {
                return headerVersion;
            }
        }
        
        if (versionFilePath.endsWith("include/arnold/ai_version.h", Qt::CaseInsensitive)) {
            QString headerVersion = extractVersionFromArnoldHeader(versionFilePath);
            if (headerVersion != "Unknown") {
                return headerVersion;
            }
        }
    }

    // 方法3: 从模块文件 (.mod) 中提取版本信息
    // Arnold 通常在模块文件中声明版本
    QStringList moduleDirs;
    
#ifdef Q_OS_WIN
    // 用户级模块
    moduleDirs << QDir::homePath() + "/Documents/maya/" + mayaVersion + "/modules";
    moduleDirs << QDir::homePath() + "/Documents/maya/modules";
    
    // 系统级模块（Arnold 通常在这里）
    moduleDirs << "C:/ProgramData/Autodesk/ApplicationPlugins";
    moduleDirs << "C:/Program Files/Common Files/Autodesk Shared/Modules/maya/" + mayaVersion;
    moduleDirs << "C:/Program Files/Common Files/Autodesk Shared/Modules/maya";
    
    // 在插件目录附近查找模块文件
    QDir parentDir = pluginDir;
    parentDir.cdUp(); // 上一级目录
    moduleDirs << parentDir.absolutePath();
    parentDir.cdUp(); // 再上一级
    moduleDirs << parentDir.absolutePath() + "/modules";
    moduleDirs << parentDir.absolutePath() + "/Contents";
    moduleDirs << parentDir.absolutePath() + "/Contents/modules";
#elif defined(Q_OS_MAC)
    moduleDirs << QDir::homePath() + "/Library/Preferences/Autodesk/maya/" + mayaVersion + "/modules";
    moduleDirs << "/Applications/Autodesk/maya" + mayaVersion + "/modules";
#elif defined(Q_OS_LINUX)
    moduleDirs << QDir::homePath() + "/maya/" + mayaVersion + "/modules";
    moduleDirs << "/usr/autodesk/modules/maya";
#endif

    for (const QString &moduleDir : moduleDirs) {
        QDir dir(moduleDir);
        if (!dir.exists()) continue;
        
        // 查找 Arnold 相关的 .mod 文件
        QStringList modFilters;
        modFilters << "*mtoa*.mod" << "*arnold*.mod" << "*.mod";
        
        // 递归搜索（最多2层深度）
        QDirIterator it(moduleDir, modFilters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString modFilePath = it.next();
            
            // 只处理 Arnold 相关的模块文件
            if (!modFilePath.contains("mtoa", Qt::CaseInsensitive) && 
                !modFilePath.contains("arnold", Qt::CaseInsensitive)) {
                continue;
            }
            
            qDebug() << "  检查模块文件:" << modFilePath;
            
            QFile modFile(modFilePath);
            if (modFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&modFile);
                QString content = in.readAll();
                modFile.close();
                
                // 解析模块文件格式，支持多种格式
                // 格式1: + MAYAVERSION:2024 mtoa 5.3.0.1 C:/Program Files/Autodesk/Arnold/maya2024
                // 格式2: + mtoa 5.2.1 ../
                // 格式3: + PLATFORM:win64 MAYAVERSION:2024 mtoa 5.3.0.1 path
                // 格式4: [r] mtoa 5.3.0.1
                
                QStringList modPatterns;
                // 最常见的格式：+ [可选参数] mtoa 版本号 [路径]
                modPatterns << "\\+[^\\n]*?mtoa\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)";  // 4位版本
                modPatterns << "\\+[^\\n]*?mtoa\\s+(\\d+\\.\\d+\\.\\d+)";        // 3位版本
                // [r] 格式
                modPatterns << "\\[r\\]\\s*mtoa\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)";
                modPatterns << "\\[r\\]\\s*mtoa\\s+(\\d+\\.\\d+\\.\\d+)";
                // VERSION= 格式
                modPatterns << "VERSION\\s*[=:]\\s*(\\d+\\.\\d+\\.\\d+\\.\\d+)";
                modPatterns << "VERSION\\s*[=:]\\s*(\\d+\\.\\d+\\.\\d+)";
                // mtoa version 格式
                modPatterns << "mtoa.*?version[:\\s]+(\\d+\\.\\d+\\.\\d+\\.\\d+)";
                modPatterns << "mtoa.*?version[:\\s]+(\\d+\\.\\d+\\.\\d+)";
                // Arnold for Maya 详细格式支持
                modPatterns << "MtoA\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)";
                modPatterns << "MtoA\\s+(\\d+\\.\\d+\\.\\d+)";
                modPatterns << "Arnold\\s+Core\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)";
                modPatterns << "Arnold\\s+Core\\s+(\\d+\\.\\d+\\.\\d+)";
                
                // 特殊处理：从路径中提取版本信息
                // 格式: + mtoa any C:\Program Files\Autodesk\Arnold\maya2022
                // 如果模块文件中没有直接版本号，尝试从路径中提取
                QRegularExpression pathPattern("\\+\\s+mtoa\\s+\\w+\\s+(.+)");
                QRegularExpressionMatch pathMatch = pathPattern.match(content);
                if (pathMatch.hasMatch()) {
                    QString arnoldPath = pathMatch.captured(1).trimmed();
                    qDebug() << "  从模块文件路径提取到:" << arnoldPath;
                    
                    // 从路径中提取版本号
                    QStringList pathVersionPatterns;
                    pathVersionPatterns << "maya(\\d{4})";  // maya2022
                    pathVersionPatterns << "arnold[/_-](\\d+\\.\\d+\\.\\d+\\.\\d+)";  // arnold-5.0.0.2
                    pathVersionPatterns << "arnold[/_-](\\d+\\.\\d+\\.\\d+)";  // arnold-5.0.0
                    pathVersionPatterns << "mtoa[/_-](\\d+\\.\\d+\\.\\d+\\.\\d+)";  // mtoa-5.0.0.2
                    pathVersionPatterns << "mtoa[/_-](\\d+\\.\\d+\\.\\d+)";  // mtoa-5.0.0
                    
                    for (const QString &pathPatternStr : pathVersionPatterns) {
                        QRegularExpression pathVersionPattern(pathPatternStr, QRegularExpression::CaseInsensitiveOption);
                        QRegularExpressionMatch pathVersionMatch = pathVersionPattern.match(arnoldPath);
                        if (pathVersionMatch.hasMatch()) {
                            QString version = pathVersionMatch.captured(1);
                            qDebug() << "  ✓ 从模块文件路径提取到版本:" << version;
                            return version;
                        }
                    }
                    
                    // 如果路径中包含Arnold目录，尝试在该目录下查找版本文件
                    if (arnoldPath.contains("Arnold", Qt::CaseInsensitive)) {
                        qDebug() << "  尝试在Arnold目录中查找版本文件:" << arnoldPath;
                        QStringList arnoldVersionFiles = searchArnoldVersionFiles(arnoldPath + "/plug-ins/mtoa.mll");
                        for (const QString &versionFile : arnoldVersionFiles) {
                            QFile file(versionFile);
                            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                                QTextStream in(&file);
                                QString fileContent = in.readAll();
                                file.close();
                                
                                QString parsedVersion = parseArnoldVersionText(fileContent);
                                if (parsedVersion != "Unknown") {
                                    qDebug() << "  ✓ 从Arnold目录版本文件提取到版本:" << parsedVersion;
                                    return parsedVersion;
                                }
                            }
                        }
                    }
                }
                
                for (const QString &patternStr : modPatterns) {
                    QRegularExpression modPattern(patternStr, QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch modMatch = modPattern.match(content);
                    if (modMatch.hasMatch()) {
                        QString version = modMatch.captured(1);
                        qDebug() << "  ✓ 从模块文件提取到版本:" << version << "（模式:" << patternStr.left(20) << "...\")";
                        return version;
                    }
                }
            }
        }
    }

    // 方法4: 从插件文件属性中读取版本信息（Windows特定）
#ifdef Q_OS_WIN
    // 尝试读取文件版本信息（需要Windows API）
    // 这里简化处理，可以使用QFileInfo获取部分信息
    QFileInfo fileInfo(pluginPath);
    if (fileInfo.exists()) {
        // 文件大小可以帮助判断大致版本（仅供参考）
        qint64 fileSize = fileInfo.size();
        qDebug() << "  插件文件大小:" << fileSize << "bytes";
        
        // 检查文件修改时间，可能暗示版本年份
        QDateTime lastModified = fileInfo.lastModified();
        qDebug() << "  插件最后修改时间:" << lastModified.toString("yyyy-MM-dd");
    }
#endif

    // 方法5: 根据Maya版本推断Arnold版本（作为最后的备选方案）
    // 基于Autodesk官方的Arnold for Maya版本对应关系
    QMap<QString, QString> mayaArnoldVersionMap;
    mayaArnoldVersionMap["2025"] = "5.4.x";    // Arnold 5.4 for Maya 2025
    mayaArnoldVersionMap["2024"] = "5.3.x";    // Arnold 5.3 for Maya 2024
    mayaArnoldVersionMap["2023"] = "5.2.x";    // Arnold 5.2 for Maya 2023
    mayaArnoldVersionMap["2022"] = "4.2.x";    // Arnold 4.2 for Maya 2022
    mayaArnoldVersionMap["2020"] = "4.0.x";    // Arnold 4.0 for Maya 2020
    mayaArnoldVersionMap["2019"] = "3.3.x";    // Arnold 3.3 for Maya 2019
    mayaArnoldVersionMap["2018"] = "3.1.x";    // Arnold 3.1 for Maya 2018
    mayaArnoldVersionMap["2017"] = "2.0.x";    // Arnold 2.0 for Maya 2017
    mayaArnoldVersionMap["2016"] = "1.4.x";    // Arnold 1.4 for Maya 2016
    
    if (mayaArnoldVersionMap.contains(mayaVersion)) {
        QString estimatedVersion = mayaArnoldVersionMap[mayaVersion];
        qDebug() << "  ⚠ 根据 Maya" << mayaVersion << "推断 Arnold 版本约为:" << estimatedVersion;
        qDebug() << "  提示：这是基于Maya版本的估算值，实际版本可能不同";
        qDebug() << "========== Arnold 版本提取完成（推断） ==========";
        return estimatedVersion;
    }

    qDebug() << "  ✗ 无法提取 Arnold 版本信息";
    qDebug() << "========== Arnold 版本提取完成（失败） ==========";
    return "Unknown";
}


QStringList MayaDetector::searchArnoldVersionFiles(const QString &pluginPath)
{
    QStringList foundFiles;
    QFileInfo pluginFileInfo(pluginPath);
    QDir pluginDir = pluginFileInfo.absoluteDir();
    
    qDebug() << "========== 搜索 Arnold 版本信息文件 ==========";
    qDebug() << "插件路径:" << pluginPath;
    qDebug() << "插件目录:" << pluginDir.absolutePath();
    
    // 构建搜索目录列表（按优先级排序）
    QStringList searchDirs;
    
    // 1. 插件所在目录
    searchDirs << pluginDir.absolutePath();
    
    // 2. 插件父目录（Arnold安装根目录）
    QDir parentDir = pluginDir;
    if (parentDir.cdUp()) {
        searchDirs << parentDir.absolutePath();
        qDebug() << "父目录:" << parentDir.absolutePath();
        
        // 3. 再上一级目录
        if (parentDir.cdUp()) {
            searchDirs << parentDir.absolutePath();
            qDebug() << "祖父目录:" << parentDir.absolutePath();
        }
    }
    
    // 只搜索关键的版本头文件
    QStringList versionFiles;
    versionFiles << "include/mtoa/utils/Version.h"  // MtoA版本头文件
                 << "include/arnold/ai_version.h";  // Arnold核心版本头文件
    
    // 搜索每个目录中的版本文件
    for (const QString &searchDir : searchDirs) {
        QDir dir(searchDir);
        if (!dir.exists()) {
            qDebug() << "跳过不存在的目录:" << searchDir;
            continue;
        }
        
        qDebug() << "搜索目录:" << searchDir;
        
        for (const QString &versionFileName : versionFiles) {
            QString versionFilePath = dir.absoluteFilePath(versionFileName);
            if (QFile::exists(versionFilePath)) {
                qDebug() << "  ✓ 找到版本文件:" << versionFilePath;
                foundFiles.append(versionFilePath);
            }
        }
    }
    
    // 去重并保持优先级
    foundFiles.removeDuplicates();
    
    qDebug() << "共找到" << foundFiles.size() << "个版本信息文件";
    for (const QString &file : foundFiles) {
        qDebug() << "  -" << file;
    }
    qDebug() << "========== 版本文件搜索完成 ==========";
    
    return foundFiles;
}

QString MayaDetector::extractVersionFromMtoaMod(const QString &modFilePath)
{
    qDebug() << "========== 从 mtoa.mod 文件提取版本信息 ==========";
    qDebug() << "模块文件路径:" << modFilePath;
    
    QFile modFile(modFilePath);
    if (!modFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "  ✗ 无法打开模块文件:" << modFilePath;
        return "Unknown";
    }
    
    QTextStream in(&modFile);
    QString content = in.readAll();
    modFile.close();
    
    qDebug() << "模块文件内容:";
    qDebug() << content;
    
    // 解析模块文件格式: + mtoa any C:\Program Files\Autodesk\Arnold\maya2022
    QRegularExpression pathPattern("\\+\\s+mtoa\\s+\\w+\\s+(.+)");
    QRegularExpressionMatch pathMatch = pathPattern.match(content);
    
    if (pathMatch.hasMatch()) {
        QString arnoldPath = pathMatch.captured(1).trimmed();
        qDebug() << "  ✓ 从模块文件提取到Arnold路径:" << arnoldPath;
        
        // 从路径中提取Maya版本号
        QRegularExpression mayaVersionPattern("maya(\\d{4})", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch mayaVersionMatch = mayaVersionPattern.match(arnoldPath);
        if (mayaVersionMatch.hasMatch()) {
            QString mayaVersion = mayaVersionMatch.captured(1);
            qDebug() << "  ✓ 从路径提取到Maya版本:" << mayaVersion;
        }
        
        // 在Arnold目录中搜索版本信息文件
        qDebug() << "  在Arnold目录中搜索版本信息文件:" << arnoldPath;
        
        // 直接检查两个关键的头文件
        QString mtoaVersionFile = arnoldPath + "/include/mtoa/utils/Version.h";
        QString arnoldVersionFile = arnoldPath + "/include/arnold/ai_version.h";
        
        // 优先检查 MtoA 版本头文件
        if (QFile::exists(mtoaVersionFile)) {
            qDebug() << "    ✓ 找到 MtoA 版本头文件:" << mtoaVersionFile;
            QString version = extractVersionFromMtoaHeader(mtoaVersionFile);
            if (version != "Unknown") {
                qDebug() << "========== mtoa.mod 版本提取完成 ==========";
                return version;
            }
        }
        
        // 如果 MtoA 版本文件不存在，检查 Arnold 核心版本头文件
        if (QFile::exists(arnoldVersionFile)) {
            qDebug() << "    ✓ 找到 Arnold 核心版本头文件:" << arnoldVersionFile;
            QString version = extractVersionFromArnoldHeader(arnoldVersionFile);
            if (version != "Unknown") {
                qDebug() << "========== mtoa.mod 版本提取完成 ==========";
                return version;
            }
        }
        
        
        qDebug() << "  ✗ 在Arnold目录中未找到版本信息";
    } else {
        qDebug() << "  ✗ 无法从模块文件中提取Arnold路径";
    }
    
    qDebug() << "========== mtoa.mod 版本提取完成（失败） ==========";
    return "Unknown";
}

QString MayaDetector::extractVersionFromMtoaHeader(const QString &headerFilePath)
{
    qDebug() << "========== 从 MtoA 版本头文件提取版本信息 ==========";
    qDebug() << "头文件路径:" << headerFilePath;
    
    QFile headerFile(headerFilePath);
    if (!headerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "  ✗ 无法打开头文件:" << headerFilePath;
        return "Unknown";
    }
    
    QTextStream in(&headerFile);
    QString content = in.readAll();
    headerFile.close();
    
    qDebug() << "头文件内容长度:" << content.size() << "字符";
    
    // 解析 MtoA 版本头文件格式
    // #define MTOA_ARCH_VERSION_NUM 5
    // #define MTOA_MAJOR_VERSION_NUM 0
    // #define MTOA_MINOR_VERSION_NUM 0
    // #define MTOA_FIX_VERSION "2"
    
    QString archVersion, majorVersion, minorVersion, fixVersion;
    
    // 提取各个版本号
    QRegularExpression archPattern("#define\\s+MTOA_ARCH_VERSION_NUM\\s+(\\d+)");
    QRegularExpressionMatch archMatch = archPattern.match(content);
    if (archMatch.hasMatch()) {
        archVersion = archMatch.captured(1);
        qDebug() << "  ✓ 提取到架构版本:" << archVersion;
    }
    
    QRegularExpression majorPattern("#define\\s+MTOA_MAJOR_VERSION_NUM\\s+(\\d+)");
    QRegularExpressionMatch majorMatch = majorPattern.match(content);
    if (majorMatch.hasMatch()) {
        majorVersion = majorMatch.captured(1);
        qDebug() << "  ✓ 提取到主版本:" << majorVersion;
    }
    
    QRegularExpression minorPattern("#define\\s+MTOA_MINOR_VERSION_NUM\\s+(\\d+)");
    QRegularExpressionMatch minorMatch = minorPattern.match(content);
    if (minorMatch.hasMatch()) {
        minorVersion = minorMatch.captured(1);
        qDebug() << "  ✓ 提取到次版本:" << minorVersion;
    }
    
    QRegularExpression fixPattern("#define\\s+MTOA_FIX_VERSION\\s+\"([^\"]+)\"");
    QRegularExpressionMatch fixMatch = fixPattern.match(content);
    if (fixMatch.hasMatch()) {
        fixVersion = fixMatch.captured(1);
        qDebug() << "  ✓ 提取到修复版本:" << fixVersion;
    }
    
    // 组合版本号
    if (!archVersion.isEmpty() && !majorVersion.isEmpty() && !minorVersion.isEmpty()) {
        QString version = archVersion + "." + majorVersion + "." + minorVersion;
        if (!fixVersion.isEmpty()) {
            version += "." + fixVersion;
        }
        qDebug() << "  ✓ 组合版本号:" << version;
        qDebug() << "========== MtoA 版本头文件解析完成 ==========";
        return version;
    }
    
    qDebug() << "  ✗ 无法从头文件中提取完整版本信息";
    qDebug() << "========== MtoA 版本头文件解析完成（失败） ==========";
    return "Unknown";
}

QString MayaDetector::extractVersionFromArnoldHeader(const QString &headerFilePath)
{
    qDebug() << "========== 从 Arnold 核心版本头文件提取版本信息 ==========";
    qDebug() << "头文件路径:" << headerFilePath;
    
    QFile headerFile(headerFilePath);
    if (!headerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "  ✗ 无法打开头文件:" << headerFilePath;
        return "Unknown";
    }
    
    QTextStream in(&headerFile);
    QString content = in.readAll();
    headerFile.close();
    
    qDebug() << "头文件内容长度:" << content.size() << "字符";
    
    // 解析 Arnold 核心版本头文件格式
    // #define AI_VERSION_ARCH_NUM    7
    // #define AI_VERSION_MAJOR_NUM   0
    // #define AI_VERSION_MINOR_NUM   0
    // #define AI_VERSION_FIX         "1"
    
    QString archVersion, majorVersion, minorVersion, fixVersion;
    
    // 提取各个版本号
    QRegularExpression archPattern("#define\\s+AI_VERSION_ARCH_NUM\\s+(\\d+)");
    QRegularExpressionMatch archMatch = archPattern.match(content);
    if (archMatch.hasMatch()) {
        archVersion = archMatch.captured(1);
        qDebug() << "  ✓ 提取到架构版本:" << archVersion;
    }
    
    QRegularExpression majorPattern("#define\\s+AI_VERSION_MAJOR_NUM\\s+(\\d+)");
    QRegularExpressionMatch majorMatch = majorPattern.match(content);
    if (majorMatch.hasMatch()) {
        majorVersion = majorMatch.captured(1);
        qDebug() << "  ✓ 提取到主版本:" << majorVersion;
    }
    
    QRegularExpression minorPattern("#define\\s+AI_VERSION_MINOR_NUM\\s+(\\d+)");
    QRegularExpressionMatch minorMatch = minorPattern.match(content);
    if (minorMatch.hasMatch()) {
        minorVersion = minorMatch.captured(1);
        qDebug() << "  ✓ 提取到次版本:" << minorVersion;
    }
    
    QRegularExpression fixPattern("#define\\s+AI_VERSION_FIX\\s+\"([^\"]+)\"");
    QRegularExpressionMatch fixMatch = fixPattern.match(content);
    if (fixMatch.hasMatch()) {
        fixVersion = fixMatch.captured(1);
        qDebug() << "  ✓ 提取到修复版本:" << fixVersion;
    }
    
    // 组合版本号
    if (!archVersion.isEmpty() && !majorVersion.isEmpty() && !minorVersion.isEmpty()) {
        QString version = archVersion + "." + majorVersion + "." + minorVersion;
        if (!fixVersion.isEmpty()) {
            version += "." + fixVersion;
        }
        qDebug() << "  ✓ 组合版本号:" << version;
        qDebug() << "========== Arnold 核心版本头文件解析完成 ==========";
        return version;
    }
    
    qDebug() << "  ✗ 无法从头文件中提取完整版本信息";
    qDebug() << "========== Arnold 核心版本头文件解析完成（失败） ==========";
    return "Unknown";
}

QList<RendererInfo> MayaDetector::readPluginsFromPrefs(const QString &mayaVersion)
{
    QList<RendererInfo> plugins;
    
    qDebug() << "========== 从 Maya 插件配置文件读取插件信息 ==========";
    qDebug() << "Maya 版本:" << mayaVersion;
    
    // 构建插件配置文件路径
    QString prefsPath = QDir::homePath() + "/Documents/maya/" + mayaVersion + "/prefs/pluginPrefs.mel";
    qDebug() << "插件配置文件路径:" << prefsPath;
    
    QFile prefsFile(prefsPath);
    if (!prefsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "  ✗ 无法打开插件配置文件:" << prefsPath;
        return plugins;
    }
    
    QTextStream in(&prefsFile);
    QString content = in.readAll();
    prefsFile.close();
    
    qDebug() << "配置文件内容长度:" << content.size() << "字符";
    
    // 解析 autoLoadPlugin 调用
    // 格式: evalDeferred("autoLoadPlugin(\"\", \"mtoa\", \"mtoa\")");
    QRegularExpression pluginPattern("autoLoadPlugin\\(\\s*\"([^\"]*)\"\\s*,\\s*\"([^\"]+)\"\\s*,\\s*\"([^\"]+)\"\\s*\\)");
    QRegularExpressionMatchIterator matches = pluginPattern.globalMatch(content);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString pluginPath = match.captured(1);
        QString pluginName = match.captured(2);
        QString pluginDisplayName = match.captured(3);
        
        qDebug() << "  找到插件:" << pluginName << "显示名称:" << pluginDisplayName << "路径:" << pluginPath;
        
        // 只处理渲染器插件
        if (pluginName.contains("mtoa", Qt::CaseInsensitive) || 
            pluginName.contains("vray", Qt::CaseInsensitive) ||
            pluginName.contains("redshift", Qt::CaseInsensitive) ||
            pluginName.contains("arnold", Qt::CaseInsensitive)) {
            
            RendererInfo renderer;
            renderer.name = pluginDisplayName;
            renderer.pluginPath = pluginPath;
            renderer.version = "Unknown";
            renderer.isLoaded = true; // 在 pluginPrefs.mel 中的插件通常是已加载的
            
            // 如果路径为空，尝试从 Maya 插件目录查找
            if (pluginPath.isEmpty()) {
                QString mayaPluginsDir = "C:/Program Files/Autodesk/Maya" + mayaVersion + "/plug-ins";
                QString possiblePath = mayaPluginsDir + "/" + pluginName + ".mll";
                if (QFile::exists(possiblePath)) {
                    renderer.pluginPath = possiblePath;
                    qDebug() << "    找到插件文件:" << possiblePath;
                }
            }
            
            plugins.append(renderer);
            qDebug() << "    添加渲染器插件:" << renderer.name;
        }
    }
    
    qDebug() << "共找到" << plugins.size() << "个渲染器插件";
    qDebug() << "========== 插件配置文件读取完成 ==========";
    
    return plugins;
}

QStringList MayaDetector::getPluginPathsFromEnvironment()
{
    QStringList paths;
    
    qDebug() << "========== 从环境变量获取插件路径 ==========";
    
    // 获取主要环境变量
    QStringList envVars = {
        "MAYA_PLUG_IN_PATH",
        "MAYA_MODULE_PATH", 
        "MAYA_SCRIPT_PATH",
        "MAYA_PRESET_PATH",
        "MAYA_SHELF_PATH"
    };
    
    for (const QString &envVar : envVars) {
        QString envValue = qgetenv(envVar.toLocal8Bit());
        if (!envValue.isEmpty()) {
            qDebug() << "环境变量" << envVar << ":" << envValue;
            
            // 分割路径（Windows用分号，Unix用冒号）
            QStringList envPaths = envValue.split(
#ifdef Q_OS_WIN
                ";"
#else
                ":"
#endif
            );
            
            for (const QString &path : envPaths) {
                QString cleanPath = path.trimmed();
                if (!cleanPath.isEmpty() && QDir(cleanPath).exists()) {
                    paths.append(cleanPath);
                    qDebug() << "  添加路径:" << cleanPath;
                }
            }
        }
    }
    
    // 获取系统环境变量
    QStringList systemPaths = {
        "PATH",
        "MAYA_LOCATION"
    };
    
    for (const QString &sysVar : systemPaths) {
        QString sysValue = qgetenv(sysVar.toLocal8Bit());
        if (!sysValue.isEmpty() && sysVar == "MAYA_LOCATION") {
            QString mayaPlugins = sysValue + "/plug-ins";
            if (QDir(mayaPlugins).exists()) {
                paths.append(mayaPlugins);
                qDebug() << "从MAYA_LOCATION添加:" << mayaPlugins;
            }
        }
    }
    
    paths.removeDuplicates();
    qDebug() << "共找到" << paths.size() << "个环境变量路径";
    qDebug() << "========== 环境变量路径获取完成 ==========";
    
    return paths;
}

QList<RendererInfo> MayaDetector::scanAllPluginDirectories(const QString &mayaVersion)
{
    QList<RendererInfo> plugins;
    
    qDebug() << "========== 扫描所有插件目录 ==========";
    qDebug() << "Maya 版本:" << mayaVersion;
    
    // 构建所有可能的插件目录
    QStringList allPluginDirs;
    
    // 1. Maya 标准目录
    allPluginDirs << "C:/Program Files/Autodesk/Maya" + mayaVersion + "/plug-ins";
    allPluginDirs << "C:/Program Files/Autodesk/Maya" + mayaVersion + "/bin/plug-ins";
    allPluginDirs << "C:/Program Files/Autodesk/Maya" + mayaVersion + "/modules";
    
    // 2. 用户目录
    allPluginDirs << QDir::homePath() + "/Documents/maya/" + mayaVersion + "/plug-ins";
    allPluginDirs << QDir::homePath() + "/Documents/maya/" + mayaVersion + "/modules";
    allPluginDirs << QDir::homePath() + "/Documents/maya/modules";
    
    // 3. 第三方插件常见位置
    allPluginDirs << "C:/Program Files/Chaos Group/V-Ray/Maya " + mayaVersion + "/bin";
    allPluginDirs << "C:/Program Files/Redshift/Plugins/Maya/" + mayaVersion + "/plug-ins";
    allPluginDirs << "C:/Program Files/Autodesk/Arnold/maya" + mayaVersion + "/plug-ins";
    allPluginDirs << "C:/Program Files/solidangle/mtoadeploy/" + mayaVersion + "/plug-ins";
    allPluginDirs << "C:/Program Files/Thinkbox/Deadline/10/plugins/Maya/" + mayaVersion;
    
    // 4. 从环境变量获取的路径
    QStringList envPaths = getPluginPathsFromEnvironment();
    allPluginDirs.append(envPaths);
    
    // 5. 网络共享路径（如果存在）
    QStringList networkPaths = {
        "//server/plugins/maya" + mayaVersion,
        "//shared/autodesk/maya/plugins",
        "//network/maya/plug-ins"
    };
    allPluginDirs.append(networkPaths);
    
    // 去重并过滤存在的目录
    allPluginDirs.removeDuplicates();
    QStringList existingDirs;
    for (const QString &dir : allPluginDirs) {
        if (QDir(dir).exists()) {
            existingDirs.append(dir);
            qDebug() << "  ✓ 目录存在:" << dir;
        } else {
            qDebug() << "  ✗ 目录不存在:" << dir;
        }
    }
    
    // 扫描每个目录中的插件文件
    QStringList pluginExtensions;
#ifdef Q_OS_WIN
    pluginExtensions << "*.mll" << "*.dll" << "*.py";
#elif defined(Q_OS_MAC)
    pluginExtensions << "*.bundle" << "*.py";
#elif defined(Q_OS_LINUX)
    pluginExtensions << "*.so" << "*.py";
#endif
    
    for (const QString &dir : existingDirs) {
        QDir pluginDir(dir);
        qDebug() << "扫描目录:" << dir;
        
        QFileInfoList files = pluginDir.entryInfoList(pluginExtensions, QDir::Files);
        for (const QFileInfo &file : files) {
            QString pluginName = file.baseName();
            QString pluginPath = file.absoluteFilePath();
            
            // 只处理渲染器相关插件
            if (pluginName.contains("mtoa", Qt::CaseInsensitive) ||
                pluginName.contains("vray", Qt::CaseInsensitive) ||
                pluginName.contains("redshift", Qt::CaseInsensitive) ||
                pluginName.contains("arnold", Qt::CaseInsensitive) ||
                pluginName.contains("yeti", Qt::CaseInsensitive) ||
                pluginName.contains("miarmy", Qt::CaseInsensitive)) {
                
                RendererInfo renderer;
                renderer.name = pluginName;
                renderer.pluginPath = pluginPath;
                renderer.version = "Unknown";
                renderer.isLoaded = false; // 需要进一步检测
                
                // 提取版本信息
                if (pluginName.contains("mtoa", Qt::CaseInsensitive) || 
                    pluginName.contains("arnold", Qt::CaseInsensitive)) {
                    renderer.version = extractArnoldVersion(pluginPath, mayaVersion);
                }
                
                plugins.append(renderer);
                qDebug() << "    找到插件:" << pluginName << "版本:" << renderer.version;
            }
        }
    }
    
    qDebug() << "共扫描" << existingDirs.size() << "个目录，找到" << plugins.size() << "个渲染器插件";
    qDebug() << "========== 插件目录扫描完成 ==========";
    
    return plugins;
}

QList<RendererInfo> MayaDetector::getAllMayaPlugins(const QString &mayaVersion)
{
    QList<RendererInfo> allPlugins;
    
    qDebug() << "========== 获取 Maya 所有插件信息 ==========";
    qDebug() << "Maya 版本:" << mayaVersion;
    
    // 方法1: 通过 Maya 命令获取插件信息（最准确的方法）
    QList<RendererInfo> mayaCommandPlugins = getPluginsFromMayaCommands(mayaVersion);
    for (const RendererInfo &plugin : mayaCommandPlugins) {
        allPlugins.append(plugin);
    }
    
    // 方法2: 从配置文件读取已加载插件（备用方法）
    QList<RendererInfo> prefsPlugins = readPluginsFromPrefs(mayaVersion);
    for (const RendererInfo &plugin : prefsPlugins) {
        // 检查是否已经存在
        bool alreadyExists = false;
        for (const RendererInfo &existingPlugin : allPlugins) {
            if (existingPlugin.name == plugin.name) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            allPlugins.append(plugin);
        }
    }
    
    // 方法3: 扫描所有插件目录（最后的备用方法）
    QList<RendererInfo> scannedPlugins = scanAllPluginDirectories(mayaVersion);
    for (const RendererInfo &scannedPlugin : scannedPlugins) {
        // 检查是否已经存在
        bool alreadyExists = false;
        for (const RendererInfo &existingPlugin : allPlugins) {
            if (existingPlugin.name == scannedPlugin.name && 
                existingPlugin.pluginPath == scannedPlugin.pluginPath) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            allPlugins.append(scannedPlugin);
        }
    }
    
    qDebug() << "总计找到" << allPlugins.size() << "个插件";
    qDebug() << "  - Maya 命令检测:" << mayaCommandPlugins.size() << "个";
    qDebug() << "  - 配置文件检测:" << prefsPlugins.size() << "个";
    qDebug() << "  - 目录扫描检测:" << scannedPlugins.size() << "个";
    qDebug() << "========== 所有插件信息获取完成 ==========";
    
    return allPlugins;
}

QString MayaDetector::executeMayaMelCommand(const QString &mayaExecutablePath, const QString &melCommand)
{
    qDebug() << "========== 执行 Maya MEL 命令 ==========";
    qDebug() << "Maya 路径:" << mayaExecutablePath;
    qDebug() << "MEL 命令:" << melCommand;
    
    if (!QFile::exists(mayaExecutablePath)) {
        qDebug() << "Maya 可执行文件不存在:" << mayaExecutablePath;
        return QString();
    }
    
    QProcess mayaProcess;
    
    // 构建完整的命令
    QStringList arguments;
    arguments << "-batch";           // 批处理模式
    arguments << "-noAutoloadPlugins"; // 不自动加载插件（加快启动）
    arguments << "-command";         // 执行命令
    arguments << melCommand;         // MEL 命令
    
    qDebug() << "执行命令:" << mayaExecutablePath << arguments.join(" ");
    
    mayaProcess.start(mayaExecutablePath, arguments);
    
    if (!mayaProcess.waitForStarted(30000)) { // 30秒超时
        qDebug() << "Maya 进程启动失败";
        return QString();
    }
    
    if (!mayaProcess.waitForFinished(60000)) { // 60秒超时
        qDebug() << "Maya 进程执行超时";
        mayaProcess.kill();
        return QString();
    }
    
    QString output = mayaProcess.readAllStandardOutput();
    QString error = mayaProcess.readAllStandardError();
    
    qDebug() << "标准输出:" << output;
    if (!error.isEmpty()) {
        qDebug() << "错误输出:" << error;
    }
    
    qDebug() << "========== Maya MEL 命令执行完成 ==========";
    
    return output;
}

QList<RendererInfo> MayaDetector::getPluginsFromMayaCommands(const QString &mayaVersion)
{
    QList<RendererInfo> plugins;
    
    qDebug() << "========== 通过 Maya 命令获取插件信息 ==========";
    qDebug() << "Maya 版本:" << mayaVersion;
    
    // 构建 Maya 可执行文件路径
    QString mayaExecutablePath = "C:/Program Files/Autodesk/Maya" + mayaVersion + "/bin/maya.exe";
    
    if (!QFile::exists(mayaExecutablePath)) {
        qDebug() << "Maya 可执行文件不存在:" << mayaExecutablePath;
        return plugins;
    }
    
    // 方法1: 获取所有已加载的插件
    QString loadedPluginsCommand = R"(
        string $loadedPlugins[] = `pluginInfo -query -list`;
        for ($plugin in $loadedPlugins) {
            print($plugin + "\n");
        }
    )";
    
    QString loadedOutput = executeMayaMelCommand(mayaExecutablePath, loadedPluginsCommand);
    QStringList loadedPluginNames = loadedOutput.split('\n', Qt::SkipEmptyParts);
    
    qDebug() << "已加载插件数量:" << loadedPluginNames.size();
    for (const QString &pluginName : loadedPluginNames) {
        qDebug() << "  已加载:" << pluginName;
        
        // 只处理渲染器相关插件
        if (pluginName.contains("mtoa", Qt::CaseInsensitive) ||
            pluginName.contains("vray", Qt::CaseInsensitive) ||
            pluginName.contains("redshift", Qt::CaseInsensitive) ||
            pluginName.contains("arnold", Qt::CaseInsensitive) ||
            pluginName.contains("yeti", Qt::CaseInsensitive) ||
            pluginName.contains("miarmy", Qt::CaseInsensitive)) {
            
            // 获取插件详细信息
            QString pluginInfoCommand = QString(R"(
                string $plugin = "%1";
                string $version = `pluginInfo $plugin -query -version`;
                string $path = `pluginInfo $plugin -query -path`;
                string $vendor = `pluginInfo $plugin -query -vendor`;
                print("PLUGIN_INFO:" + $plugin + "|" + $version + "|" + $path + "|" + $vendor + "\n");
            )").arg(pluginName);
            
            QString pluginInfoOutput = executeMayaMelCommand(mayaExecutablePath, pluginInfoCommand);
            
            // 解析插件信息
            QRegularExpression infoRegex(R"(PLUGIN_INFO:([^|]+)\|([^|]+)\|([^|]+)\|([^|]+))");
            QRegularExpressionMatch match = infoRegex.match(pluginInfoOutput);
            
            if (match.hasMatch()) {
                RendererInfo renderer;
                renderer.name = match.captured(1);
                renderer.version = match.captured(2);
                renderer.pluginPath = match.captured(3);
                renderer.isLoaded = true;
                
                plugins.append(renderer);
                qDebug() << "    插件信息:" << renderer.name << "版本:" << renderer.version << "路径:" << renderer.pluginPath;
            }
        }
    }
    
    // 方法2: 获取所有可用的插件（包括未加载的）
    QString allPluginsCommand = R"(
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
        
        // 也检查 Maya 安装目录
        string $mayaPath = `getenv "MAYA_LOCATION"`;
        if ($mayaPath != "") {
            string $mayaPlugins = $mayaPath + "/bin/plug-ins";
            if (`filetest -d $mayaPlugins`) {
                string $files[] = `getFileList -folder $mayaPlugins -filespec "*.mll"`;
                for ($file in $files) {
                    string $pluginName = `substitute ".mll" $file ""`;
                    $allPlugins[size($allPlugins)] = $pluginName;
                }
            }
        }
        
        // 输出所有插件
        for ($plugin in $allPlugins) {
            print("AVAILABLE_PLUGIN:" + $plugin + "\n");
        }
    )";
    
    QString allPluginsOutput = executeMayaMelCommand(mayaExecutablePath, allPluginsCommand);
    QStringList allPluginLines = allPluginsOutput.split('\n', Qt::SkipEmptyParts);
    
    qDebug() << "所有可用插件数量:" << allPluginLines.size();
    
    for (const QString &line : allPluginLines) {
        if (line.startsWith("AVAILABLE_PLUGIN:")) {
            QString pluginName = line.mid(17); // 移除 "AVAILABLE_PLUGIN:" 前缀
            
            // 只处理渲染器相关插件
            if (pluginName.contains("mtoa", Qt::CaseInsensitive) ||
                pluginName.contains("vray", Qt::CaseInsensitive) ||
                pluginName.contains("redshift", Qt::CaseInsensitive) ||
                pluginName.contains("arnold", Qt::CaseInsensitive) ||
                pluginName.contains("yeti", Qt::CaseInsensitive) ||
                pluginName.contains("miarmy", Qt::CaseInsensitive)) {
                
                // 检查是否已经在已加载列表中
                bool alreadyLoaded = false;
                for (const RendererInfo &existingPlugin : plugins) {
                    if (existingPlugin.name == pluginName) {
                        alreadyLoaded = true;
                        break;
                    }
                }
                
                if (!alreadyLoaded) {
                    // 获取插件路径信息
                    QString pluginPathCommand = QString(R"(
                        string $plugin = "%1";
                        string $path = `pluginInfo $plugin -query -path`;
                        print("PLUGIN_PATH:" + $plugin + "|" + $path + "\n");
                    )").arg(pluginName);
                    
                    QString pathOutput = executeMayaMelCommand(mayaExecutablePath, pluginPathCommand);
                    
                    QRegularExpression pathRegex(R"(PLUGIN_PATH:([^|]+)\|([^|]+))");
                    QRegularExpressionMatch pathMatch = pathRegex.match(pathOutput);
                    
                    if (pathMatch.hasMatch()) {
                        RendererInfo renderer;
                        renderer.name = pathMatch.captured(1);
                        renderer.pluginPath = pathMatch.captured(2);
                        renderer.version = "Unknown";
                        renderer.isLoaded = false;
                        
                        plugins.append(renderer);
                        qDebug() << "    可用插件:" << renderer.name << "路径:" << renderer.pluginPath;
                    }
                }
            }
        }
    }
    
    qDebug() << "通过 Maya 命令共找到" << plugins.size() << "个渲染器插件";
    qDebug() << "========== Maya 命令插件检测完成 ==========";
    
    return plugins;
}
