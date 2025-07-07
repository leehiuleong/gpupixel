/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gpupixel/filter/filter.h"
#include "gpupixel/gpupixel_define.h"
#include "gpupixel/source/source_image.h"

namespace gpupixel {
class GPUPIXEL_API PortraitSegFilter : public Filter {
 public:
  static std::shared_ptr<PortraitSegFilter> Create();
  bool Init();
  virtual bool DoRender(bool updateSinks = true) override;

  void SetPortraitSegMask(const uint8_t* mask, int width, int height);
  void SetBackgroundImage(std::shared_ptr<SourceImage> image);

 protected:
  PortraitSegFilter() {};

  uint32_t portrait_seg_mask_;
  std::shared_ptr<SourceImage> background_image_;
  float blur_radius_ = 20.0f;
};

}  // namespace gpupixel
