/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "blend_filter.h"
#include "gpupixel_context.h"
#include "util.h"
#include "stb_image.h"
#include <sstream>

USING_NS_GPUPIXEL

REGISTER_FILTER_CLASS(BlendFilter)

// 混合模式shader函数
const std::string kBlendModeFunctions = R"(
// 正片叠底 (Multiply)
vec4 blendMultiply(vec4 base, vec4 blend) {
    return base * blend;
}

// 滤色 (Screen)
vec4 blendScreen(vec4 base, vec4 blend) {
    return 1.0 - (1.0 - base) * (1.0 - blend);
}

// 叠加 (Overlay)
vec4 blendOverlay(vec4 base, vec4 blend) {
    return mix(2.0 * base * blend, 1.0 - 2.0 * (1.0 - base) * (1.0 - blend), 
               step(0.5, base));
}

// 柔光 (Soft Light)
vec4 blendSoftLight(vec4 base, vec4 blend) {
    vec4 result1 = 2.0 * base * blend + base * base * (1.0 - 2.0 * blend);
    vec4 result2 = sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend);
    return mix(result1, result2, step(0.5, blend));
}

// 强光 (Hard Light)
vec4 blendHardLight(vec4 base, vec4 blend) {
    return mix(2.0 * base * blend, 1.0 - 2.0 * (1.0 - base) * (1.0 - blend), 
               step(0.5, blend));
}

// 颜色减淡 (Color Dodge)
vec4 blendColorDodge(vec4 base, vec4 blend) {
    return base / (1.0 - blend);
}

// 颜色加深 (Color Burn)
vec4 blendColorBurn(vec4 base, vec4 blend) {
    return 1.0 - (1.0 - base) / blend;
}

// 变暗 (Darken)
vec4 blendDarken(vec4 base, vec4 blend) {
    return min(base, blend);
}

// 变亮 (Lighten)
vec4 blendLighten(vec4 base, vec4 blend) {
    return max(base, blend);
}

// 差值 (Difference)
vec4 blendDifference(vec4 base, vec4 blend) {
    return abs(base - blend);
}

// 排除 (Exclusion)
vec4 blendExclusion(vec4 base, vec4 blend) {
    return base + blend - 2.0 * base * blend;
}

// 线性减淡 (Linear Dodge)
vec4 blendLinearDodge(vec4 base, vec4 blend) {
    return base + blend;
}

// 线性加深 (Linear Burn)
vec4 blendLinearBurn(vec4 base, vec4 blend) {
    return base + blend - 1.0;
}

// 亮光 (Vivid Light)
vec4 blendVividLight(vec4 base, vec4 blend) {
    vec4 result1 = blendColorDodge(base, 2.0 * blend);
    vec4 result2 = blendColorBurn(base, 2.0 * (blend - 0.5));
    return mix(result1, result2, step(0.5, blend));
}

// 线性光 (Linear Light)
vec4 blendLinearLight(vec4 base, vec4 blend) {
    vec4 result1 = blendLinearDodge(base, 2.0 * blend);
    vec4 result2 = blendLinearBurn(base, 2.0 * (blend - 0.5));
    return mix(result1, result2, step(0.5, blend));
}

// 点光 (Pin Light)
vec4 blendPinLight(vec4 base, vec4 blend) {
    vec4 result1 = blendDarken(base, 2.0 * blend);
    vec4 result2 = blendLighten(base, 2.0 * (blend - 0.5));
    return mix(result1, result2, step(0.5, blend));
}

// 实色混合 (Hard Mix)
vec4 blendHardMix(vec4 base, vec4 blend) {
    vec4 result = base + blend;
    return vec4(step(1.0, result.r), step(1.0, result.g), step(1.0, result.b), result.a);
}

// 根据混合模式选择对应的函数
vec4 applyBlendMode(vec4 base, vec4 blend, int mode) {
    if (mode == 0) return blend;                    // NORMAL
    else if (mode == 1) return blendMultiply(base, blend);     // MULTIPLY
    else if (mode == 2) return blendScreen(base, blend);       // SCREEN
    else if (mode == 3) return blendOverlay(base, blend);      // OVERLAY
    else if (mode == 4) return blendSoftLight(base, blend);    // SOFT_LIGHT
    else if (mode == 5) return blendHardLight(base, blend);    // HARD_LIGHT
    else if (mode == 6) return blendColorDodge(base, blend);   // COLOR_DODGE
    else if (mode == 7) return blendColorBurn(base, blend);    // COLOR_BURN
    else if (mode == 8) return blendDarken(base, blend);       // DARKEN
    else if (mode == 9) return blendLighten(base, blend);      // LIGHTEN
    else if (mode == 10) return blendDifference(base, blend);   // DIFFERENCE
    else if (mode == 11) return blendExclusion(base, blend);    // EXCLUSION
    else if (mode == 12) return blendLinearDodge(base, blend);  // LINEAR_DODGE
    else if (mode == 13) return blendLinearBurn(base, blend);   // LINEAR_BURN
    else if (mode == 14) return blendVividLight(base, blend);   // VIVID_LIGHT
    else if (mode == 15) return blendLinearLight(base, blend);  // LINEAR_LIGHT
    else if (mode == 16) return blendPinLight(base, blend);     // PIN_LIGHT
    else if (mode == 17) return blendHardMix(base, blend);      // HARD_MIX
    else return blend;
}
)";

std::shared_ptr<BlendFilter> BlendFilter::create() {
    auto ret = std::shared_ptr<BlendFilter>(new BlendFilter());
    if (ret && !ret->init()) {
        ret.reset();
    }
    return ret;
}

BlendFilter::BlendFilter() : _globalOpacity(1.0f), _globalIntensity(1.0f), _shaderNeedsUpdate(true) {
}

bool BlendFilter::init() {
    _shaderNeedsUpdate = true;
    return true;
}

std::string BlendFilter::generateFragmentShader() const {
    std::stringstream shader;
    
    // 添加精度限定符
    shader << "precision mediump float;\n\n";
    
    // 添加混合模式函数
    shader << kBlendModeFunctions;
    
    // 添加uniform声明
    shader << "uniform sampler2D inputImageTexture;\n";
    shader << "uniform float globalOpacity;\n";
    shader << "uniform float globalIntensity;\n";
    
    // 为每个图层添加uniform声明
    for (size_t i = 0; i < _layers.size(); ++i) {
        shader << "uniform sampler2D layerTexture" << i << ";\n";
        shader << "uniform int layerBlendMode" << i << ";\n";
        shader << "uniform float layerOpacity" << i << ";\n";
        shader << "uniform float layerIntensity" << i << ";\n";
        shader << "uniform bool layerEnabled" << i << ";\n";
    }
    
    shader << "varying vec2 textureCoordinate;\n\n";
    
    shader << "void main() {\n";
    shader << "    vec4 baseColor = texture2D(inputImageTexture, textureCoordinate);\n";
    shader << "    vec4 result = baseColor;\n\n";
    
    // 对每个图层进行混合
    for (size_t i = 0; i < _layers.size(); ++i) {
        shader << "    if (layerEnabled" << i << ") {\n";
        shader << "        vec4 layerColor" << i << " = texture2D(layerTexture" << i << ", textureCoordinate);\n";
        shader << "        vec4 blended" << i << " = applyBlendMode(result, layerColor" << i << ", layerBlendMode" << i << ");\n";
        shader << "        blended" << i << " = mix(result, blended" << i << ", layerIntensity" << i << " * layerOpacity" << i << ");\n";
        shader << "        result = blended" << i << ";\n";
        shader << "    }\n\n";
    }
    
    shader << "    result = mix(baseColor, result, globalIntensity * globalOpacity);\n";
    shader << "    gl_FragColor = result;\n";
    shader << "}\n";
    
    return shader.str();
}

bool BlendFilter::proceed(bool bUpdateTargets, int64_t frameTime) {
    // 强制测试shader初始化（临时调试）
    // Util::Log("BlendFilter", "Proceed called, _shaderNeedsUpdate=%s, _filterProgram=%p", 
            //   _shaderNeedsUpdate ? "true" : "false", _filterProgram);
    
    // 强制重新初始化shader（临时调试）
    _shaderNeedsUpdate = true;
    
    // 如果shader需要更新，重新生成并初始化
    if (_shaderNeedsUpdate) {
        std::string fragmentShader = generateFragmentShader();
        // Util::Log("BlendFilter", "Generated shader with %zu layers", _layers.size());
        // Util::Log("BlendFilter", "Shader source: %s", fragmentShader.c_str());
        // 先尝试一个简单的shader来测试
        std::string simpleShader = R"(
            precision mediump float;
            uniform sampler2D inputImageTexture;
            varying vec2 textureCoordinate;
            
            void main() {
                gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
            }
        )";
        
        // Util::Log("BlendFilter", "Trying simple shader first");
        if (!initWithFragmentShaderString(simpleShader)) {
            // Util::Log("BlendFilter", "Failed to initialize simple shader");
            return false;
        }
        
        // Util::Log("BlendFilter", "Simple shader initialized successfully, now trying complex shader");
        if (!initWithFragmentShaderString(fragmentShader)) {
            // Util::Log("BlendFilter", "Failed to initialize complex shader");
            return false;
        }
        
        // 检查shader程序是否有效
        if (!_filterProgram) {
            // Util::Log("BlendFilter", "Shader program is null after initialization");
            return false;
        }
        // Util::Log("BlendFilter", "Shader initialized successfully");
        _shaderNeedsUpdate = false;
    }
    
    // 检查shader是否有效
    if (!_filterProgram) {
        // Util::Log("BlendFilter", "Shader program is null, skipping proceed");
        return false;
    }
    
    // 如果没有图层，直接返回
    if (_layers.empty()) {
        // Util::Log("BlendFilter", "No layers, skipping proceed");
        return Filter::proceed(bUpdateTargets, frameTime);
    }
    
    // 检查OpenGL状态
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        // Util::Log("BlendFilter", "OpenGL error before setting uniforms: 0x%x", error);
        return false;
    }
    
    // 设置全局uniform
    // Util::Log("BlendFilter", "Setting global uniforms: opacity=%f, intensity=%f", _globalOpacity, _globalIntensity);
    _filterProgram->setUniformValue("globalOpacity", _globalOpacity);
    _filterProgram->setUniformValue("globalIntensity", _globalIntensity);
    
    // 为每个图层设置uniform
    for (size_t i = 0; i < _layers.size(); ++i) {
        const BlendLayer& layer = _layers[i];
        
        if (layer.textureLoaded && layer.texture != 0) {
            // 绑定图层纹理
            GLenum textureUnit = GL_TEXTURE1 + i;
            if (textureUnit > GL_TEXTURE7) {
                textureUnit = GL_TEXTURE1 + (i % 7); // 循环使用纹理单元
            }
            
            // Util::Log("BlendFilter", "Binding layer %zu to texture unit %d", i, textureUnit - GL_TEXTURE0);
            
            // 检查OpenGL状态
            GLenum error = glGetError();
            // if (error != GL_NO_ERROR) {
                // Util::Log("BlendFilter", "OpenGL error before binding texture: 0x%x", error);
            // }
            
            CHECK_GL(glActiveTexture(textureUnit));
            CHECK_GL(glBindTexture(GL_TEXTURE_2D, layer.texture));
            
            // 设置uniform
            std::string textureUniformName = "layerTexture" + std::to_string(i);
            std::string blendModeUniformName = "layerBlendMode" + std::to_string(i);
            std::string opacityUniformName = "layerOpacity" + std::to_string(i);
            std::string intensityUniformName = "layerIntensity" + std::to_string(i);
            std::string enabledUniformName = "layerEnabled" + std::to_string(i);
            
            // Util::Log("BlendFilter", "Setting uniform %s = %d", textureUniformName.c_str(), (int)(textureUnit - GL_TEXTURE0));
            _filterProgram->setUniformValue(textureUniformName, (int)(textureUnit - GL_TEXTURE0));
            
            // Util::Log("BlendFilter", "Setting uniform %s = %d", blendModeUniformName.c_str(), (int)layer.blendMode);
            _filterProgram->setUniformValue(blendModeUniformName, (int)layer.blendMode);
            
            // Util::Log("BlendFilter", "Setting uniform %s = %f", opacityUniformName.c_str(), layer.opacity);
            _filterProgram->setUniformValue(opacityUniformName, layer.opacity);
            
            // Util::Log("BlendFilter", "Setting uniform %s = %f", intensityUniformName.c_str(), layer.intensity);
            _filterProgram->setUniformValue(intensityUniformName, layer.intensity);
            
            // Util::Log("BlendFilter", "Setting uniform %s = %s", enabledUniformName.c_str(), layer.enabled ? "true" : "false");
            _filterProgram->setUniformValue(enabledUniformName, layer.enabled);
        } else {
            // 如果纹理未加载，禁用该图层
            // Util::Log("BlendFilter", "Layer %zu not loaded, disabling", i);
            _filterProgram->setUniformValue("layerEnabled" + std::to_string(i), false);
        }
    }
    
    return Filter::proceed(bUpdateTargets, frameTime);
}

int BlendFilter::addLayer(const std::string& imagePath, BlendMode blendMode, float opacity, float intensity) {
    BlendLayer layer;
    layer.imagePath = imagePath;
    layer.blendMode = blendMode;
    layer.opacity = opacity;
    layer.intensity = intensity;
    layer.enabled = true;
    
    _layers.push_back(layer);
    _shaderNeedsUpdate = true;
    
    // 加载纹理
    loadLayerTexture(_layers.back());
    
    return (int)_layers.size() - 1;
}

void BlendFilter::removeLayer(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        unloadLayerTexture(_layers[layerIndex]);
        _layers.erase(_layers.begin() + layerIndex);
        _shaderNeedsUpdate = true;
    }
}

void BlendFilter::removeAllLayers() {
    for (auto& layer : _layers) {
        unloadLayerTexture(layer);
    }
    _layers.clear();
    _shaderNeedsUpdate = true;
}

void BlendFilter::setLayerImage(int layerIndex, const std::string& imagePath) {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        unloadLayerTexture(_layers[layerIndex]);
        _layers[layerIndex].imagePath = imagePath;
        loadLayerTexture(_layers[layerIndex]);
    }
}

void BlendFilter::setLayerBlendMode(int layerIndex, BlendMode blendMode) {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        _layers[layerIndex].blendMode = blendMode;
    }
}

void BlendFilter::setLayerOpacity(int layerIndex, float opacity) {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        _layers[layerIndex].opacity = std::max(0.0f, std::min(1.0f, opacity));
    }
}

void BlendFilter::setLayerIntensity(int layerIndex, float intensity) {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        _layers[layerIndex].intensity = std::max(0.0f, std::min(1.0f, intensity));
    }
}

void BlendFilter::setLayerEnabled(int layerIndex, bool enabled) {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        _layers[layerIndex].enabled = enabled;
    }
}

void BlendFilter::setGlobalOpacity(float opacity) {
    _globalOpacity = std::max(0.0f, std::min(1.0f, opacity));
}

void BlendFilter::setGlobalIntensity(float intensity) {
    _globalIntensity = std::max(0.0f, std::min(1.0f, intensity));
}

BlendLayer* BlendFilter::getLayer(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        return &_layers[layerIndex];
    }
    return nullptr;
}

const BlendLayer* BlendFilter::getLayer(int layerIndex) const {
    if (layerIndex >= 0 && layerIndex < (int)_layers.size()) {
        return &_layers[layerIndex];
    }
    return nullptr;
}

void BlendFilter::loadLayerTexture(BlendLayer& layer) {
    if (layer.imagePath.empty()) {
        // Util::Log("BlendFilter", "Layer image path is empty, skipping texture load");
        return;
    }
    
    // Util::Log("BlendFilter", "Loading layer texture: %s", layer.imagePath.c_str());
    
    // 清理现有纹理
    unloadLayerTexture(layer);
    
    // 加载图片
    int width, height, channels;
    unsigned char* data = stbi_load(layer.imagePath.c_str(), &width, &height, &channels, 0);
    
    if (data == nullptr) {
        // Util::Log("BlendFilter", "Failed to load layer image: %s", layer.imagePath.c_str());
        // Util::Log("BlendFilter", "STBI error: %s", stbi_failure_reason());
        return;
    }
    
    // Util::Log("BlendFilter", "Image loaded successfully: %dx%d, %d channels", width, height, channels);
    
    // 创建OpenGL纹理
    CHECK_GL(glGenTextures(1, &layer.texture));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, layer.texture));
    
    // 设置纹理参数
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    
    // 上传纹理数据
    if (channels == 3) {
        CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data));
    } else if (channels == 4) {
        CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
    } else {
        // Util::Log("BlendFilter", "Unsupported image format: %d channels", channels);
        CHECK_GL(glDeleteTextures(1, &layer.texture));
        layer.texture = 0;
        stbi_image_free(data);
        return;
    }
    
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, 0));
    stbi_image_free(data);
    
    layer.textureLoaded = true;
    // Util::Log("BlendFilter", "Successfully loaded layer texture: %s (%dx%d, %d channels)", 
    //           layer.imagePath.c_str(), width, height, channels);
}

void BlendFilter::unloadLayerTexture(BlendLayer& layer) {
    if (layer.textureLoaded && layer.texture != 0) {
        CHECK_GL(glDeleteTextures(1, &layer.texture));
        layer.texture = 0;
        layer.textureLoaded = false;
    }
}
