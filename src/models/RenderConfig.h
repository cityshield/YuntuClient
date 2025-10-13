/**
 * @file RenderConfig.h
 * @brief 渲染配置模型
 */

#ifndef RENDERCONFIG_H
#define RENDERCONFIG_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVariantMap>

/**
 * @brief 渲染器类型枚举
 */
enum class RendererType {
    Arnold = 0,         // Arnold 渲染器
    VRay = 1,           // V-Ray 渲染器
    Redshift = 2,       // Redshift 渲染器
    MayaSoftware = 3,   // Maya Software 渲染器
    MayaHardware = 4,   // Maya Hardware 渲染器
    Other = 99          // 其他渲染器
};

/**
 * @brief 图像格式枚举
 */
enum class ImageFormat {
    PNG = 0,            // PNG 格式
    JPEG = 1,           // JPEG 格式
    EXR = 2,            // OpenEXR 格式
    TIFF = 3,           // TIFF 格式
    TGA = 4             // TGA 格式
};

/**
 * @brief 色彩空间枚举
 */
enum class ColorSpace {
    sRGB = 0,           // sRGB 色彩空间
    Linear = 1,         // 线性色彩空间
    ACES = 2,           // ACES 色彩空间
    AcescG = 3,         // ACEScg 色彩空间
    Rec709 = 4          // Rec.709 色彩空间
};

/**
 * @brief 质量预设枚举
 */
enum class QualityPreset {
    Draft = 0,          // 草稿质量
    Low = 1,            // 低质量
    Medium = 2,         // 中等质量
    High = 3,           // 高质量
    Production = 4,     // 生产质量
    Custom = 99         // 自定义
};

/**
 * @brief 渲染配置模型
 *
 * 存储渲染配置信息，包括渲染器设置、质量参数、输出设置等
 */
class RenderConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString configId READ configId WRITE setConfigId NOTIFY configIdChanged)
    Q_PROPERTY(QString configName READ configName WRITE setConfigName NOTIFY configNameChanged)
    Q_PROPERTY(RendererType renderer READ renderer WRITE setRenderer NOTIFY rendererChanged)
    Q_PROPERTY(QualityPreset quality READ quality WRITE setQuality NOTIFY qualityChanged)

public:
    explicit RenderConfig(QObject *parent = nullptr);
    ~RenderConfig();

    // Getters
    QString configId() const { return m_configId; }
    QString configName() const { return m_configName; }
    RendererType renderer() const { return m_renderer; }
    QualityPreset quality() const { return m_quality; }

    // Quality settings
    int samples() const { return m_samples; }
    int rayDepth() const { return m_rayDepth; }
    int diffuseSamples() const { return m_diffuseSamples; }
    int specularSamples() const { return m_specularSamples; }
    int transmissionSamples() const { return m_transmissionSamples; }
    int sssSamples() const { return m_sssSamples; }
    int volumeSamples() const { return m_volumeSamples; }

    // Output settings
    ImageFormat imageFormat() const { return m_imageFormat; }
    ColorSpace colorSpace() const { return m_colorSpace; }
    int bitDepth() const { return m_bitDepth; }
    bool useAlpha() const { return m_useAlpha; }

    // Performance settings
    int threadCount() const { return m_threadCount; }
    int memoryLimit() const { return m_memoryLimit; }
    int bucketSize() const { return m_bucketSize; }

    // Features
    bool enableMotionBlur() const { return m_enableMotionBlur; }
    bool enableDepthOfField() const { return m_enableDepthOfField; }
    bool enableGlobalIllumination() const { return m_enableGlobalIllumination; }
    bool enableCaustics() const { return m_enableCaustics; }
    bool enableSubsurfaceScattering() const { return m_enableSubsurfaceScattering; }
    bool enableDisplacement() const { return m_enableDisplacement; }

    // Motion blur settings
    int motionBlurSamples() const { return m_motionBlurSamples; }
    double shutterAngle() const { return m_shutterAngle; }

    // Depth of field settings
    double focalLength() const { return m_focalLength; }
    double fStop() const { return m_fStop; }

    // Renderer-specific settings
    QVariantMap rendererSettings() const { return m_rendererSettings; }
    QVariant getRendererSetting(const QString &key, const QVariant &defaultValue = QVariant()) const;

    // Setters
    void setConfigId(const QString &configId);
    void setConfigName(const QString &configName);
    void setRenderer(RendererType renderer);
    void setQuality(QualityPreset quality);

    // Quality settings
    void setSamples(int samples);
    void setRayDepth(int depth);
    void setDiffuseSamples(int samples);
    void setSpecularSamples(int samples);
    void setTransmissionSamples(int samples);
    void setSssSamples(int samples);
    void setVolumeSamples(int samples);

    // Output settings
    void setImageFormat(ImageFormat format);
    void setColorSpace(ColorSpace colorSpace);
    void setBitDepth(int bitDepth);
    void setUseAlpha(bool use);

    // Performance settings
    void setThreadCount(int count);
    void setMemoryLimit(int limitMB);
    void setBucketSize(int size);

    // Features
    void setEnableMotionBlur(bool enable);
    void setEnableDepthOfField(bool enable);
    void setEnableGlobalIllumination(bool enable);
    void setEnableCaustics(bool enable);
    void setEnableSubsurfaceScattering(bool enable);
    void setEnableDisplacement(bool enable);

    // Motion blur settings
    void setMotionBlurSamples(int samples);
    void setShutterAngle(double angle);

    // Depth of field settings
    void setFocalLength(double length);
    void setFStop(double fStop);

    // Renderer-specific settings
    void setRendererSettings(const QVariantMap &settings);
    void setRendererSetting(const QString &key, const QVariant &value);

    // 序列化/反序列化
    QJsonObject toJson() const;
    static RenderConfig* fromJson(const QJsonObject &json, QObject *parent = nullptr);

    // 工具方法
    QString rendererString() const;
    QString imageFormatString() const;
    QString colorSpaceString() const;
    QString qualityString() const;
    void applyQualityPreset(QualityPreset preset);
    void loadDefaultSettings();
    void clear();

signals:
    void configIdChanged();
    void configNameChanged();
    void rendererChanged();
    void qualityChanged();
    void configDataChanged();

private:
    QString m_configId;
    QString m_configName;
    RendererType m_renderer;
    QualityPreset m_quality;

    // Quality settings
    int m_samples;
    int m_rayDepth;
    int m_diffuseSamples;
    int m_specularSamples;
    int m_transmissionSamples;
    int m_sssSamples;
    int m_volumeSamples;

    // Output settings
    ImageFormat m_imageFormat;
    ColorSpace m_colorSpace;
    int m_bitDepth;
    bool m_useAlpha;

    // Performance settings
    int m_threadCount;
    int m_memoryLimit;
    int m_bucketSize;

    // Features
    bool m_enableMotionBlur;
    bool m_enableDepthOfField;
    bool m_enableGlobalIllumination;
    bool m_enableCaustics;
    bool m_enableSubsurfaceScattering;
    bool m_enableDisplacement;

    // Motion blur settings
    int m_motionBlurSamples;
    double m_shutterAngle;

    // Depth of field settings
    double m_focalLength;
    double m_fStop;

    // Renderer-specific settings
    QVariantMap m_rendererSettings;
};

#endif // RENDERCONFIG_H
