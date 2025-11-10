/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/eye_dero_filter.h"
#include "core/gpupixel_context.h"
#include "core/gpupixel_gl_include.h"
#include "gpupixel/source/source_image.h"
#include "utils/util.h"

namespace gpupixel {

#if defined(GPUPIXEL_GLES_SHADER)
const std::string kEyeDeroVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kEyeDeroFragmentShaderString = R"(
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

      overlayColor = vec4(overlayColor.b, overlayColor.g, overlayColor.r, overlayColor.a);
      overlayColor.rgb = overlayColor.rgb * blendLevel;
      float alpha = overlayColor.a * opacity;
      
      vec3 color = mix(bgColor.rgb, overlayColor.rgb, alpha);
      gl_FragColor = vec4(color, bgColor.a);
    })";
#elif defined(GPUPIXEL_GL_SHADER)
const std::string kEyeDeroVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kEyeDeroFragmentShaderString = R"(
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

      overlayColor = vec4(overlayColor.b, overlayColor.g, overlayColor.r, overlayColor.a);
      overlayColor.rgb = overlayColor.rgb * blendLevel;
      float alpha = overlayColor.a * opacity;
      
      vec3 color = mix(bgColor.rgb, overlayColor.rgb, alpha);
      gl_FragColor = vec4(color, bgColor.a);
    })";
#endif

EyeDeroFilter::EyeDeroFilter() {}

EyeDeroFilter::~EyeDeroFilter() {
  if (filter_program2_) {
    delete filter_program2_;
    filter_program2_ = nullptr;
  }
}

std::shared_ptr<EyeDeroFilter> EyeDeroFilter::Create() {
  auto ret = std::shared_ptr<EyeDeroFilter>(new EyeDeroFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool EyeDeroFilter::Init() {
  if (!Filter::InitWithShaderString(kEyeDeroVertexShaderString,
                                    kEyeDeroFragmentShaderString)) {
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
                   "The face landmark for eye dero.",
                   [this](std::vector<float> val) { SetFaceLandmarks(val); });

  return true;
}

void EyeDeroFilter::SetImageTexture(std::shared_ptr<SourceImage> texture) {
  image_texture_ = texture;
}

void EyeDeroFilter::SetFaceLandmarks(std::vector<float> landmarks) {
  if (landmarks.size() == 0) {
    has_face_ = false;
    return;
  }
  
  std::vector<float> vect;
  for (auto it : landmarks) {
    vect.push_back(2 * it - 1);
  }
  face_landmarks_ = vect;
  has_face_ = true;
}

bool EyeDeroFilter::DoRender(bool updateSinks) {
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

  // 第二步：渲染眼镜
  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);

  // 点33、32、30、2构成四边形
  // 点33: 索引33*2=66, 33*2+1=67
  // 点32: 索引32*2=64, 32*2+1=65
  // 点30: 索引30*2=60, 30*2+1=61
  // 点2:  索引2*2=4,   2*2+1=5
  if (face_landmarks_.size() < 68) {  // 至少需要34个点
    framebuffer_->Deactivate();
    return Source::DoRender(updateSinks);
  }

  // 获取四个点的坐标
  float x33 = face_landmarks_[66];  // 点33的x
  float y33 = face_landmarks_[67];  // 点33的y
  float x32 = face_landmarks_[64];  // 点32的x
  float y32 = face_landmarks_[65];  // 点32的y
  float x30 = face_landmarks_[58];  // 点29的x
  float y30 = face_landmarks_[59];  // 点29的y
  float x2 = face_landmarks_[4];    // 点2的x
  float y2 = face_landmarks_[5];     // 点2的y

  // 构建四边形顶点（按顺序：左上、右上、左下、右下）
  // 假设点33是左上，点32是右上，点2是左下，点30是右下
  std::vector<float> overlayVertices = {
      x33, y33,  // 左上（点33）
      x32, y32,  // 右上（点32）
      x2,  y2,   // 左下（点2）
      x30, y30   // 右下（点30）
  };
  
  std::vector<float> overlayTexCoords = {
      0.0f, 0.0f,  // 左上
      1.0f, 0.0f,  // 右上
      0.0f, 1.0f,  // 左下
      1.0f, 1.0f   // 右下
  };

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
                                overlayVertices.data()));

  GL_CALL(glEnableVertexAttribArray(tex_coord_attribute_));
  GL_CALL(glVertexAttribPointer(tex_coord_attribute_, 2, GL_FLOAT, 0, 0,
                                overlayTexCoords.data()));

  static const uint32_t indices[] = {0, 1, 2, 1, 2, 3};
  GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices));

  framebuffer_->Deactivate();
  return Source::DoRender(updateSinks);
}

}  // namespace gpupixel

