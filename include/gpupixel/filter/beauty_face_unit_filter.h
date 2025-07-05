/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "gpupixel/filter/filter.h"
#include "gpupixel/gpupixel_define.h"

namespace gpupixel {
class SourceImage;

class GPUPIXEL_API BeautyFaceUnitFilter : public Filter {
 public:
  static std::shared_ptr<BeautyFaceUnitFilter> Create();
  ~BeautyFaceUnitFilter();
  bool Init();
  bool DoRender(bool updateSinks = true) override;

  void SetSharpen(float sharpen);
  void SetBlurAlpha(float blurAlpha);
  void SetWhite(float white,
                float warmth = 0.0f);  // warmth: -1.0(冷白) 到 1.0(暖白)
  void SetRuddy(float ruddy);          // ruddy: 0.0(无红润) 到 1.0(最大红润)

  // 设置查找表图像的接口
  // lookup_images 顺序：[0]gray, [1]original, [2]skin, [3]custom
  void SetLookupImages(
      const std::vector<std::shared_ptr<SourceImage>>& lookup_images);

 protected:
  BeautyFaceUnitFilter();

  std::shared_ptr<SourceImage> gray_image_;
  std::shared_ptr<SourceImage> original_image_;
  std::shared_ptr<SourceImage> skin_image_;
  std::shared_ptr<SourceImage> custom_image_;

 private:
  float sharpen_factor_ = 0.0;
  float blur_alpha_ = 0.0;
  float white_balance_ = 0.0;
  float white_warmth_ = 0.0;  // 美白暖度参数 -1.0(冷白) 到 1.0(暖白)
  float ruddy_factor_ = 0.0;  // 红润程度参数 0.0(无红润) 到 1.0(最大红润)
};

}  // namespace gpupixel
