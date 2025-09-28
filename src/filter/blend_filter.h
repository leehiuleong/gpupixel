/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "filter.h"
#include "gpupixel_macros.h"
#include <vector>
#include <string>

NS_GPUPIXEL_BEGIN

// 混合模式枚举
enum class BlendMode {
    NORMAL = 0,           // 正常
    MULTIPLY = 1,         // 正片叠底
    SCREEN = 2,           // 滤色
    OVERLAY = 3,          // 叠加
    SOFT_LIGHT = 4,       // 柔光
    HARD_LIGHT = 5,       // 强光
    COLOR_DODGE = 6,      // 颜色减淡
    COLOR_BURN = 7,       // 颜色加深
    DARKEN = 8,           // 变暗
    LIGHTEN = 9,          // 变亮
    DIFFERENCE = 10,      // 差值
    EXCLUSION = 11,       // 排除
    LINEAR_DODGE = 12,    // 线性减淡
    LINEAR_BURN = 13,     // 线性加深
    VIVID_LIGHT = 14,     // 亮光
    LINEAR_LIGHT = 15,    // 线性光
    PIN_LIGHT = 16,       // 点光
    HARD_MIX = 17         // 实色混合
};

// 图层结构
struct BlendLayer {
    std::string imagePath;    // 图片路径
    GLuint texture;           // OpenGL纹理ID
    bool textureLoaded;       // 纹理是否已加载
    BlendMode blendMode;      // 混合模式
    float opacity;            // 不透明度 (0.0 - 1.0)
    float intensity;          // 强度 (0.0 - 1.0)
    bool enabled;             // 是否启用
    
    BlendLayer() : texture(0), textureLoaded(false), blendMode(BlendMode::NORMAL), 
                   opacity(1.0f), intensity(1.0f), enabled(true) {}
};

class BlendFilter : public Filter {
public:
    static std::shared_ptr<BlendFilter> create();
    bool init();
    virtual bool proceed(bool bUpdateTargets = true, int64_t frameTime = 0) override;
    
    // 图层管理
    int addLayer(const std::string& imagePath, BlendMode blendMode = BlendMode::MULTIPLY, 
                 float opacity = 1.0f, float intensity = 1.0f);
    void removeLayer(int layerIndex);
    void removeAllLayers();
    int getLayerCount() const { return _layers.size(); }
    
    // 图层属性设置
    void setLayerImage(int layerIndex, const std::string& imagePath);
    void setLayerBlendMode(int layerIndex, BlendMode blendMode);
    void setLayerOpacity(int layerIndex, float opacity);
    void setLayerIntensity(int layerIndex, float intensity);
    void setLayerEnabled(int layerIndex, bool enabled);
    
    // 全局设置
    void setGlobalOpacity(float opacity);
    void setGlobalIntensity(float intensity);
    
    // 属性访问
    BlendLayer* getLayer(int layerIndex);
    const BlendLayer* getLayer(int layerIndex) const;

protected:
    BlendFilter();
    
    void loadLayerTexture(BlendLayer& layer);
    void unloadLayerTexture(BlendLayer& layer);
    std::string getBlendModeShaderFunction(BlendMode mode) const;
    std::string generateFragmentShader() const;
    
    std::vector<BlendLayer> _layers;
    float _globalOpacity;
    float _globalIntensity;
    bool _shaderNeedsUpdate;
};

NS_GPUPIXEL_END
