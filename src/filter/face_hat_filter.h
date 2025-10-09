/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "face_decorative_filter.h"

NS_GPUPIXEL_BEGIN

class FaceHatFilter : public FaceDecorativeFilter {
public:
    static std::shared_ptr<FaceHatFilter> create();
    bool init();
    
    // 设置帽子图片
    void setHatImage(const std::string& imagePath);
    
    // 设置帽子大小
    void setHatScale(float scale);
    
    // 设置帽子位置偏移
    void setHatOffset(float offsetX, float offsetY);
    
    // 设置帽子旋转角度
    void setHatRotation(float rotation);

private:
    FaceHatFilter();
};

NS_GPUPIXEL_END
