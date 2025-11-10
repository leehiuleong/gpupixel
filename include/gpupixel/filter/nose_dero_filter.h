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

class GPUPIXEL_API NoseDeroFilter : public Filter {
 public:
  static std::shared_ptr<NoseDeroFilter> Create();
  ~NoseDeroFilter();
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

  // 设置图片大小（归一化坐标，相对于人脸大小）
  void SetImageSize(float width, float height) {
    image_width_ = width;
    image_height_ = height;
  }

 protected:
  NoseDeroFilter();

 private:
  // 人脸关键点（OpenGL 坐标 -1 到 1）
  std::vector<float> face_landmarks_;
  bool has_face_ = false;

  // 图片纹理
  std::shared_ptr<SourceImage> image_texture_;

  // 渲染参数
  float opacity_ = 1.0f;
  float blend_level_ = 1.0f;
  float image_width_ = 0.2f;   // 默认宽度
  float image_height_ = 0.2f;  // 默认高度

  // OpenGL 程序
  GPUPixelGLProgram* filter_program2_ = nullptr;
  uint32_t position_attribute2_ = 0;
  uint32_t tex_coord_attribute2_ = 0;
  
  uint32_t position_attribute_ = 0;
  uint32_t tex_coord_attribute_ = 0;
};
}  // namespace gpupixel

