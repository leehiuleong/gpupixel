/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/blend_filter.h"
#include "core/gpupixel_context.h"
#include "core/gpupixel_gl_include.h"
#include "utils/util.h"
#include "stb/stb_image.h"
#include <sstream>

namespace gpupixel {

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

std::shared_ptr<BlendFilter> BlendFilter::Create() {
  auto ret = std::shared_ptr<BlendFilter>(new BlendFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

BlendFilter::BlendFilter()
    : global_opacity_(1.0f),
      global_intensity_(1.0f),
      shader_needs_update_(true) {
}

BlendFilter::~BlendFilter() {
  GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    for (auto& layer : layers_) {
      UnloadLayerTexture(layer);
    }
  });
  layers_.clear();
}

bool BlendFilter::Init() {
  // Initialize with default pass-through shader
  // This ensures the filter works even without layers
#if defined(GPUPIXEL_GLES_SHADER)
  const std::string defaultShader = R"(
    precision mediump float;
    uniform sampler2D inputImageTexture;
    varying highp vec2 textureCoordinate;
    
    void main() {
      gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
    }
  )";
#else
  const std::string defaultShader = R"(
    uniform sampler2D inputImageTexture;
    varying vec2 textureCoordinate;
    
    void main() {
      gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
    }
  )";
#endif
  
  if (!Filter::InitWithFragmentShaderString(defaultShader)) {
    return false;
  }
  
  shader_needs_update_ = false;
  return true;
}

std::string BlendFilter::GenerateFragmentShader() const {
  std::stringstream shader;
  
#if defined(GPUPIXEL_GLES_SHADER)
  shader << "precision mediump float;\n\n";
#endif
  
  // 添加混合模式函数
  shader << kBlendModeFunctions;
  
  // 添加uniform声明
  shader << "uniform sampler2D inputImageTexture;\n";
  shader << "uniform float globalOpacity;\n";
  shader << "uniform float globalIntensity;\n";
  
  // 为每个图层添加uniform声明
  for (size_t i = 0; i < layers_.size(); ++i) {
    shader << "uniform sampler2D layerTexture" << i << ";\n";
    shader << "uniform int layerBlendMode" << i << ";\n";
    shader << "uniform float layerOpacity" << i << ";\n";
    shader << "uniform float layerIntensity" << i << ";\n";
    shader << "uniform int layerEnabled" << i << ";\n";
  }
  
  shader << "varying vec2 textureCoordinate;\n\n";
  
  shader << "void main() {\n";
  shader << "    vec4 baseColor = texture2D(inputImageTexture, textureCoordinate);\n";
  shader << "    vec4 result = baseColor;\n\n";
  
  // 对每个图层进行混合
  for (size_t i = 0; i < layers_.size(); ++i) {
    shader << "    if (layerEnabled" << i << " != 0) {\n";
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

bool BlendFilter::DoRender(bool updateSinks) {
  // 如果shader需要更新，重新生成并初始化
  if (shader_needs_update_) {
    // 清理旧的 shader 程序
    if (filter_program_) {
      delete filter_program_;
      filter_program_ = nullptr;
    }
    
    if (layers_.empty()) {
      // 如果没有图层，使用默认shader
#if defined(GPUPIXEL_GLES_SHADER)
      const std::string defaultShader = R"(
        precision mediump float;
        uniform sampler2D inputImageTexture;
        varying highp vec2 textureCoordinate;
        
        void main() {
          gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
        }
      )";
#else
      const std::string defaultShader = R"(
        uniform sampler2D inputImageTexture;
        varying vec2 textureCoordinate;
        
        void main() {
          gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
        }
      )";
#endif
      if (!Filter::InitWithFragmentShaderString(defaultShader)) {
        return false;
      }
    } else {
      std::string fragmentShader = GenerateFragmentShader();
      if (!Filter::InitWithFragmentShaderString(fragmentShader)) {
        return false;
      }
    }
    
    // 检查shader程序是否有效
    if (!filter_program_) {
      return false;
    }
    shader_needs_update_ = false;
  }
  
  // 检查shader是否有效
  if (!filter_program_) {
    return Filter::DoRender(updateSinks);
  }
  
  // 如果没有图层，直接透传（不需要设置任何uniform）
  if (layers_.empty()) {
    return Filter::DoRender(updateSinks);
  }
  
  // 设置全局uniform
  filter_program_->SetUniformValue("globalOpacity", global_opacity_);
  filter_program_->SetUniformValue("globalIntensity", global_intensity_);
  
  // 为每个图层设置uniform
  for (size_t i = 0; i < layers_.size(); ++i) {
    const BlendLayer& layer = layers_[i];
    
    if (layer.texture_loaded && layer.texture != 0) {
      // 绑定图层纹理
      GLenum textureUnit = GL_TEXTURE1 + i;
      if (textureUnit > GL_TEXTURE7) {
        textureUnit = GL_TEXTURE1 + (i % 7); // 循环使用纹理单元
      }
      
      GL_CALL(glActiveTexture(textureUnit));
      GL_CALL(glBindTexture(GL_TEXTURE_2D, layer.texture));
      
      // 设置uniform
      std::string textureUniformName = "layerTexture" + std::to_string(i);
      std::string blendModeUniformName = "layerBlendMode" + std::to_string(i);
      std::string opacityUniformName = "layerOpacity" + std::to_string(i);
      std::string intensityUniformName = "layerIntensity" + std::to_string(i);
      std::string enabledUniformName = "layerEnabled" + std::to_string(i);
      
      filter_program_->SetUniformValue(textureUniformName, (int)(textureUnit - GL_TEXTURE0));
      filter_program_->SetUniformValue(blendModeUniformName, (int)layer.blend_mode);
      filter_program_->SetUniformValue(opacityUniformName, layer.opacity);
      filter_program_->SetUniformValue(intensityUniformName, layer.intensity);
      filter_program_->SetUniformValue(enabledUniformName, layer.enabled ? 1 : 0);
    } else {
      // 如果纹理未加载，禁用该图层
      filter_program_->SetUniformValue("layerEnabled" + std::to_string(i), 0);
    }
  }
  
  return Filter::DoRender(updateSinks);
}

int BlendFilter::AddLayer(const std::string& imagePath, BlendMode blendMode, float opacity, float intensity) {
  BlendLayer layer;
  layer.image_path = imagePath;
  layer.blend_mode = blendMode;
  layer.opacity = opacity;
  layer.intensity = intensity;
  layer.enabled = true;
  
  layers_.push_back(layer);
  shader_needs_update_ = true;
  
  // 在 OpenGL 上下文中加载纹理
  GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    LoadLayerTexture(layers_.back());
  });
  
  return (int)layers_.size() - 1;
}

void BlendFilter::RemoveLayer(int layerIndex) {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      UnloadLayerTexture(layers_[layerIndex]);
    });
    layers_.erase(layers_.begin() + layerIndex);
    shader_needs_update_ = true;
  }
}

void BlendFilter::RemoveAllLayers() {
  GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    for (auto& layer : layers_) {
      UnloadLayerTexture(layer);
    }
  });
  layers_.clear();
  shader_needs_update_ = true;
}

void BlendFilter::SetLayerImage(int layerIndex, const std::string& imagePath) {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      UnloadLayerTexture(layers_[layerIndex]);
      layers_[layerIndex].image_path = imagePath;
      LoadLayerTexture(layers_[layerIndex]);
    });
  }
}

void BlendFilter::SetLayerBlendMode(int layerIndex, BlendMode blendMode) {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    layers_[layerIndex].blend_mode = blendMode;
  }
}

void BlendFilter::SetLayerOpacity(int layerIndex, float opacity) {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    layers_[layerIndex].opacity = std::max(0.0f, std::min(1.0f, opacity));
  }
}

void BlendFilter::SetLayerIntensity(int layerIndex, float intensity) {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    layers_[layerIndex].intensity = std::max(0.0f, std::min(1.0f, intensity));
  }
}

void BlendFilter::SetLayerEnabled(int layerIndex, bool enabled) {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    layers_[layerIndex].enabled = enabled;
  }
}

void BlendFilter::SetGlobalOpacity(float opacity) {
  global_opacity_ = std::max(0.0f, std::min(1.0f, opacity));
}

void BlendFilter::SetGlobalIntensity(float intensity) {
  global_intensity_ = std::max(0.0f, std::min(1.0f, intensity));
}

BlendLayer* BlendFilter::GetLayer(int layerIndex) {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    return &layers_[layerIndex];
  }
  return nullptr;
}

const BlendLayer* BlendFilter::GetLayer(int layerIndex) const {
  if (layerIndex >= 0 && layerIndex < (int)layers_.size()) {
    return &layers_[layerIndex];
  }
  return nullptr;
}

void BlendFilter::LoadLayerTexture(BlendLayer& layer) {
  if (layer.image_path.empty()) {
    return;
  }
  
  // 清理现有纹理
  UnloadLayerTexture(layer);
  
  // 加载图片
  int width, height, channels;
  unsigned char* data = stbi_load(layer.image_path.c_str(), &width, &height, &channels, 0);
  
  if (data == nullptr) {
    return;
  }
  
  // 创建OpenGL纹理
  GL_CALL(glGenTextures(1, &layer.texture));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, layer.texture));
  
  // 设置纹理参数
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  
  // 上传纹理数据
  if (channels == 3) {
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data));
  } else if (channels == 4) {
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
  } else {
    GL_CALL(glDeleteTextures(1, &layer.texture));
    layer.texture = 0;
    stbi_image_free(data);
    return;
  }
  
  GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
  stbi_image_free(data);
  
  layer.texture_loaded = true;
}

void BlendFilter::UnloadLayerTexture(BlendLayer& layer) {
  if (layer.texture_loaded && layer.texture != 0) {
    GL_CALL(glDeleteTextures(1, &layer.texture));
    layer.texture = 0;
    layer.texture_loaded = false;
  }
}

}  // namespace gpupixel
