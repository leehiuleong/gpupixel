/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "face_decorative_filter.h"

NS_GPUPIXEL_BEGIN

class FaceGlassesFilter : public FaceDecorativeFilter {
public:
    static std::shared_ptr<FaceGlassesFilter> create();
    bool init();
    
    // 设置眼镜图片
    void setGlassesImage(const std::string& imagePath);
    
    // 设置眼镜大小
    void setGlassesScale(float scale);
    
    // 设置眼镜位置偏移
    void setGlassesOffset(float offsetX, float offsetY);
    
    // 设置眼镜旋转角度
    void setGlassesRotation(float rotation);

private:
    FaceGlassesFilter();
};

NS_GPUPIXEL_END
