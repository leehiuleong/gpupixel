/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "gpupixel/filter/filter.h"

namespace gpupixel {

class GPUPIXEL_API LookupFilter : public Filter {
 public:
  static std::shared_ptr<LookupFilter> Create();
  static std::shared_ptr<LookupFilter> Create(const std::string& lookupImagePath);
  ~LookupFilter();
  virtual bool Init();
  virtual bool DoRender(bool updateSinks = true) override;

  void SetLookupImagePath(const std::string& lookupImagePath);
  void SetIntensity(float intensity);

 protected:
  LookupFilter();

 private:
  void LoadLookupTexture();

  std::string lookup_image_path_;
  float intensity_;
  uint32_t lookup_texture_;
  bool lookup_texture_loaded_;
};

}  // namespace gpupixel

