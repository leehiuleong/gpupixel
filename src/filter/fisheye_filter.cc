/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "fisheye_filter.h"

USING_NS_GPUPIXEL

REGISTER_FILTER_CLASS(FisheyeFilter)

#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_ANDROID) || defined(GPUPIXEL_MAC)
const std::string kFisheyeFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    varying highp vec2 textureCoordinate;

    uniform highp vec2 center;
    uniform highp float aspectRatio;
    uniform highp float strength; // >0 for barrel (fisheye), <0 for pincushion
    uniform highp float zoom;     // >1.0 zoom-in to hide borders

    void main() {
      highp vec2 tc = (textureCoordinate - 0.5) / zoom + 0.5; // pre-zoom in
      highp vec2 centered = vec2(tc.x - center.x,
                                 (tc.y - center.y) * aspectRatio);

      highp float r2 = dot(centered, centered);
      // Simple barrel distortion model: r' = r * (1 + k * r^2)
      highp float k = strength; // typical range [-1.0, 1.0]
      highp float factor = 1.0 + k * r2;
      highp vec2 distorted = centered * factor;

      // Unscale Y back from aspect
      distorted = vec2(distorted.x, distorted.y / aspectRatio);
      highp vec2 uv = distorted + center;

      // clamp to edge to avoid black borders
      uv = clamp(uv, vec2(0.0), vec2(1.0));
      gl_FragColor = texture2D(inputImageTexture, uv);
    })";
#elif defined(GPUPIXEL_WIN) || defined(GPUPIXEL_LINUX)
const std::string kFisheyeFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    varying vec2 textureCoordinate;

    uniform vec2 center;
    uniform float aspectRatio;
    uniform float strength; // >0 for barrel (fisheye), <0 for pincushion
    uniform float zoom;     // >1.0 zoom-in to hide borders

    void main() {
      vec2 tc = (textureCoordinate - 0.5) / zoom + 0.5; // pre-zoom in
      vec2 centered = vec2(tc.x - center.x,
                           (tc.y - center.y) * aspectRatio);

      float r2 = dot(centered, centered);
      float k = strength; // typical range [-1.0, 1.0]
      float factor = 1.0 + k * r2;
      vec2 distorted = centered * factor;

      distorted = vec2(distorted.x, distorted.y / aspectRatio);
      vec2 uv = distorted + center;
      uv = clamp(uv, vec2(0.0), vec2(1.0));
      gl_FragColor = texture2D(inputImageTexture, uv);
    })";
#endif

std::shared_ptr<FisheyeFilter> FisheyeFilter::create() {
  auto ret = std::shared_ptr<FisheyeFilter>(new FisheyeFilter());
  if (ret && !ret->init()) {
    ret.reset();
  }
  return ret;
}

bool FisheyeFilter::init() {
  if (!initWithFragmentShaderString(kFisheyeFragmentShaderString)) {
    return false;
  }

  setPositionX(0.5f);
  registerProperty("positionX", _position.x,
                   "The x center of fisheye distortion (0..1)",
                   [this](float& x) { setPositionX(x); });

  setPositionY(0.5f);
  registerProperty("positionY", _position.y,
                   "The y center of fisheye distortion (0..1)",
                   [this](float& y) { setPositionY(y); });

  setStrength(0.5f);
  registerProperty("strength", _strength,
                   "Fisheye strength, [-1,1], >0 barrel, <0 pincushion",
                   [this](float& s) { setStrength(s); });

  setZoom(1.1f);
  registerProperty("zoom", _zoom,
                   "Overall zoom-in to reduce black borders (>1.0)",
                   [this](float& z) { setZoom(z); });

  return true;
}

bool FisheyeFilter::proceed(bool bUpdateTargets, int64_t frameTime) {
  _filterProgram->setUniformValue("center", _position);
  _filterProgram->setUniformValue("strength", _strength);
  _filterProgram->setUniformValue("zoom", _zoom);

  float aspectRatio = 1.0f;
  std::shared_ptr<Framebuffer> firstInputFramebuffer =
      _inputFramebuffers.begin()->second.frameBuffer;
  aspectRatio = firstInputFramebuffer->getHeight() /
                (float)(firstInputFramebuffer->getWidth());
  _filterProgram->setUniformValue("aspectRatio", aspectRatio);

  return Filter::proceed(bUpdateTargets, frameTime);
}

void FisheyeFilter::setStrength(float strength) {
  _strength = strength;
  if (_strength > 1.5f) _strength = 1.5f;
  if (_strength < -1.5f) _strength = -1.5f;
}

void FisheyeFilter::setPositionX(float x) { _position.x = x; }
void FisheyeFilter::setPositionY(float y) { _position.y = y; }

void FisheyeFilter::setZoom(float zoom) {
  _zoom = zoom;
  if (_zoom < 1.0f) _zoom = 1.0f;
  if (_zoom > 3.0f) _zoom = 3.0f;
}


