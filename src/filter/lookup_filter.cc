/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "lookup_filter.h"
#include "gpupixel_context.h"
#include "util.h"

#include "stb_image.h"

USING_NS_GPUPIXEL

REGISTER_FILTER_CLASS(LookupFilter)

#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_ANDROID) || defined(GPUPIXEL_MAC)
const std::string kLookupFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    uniform sampler2D lookupTexture;
    uniform lowp float intensity;
    varying highp vec2 textureCoordinate;

    void main() {
        lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
        
        mediump float blueColor = textureColor.b * 63.0;
        
        mediump vec2 quad1;
        quad1.y = floor(floor(blueColor) / 8.0);
        quad1.x = floor(blueColor) - (quad1.y * 8.0);
        
        mediump vec2 quad2;
        quad2.y = floor(ceil(blueColor) / 8.0);
        quad2.x = ceil(blueColor) - (quad2.y * 8.0);
        
        highp vec2 texPos1;
        texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);
        
        highp vec2 texPos2;
        texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);
        
        lowp vec4 newColor1 = texture2D(lookupTexture, texPos1);
        lowp vec4 newColor2 = texture2D(lookupTexture, texPos2);
        
        lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
        gl_FragColor = mix(textureColor, vec4(newColor.rgb, textureColor.a), intensity);
    })";
#elif defined(GPUPIXEL_WIN) || defined(GPUPIXEL_MAC) || defined(GPUPIXEL_LINUX)
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

std::shared_ptr<LookupFilter> LookupFilter::create(
    const std::string& lookupImagePath /* = ""*/) {
  auto ret = std::shared_ptr<LookupFilter>(new LookupFilter());
  if (ret && !ret->init(lookupImagePath)) {
    ret.reset();
  }
  return ret;
}

bool LookupFilter::init(const std::string& lookupImagePath) {
  if (!initWithFragmentShaderString(kLookupFragmentShaderString)) {
    return false;
  }

  _lookupImagePath = lookupImagePath;
  _intensity = 1.0;
  _lookupTexture = 0;
  _lookupTextureLoaded = false;

  // Register properties
  registerProperty("intensity", _intensity,
                   "The intensity of the lookup filter effect (0.0 to 1.0).",
                   [this](float& intensity) { setIntensity(intensity); });

  // Load lookup texture if path is provided
  if (!_lookupImagePath.empty()) {
    loadLookupTexture();
  }

  return true;
}

void LookupFilter::setLookupImagePath(const std::string& lookupImagePath) {
  if (_lookupImagePath != lookupImagePath) {
    _lookupImagePath = lookupImagePath;
    loadLookupTexture();
  }
}

void LookupFilter::setIntensity(float intensity) {
  _intensity = intensity;
  if (_intensity > 1.0) {
    _intensity = 1.0;
  } else if (_intensity < 0.0) {
    _intensity = 0.0;
  }
}

void LookupFilter::loadLookupTexture() {
  // Clean up existing texture
  if (_lookupTextureLoaded && _lookupTexture != 0) {
    CHECK_GL(glDeleteTextures(1, &_lookupTexture));
    _lookupTexture = 0;
    _lookupTextureLoaded = false;
  }

  if (_lookupImagePath.empty()) {
    return;
  }

  // Load image using stb_image
  int width, height, channels;
  unsigned char* data = stbi_load(_lookupImagePath.c_str(), &width, &height, &channels, 0);
  
  if (data == nullptr) {
    Util::Log("LookupFilter", "Failed to load lookup image: %s", _lookupImagePath.c_str());
    return;
  }

  // Create OpenGL texture
  CHECK_GL(glGenTextures(1, &_lookupTexture));
  CHECK_GL(glBindTexture(GL_TEXTURE_2D, _lookupTexture));
  
  // Set texture parameters
  CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  // Upload texture data
  if (channels == 3) {
    CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data));
  } else if (channels == 4) {
    CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
  } else {
    Util::Log("LookupFilter", "Unsupported image format: %d channels", channels);
    CHECK_GL(glDeleteTextures(1, &_lookupTexture));
    _lookupTexture = 0;
    stbi_image_free(data);
    return;
  }

  CHECK_GL(glBindTexture(GL_TEXTURE_2D, 0));
  
  // Free image data
  stbi_image_free(data);
  
  _lookupTextureLoaded = true;
  Util::Log("LookupFilter", "Successfully loaded lookup texture: %s (%dx%d, %d channels)", 
            _lookupImagePath.c_str(), width, height, channels);
}

bool LookupFilter::proceed(bool bUpdateTargets, int64_t frameTime) {
  if (_lookupTextureLoaded && _lookupTexture != 0) {
    // Bind lookup texture to texture unit 1
    CHECK_GL(glActiveTexture(GL_TEXTURE1));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, _lookupTexture));
    
    // Set uniforms
    _filterProgram->setUniformValue("lookupTexture", 1);
    _filterProgram->setUniformValue("intensity", _intensity);
  } else {
    // If no lookup texture, set intensity to 0 to show original image
    _filterProgram->setUniformValue("intensity", 0.0f);
  }
  
  return Filter::proceed(bUpdateTargets, frameTime);
} 