/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/head_accessory_filter.h"
#include "core/gpupixel_context.h"
#include "core/gpupixel_gl_include.h"
#include "gpupixel/source/source_image.h"
#include "utils/util.h"
#include <cmath>

namespace gpupixel {

#if defined(GPUPIXEL_GLES_SHADER)
const std::string kHeadAccessoryVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kHeadAccessoryFragmentShaderString = R"(
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
const std::string kHeadAccessoryVertexShaderString = R"(
    attribute vec3 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    varying vec2 screenCoordinate;

    void main(void) {
      gl_Position = vec4(position, 1.);
      textureCoordinate = inputTextureCoordinate;
      screenCoordinate = position.xy * 0.5 + 0.5;
    })";

const std::string kHeadAccessoryFragmentShaderString = R"(
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

HeadAccessoryFilter::HeadAccessoryFilter() {}

HeadAccessoryFilter::~HeadAccessoryFilter() {
  if (filter_program2_) {
    delete filter_program2_;
    filter_program2_ = nullptr;
  }
}

std::shared_ptr<HeadAccessoryFilter> HeadAccessoryFilter::Create() {
  auto ret = std::shared_ptr<HeadAccessoryFilter>(new HeadAccessoryFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool HeadAccessoryFilter::Init() {
  if (!Filter::InitWithShaderString(kHeadAccessoryVertexShaderString,
                                    kHeadAccessoryFragmentShaderString)) {
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
                   "The face landmark for head accessory.",
                   [this](std::vector<float> val) { SetFaceLandmarks(val); });

  RegisterProperty("head_offset", 0.15f,
                   "The offset from eyebrow to head top (relative to head width).",
                   [this](float& val) { SetHeadOffset(val); });

  RegisterProperty("location", 1,  // 1 = MIDDLE (默认值)
                   "The location of head accessory: 0=LEFT, 1=MIDDLE, 2=RIGHT.",
                   [this](int& val) {
                     if (val == 0) {
                       SetLocation(HeadAccessoryLocation::LEFT);
                     } else if (val == 1) {
                       SetLocation(HeadAccessoryLocation::MIDDLE);
                     } else if (val == 2) {
                       SetLocation(HeadAccessoryLocation::RIGHT);
                     }
                   });

  return true;
}

void HeadAccessoryFilter::SetImageTexture(std::shared_ptr<SourceImage> texture) {
  image_texture_ = texture;
}

void HeadAccessoryFilter::SetFaceLandmarks(std::vector<float> landmarks) {
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

bool HeadAccessoryFilter::DoRender(bool updateSinks) {
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

  // 第二步：渲染头饰
  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);

  // 至少需要44个点（点43需要索引43*2+1=87，点16需要索引16*2+1=33）
  if (face_landmarks_.size() < 88) {
    framebuffer_->Deactivate();
    return Source::DoRender(updateSinks);
  }

  // 计算头部宽度：点0（左脸边缘）和点32（右脸边缘）的x坐标差
  float left_face_x = face_landmarks_[0];    // 点0的x (索引0*2=0)
  float right_face_x = face_landmarks_[64]; // 点32的x (索引32*2=64)
  float head_width = std::abs(right_face_x - left_face_x);

  // 获取关键点坐标
  float left_eyebrow_x = face_landmarks_[66];   // 点33的x (索引33*2=66) - 左眉毛最左侧
  float middle_brow_x = face_landmarks_[86];    // 点43的x (索引43*2=86) - 眉心
  float middle_brow_y = face_landmarks_[87];    // 点43的y (索引43*2+1=87) - 眉心
  float right_eyebrow_x = face_landmarks_[84];  // 点42的x (索引42*2=84) - 右眉毛最右侧
  float chin_y = face_landmarks_[33];           // 点16的y (索引16*2+1=33) - 下巴中心

  // 根据位置枚举选择x坐标
  float head_accessory_x = 0.0f;
  switch (location_) {
    case HeadAccessoryLocation::LEFT:
      head_accessory_x = left_eyebrow_x;
      break;
    case HeadAccessoryLocation::MIDDLE:
      head_accessory_x = middle_brow_x;
      break;
    case HeadAccessoryLocation::RIGHT:
      head_accessory_x = right_eyebrow_x;
      break;
  }

  // 计算y坐标：距离眉心的y轴距离 = 眉心到下巴的距离的0.7倍
  float brow_to_chin_distance = std::abs(middle_brow_y - chin_y);
  float y_offset = brow_to_chin_distance * 0.7f;
  float head_accessory_y = middle_brow_y - y_offset;  // 向上偏移

  // 计算图片宽度：头部宽度的0.4倍
  float image_width = head_width * 0.4f;

  // 根据图片宽高比计算高度
  int texture_width = image_texture_->GetWidth();
  int texture_height = image_texture_->GetHeight();
  float aspect_ratio = 1.0f;
  if (texture_width > 0 && texture_height > 0) {
    aspect_ratio = static_cast<float>(texture_width) / static_cast<float>(texture_height);
  }
  float image_height = image_width / aspect_ratio;

  // 构建矩形顶点
  float half_width = image_width * 0.5f;
  float half_height = image_height * 0.5f;

  float x1 = head_accessory_x - half_width;
  float y1 = head_accessory_y - half_height;
  float x2 = head_accessory_x + half_width;
  float y2 = head_accessory_y + half_height;

  std::vector<float> overlayVertices = {x1, y1, x2, y1, x1, y2, x2, y2};
  std::vector<float> overlayTexCoords = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

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

