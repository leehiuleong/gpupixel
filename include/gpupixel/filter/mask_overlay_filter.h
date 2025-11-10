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

class GPUPIXEL_API MaskOverlayFilter : public Filter {
 public:
  static std::shared_ptr<MaskOverlayFilter> Create();
  ~MaskOverlayFilter();
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

 protected:
  MaskOverlayFilter();

 private:
  // 构建从点2到30的闭合路径的三角形网格
  void BuildMaskMesh();
  
  // 人脸关键点（OpenGL 坐标 -1 到 1）
  std::vector<float> face_landmarks_;
  bool has_face_ = false;

  // 图片纹理
  std::shared_ptr<SourceImage> image_texture_;

  // 渲染参数
  float opacity_ = 1.0f;
  float blend_level_ = 1.0f;

  // 三角形网格顶点和索引
  std::vector<float> mask_vertices_;
  std::vector<float> mask_tex_coords_;
  std::vector<uint32_t> mask_indices_;

  // OpenGL 程序
  GPUPixelGLProgram* filter_program2_ = nullptr;  // 用于渲染原图
  uint32_t position_attribute2_ = 0;
  uint32_t tex_coord_attribute2_ = 0;
  
  // 叠加图片着色器程序的属性
  uint32_t position_attribute_ = 0;
  uint32_t tex_coord_attribute_ = 0;
};
}  // namespace gpupixel

