/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "filter.h"
#include "gpupixel_macros.h"

NS_GPUPIXEL_BEGIN

class FisheyeFilter : public Filter {
 public:
  static std::shared_ptr<FisheyeFilter> create();
  bool init();
  virtual bool proceed(bool bUpdateTargets = true, int64_t frameTime = 0) override;

  void setStrength(float strength);
  void setPositionX(float x);
  void setPositionY(float y);
  void setZoom(float zoom);

 protected:
  FisheyeFilter() {};

  // Distortion strength (>0 barrel/fisheye, <0 pincushion)
  float _strength;

  // Distortion center in normalized coords [0,1]
  Vector2 _position;

  // Overall zoom-in to avoid black borders after distortion (>1 zooms in)
  float _zoom;
};

NS_GPUPIXEL_END


