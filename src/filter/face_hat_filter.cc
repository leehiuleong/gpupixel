/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "face_hat_filter.h"

NS_GPUPIXEL_BEGIN

FaceHatFilter::FaceHatFilter() {
}

std::shared_ptr<FaceHatFilter> FaceHatFilter::create() {
    auto ret = std::shared_ptr<FaceHatFilter>(new FaceHatFilter());
    if (ret && !ret->init()) {
        ret.reset();
    }
    return ret;
}

bool FaceHatFilter::init() {
    if (!FaceDecorativeFilter::init()) {
        return false;
    }
    
    // 设置默认帽子配置
    DecorativeConfig config;
    config.type = FACE_DECORATIVE_TYPE_HEAD;
    config.scale = 1.2f;  // 帽子比头部稍大
    config.offsetX = 0.0f;
    config.offsetY = -0.1f;  // 向上偏移一点
    config.rotation = 0.0f;
    config.alpha = 1.0f;
    config.enabled = true;
    
    setDecorativeConfig(config);
    
    // 注册属性
    registerProperty("hat_scale", 1.2f, "Hat scale factor", [this](float& val) {
        setHatScale(val);
    });
    
    registerProperty("hat_offset_x", 0.0f, "Hat X offset", [this](float& val) {
        auto config = current_config_;
        config.offsetX = val;
        setDecorativeConfig(config);
    });
    
    registerProperty("hat_offset_y", -0.1f, "Hat Y offset", [this](float& val) {
        auto config = current_config_;
        config.offsetY = val;
        setDecorativeConfig(config);
    });
    
    registerProperty("hat_rotation", 0.0f, "Hat rotation angle", [this](float& val) {
        setHatRotation(val);
    });
    
    return true;
}

void FaceHatFilter::setHatImage(const std::string& imagePath) {
    setDecorativeImage(imagePath, FACE_DECORATIVE_TYPE_HEAD);
}

void FaceHatFilter::setHatScale(float scale) {
    auto config = current_config_;
    config.scale = scale;
    setDecorativeConfig(config);
}

void FaceHatFilter::setHatOffset(float offsetX, float offsetY) {
    auto config = current_config_;
    config.offsetX = offsetX;
    config.offsetY = offsetY;
    setDecorativeConfig(config);
}

void FaceHatFilter::setHatRotation(float rotation) {
    auto config = current_config_;
    config.rotation = rotation;
    setDecorativeConfig(config);
}

NS_GPUPIXEL_END
