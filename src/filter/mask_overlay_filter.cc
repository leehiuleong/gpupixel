/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/mask_overlay_filter.h"
#include "core/gpupixel_context.h"
#include "core/gpupixel_gl_include.h"
#include "gpupixel/source/source_image.h"
#include "utils/util.h"

namespace gpupixel {

#if defined(GPUPIXEL_GLES_SHADER)
const std::string kMaskOverlayVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kMaskOverlayFragmentShaderString = R"(
    precision mediump float;
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;
    uniform sampler2D inputImageTexture;
    uniform sampler2D overlayTexture;
    uniform float opacity;
    uniform float blendLevel;

    void main() {
      vec4 overlayColor = texture2D(overlayTexture, textureCoordinate);
      vec4 bgColor = texture2D(inputImageTexture, screenCoordinate);
      
      if (overlayColor.a == 0.0) {
        gl_FragColor = bgColor;
        return;
      }

      // 调整颜色通道顺序：RGBA -> BGRA
      overlayColor = vec4(overlayColor.b, overlayColor.g, overlayColor.r, overlayColor.a);
      overlayColor.rgb = overlayColor.rgb * blendLevel;
      float alpha = overlayColor.a * opacity;
      
      vec3 color = mix(bgColor.rgb, overlayColor.rgb, alpha);
      gl_FragColor = vec4(color, bgColor.a);
    })";
#elif defined(GPUPIXEL_GL_SHADER)
const std::string kMaskOverlayVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kMaskOverlayFragmentShaderString = R"(
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;
    uniform sampler2D inputImageTexture;
    uniform sampler2D overlayTexture;
    uniform float opacity;
    uniform float blendLevel;

    void main() {
      vec4 overlayColor = texture2D(overlayTexture, textureCoordinate);
      vec4 bgColor = texture2D(inputImageTexture, screenCoordinate);
      
      if (overlayColor.a == 0.0) {
        gl_FragColor = bgColor;
        return;
      }

      // 调整颜色通道顺序：RGBA -> BGRA
      overlayColor = vec4(overlayColor.b, overlayColor.g, overlayColor.r, overlayColor.a);
      overlayColor.rgb = overlayColor.rgb * blendLevel;
      float alpha = overlayColor.a * opacity;
      
      vec3 color = mix(bgColor.rgb, overlayColor.rgb, alpha);
      gl_FragColor = vec4(color, bgColor.a);
    })";
#endif

MaskOverlayFilter::MaskOverlayFilter() {}

MaskOverlayFilter::~MaskOverlayFilter() {
  if (filter_program2_) {
    delete filter_program2_;
    filter_program2_ = nullptr;
  }
}

std::shared_ptr<MaskOverlayFilter> MaskOverlayFilter::Create() {
  auto ret = std::shared_ptr<MaskOverlayFilter>(new MaskOverlayFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool MaskOverlayFilter::Init() {
  if (!Filter::InitWithShaderString(kMaskOverlayVertexShaderString,
                                    kMaskOverlayFragmentShaderString)) {
    return false;
  }

  position_attribute_ = filter_program_->GetAttribLocation("position");
  tex_coord_attribute_ = filter_program_->GetAttribLocation("inputTextureCoordinate");

  filter_program2_ = GPUPixelGLProgram::CreateWithShaderString(
      kDefaultVertexShader, kDefaultFragmentShader);
  position_attribute2_ = filter_program2_->GetAttribLocation("position");
  tex_coord_attribute2_ = filter_program2_->GetAttribLocation("inputTextureCoordinate");

  RegisterProperty("opacity", 1.0f,
                   "The opacity of overlay image with range between 0 and 1.",
                   [this](float& val) { SetOpacity(val); });

  RegisterProperty("blend_level", 1.0f,
                   "The blend level of overlay image with range between 0 and 1.",
                   [this](float& val) { SetBlendLevel(val); });

  std::vector<float> default_landmarks;
  RegisterProperty("face_landmark", default_landmarks,
                   "The face landmark for mask overlay.",
                   [this](std::vector<float> val) { SetFaceLandmarks(val); });

  return true;
}

void MaskOverlayFilter::SetImageTexture(std::shared_ptr<SourceImage> texture) {
  image_texture_ = texture;
}

void MaskOverlayFilter::SetFaceLandmarks(std::vector<float> landmarks) {
  if (landmarks.size() == 0) {
    has_face_ = false;
    return;
  }
  
  // 将归一化坐标（0-1）转换为 OpenGL 坐标（-1 到 1）
  std::vector<float> vect;
  for (auto it : landmarks) {
    vect.push_back(2 * it - 1);
  }
  face_landmarks_ = vect;
  has_face_ = true;
  BuildMaskMesh();
}

void MaskOverlayFilter::BuildMaskMesh() {
  mask_vertices_.clear();
  mask_tex_coords_.clear();
  mask_indices_.clear();

  if (!has_face_ || face_landmarks_.size() < 62) {  // 至少需要31个点（点2到30，索引2*2到30*2+1）
    return;
  }

  // 点2到30构成闭合路径（共29个点）
  // 使用三角扇（Triangle Fan）从中心点构建三角形
  // 中心点：计算点2到30的质心
  float center_x = 0.0f, center_y = 0.0f;
  int point_count = 29;  // 点2到30共29个点
  for (int i = 0; i < point_count; i++) {
    int point_idx = 2 + i;  // 点索引从2开始
    center_x += face_landmarks_[point_idx * 2];
    center_y += face_landmarks_[point_idx * 2 + 1];
  }
  center_x /= point_count;
  center_y /= point_count;

  // 添加中心点
  mask_vertices_.push_back(center_x);
  mask_vertices_.push_back(center_y);
  mask_tex_coords_.push_back(0.5f);
  mask_tex_coords_.push_back(0.5f);

  // 添加点2到30的顶点
  for (int i = 0; i < point_count; i++) {
    int point_idx = 2 + i;
    mask_vertices_.push_back(face_landmarks_[point_idx * 2]);
    mask_vertices_.push_back(face_landmarks_[point_idx * 2 + 1]);
    
    // 计算纹理坐标（基于点的位置）
    float u = (face_landmarks_[point_idx * 2] + 1.0f) * 0.5f;
    float v = (face_landmarks_[point_idx * 2 + 1] + 1.0f) * 0.5f;
    mask_tex_coords_.push_back(u);
    mask_tex_coords_.push_back(v);
  }

  // 构建三角形索引（三角扇）
  uint32_t center_idx = 0;
  for (int i = 0; i < point_count; i++) {
    uint32_t idx1 = center_idx;
    uint32_t idx2 = 1 + i;
    uint32_t idx3 = 1 + ((i + 1) % point_count);
    
    mask_indices_.push_back(idx1);
    mask_indices_.push_back(idx2);
    mask_indices_.push_back(idx3);
  }
}

bool MaskOverlayFilter::DoRender(bool updateSinks) {
  if (!image_texture_ || !has_face_) {
    return Filter::DoRender(updateSinks);
  }

  framebuffer_->Activate();
  
  // 第一步：渲染原图
  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program2_);
  GL_CALL(glClearColor(background_color_.r, background_color_.g,
                       background_color_.b, background_color_.a));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

  static const float imageVertices[] = {
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
  };

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        input_framebuffers_[0].frame_buffer->GetTexture()));
  filter_program2_->SetUniformValue("inputImageTexture", 0);

  GL_CALL(glEnableVertexAttribArray(position_attribute2_));
  GL_CALL(glVertexAttribPointer(position_attribute2_, 2, GL_FLOAT, 0, 0, imageVertices));

  GL_CALL(glEnableVertexAttribArray(tex_coord_attribute2_));
  GL_CALL(glVertexAttribPointer(tex_coord_attribute2_, 2, GL_FLOAT, 0, 0,
                                GetTextureCoordinate(NoRotation)));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  // 第二步：渲染面罩
  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        input_framebuffers_[0].frame_buffer->GetTexture()));
  filter_program_->SetUniformValue("inputImageTexture", 0);

  GL_CALL(glActiveTexture(GL_TEXTURE1));
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        image_texture_->GetFramebuffer()->GetTexture()));
  filter_program_->SetUniformValue("overlayTexture", 1);

  filter_program_->SetUniformValue("opacity", opacity_);
  filter_program_->SetUniformValue("blendLevel", blend_level_);

  GL_CALL(glEnableVertexAttribArray(position_attribute_));
  GL_CALL(glVertexAttribPointer(position_attribute_, 2, GL_FLOAT, 0, 0,
                                mask_vertices_.data()));

  GL_CALL(glEnableVertexAttribArray(tex_coord_attribute_));
  GL_CALL(glVertexAttribPointer(tex_coord_attribute_, 2, GL_FLOAT, 0, 0,
                                mask_tex_coords_.data()));

  GL_CALL(glDrawElements(GL_TRIANGLES, (GLsizei)mask_indices_.size(), GL_UNSIGNED_INT,
                         mask_indices_.data()));

  framebuffer_->Deactivate();
  return Source::DoRender(updateSinks);
}

}  // namespace gpupixel

