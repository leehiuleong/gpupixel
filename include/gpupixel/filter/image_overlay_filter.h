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

// 位置模式枚举
enum class ImageOverlayPositionMode {
  FIXED = 0,      // 固定位置模式
  FACE_LANDMARK   // 人脸关键点模式
};

class GPUPIXEL_API ImageOverlayFilter : public Filter {
 public:
  static std::shared_ptr<ImageOverlayFilter> Create();
  ~ImageOverlayFilter();
  virtual bool Init();
  virtual bool DoRender(bool updateSinks = true) override;

  // 设置图片纹理
  void SetImageTexture(std::shared_ptr<SourceImage> texture);

  // 设置位置模式
  void SetPositionMode(ImageOverlayPositionMode mode) { position_mode_ = mode; }

  // 固定位置模式：设置图片在画面中的位置和大小（归一化坐标 0-1）
  void SetFixedPosition(float x, float y, float width, float height);

  // 人脸关键点模式：设置人脸关键点
  void SetFaceLandmarks(std::vector<float> landmarks);

  // 设置透明度 [0.0, 1.0]
  void SetOpacity(float opacity) { opacity_ = opacity; }

  // 设置混合强度 [0.0, 1.0]
  void SetBlendLevel(float level) { blend_level_ = level; }

 protected:
  ImageOverlayFilter();

 private:
  // 位置模式
  ImageOverlayPositionMode position_mode_ = ImageOverlayPositionMode::FIXED;

  // 固定位置参数（归一化坐标 0-1）
  float fixed_x_ = 0.0f;
  float fixed_y_ = 0.0f;
  float fixed_width_ = 0.5f;
  float fixed_height_ = 0.5f;

  // 人脸关键点（OpenGL 坐标 -1 到 1）
  std::vector<float> face_landmarks_;
  bool has_face_ = false;

  // 图片纹理
  std::shared_ptr<SourceImage> image_texture_;

  // 渲染参数
  float opacity_ = 1.0f;
  float blend_level_ = 1.0f;

  // OpenGL 程序
  GPUPixelGLProgram* filter_program2_ = nullptr;  // 用于渲染原图（使用默认着色器）
  uint32_t position_attribute2_ = 0;
  uint32_t tex_coord_attribute2_ = 0;
  
  // 叠加图片着色器程序的属性
  uint32_t position_attribute_ = 0;
  uint32_t tex_coord_attribute_ = 0;
};
}  // namespace gpupixel

