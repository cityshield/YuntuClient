#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief Maya 软件信息结构体
 */
struct MayaSoftwareInfo {
    QString name;              // 软件名称 (Maya)
    QString version;           // 版本号 (2024, 2023等)
    QString fullVersion;       // 完整版本号 (2024.1, 2023.3等)
    QString installPath;       // 安装路径
    QString executablePath;    // 可执行文件路径
    QStringList renderers;     // 渲染器列表 (Arnold, V-Ray, Redshift等)
    QStringList plugins;       // 插件列表
    bool isValid;              // 是否有效

    MayaSoftwareInfo()
        : isValid(false) {}
};

/**
 * @brief Maya 渲染器信息
 */
struct RendererInfo {
    QString name;              // 渲染器名称
    QString version;           // 版本号
    QString pluginPath;        // 插件路径
    bool isLoaded;             // 是否已加载
};

/**
 * @brief Maya 环境检测服务
 *
 * 功能：
 * 1. 自动扫描系统中安装的 Maya 版本
 * 2. 检测 Maya 使用的渲染器（Arnold、V-Ray、Redshift 等）
 * 3. 识别已安装的插件
 * 4. 验证场景文件依赖
 */
class MayaDetector : public QObject
{
    Q_OBJECT

public:
    explicit MayaDetector(QObject *parent = nullptr);
    ~MayaDetector();

    /**
     * @brief 扫描系统中安装的所有 Maya 版本
     * @return Maya 软件信息列表
     */
    QVector<MayaSoftwareInfo> detectAllMayaVersions();

    /**
     * @brief 检测指定路径的 Maya 安装信息
     * @param installPath Maya 安装目录
     * @return Maya 软件信息
     */
    MayaSoftwareInfo detectMayaAtPath(const QString &installPath);

    /**
     * @brief 扫描 Maya 的渲染器
     * @param mayaInfo Maya 软件信息
     * @return 渲染器列表
     */
    QVector<RendererInfo> detectRenderers(const MayaSoftwareInfo &mayaInfo);

    /**
     * @brief 扫描 Maya 插件
     * @param mayaInfo Maya 软件信息
     * @return 插件名称列表
     */
    QStringList detectPlugins(const MayaSoftwareInfo &mayaInfo);

    /**
     * @brief 从场景文件中提取 Maya 版本信息
     * @param sceneFilePath 场景文件路径 (.ma 或 .mb)
     * @return Maya 版本号
     */
    QString extractMayaVersionFromScene(const QString &sceneFilePath);

    /**
     * @brief 从场景文件中提取所需的渲染器
     * @param sceneFilePath 场景文件路径
     * @return 渲染器名称
     */
    QString extractRendererFromScene(const QString &sceneFilePath);

    /**
     * @brief 扫描场景文件的纹理和素材依赖
     * @param sceneFilePath 场景文件路径
     * @return 素材文件路径列表
     */
    QStringList scanSceneAssets(const QString &sceneFilePath);

    /**
     * @brief 检测缺失的素材文件
     * @param sceneFilePath 场景文件路径
     * @return 缺失的文件路径列表
     */
    QStringList detectMissingAssets(const QString &sceneFilePath);

signals:
    /**
     * @brief 检测进度信号
     * @param progress 进度 (0-100)
     * @param message 当前操作描述
     */
    void detectProgress(int progress, const QString &message);

    /**
     * @brief 检测完成信号
     */
    void detectFinished();

private:
    /**
     * @brief 从注册表读取 Maya 安装信息 (Windows)
     * @return Maya 安装路径列表
     */
    QStringList readMayaPathsFromRegistry();

    /**
     * @brief 扫描常用安装目录
     * @return Maya 安装路径列表
     */
    QStringList scanCommonInstallPaths();

    /**
     * @brief 从路径提取 Maya 版本号
     * @param path 路径字符串
     * @return 版本号 (如 "2024")
     */
    QString extractVersionFromPath(const QString &path);

    /**
     * @brief 获取 Maya 可执行文件路径
     * @param installPath 安装目录
     * @return 可执行文件完整路径
     */
    QString getMayaExecutablePath(const QString &installPath);

    /**
     * @brief 验证路径是否为有效的 Maya 安装目录
     * @param path 路径
     * @return 是否有效
     */
    bool isValidMayaInstall(const QString &path);

    /**
     * @brief 读取 Maya 场景文件的文本内容 (.ma文件)
     * @param sceneFilePath 场景文件路径
     * @return 文件内容
     */
    QString readMayaAsciiScene(const QString &sceneFilePath);

    /**
     * @brief 解析 Maya 二进制场景文件 (.mb文件)
     * @param sceneFilePath 场景文件路径
     * @return 解析的文本内容
     */
    QString parseMayaBinaryScene(const QString &sceneFilePath);

    /**
     * @brief 检测 Arnold 渲染器
     * @param mayaPath Maya 安装路径
     * @return 渲染器信息
     */
    RendererInfo detectArnold(const QString &mayaPath);

    /**
     * @brief 检测 V-Ray 渲染器
     * @param mayaPath Maya 安装路径
     * @return 渲染器信息
     */
    RendererInfo detectVRay(const QString &mayaPath);

    /**
     * @brief 检测 Redshift 渲染器
     * @param mayaPath Maya 安装路径
     * @return 渲染器信息
     */
    RendererInfo detectRedshift(const QString &mayaPath);

    /**
     * @brief 读取 Maya.env 文件获取插件路径
     * @param mayaVersion Maya 版本号
     * @return 插件路径列表
     */
    QStringList readMayaEnvPaths(const QString &mayaVersion);

    /**
     * @brief 读取 pluginPrefs.mel 获取已加载插件
     * @param mayaVersion Maya 版本号
     * @return 插件名称和路径的映射
     */
    QMap<QString, QString> readPluginPrefs(const QString &mayaVersion);

    /**
     * @brief 从模块路径文件读取插件路径
     * @param mayaVersion Maya 版本号
     * @return 模块路径列表
     */
    QStringList readModulePaths(const QString &mayaVersion);

    /**
     * @brief 扫描第三方插件的注册表安装信息
     * @param mayaVersion Maya 版本号
     * @return 插件路径列表
     */
    QStringList scanThirdPartyPluginRegistry(const QString &mayaVersion);

    /**
     * @brief 暴力搜索特定插件文件（全盘扫描）
     * @param pluginFileName 插件文件名 (如 "mtoa.mll", "pgYetiMaya.mll")
     * @param mayaVersion Maya 版本号（用于优先搜索）
     * @return 找到的插件完整路径列表
     */
    QStringList bruteForceSearchPlugin(const QString &pluginFileName, const QString &mayaVersion);
};
