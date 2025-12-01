/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/lookup_filter.h"
#include "core/gpupixel_context.h"
#include "core/gpupixel_gl_include.h"
#include "utils/util.h"
#include "stb/stb_image.h"

namespace gpupixel {

#if defined(GPUPIXEL_GLES_SHADER)
const std::string kLookupFragmentShaderString = R"(
    precision mediump float;
    uniform sampler2D inputImageTexture;
    uniform sampler2D lookupTexture;
    uniform float intensity;
    varying vec2 textureCoordinate;

    void main() {
        vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
        
        float blueColor = textureColor.b * 63.0;
        
        vec2 quad1;
        quad1.y = floor(floor(blueColor) / 8.0);
        quad1.x = floor(blueColor) - (quad1.y * 8.0);
        
        vec2 quad2;
        quad2.y = floor(ceil(blueColor) / 8.0);
        quad2.x = ceil(blueColor) - (quad2.y * 8.0);
        
        vec2 texPos1;
        texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);
        
        vec2 texPos2;
        texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);
        
        vec4 newColor1 = texture2D(lookupTexture, texPos1);
        vec4 newColor2 = texture2D(lookupTexture, texPos2);
        
        vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
        gl_FragColor = mix(textureColor, vec4(newColor.rgb, textureColor.a), intensity);
    })";
#elif defined(GPUPIXEL_GL_SHADER)
const std::string kLookupFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    uniform sampler2D lookupTexture;
    uniform float intensity;
    varying vec2 textureCoordinate;

    void main() {
        vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
        
        float blueColor = textureColor.b * 63.0;
        
        vec2 quad1;
        quad1.y = floor(floor(blueColor) / 8.0);
        quad1.x = floor(blueColor) - (quad1.y * 8.0);
        
        vec2 quad2;
        quad2.y = floor(ceil(blueColor) / 8.0);
        quad2.x = ceil(blueColor) - (quad2.y * 8.0);
        
        vec2 texPos1;
        texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);
        
        vec2 texPos2;
        texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);
        
        vec4 newColor1 = texture2D(lookupTexture, texPos1);
        vec4 newColor2 = texture2D(lookupTexture, texPos2);
        
        vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
        gl_FragColor = mix(textureColor, vec4(newColor.rgb, textureColor.a), intensity);
    })";
#endif

LookupFilter::LookupFilter()
    : lookup_image_path_(""),
      intensity_(1.0f),
      lookup_texture_(0),
      lookup_texture_loaded_(false) {}

LookupFilter::~LookupFilter() {
  if (lookup_texture_loaded_ && lookup_texture_ != 0) {
    GL_CALL(glDeleteTextures(1, &lookup_texture_));
    lookup_texture_ = 0;
    lookup_texture_loaded_ = false;
  }
}

std::shared_ptr<LookupFilter> LookupFilter::Create() {
  return Create("");
}

std::shared_ptr<LookupFilter> LookupFilter::Create(
    const std::string& lookupImagePath) {
  auto ret = std::shared_ptr<LookupFilter>(new LookupFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    } else if (ret && !lookupImagePath.empty()) {
      ret->SetLookupImagePath(lookupImagePath);
    }
  });
  return ret;
}

bool LookupFilter::Init() {
  if (!Filter::InitWithFragmentShaderString(kLookupFragmentShaderString)) {
    return false;
  }

  intensity_ = 1.0f;
  lookup_texture_ = 0;
  lookup_texture_loaded_ = false;

  RegisterProperty("intensity", intensity_,
                   "The intensity of the lookup filter effect (0.0 to 1.0).",
                   [this](float& val) { SetIntensity(val); });

  std::string default_path;
  RegisterProperty("lookup_image_path", default_path,
                   "The path to the lookup image file.",
                   [this](std::string& val) { SetLookupImagePath(val); });

  return true;
}

void LookupFilter::SetLookupImagePath(const std::string& lookupImagePath) {
  if (lookup_image_path_ != lookupImagePath) {
    lookup_image_path_ = lookupImagePath;
    LoadLookupTexture();
  }
}

void LookupFilter::SetIntensity(float intensity) {
  intensity_ = intensity;
  if (intensity_ > 1.0f) {
    intensity_ = 1.0f;
  } else if (intensity_ < 0.0f) {
    intensity_ = 0.0f;
  }
}

void LookupFilter::LoadLookupTexture() {
  // Clean up existing texture
  if (lookup_texture_loaded_ && lookup_texture_ != 0) {
    GL_CALL(glDeleteTextures(1, &lookup_texture_));
    lookup_texture_ = 0;
    lookup_texture_loaded_ = false;
  }

  if (lookup_image_path_.empty()) {
    return;
  }

  // Load image using stb_image
  int width, height, channels;
  unsigned char* data =
      stbi_load(lookup_image_path_.c_str(), &width, &height, &channels, 0);

  if (data == nullptr) {
    return;
  }

  // Create OpenGL texture
  GL_CALL(glGenTextures(1, &lookup_texture_));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, lookup_texture_));

  // Set texture parameters
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  // Upload texture data
  if (channels == 3) {
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data));
  } else if (channels == 4) {
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, data));
  } else {
    GL_CALL(glDeleteTextures(1, &lookup_texture_));
    lookup_texture_ = 0;
    stbi_image_free(data);
    return;
  }

  GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));

  // Free image data
  stbi_image_free(data);

  lookup_texture_loaded_ = true;
}

bool LookupFilter::DoRender(bool updateSinks) {
  if (lookup_texture_loaded_ && lookup_texture_ != 0) {
    // Bind lookup texture to texture unit 1
    GL_CALL(glActiveTexture(GL_TEXTURE1));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, lookup_texture_));

    // Set uniforms
    filter_program_->SetUniformValue("lookupTexture", 1);
    filter_program_->SetUniformValue("intensity", intensity_);
  } else {
    // If no lookup texture, set intensity to 0 to show original image
    filter_program_->SetUniformValue("intensity", 0.0f);
  }

  return Filter::DoRender(updateSinks);
}

}  // namespace gpupixel
