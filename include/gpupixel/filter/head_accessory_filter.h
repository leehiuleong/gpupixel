/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "gpupixel/filter/filter.h"

namespace gpupixel {
class SourceImage;

enum class HeadAccessoryLocation {
  LEFT,    // 左眉毛最左侧
  MIDDLE,  // 眉心
  RIGHT    // 右眉毛最右侧
};

class GPUPIXEL_API HeadAccessoryFilter : public Filter {
 public:
  static std::shared_ptr<HeadAccessoryFilter> Create();
  ~HeadAccessoryFilter();
  virtual bool Init();
  virtual bool DoRender(bool updateSinks = true) override;

  // 设置图片纹理
  void SetImageTexture(std::shared_ptr<SourceImage> texture);

  // 设置人脸关键点（归一化坐标 0-1）
  void SetFaceLandmarks(std::vector<float> landmarks);

  // 设置透明度 [0.0, 1.0]
  void SetOpacity(float opacity) { opacity_ = opacity; }

  // 设置混合强度 [0.0, 1.0]
  void SetBlendLevel(float level) { blend_level_ = level; }

  // 设置头顶偏移量（相对于头部宽度，默认0.15）
  void SetHeadOffset(float offset) { head_offset_ = offset; }

  // 设置头饰位置（left/middle/right，默认middle）
  void SetLocation(HeadAccessoryLocation location) { location_ = location; }

 protected:
  HeadAccessoryFilter();

 private:
  // 人脸关键点（OpenGL 坐标 -1 到 1）
  std::vector<float> face_landmarks_;
  bool has_face_ = false;

  // 图片纹理
  std::shared_ptr<SourceImage> image_texture_;

  // 渲染参数
  float opacity_ = 1.0f;
  float blend_level_ = 1.0f;
  float head_offset_ = 0.15f;  // 头顶偏移量（相对于头部宽度）
  HeadAccessoryLocation location_ = HeadAccessoryLocation::MIDDLE;  // 头饰位置

  // OpenGL 程序
  GPUPixelGLProgram* filter_program2_ = nullptr;
  uint32_t position_attribute2_ = 0;
  uint32_t tex_coord_attribute2_ = 0;
  
  uint32_t position_attribute_ = 0;
  uint32_t tex_coord_attribute_ = 0;
};
}  // namespace gpupixel

