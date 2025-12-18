/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "gpupixel/filter/filter.h"
#include "gpupixel/gpupixel_define.h"
#include <vector>
#include <string>

namespace gpupixel {

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
  std::string image_path;    // 图片路径
  uint32_t texture;          // OpenGL纹理ID
  bool texture_loaded;       // 纹理是否已加载
  BlendMode blend_mode;      // 混合模式
  float opacity;            // 不透明度 (0.0 - 1.0)
  float intensity;          // 强度 (0.0 - 1.0)
  bool enabled;             // 是否启用
  
  BlendLayer() : texture(0), texture_loaded(false), blend_mode(BlendMode::NORMAL), 
                 opacity(1.0f), intensity(1.0f), enabled(true) {}
};

class GPUPIXEL_API BlendFilter : public Filter {
 public:
  static std::shared_ptr<BlendFilter> Create();
  ~BlendFilter();
  virtual bool Init();
  virtual bool DoRender(bool updateSinks = true) override;
  
  // 图层管理
  int AddLayer(const std::string& imagePath, BlendMode blendMode = BlendMode::MULTIPLY, 
               float opacity = 1.0f, float intensity = 1.0f);
  void RemoveLayer(int layerIndex);
  void RemoveAllLayers();
  int GetLayerCount() const { return layers_.size(); }
  
  // 图层属性设置
  void SetLayerImage(int layerIndex, const std::string& imagePath);
  void SetLayerBlendMode(int layerIndex, BlendMode blendMode);
  void SetLayerOpacity(int layerIndex, float opacity);
  void SetLayerIntensity(int layerIndex, float intensity);
  void SetLayerEnabled(int layerIndex, bool enabled);
  
  // 全局设置
  void SetGlobalOpacity(float opacity);
  void SetGlobalIntensity(float intensity);
  
  // 属性访问
  BlendLayer* GetLayer(int layerIndex);
  const BlendLayer* GetLayer(int layerIndex) const;

 protected:
  BlendFilter();
  
  void LoadLayerTexture(BlendLayer& layer);
  void UnloadLayerTexture(BlendLayer& layer);
  std::string GenerateFragmentShader() const;
  
 private:
  std::vector<BlendLayer> layers_;
  float global_opacity_;
  float global_intensity_;
  bool shader_needs_update_;
};

}  // namespace gpupixel

