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
    // 修改为与 NDC 空间匹配的正交投影
    float aspect = (float)framebuffer_->GetHeight() / framebuffer_->GetWidth();

  float position_x = 0.0f;
  float position_y = 0.0f;

  if (face_landmarks_.size() > 0) {
    float ertou_x = face_landmarks_[43 * 2];      // x
    float dis = face_landmarks_[49 * 2 + 1] - face_landmarks_[43 * 2 + 1];
    float ertou_y = face_landmarks_[35 * 2 + 1] - 2*dis;  // y
    position_x = ertou_x * 2 - 1; // [-1, 1]
    position_y = ertou_y * 2 - 1; // [-1, 1]
  }

  LOG_INFO("pitch_ = {}, yaw_ = {}, roll_ = {}", pitch_, yaw_, roll_);

  glm::mat4 scale_matrix =
      glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.1f, 1.0f));

  static float angle = 0.0f;
  angle += 1.0f;
  glm::mat4 rotate_matrix = glm::rotate(glm::mat4(1.0f), -roll_,
                                        glm::vec3(0.0f, 0.0f, 1.0f));

  rotate_matrix = glm::rotate(rotate_matrix, pitch_, glm::vec3(1.0f, 0.0f, 0.0f));
  rotate_matrix = glm::rotate(rotate_matrix, -yaw_, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 translate_matrix = glm::translate(
      glm::mat4(1.0f), glm::vec3(position_x, position_y*aspect, 0.0f));

  // 组合所有变换矩阵，注意变换顺序：先缩放，再旋转，最后平移（矩阵则从右向左的顺序生肖）
//  glm::mat4 model = rotate_matrix * scale_matrix * translate_matrix;
  glm::mat4 model = translate_matrix*rotate_matrix*scale_matrix;

  filter_program_->SetUniformValue("test", 0.6f);
  glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -aspect, aspect, -1.0f, 1.0f);
  filter_program_->SetUniformValue("projection", projection);
  filter_program_->SetUniformValue("model", model);

  // 设置叠加纹理
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        source_image_->GetFramebuffer()->GetTexture()));

  float vert[] = {
      -1.0f, -0.5f, 1.0f, -0.5f, -1.0f, 0.5f, 1.0f, 0.5f,
  };

  // GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0,
  // 0,
  //                               vert));

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
