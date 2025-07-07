/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/portrait_seg_filter.h"
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "core/gpupixel_context.h"
#include "core/gpupixel_gl_include.h"

namespace gpupixel {

const std::string kPortraitSegVertexShaderString = R"(
    attribute vec2 position;
    attribute vec4 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        textureCoordinate = inputTextureCoordinate.xy;
    })";

const std::string kPortraitSegFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    uniform sampler2D maskTexture;
    uniform sampler2D backgroundTexture;
    uniform vec2 texelSize;
    uniform float blurRadius;
    uniform bool hasBackgroundImage;
    varying vec2 textureCoordinate;
    
    void main() {
        // 获取 mask 值 (从 GL_RED 纹理的 r 通道)
        float maskValue = texture2D(maskTexture, textureCoordinate).r;
        
        // 获取原图像颜色
        vec4 originalColor = texture2D(inputImageTexture, textureCoordinate);
        
        vec4 backgroundColor;
        
        // 判断是否有背景图片
        if (hasBackgroundImage) {
            // 使用背景图片
            backgroundColor = texture2D(backgroundTexture, textureCoordinate);
        } else {
            // 使用模糊背景
            // 优化的十字形采样模糊 - 性能提升10-50倍
            backgroundColor = originalColor; // 中心点
            float totalWeight = 1.0;
            
            // 只采样十字形方向，大幅减少采样次数
            // 从 O(r²) 降到 O(r)，性能提升巨大
            for (float i = 1.0; i <= blurRadius; i += 1.0) {
                float weight = 1.0 / (1.0 + i * 0.3); // 距离衰减权重
                
                // 水平方向采样
                backgroundColor += texture2D(inputImageTexture, textureCoordinate + vec2(i * texelSize.x, 0.0)) * weight;
                backgroundColor += texture2D(inputImageTexture, textureCoordinate - vec2(i * texelSize.x, 0.0)) * weight;
                
                // 垂直方向采样
                backgroundColor += texture2D(inputImageTexture, textureCoordinate + vec2(0.0, i * texelSize.y)) * weight;
                backgroundColor += texture2D(inputImageTexture, textureCoordinate - vec2(0.0, i * texelSize.y)) * weight;
                
                totalWeight += weight * 4.0;
            }
            
            // 归一化
            backgroundColor /= totalWeight;
        }
        
        // 使用 maskValue 作为权重进行混合
        // maskValue = 1.0 时显示原图像
        // maskValue = 0.0 时显示背景（图片或模糊）
        // maskValue 介于 0-1 之间时按权重混合
        gl_FragColor = mix(backgroundColor, originalColor, maskValue);
    })";

std::shared_ptr<PortraitSegFilter> PortraitSegFilter::Create() {
  auto ret = std::shared_ptr<PortraitSegFilter>(new PortraitSegFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool PortraitSegFilter::Init() {
  if (!InitWithShaderString(kPortraitSegVertexShaderString,
                            kPortraitSegFragmentShaderString)) {
    return false;
  }

  glGenTextures(1, &portrait_seg_mask_);
  glBindTexture(GL_TEXTURE_2D, portrait_seg_mask_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  return true;
}

bool PortraitSegFilter::DoRender(bool updateSinks) {
  // 如果没有设置 mask，就直接渲染原图像
  if (portrait_seg_mask_ == 0) {
    return Filter::DoRender(updateSinks);
  }

  float image_vertices[] = {
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
  };

  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);
  framebuffer_->Activate();
  GL_CALL(glClearColor(background_color_.r, background_color_.g,
                       background_color_.b, background_color_.a));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

  // 设置原图像纹理
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        input_framebuffers_[0].frame_buffer->GetTexture()));
  filter_program_->SetUniformValue("inputImageTexture", 0);

  // 设置 mask 纹理
  GL_CALL(glActiveTexture(GL_TEXTURE0 + 1));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, portrait_seg_mask_));
  filter_program_->SetUniformValue("maskTexture", 1);

  // 设置背景相关参数
  if (background_image_) {
    // 有背景图片时设置背景纹理
    GL_CALL(glActiveTexture(GL_TEXTURE0 + 2));
    GL_CALL(glBindTexture(GL_TEXTURE_2D,
                          background_image_->GetFramebuffer()->GetTexture()));
    filter_program_->SetUniformValue("backgroundTexture", 2);
    filter_program_->SetUniformValue("hasBackgroundImage", true);
  } else {
    // 没有背景图片时使用模糊背景
    filter_program_->SetUniformValue("hasBackgroundImage", false);

    // 设置纹理大小用于模糊计算
    float texel_size_x = 1.0f / framebuffer_->GetWidth();
    float texel_size_y = 1.0f / framebuffer_->GetHeight();
    filter_program_->SetUniformValue("texelSize",
                                     glm::vec2(texel_size_x, texel_size_y));

    // 设置模糊半径
    filter_program_->SetUniformValue("blurRadius", blur_radius_);
  }

  // texcoord attribute
  uint32_t filter_tex_coord_attribute =
      filter_program_->GetAttribLocation("inputTextureCoordinate");
  GL_CALL(glEnableVertexAttribArray(filter_tex_coord_attribute));
  GL_CALL(glVertexAttribPointer(filter_tex_coord_attribute, 2, GL_FLOAT, 0, 0,
                                GetTextureCoordinate(NoRotation)));

  GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                image_vertices));
  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  framebuffer_->Deactivate();

  return Source::DoRender(updateSinks);
}

void PortraitSegFilter::SetPortraitSegMask(const uint8_t* mask,
                                           int width,
                                           int height) {
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    glBindTexture(GL_TEXTURE_2D, portrait_seg_mask_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, mask);
  });
}

void PortraitSegFilter::SetBackgroundImage(std::shared_ptr<SourceImage> image) {
  background_image_ = image;
}
}  // namespace gpupixel
