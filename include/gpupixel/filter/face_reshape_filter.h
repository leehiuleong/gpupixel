/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "gpupixel/filter/filter.h"

namespace gpupixel {
class GPUPIXEL_API FaceReshapeFilter : public Filter {
 public:
  static std::shared_ptr<FaceReshapeFilter> Create();
  FaceReshapeFilter();
  ~FaceReshapeFilter();

  bool Init();
  bool DoRender(bool updateSinks = true) override;

  // 瘦脸
  void SetFaceSlimLevel(float level);
  // V脸
  void SetVShapeLevel(float level);
  // 窄脸
  void SetNarrowFaceLevel(float level);
  // 短脸
  void SetShortFaceLevel(float level);
  // 颧骨
  void SetCheekbonesLevel(float level);
  // 下颌骨
  void SetJawboneLevel(float level);
  // 下巴
  void SetChinLevel(float level);
  // 瘦鼻梁
  void SetNoseLevel(float level);
  // 长鼻
  // 鼻翼
  // 嘴巴
  void SetMouthSizeLevel(float level);

  // 嘴巴位置
  void SetMouthPositionLevel(float level);
  // 嘴型（微笑）
  // 眼距（左右）
  void SetEyeDistanceLevel(float level);
  // 眼位置（上下）

  // 大眼
  void SetEyeZoomLevel(float level);

  // 额头
  //
  // 设置人脸关键点
  void SetFaceLandmarks(std::vector<float> landmarks);

 private:
  float thin_face_delta_ = 0.0;
  float big_eye_delta_ = 0.0;
  float vshape_delta_ = 0.0;
  float narrow_face_delta_ = 0.0;
  float short_face_delta_ = 0.0;
  float cheekbones_delta_ = 0.0;
  float jawbone_delta_ = 0.0;
  float chin_delta_ = 0.0;
  float nose_delta_ = 0.0;
  float mouth_size_delta_ = 0.0;
  float eye_distance_delta_ = 0.0;
  float mouth_position_delta_ = 0.0;

  std::vector<float> face_landmarks_;
  int face_count_ = 0;
};

}  // namespace gpupixel
