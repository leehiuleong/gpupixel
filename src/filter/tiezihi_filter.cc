/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/tiezihi_filter.h"
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "core/gpupixel_context.h"

namespace gpupixel {

const std::string kTieZhiVertexShaderString = R"(
    attribute vec2 position;
    attribute vec4 inputTextureCoordinate;
    uniform mat4 model;
    uniform mat4 projection; 
    varying vec2 textureCoordinate;
    void main() {
        gl_Position = projection * model * vec4(position, 0.0, 1.0);
        textureCoordinate = inputTextureCoordinate.xy;
    })";

const std::string kTieZhiFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    varying vec2 textureCoordinate;
    uniform float test;
    void main() {
    if(test > 0.5) {
        gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
    } else {
        gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    })";

std::shared_ptr<TieZhiFilter> TieZhiFilter::Create() {
  auto ret = std::shared_ptr<TieZhiFilter>(new TieZhiFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool TieZhiFilter::Init() {
  if (!InitWithShaderString(kTieZhiVertexShaderString,
                            kTieZhiFragmentShaderString)) {
    return false;
  }

  source_image_ = SourceImage::Create("/Users/admin/Documents/logo.png");

  return true;
}

bool TieZhiFilter::DoRender(bool updateSinks) {
  float image_vertices[] = {
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
  };

  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);
  framebuffer_->Activate();
  GL_CALL(glClearColor(background_color_.r, background_color_.g,
                       background_color_.b, background_color_.a));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

  // 启用混合模式
  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  // 设置project_matrix uniform
  filter_program_->SetUniformValue("projection", glm::mat4(1.0f));
  filter_program_->SetUniformValue("model", glm::mat4(1.0f));

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        input_framebuffers_[0].frame_buffer->GetTexture()));
  filter_program_->SetUniformValue("inputImageTexture", 0);
  filter_program_->SetUniformValue("test", 1.0f);
  // texcoord attribute
  uint32_t filter_tex_coord_attribute =
      filter_program_->GetAttribLocation("inputTextureCoordinate");
  GL_CALL(glEnableVertexAttribArray(filter_tex_coord_attribute));
  GL_CALL(glVertexAttribPointer(filter_tex_coord_attribute, 2, GL_FLOAT, 0, 0,
                                GetTextureCoordinate(NoRotation)));

  GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                image_vertices));
  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  float position_x = 0.0f;
  float position_y = 0.0f;

  if (face_landmarks_.size() > 0) {
    float x = face_landmarks_[43 * 2];      // x
    float y = face_landmarks_[43 * 2 + 1];  // y
    position_x = x * 2 - 1;
    position_y = y * 2 - 1;
  }

  LOG_INFO("pitch_ = {}, yaw_ = {}, roll_ = {}", pitch_, yaw_, roll_);
  // 组合所有变换矩阵，注意变换顺序：先缩放，再旋转，最后平移
  glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f));

  model = glm::rotate(model, -roll_, glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::translate(model,
                         glm::vec3(position_x / 0.15, position_y / 0.15, 0.0f));

  // 修改为与 NDC 空间匹配的正交投影
  float aspect = (float)framebuffer_->GetHeight() / framebuffer_->GetWidth();

  filter_program_->SetUniformValue("test", 0.6f);
  glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -aspect, aspect, -1.0f, 0.0f);
  filter_program_->SetUniformValue("projection", projection);
  filter_program_->SetUniformValue("model", model);

  // 设置叠加纹理
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        source_image_->GetFramebuffer()->GetTexture()));

  float vert[] = {
      -1.0f, -0.5f, 1.0f, -0.5f, -1.0f, 0.5f, 1.0f, 0.5f,
  };

  GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                vert));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  framebuffer_->Deactivate();

  return Source::DoRender(updateSinks);
}

void TieZhiFilter::SetAngle(float pitch, float yaw, float roll) {
  pitch_ = pitch;
  yaw_ = yaw;
  roll_ = roll;
}

glm::mat4 TieZhiFilter::CreateRotationMatrix(float angle,
                                             float x,
                                             float y,
                                             float z) {
  return glm::rotate(angle, glm::vec3(x, y, z));
}

void TieZhiFilter::SetLandmark(const std::vector<float>& landmarks) {
  face_landmarks_ = landmarks;
}

}  // namespace gpupixel
