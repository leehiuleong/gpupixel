/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/image_overlay_filter.h"
#include "core/gpupixel_context.h"
#include "core/gpupixel_gl_include.h"
#include "gpupixel/source/source_image.h"
#include "utils/util.h"

namespace gpupixel {

#if defined(GPUPIXEL_GLES_SHADER)
const std::string kImageOverlayVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;  // 叠加图片的纹理坐标 (0-1)
    varying vec2 screenCoordinate;   // 屏幕坐标（用于采样原图，从 -1 到 1 转换为 0 到 1）

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;  // 叠加图片纹理坐标
      // 将屏幕坐标转换为纹理坐标 (position.xy 从 -1 到 1，转换为 0 到 1)
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kImageOverlayFragmentShaderString = R"(
    precision mediump float;
    varying vec2 textureCoordinate;  // 叠加图片纹理坐标
    varying vec2 screenCoordinate;   // 屏幕坐标（用于采样原图）
    uniform sampler2D inputImageTexture;  // 原图
    uniform sampler2D overlayTexture;     // 叠加图片
    uniform float opacity;
    uniform float blendLevel;

    void main() {
      // 使用叠加图片的纹理坐标采样叠加图片
      vec4 overlayColor = texture2D(overlayTexture, textureCoordinate);
      
      // 使用屏幕坐标采样原图
      vec4 bgColor = texture2D(inputImageTexture, screenCoordinate);
      
      // 如果叠加图片是透明的，直接输出原图
      if (overlayColor.a == 0.0) {
        gl_FragColor = bgColor;
        return;
      }

      // 调整颜色通道顺序：RGBA -> BGRA（交换 R 和 B 通道）
      overlayColor = vec4(overlayColor.b, overlayColor.g, overlayColor.r, overlayColor.a);

      // 应用混合强度和透明度
      overlayColor.rgb = overlayColor.rgb * blendLevel;
      float alpha = overlayColor.a * opacity;
      
      // 在片段着色器中混合，避免 OpenGL 混合导致的偏色
      vec3 color = mix(bgColor.rgb, overlayColor.rgb, alpha);
      
      gl_FragColor = vec4(color, bgColor.a);
    })";
#elif defined(GPUPIXEL_GL_SHADER)
const std::string kImageOverlayVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;  // 叠加图片的纹理坐标 (0-1)
    varying vec2 screenCoordinate;   // 屏幕坐标（用于采样原图，从 -1 到 1 转换为 0 到 1）

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;  // 叠加图片纹理坐标
      // 将屏幕坐标转换为纹理坐标 (position.xy 从 -1 到 1，转换为 0 到 1)
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kImageOverlayFragmentShaderString = R"(
    varying vec2 textureCoordinate;  // 叠加图片纹理坐标
    varying vec2 screenCoordinate;   // 屏幕坐标（用于采样原图）
    uniform sampler2D inputImageTexture;  // 原图
    uniform sampler2D overlayTexture;     // 叠加图片
    uniform float opacity;
    uniform float blendLevel;

    void main() {
      // 使用叠加图片的纹理坐标采样叠加图片
      vec4 overlayColor = texture2D(overlayTexture, textureCoordinate);
      
      // 使用屏幕坐标采样原图
      vec4 bgColor = texture2D(inputImageTexture, screenCoordinate);
      
      // 如果叠加图片是透明的，直接输出原图
      if (overlayColor.a == 0.0) {
        gl_FragColor = bgColor;
        return;
      }

      // 调整颜色通道顺序：RGBA -> BGRA（交换 R 和 B 通道）
      overlayColor = vec4(overlayColor.b, overlayColor.g, overlayColor.r, overlayColor.a);

      // 应用混合强度和透明度
      overlayColor.rgb = overlayColor.rgb * blendLevel;
      float alpha = overlayColor.a * opacity;
      
      // 在片段着色器中混合，避免 OpenGL 混合导致的偏色
      vec3 color = mix(bgColor.rgb, overlayColor.rgb, alpha);
      
      gl_FragColor = vec4(color, bgColor.a);
    })";
#endif

ImageOverlayFilter::ImageOverlayFilter() {}

ImageOverlayFilter::~ImageOverlayFilter() {
  if (filter_program2_) {
    delete filter_program2_;
    filter_program2_ = nullptr;
  }
}

std::shared_ptr<ImageOverlayFilter> ImageOverlayFilter::Create() {
  auto ret = std::shared_ptr<ImageOverlayFilter>(new ImageOverlayFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool ImageOverlayFilter::Init() {
  // 创建叠加图片的着色器程序
  if (!Filter::InitWithShaderString(kImageOverlayVertexShaderString,
                                    kImageOverlayFragmentShaderString)) {
    return false;
  }

  // 获取叠加图片着色器的属性位置
  position_attribute_ = filter_program_->GetAttribLocation("position");
  tex_coord_attribute_ = filter_program_->GetAttribLocation("inputTextureCoordinate");

  // 创建原图渲染的着色器程序（使用默认着色器）
  filter_program2_ = GPUPixelGLProgram::CreateWithShaderString(
      kDefaultVertexShader, kDefaultFragmentShader);
  position_attribute2_ = filter_program2_->GetAttribLocation("position");
  tex_coord_attribute2_ = filter_program2_->GetAttribLocation("inputTextureCoordinate");

  // 注册属性
  RegisterProperty("opacity", 1.0f,
                   "The opacity of overlay image with range between 0 and 1.",
                   [this](float& val) { SetOpacity(val); });

  RegisterProperty("blend_level", 1.0f,
                   "The blend level of overlay image with range between 0 and 1.",
                   [this](float& val) { SetBlendLevel(val); });

  std::vector<float> default_landmarks;
  RegisterProperty("face_landmark", default_landmarks,
                   "The face landmark for positioning overlay image.",
                   [this](std::vector<float> val) { SetFaceLandmarks(val); });

  return true;
}

void ImageOverlayFilter::SetImageTexture(std::shared_ptr<SourceImage> texture) {
  image_texture_ = texture;
}

void ImageOverlayFilter::SetFixedPosition(float x, float y, float width, float height) {
  fixed_x_ = x;
  fixed_y_ = y;
  fixed_width_ = width;
  fixed_height_ = height;
  position_mode_ = ImageOverlayPositionMode::FIXED;
}

void ImageOverlayFilter::SetFaceLandmarks(std::vector<float> landmarks) {
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
  position_mode_ = ImageOverlayPositionMode::FACE_LANDMARK;
}

bool ImageOverlayFilter::DoRender(bool updateSinks) {
  if (!image_texture_) {
    // 如果没有设置图片，直接渲染原图
    return Filter::DoRender(updateSinks);
  }

  framebuffer_->Activate();
  
  // 第一步：使用默认着色器渲染原图
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

  // 第二步：使用叠加着色器渲染叠加图片
  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);

  // 渲染叠加图片
  std::vector<float> overlayVertices;
  std::vector<float> overlayTexCoords;

  if (position_mode_ == ImageOverlayPositionMode::FIXED) {
    // 固定位置模式：使用归一化坐标转换为 OpenGL 坐标
    float x1 = fixed_x_ * 2.0f - 1.0f;
    float y1 = fixed_y_ * 2.0f - 1.0f;
    float x2 = (fixed_x_ + fixed_width_) * 2.0f - 1.0f;
    float y2 = (fixed_y_ + fixed_height_) * 2.0f - 1.0f;

    // 矩形顶点（两个三角形）
    overlayVertices = {x1, y1, x2, y1, x1, y2, x2, y2};
    overlayTexCoords = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  } else if (position_mode_ == ImageOverlayPositionMode::FACE_LANDMARK && has_face_) {
    // 人脸关键点模式：使用关键点构建矩形
    // 这里使用关键点 0, 16, 32 来定义矩形（可以根据需要调整）
    if (face_landmarks_.size() >= 66) {  // 至少需要 33 个点（66 个 float）
      // 使用脸部轮廓点来定义位置
      // 点 0: 左脸边缘 (x=face_landmarks_[0], y=face_landmarks_[1])
      // 点 16: 下巴中心 (x=face_landmarks_[32], y=face_landmarks_[33])
      // 点 32: 右脸边缘 (x=face_landmarks_[64], y=face_landmarks_[65])
      float left_x = face_landmarks_[0];   // 点 0 的 x
      float right_x = face_landmarks_[64]; // 点 32 的 x (索引 32*2)
      float bottom_y = face_landmarks_[33]; // 点 16 的 y (索引 16*2+1)
      
      // 计算顶部（使用点 0 的 y 作为顶部，或使用眉毛位置）
      float top_y = face_landmarks_[1];  // 点 0 的 y
      // 如果有更多点，可以使用眉毛位置（点 33-37 是左眉毛，点 38-42 是右眉毛）
      if (face_landmarks_.size() >= 70) {
        // 使用左眉毛和右眉毛的平均 y 值
        float left_eyebrow_y = face_landmarks_[67];  // 点 33 的 y (索引 33*2+1)
        float right_eyebrow_y = face_landmarks_[77]; // 点 38 的 y (索引 38*2+1)
        top_y = (left_eyebrow_y + right_eyebrow_y) / 2.0f;
      }

      float x1 = left_x;
      float y1 = top_y;
      float x2 = right_x;
      float y2 = bottom_y;

      overlayVertices = {x1, y1, x2, y1, x1, y2, x2, y2};
      overlayTexCoords = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
  }

  if (!overlayVertices.empty()) {
    // 绑定原图纹理（用于在片段着色器中采样）
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D,
                          input_framebuffers_[0].frame_buffer->GetTexture()));
    filter_program_->SetUniformValue("inputImageTexture", 0);

    // 绑定叠加图片纹理
    GL_CALL(glActiveTexture(GL_TEXTURE1));
    GL_CALL(glBindTexture(GL_TEXTURE_2D,
                          image_texture_->GetFramebuffer()->GetTexture()));
    filter_program_->SetUniformValue("overlayTexture", 1);

    // 设置 Uniform
    filter_program_->SetUniformValue("opacity", opacity_);
    filter_program_->SetUniformValue("blendLevel", blend_level_);

    // 设置顶点（使用叠加图片的位置）
    GL_CALL(glEnableVertexAttribArray(position_attribute_));
    GL_CALL(glVertexAttribPointer(position_attribute_, 2, GL_FLOAT, 0, 0,
                                  overlayVertices.data()));

    // 设置纹理坐标（叠加图片的纹理坐标）
    GL_CALL(glEnableVertexAttribArray(tex_coord_attribute_));
    GL_CALL(glVertexAttribPointer(tex_coord_attribute_, 2, GL_FLOAT, 0, 0,
                                  overlayTexCoords.data()));

    // 绘制叠加图片（两个三角形组成矩形）
    // 注意：不需要启用 OpenGL 混合，因为混合在片段着色器中完成
    static const uint32_t indices[] = {0, 1, 2, 1, 2, 3};
    GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices));
  }

  framebuffer_->Deactivate();
  return Source::DoRender(updateSinks);
}

}  // namespace gpupixel

