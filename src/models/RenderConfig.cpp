/**
 * @file RenderConfig.cpp
 * @brief 渲染配置模型实现
 */

#include "RenderConfig.h"
#include <QtMath>

RenderConfig::RenderConfig(QObject *parent)
    : QObject(parent)
    , m_renderer(RendererType::Arnold)
    , m_quality(QualityPreset::Medium)
    , m_samples(4)
    , m_rayDepth(8)
    , m_diffuseSamples(2)
    , m_specularSamples(2)
    , m_transmissionSamples(2)
    , m_sssSamples(2)
    , m_volumeSamples(2)
    , m_imageFormat(ImageFormat::PNG)
    , m_colorSpace(ColorSpace::sRGB)
    , m_bitDepth(8)
    , m_useAlpha(true)
    , m_threadCount(0)  // 0 = auto
    , m_memoryLimit(4096)  // 4GB
    , m_bucketSize(64)
    , m_enableMotionBlur(false)
    , m_enableDepthOfField(false)
    , m_enableGlobalIllumination(true)
    , m_enableCaustics(false)
    , m_enableSubsurfaceScattering(true)
    , m_enableDisplacement(true)
    , m_motionBlurSamples(5)
    , m_shutterAngle(180.0)
    , m_focalLength(35.0)
    , m_fStop(5.6)
{
}

RenderConfig::~RenderConfig()
{
}

void RenderConfig::setConfigId(const QString &configId)
{
    if (m_configId != configId) {
        m_configId = configId;
        emit configIdChanged();
        emit configDataChanged();
    }
}

void RenderConfig::setConfigName(const QString &configName)
{
    if (m_configName != configName) {
        m_configName = configName;
        emit configNameChanged();
        emit configDataChanged();
    }
}

void RenderConfig::setRenderer(RendererType renderer)
{
    if (m_renderer != renderer) {
        m_renderer = renderer;
        emit rendererChanged();
        emit configDataChanged();
    }
}

void RenderConfig::setQuality(QualityPreset quality)
{
    if (m_quality != quality) {
        m_quality = quality;
        if (quality != QualityPreset::Custom) {
            applyQualityPreset(quality);
        }
        emit qualityChanged();
        emit configDataChanged();
    }
}

void RenderConfig::setSamples(int samples)
{
    if (m_samples != samples) {
        m_samples = samples;
        m_quality = QualityPreset::Custom;
        emit configDataChanged();
    }
}

void RenderConfig::setRayDepth(int depth)
{
    if (m_rayDepth != depth) {
        m_rayDepth = depth;
        m_quality = QualityPreset::Custom;
        emit configDataChanged();
    }
}

void RenderConfig::setDiffuseSamples(int samples)
{
    if (m_diffuseSamples != samples) {
        m_diffuseSamples = samples;
        m_quality = QualityPreset::Custom;
        emit configDataChanged();
    }
}

void RenderConfig::setSpecularSamples(int samples)
{
    if (m_specularSamples != samples) {
        m_specularSamples = samples;
        m_quality = QualityPreset::Custom;
        emit configDataChanged();
    }
}

void RenderConfig::setTransmissionSamples(int samples)
{
    if (m_transmissionSamples != samples) {
        m_transmissionSamples = samples;
        m_quality = QualityPreset::Custom;
        emit configDataChanged();
    }
}

void RenderConfig::setSssSamples(int samples)
{
    if (m_sssSamples != samples) {
        m_sssSamples = samples;
        m_quality = QualityPreset::Custom;
        emit configDataChanged();
    }
}

void RenderConfig::setVolumeSamples(int samples)
{
    if (m_volumeSamples != samples) {
        m_volumeSamples = samples;
        m_quality = QualityPreset::Custom;
        emit configDataChanged();
    }
}

void RenderConfig::setImageFormat(ImageFormat format)
{
    if (m_imageFormat != format) {
        m_imageFormat = format;
        emit configDataChanged();
    }
}

void RenderConfig::setColorSpace(ColorSpace colorSpace)
{
    if (m_colorSpace != colorSpace) {
        m_colorSpace = colorSpace;
        emit configDataChanged();
    }
}

void RenderConfig::setBitDepth(int bitDepth)
{
    if (m_bitDepth != bitDepth) {
        m_bitDepth = bitDepth;
        emit configDataChanged();
    }
}

void RenderConfig::setUseAlpha(bool use)
{
    if (m_useAlpha != use) {
        m_useAlpha = use;
        emit configDataChanged();
    }
}

void RenderConfig::setThreadCount(int count)
{
    if (m_threadCount != count) {
        m_threadCount = count;
        emit configDataChanged();
    }
}

void RenderConfig::setMemoryLimit(int limitMB)
{
    if (m_memoryLimit != limitMB) {
        m_memoryLimit = limitMB;
        emit configDataChanged();
    }
}

void RenderConfig::setBucketSize(int size)
{
    if (m_bucketSize != size) {
        m_bucketSize = size;
        emit configDataChanged();
    }
}

void RenderConfig::setEnableMotionBlur(bool enable)
{
    if (m_enableMotionBlur != enable) {
        m_enableMotionBlur = enable;
        emit configDataChanged();
    }
}

void RenderConfig::setEnableDepthOfField(bool enable)
{
    if (m_enableDepthOfField != enable) {
        m_enableDepthOfField = enable;
        emit configDataChanged();
    }
}

void RenderConfig::setEnableGlobalIllumination(bool enable)
{
    if (m_enableGlobalIllumination != enable) {
        m_enableGlobalIllumination = enable;
        emit configDataChanged();
    }
}

void RenderConfig::setEnableCaustics(bool enable)
{
    if (m_enableCaustics != enable) {
        m_enableCaustics = enable;
        emit configDataChanged();
    }
}

void RenderConfig::setEnableSubsurfaceScattering(bool enable)
{
    if (m_enableSubsurfaceScattering != enable) {
        m_enableSubsurfaceScattering = enable;
        emit configDataChanged();
    }
}

void RenderConfig::setEnableDisplacement(bool enable)
{
    if (m_enableDisplacement != enable) {
        m_enableDisplacement = enable;
        emit configDataChanged();
    }
}

void RenderConfig::setMotionBlurSamples(int samples)
{
    if (m_motionBlurSamples != samples) {
        m_motionBlurSamples = samples;
        emit configDataChanged();
    }
}

void RenderConfig::setShutterAngle(double angle)
{
    if (qAbs(m_shutterAngle - angle) > 0.01) {
        m_shutterAngle = angle;
        emit configDataChanged();
    }
}

void RenderConfig::setFocalLength(double length)
{
    if (qAbs(m_focalLength - length) > 0.01) {
        m_focalLength = length;
        emit configDataChanged();
    }
}

void RenderConfig::setFStop(double fStop)
{
    if (qAbs(m_fStop - fStop) > 0.01) {
        m_fStop = fStop;
        emit configDataChanged();
    }
}

void RenderConfig::setRendererSettings(const QVariantMap &settings)
{
    m_rendererSettings = settings;
    emit configDataChanged();
}

void RenderConfig::setRendererSetting(const QString &key, const QVariant &value)
{
    m_rendererSettings[key] = value;
    emit configDataChanged();
}

QVariant RenderConfig::getRendererSetting(const QString &key, const QVariant &defaultValue) const
{
    return m_rendererSettings.value(key, defaultValue);
}

QJsonObject RenderConfig::toJson() const
{
    QJsonObject json;
    json["configId"] = m_configId;
    json["configName"] = m_configName;
    json["renderer"] = static_cast<int>(m_renderer);
    json["quality"] = static_cast<int>(m_quality);

    // Quality settings
    json["samples"] = m_samples;
    json["rayDepth"] = m_rayDepth;
    json["diffuseSamples"] = m_diffuseSamples;
    json["specularSamples"] = m_specularSamples;
    json["transmissionSamples"] = m_transmissionSamples;
    json["sssSamples"] = m_sssSamples;
    json["volumeSamples"] = m_volumeSamples;

    // Output settings
    json["imageFormat"] = static_cast<int>(m_imageFormat);
    json["colorSpace"] = static_cast<int>(m_colorSpace);
    json["bitDepth"] = m_bitDepth;
    json["useAlpha"] = m_useAlpha;

    // Performance settings
    json["threadCount"] = m_threadCount;
    json["memoryLimit"] = m_memoryLimit;
    json["bucketSize"] = m_bucketSize;

    // Features
    json["enableMotionBlur"] = m_enableMotionBlur;
    json["enableDepthOfField"] = m_enableDepthOfField;
    json["enableGlobalIllumination"] = m_enableGlobalIllumination;
    json["enableCaustics"] = m_enableCaustics;
    json["enableSubsurfaceScattering"] = m_enableSubsurfaceScattering;
    json["enableDisplacement"] = m_enableDisplacement;

    // Motion blur settings
    json["motionBlurSamples"] = m_motionBlurSamples;
    json["shutterAngle"] = m_shutterAngle;

    // Depth of field settings
    json["focalLength"] = m_focalLength;
    json["fStop"] = m_fStop;

    // Renderer-specific settings
    QJsonObject rendererSettingsJson;
    for (auto it = m_rendererSettings.constBegin(); it != m_rendererSettings.constEnd(); ++it) {
        rendererSettingsJson[it.key()] = QJsonValue::fromVariant(it.value());
    }
    json["rendererSettings"] = rendererSettingsJson;

    return json;
}

RenderConfig* RenderConfig::fromJson(const QJsonObject &json, QObject *parent)
{
    RenderConfig *config = new RenderConfig(parent);

    config->setConfigId(json["configId"].toString());
    config->setConfigName(json["configName"].toString());
    config->setRenderer(static_cast<RendererType>(json["renderer"].toInt()));
    config->setQuality(static_cast<QualityPreset>(json["quality"].toInt()));

    // Quality settings
    config->setSamples(json["samples"].toInt());
    config->setRayDepth(json["rayDepth"].toInt());
    config->setDiffuseSamples(json["diffuseSamples"].toInt());
    config->setSpecularSamples(json["specularSamples"].toInt());
    config->setTransmissionSamples(json["transmissionSamples"].toInt());
    config->setSssSamples(json["sssSamples"].toInt());
    config->setVolumeSamples(json["volumeSamples"].toInt());

    // Output settings
    config->setImageFormat(static_cast<ImageFormat>(json["imageFormat"].toInt()));
    config->setColorSpace(static_cast<ColorSpace>(json["colorSpace"].toInt()));
    config->setBitDepth(json["bitDepth"].toInt());
    config->setUseAlpha(json["useAlpha"].toBool());

    // Performance settings
    config->setThreadCount(json["threadCount"].toInt());
    config->setMemoryLimit(json["memoryLimit"].toInt());
    config->setBucketSize(json["bucketSize"].toInt());

    // Features
    config->setEnableMotionBlur(json["enableMotionBlur"].toBool());
    config->setEnableDepthOfField(json["enableDepthOfField"].toBool());
    config->setEnableGlobalIllumination(json["enableGlobalIllumination"].toBool());
    config->setEnableCaustics(json["enableCaustics"].toBool());
    config->setEnableSubsurfaceScattering(json["enableSubsurfaceScattering"].toBool());
    config->setEnableDisplacement(json["enableDisplacement"].toBool());

    // Motion blur settings
    config->setMotionBlurSamples(json["motionBlurSamples"].toInt());
    config->setShutterAngle(json["shutterAngle"].toDouble());

    // Depth of field settings
    config->setFocalLength(json["focalLength"].toDouble());
    config->setFStop(json["fStop"].toDouble());

    // Renderer-specific settings
    QJsonObject rendererSettingsJson = json["rendererSettings"].toObject();
    QVariantMap rendererSettings;
    for (auto it = rendererSettingsJson.constBegin(); it != rendererSettingsJson.constEnd(); ++it) {
        rendererSettings[it.key()] = it.value().toVariant();
    }
    config->setRendererSettings(rendererSettings);

    return config;
}

QString RenderConfig::rendererString() const
{
    switch (m_renderer) {
        case RendererType::Arnold:
            return QString::fromUtf8("Arnold");
        case RendererType::VRay:
            return QString::fromUtf8("V-Ray");
        case RendererType::Redshift:
            return QString::fromUtf8("Redshift");
        case RendererType::MayaSoftware:
            return QString::fromUtf8("Maya Software");
        case RendererType::MayaHardware:
            return QString::fromUtf8("Maya Hardware");
        case RendererType::Other:
            return QString::fromUtf8("其他");
        default:
            return QString::fromUtf8("未知");
    }
}

QString RenderConfig::imageFormatString() const
{
    switch (m_imageFormat) {
        case ImageFormat::PNG:
            return QString::fromUtf8("PNG");
        case ImageFormat::JPEG:
            return QString::fromUtf8("JPEG");
        case ImageFormat::EXR:
            return QString::fromUtf8("OpenEXR");
        case ImageFormat::TIFF:
            return QString::fromUtf8("TIFF");
        case ImageFormat::TGA:
            return QString::fromUtf8("TGA");
        default:
            return QString::fromUtf8("未知");
    }
}

QString RenderConfig::colorSpaceString() const
{
    switch (m_colorSpace) {
        case ColorSpace::sRGB:
            return QString::fromUtf8("sRGB");
        case ColorSpace::Linear:
            return QString::fromUtf8("Linear");
        case ColorSpace::ACES:
            return QString::fromUtf8("ACES");
        case ColorSpace::AcescG:
            return QString::fromUtf8("ACEScg");
        case ColorSpace::Rec709:
            return QString::fromUtf8("Rec.709");
        default:
            return QString::fromUtf8("未知");
    }
}

QString RenderConfig::qualityString() const
{
    switch (m_quality) {
        case QualityPreset::Draft:
            return QString::fromUtf8("草稿");
        case QualityPreset::Low:
            return QString::fromUtf8("低");
        case QualityPreset::Medium:
            return QString::fromUtf8("中等");
        case QualityPreset::High:
            return QString::fromUtf8("高");
        case QualityPreset::Production:
            return QString::fromUtf8("生产");
        case QualityPreset::Custom:
            return QString::fromUtf8("自定义");
        default:
            return QString::fromUtf8("未知");
    }
}

void RenderConfig::applyQualityPreset(QualityPreset preset)
{
    switch (preset) {
        case QualityPreset::Draft:
            m_samples = 1;
            m_rayDepth = 2;
            m_diffuseSamples = 0;
            m_specularSamples = 0;
            m_transmissionSamples = 0;
            m_sssSamples = 0;
            m_volumeSamples = 0;
            break;

        case QualityPreset::Low:
            m_samples = 2;
            m_rayDepth = 4;
            m_diffuseSamples = 1;
            m_specularSamples = 1;
            m_transmissionSamples = 1;
            m_sssSamples = 1;
            m_volumeSamples = 1;
            break;

        case QualityPreset::Medium:
            m_samples = 4;
            m_rayDepth = 8;
            m_diffuseSamples = 2;
            m_specularSamples = 2;
            m_transmissionSamples = 2;
            m_sssSamples = 2;
            m_volumeSamples = 2;
            break;

        case QualityPreset::High:
            m_samples = 8;
            m_rayDepth = 12;
            m_diffuseSamples = 3;
            m_specularSamples = 3;
            m_transmissionSamples = 3;
            m_sssSamples = 3;
            m_volumeSamples = 3;
            break;

        case QualityPreset::Production:
            m_samples = 16;
            m_rayDepth = 16;
            m_diffuseSamples = 4;
            m_specularSamples = 4;
            m_transmissionSamples = 4;
            m_sssSamples = 4;
            m_volumeSamples = 4;
            break;

        case QualityPreset::Custom:
            // Don't change anything for custom
            break;
    }

    emit configDataChanged();
}

void RenderConfig::loadDefaultSettings()
{
    setRenderer(RendererType::Arnold);
    setQuality(QualityPreset::Medium);
    applyQualityPreset(QualityPreset::Medium);

    setImageFormat(ImageFormat::PNG);
    setColorSpace(ColorSpace::sRGB);
    setBitDepth(8);
    setUseAlpha(true);

    setThreadCount(0);  // Auto
    setMemoryLimit(4096);  // 4GB
    setBucketSize(64);

    setEnableMotionBlur(false);
    setEnableDepthOfField(false);
    setEnableGlobalIllumination(true);
    setEnableCaustics(false);
    setEnableSubsurfaceScattering(true);
    setEnableDisplacement(true);

    setMotionBlurSamples(5);
    setShutterAngle(180.0);
    setFocalLength(35.0);
    setFStop(5.6);

    m_rendererSettings.clear();
}

void RenderConfig::clear()
{
    setConfigId(QString());
    setConfigName(QString());
    loadDefaultSettings();
}
