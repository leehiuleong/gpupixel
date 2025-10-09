/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "filter.h"
#include "face_detector.h"
#include <map>
#include <vector>
#include <memory>

NS_GPUPIXEL_BEGIN

class SourceImage;

// 装饰类型枚举
typedef enum {
    FACE_DECORATIVE_TYPE_HEAD,      // 头部装饰（如帽子、头饰）
    FACE_DECORATIVE_TYPE_EYES,      // 眼部装饰（如眼镜、眼罩）
    FACE_DECORATIVE_TYPE_CHEEKS,    // 脸颊装饰（如腮红、贴纸）
    FACE_DECORATIVE_TYPE_MOUTH,     // 嘴部装饰（如口罩、胡须）
    FACE_DECORATIVE_TYPE_EARS,      // 耳朵装饰（如耳环、耳机）
    FACE_DECORATIVE_TYPE_CUSTOM     // 自定义装饰
} FaceDecorativeType;

// 装饰配置结构
typedef struct {
    FaceDecorativeType type;
    std::string imagePath;
    float scale;           // 缩放比例
    float offsetX;         // X轴偏移
    float offsetY;         // Y轴偏移
    float rotation;        // 旋转角度
    float alpha;           // 透明度
    bool enabled;          // 是否启用
} DecorativeConfig;

class FaceDecorativeFilter : public Filter {
public:
    static std::shared_ptr<FaceDecorativeFilter> create();
    ~FaceDecorativeFilter();
    
    bool init();
    virtual bool proceed(bool bUpdateTargets = true, int64_t frameTime = 0) override;
    
    // 设置装饰图片
    void setDecorativeImage(const std::string& imagePath, FaceDecorativeType type = FACE_DECORATIVE_TYPE_CUSTOM);
    
    // 设置装饰配置
    void setDecorativeConfig(const DecorativeConfig& config);
    
    // 更新人脸关键点
    void updateFaceLandmarks(const std::vector<float>& landmarks);
    
    // 设置是否启用装饰
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // 设置装饰强度
    void setIntensity(float intensity) { intensity_ = intensity; }
    float getIntensity() const { return intensity_; }
    
    // 获取当前配置（用于示例程序）
    const DecorativeConfig& getCurrentConfig() const { return current_config_; }

protected:
    FaceDecorativeFilter();
    
    // 初始化人脸检测器
    void initFaceDetector();
    
    // 根据装饰类型计算变换矩阵
    void calculateTransformMatrix(const std::vector<float>& landmarks, FaceDecorativeType type);
    
    // 获取特定区域的关键点索引
    std::vector<int> getKeyPointIndices(FaceDecorativeType type);
    
    // 计算装饰图片的顶点坐标
    std::vector<GLfloat> calculateDecorativeVertices(const std::vector<float>& landmarks, FaceDecorativeType type);
    
    // 计算装饰图片的纹理坐标
    std::vector<GLfloat> calculateDecorativeTexCoords();

protected:
    // 当前装饰配置
    DecorativeConfig current_config_;

private:
    // 人脸检测器
    std::shared_ptr<FaceDetector> face_detector_;
    
    // 装饰图片
    std::shared_ptr<SourceImage> decorative_image_;
    
    // 人脸关键点
    std::vector<float> face_landmarks_;
    bool has_face_;
    bool enabled_;
    float intensity_;
    
    // 变换矩阵
    float transform_matrix_[16];
    
    // OpenGL相关
    GLProgram* decorative_program_;
    GLuint position_attribute_;
    GLuint texcoord_attribute_;
    GLuint transform_uniform_;
    GLuint alpha_uniform_;
    GLuint intensity_uniform_;
    
    // 顶点缓冲区
    std::vector<GLfloat> decorative_vertices_;
    std::vector<GLfloat> decorative_texcoords_;
    
    // 关键点索引映射
    std::map<FaceDecorativeType, std::vector<int>> keypoint_indices_;
    
    // 初始化关键点索引映射
    void initKeyPointIndices();
};

NS_GPUPIXEL_END
