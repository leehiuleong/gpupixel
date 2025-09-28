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

class LookupFilter : public Filter {
 public:
  static std::shared_ptr<LookupFilter> create(const std::string& lookupImagePath = "");
  bool init(const std::string& lookupImagePath);
  virtual bool proceed(bool bUpdateTargets = true,
                       int64_t frameTime = 0) override;

  void setLookupImagePath(const std::string& lookupImagePath);
  void setIntensity(float intensity);

 protected:
  LookupFilter(){};

  void loadLookupTexture();

  std::string _lookupImagePath;
  float _intensity;
  GLuint _lookupTexture;
  bool _lookupTextureLoaded;
};

NS_GPUPIXEL_END 