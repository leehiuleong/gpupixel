/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "face_glasses_filter.h"

NS_GPUPIXEL_BEGIN

FaceGlassesFilter::FaceGlassesFilter() {
}

std::shared_ptr<FaceGlassesFilter> FaceGlassesFilter::create() {
    auto ret = std::shared_ptr<FaceGlassesFilter>(new FaceGlassesFilter());
    if (ret && !ret->init()) {
        ret.reset();
    }
    return ret;
}

bool FaceGlassesFilter::init() {
    if (!FaceDecorativeFilter::init()) {
        return false;
    }
    
    // 设置默认眼镜配置
    DecorativeConfig config;
    config.type = FACE_DECORATIVE_TYPE_EYES;
    config.scale = 1.0f;  // 眼镜与眼部大小匹配
    config.offsetX = 0.0f;
    config.offsetY = 0.0f;  // 居中
    config.rotation = 0.0f;
    config.alpha = 0.8f;  // 稍微透明
    config.enabled = true;
    
    setDecorativeConfig(config);
    
    // 注册属性
    registerProperty("glasses_scale", 1.0f, "Glasses scale factor", [this](float& val) {
        setGlassesScale(val);
    });
    
    registerProperty("glasses_offset_x", 0.0f, "Glasses X offset", [this](float& val) {
        auto config = current_config_;
        config.offsetX = val;
        setDecorativeConfig(config);
    });
    
    registerProperty("glasses_offset_y", 0.0f, "Glasses Y offset", [this](float& val) {
        auto config = current_config_;
        config.offsetY = val;
        setDecorativeConfig(config);
    });
    
    registerProperty("glasses_rotation", 0.0f, "Glasses rotation angle", [this](float& val) {
        setGlassesRotation(val);
    });
    
    registerProperty("glasses_alpha", 0.8f, "Glasses transparency", [this](float& val) {
        auto config = current_config_;
        config.alpha = val;
        setDecorativeConfig(config);
    });
    
    return true;
}

void FaceGlassesFilter::setGlassesImage(const std::string& imagePath) {
    setDecorativeImage(imagePath, FACE_DECORATIVE_TYPE_EYES);
}

void FaceGlassesFilter::setGlassesScale(float scale) {
    auto config = current_config_;
    config.scale = scale;
    setDecorativeConfig(config);
}

void FaceGlassesFilter::setGlassesOffset(float offsetX, float offsetY) {
    auto config = current_config_;
    config.offsetX = offsetX;
    config.offsetY = offsetY;
    setDecorativeConfig(config);
}

void FaceGlassesFilter::setGlassesRotation(float rotation) {
    auto config = current_config_;
    config.rotation = rotation;
    setDecorativeConfig(config);
}

NS_GPUPIXEL_END
