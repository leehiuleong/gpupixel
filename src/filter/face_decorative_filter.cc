/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "face_decorative_filter.h"
#include "source_image.h"
#include "gpupixel_context.h"
#include "util.h"
#include <cmath>
#include <algorithm>

NS_GPUPIXEL_BEGIN

// 顶点着色器
const std::string FaceDecorativeVertexShaderString = R"(
    attribute vec4 position;
    attribute vec2 inputTextureCoordinate;
    varying vec2 textureCoordinate;
    uniform mat4 transformMatrix;
    
    void main() {
        gl_Position = transformMatrix * position;
        textureCoordinate = inputTextureCoordinate;
    })";

// 片段着色器
#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_ANDROID)
const std::string FaceDecorativeFragmentShaderString = R"(
    precision mediump float;
    varying highp vec2 textureCoordinate;
    uniform sampler2D decorativeTexture;
    uniform float alpha;
    uniform float intensity;
    
    void main() {
        vec4 decorativeColor = texture2D(decorativeTexture, textureCoordinate);
        
        // 如果装饰图像是透明的，则不渲染
        if (decorativeColor.a == 0.0) {
            discard;
            return;
        }
        
        // 只渲染装饰图像，不混合原始图像
        float finalAlpha = decorativeColor.a * alpha * intensity;
        gl_FragColor = vec4(decorativeColor.rgb, finalAlpha);
    })";
#elif defined(GPUPIXEL_MAC) || defined(GPUPIXEL_WIN) || defined(GPUPIXEL_LINUX)
const std::string FaceDecorativeFragmentShaderString = R"(
    varying vec2 textureCoordinate;
    uniform sampler2D decorativeTexture;
    uniform float alpha;
    uniform float intensity;
    
    void main() {
        vec4 decorativeColor = texture2D(decorativeTexture, textureCoordinate);
        
        // 如果装饰图像是透明的，则不渲染
        if (decorativeColor.a == 0.0) {
            discard;
            return;
        }
        
        // 只渲染装饰图像，不混合原始图像
        float finalAlpha = decorativeColor.a * alpha * intensity;
        gl_FragColor = vec4(decorativeColor.rgb, finalAlpha);
    })";
#endif

FaceDecorativeFilter::FaceDecorativeFilter() 
    : has_face_(false)
    , enabled_(true)
    , intensity_(1.0f)
    , decorative_program_(nullptr)
    , position_attribute_(0)
    , texcoord_attribute_(0)
    , transform_uniform_(0)
    , alpha_uniform_(0)
    , intensity_uniform_(0) {
    
    // 初始化变换矩阵为单位矩阵
    for (int i = 0; i < 16; i++) {
        transform_matrix_[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    
    // 初始化默认配置
    current_config_.type = FACE_DECORATIVE_TYPE_CUSTOM;
    current_config_.scale = 1.0f;
    current_config_.offsetX = 0.0f;
    current_config_.offsetY = 0.0f;
    current_config_.rotation = 0.0f;
    current_config_.alpha = 1.0f;
    current_config_.enabled = true;
    
    initKeyPointIndices();
}

FaceDecorativeFilter::~FaceDecorativeFilter() {
    if (decorative_program_) {
        delete decorative_program_;
        decorative_program_ = nullptr;
    }
}

std::shared_ptr<FaceDecorativeFilter> FaceDecorativeFilter::create() {
    auto ret = std::shared_ptr<FaceDecorativeFilter>(new FaceDecorativeFilter());
    if (ret && !ret->init()) {
        ret.reset();
    }
    return ret;
}

bool FaceDecorativeFilter::init() {
    // 初始化基础滤镜
    if (!Filter::initWithShaderString(kDefaultVertexShader, kDefaultFragmentShader)) {
        return false;
    }
    
    // 初始化装饰渲染程序
    decorative_program_ = GLProgram::createByShaderString(FaceDecorativeVertexShaderString, 
                                                          FaceDecorativeFragmentShaderString);
    if (!decorative_program_) {
        return false;
    }
    
    // 获取属性位置
    position_attribute_ = decorative_program_->getAttribLocation("position");
    texcoord_attribute_ = decorative_program_->getAttribLocation("inputTextureCoordinate");
    transform_uniform_ = decorative_program_->getUniformLocation("transformMatrix");
    alpha_uniform_ = decorative_program_->getUniformLocation("alpha");
    intensity_uniform_ = decorative_program_->getUniformLocation("intensity");
    
    // 初始化人脸检测器
    initFaceDetector();
    
    // 注册属性
    registerProperty("enabled", 1, "Enable/disable decorative filter", [this](int& val) {
        setEnabled(val != 0);
    });
    
    registerProperty("intensity", 1.0f, "Decorative intensity (0.0 - 1.0)", [this](float& val) {
        setIntensity(val);
    });
    
    std::vector<float> defaultLandmarks;
    registerProperty("face_landmarks", defaultLandmarks, "Face landmarks for tracking", [this](std::vector<float> val) {
        updateFaceLandmarks(val);
    });
    
    return true;
}

void FaceDecorativeFilter::initFaceDetector() {
    face_detector_ = std::make_shared<FaceDetector>();
    if (face_detector_) {
        // 注册人脸检测回调
        face_detector_->RegCallback([this](std::vector<float> landmarks) {
            updateFaceLandmarks(landmarks);
        });
    }
}

void FaceDecorativeFilter::initKeyPointIndices() {
    // 头部装饰关键点（额头区域）- 使用头部轮廓关键点
    // 根据VNN 278点模型，头部轮廓通常是前33个点
    keypoint_indices_[FACE_DECORATIVE_TYPE_HEAD] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    
    // 眼部装饰关键点（眼睛区域）- 使用眼部关键点
    // 眼部关键点通常在33-79范围内
    keypoint_indices_[FACE_DECORATIVE_TYPE_EYES] = {33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79};
        
    // 嘴部装饰关键点（嘴部区域）- 使用嘴部关键点
    // 嘴部关键点通常在84-110范围内
    keypoint_indices_[FACE_DECORATIVE_TYPE_MOUTH] = {84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110};

    // 额头装饰关键点（额头区域）- 使用额头关键点
    // 额头关键点通常在139-142范围内
    keypoint_indices_[FACE_DECORATIVE_TYPE_FOREHEAD] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    
    // 鼻子装饰关键点（鼻子区域）- 使用鼻子关键点
    // 鼻子关键点通常在143-148范围内
    keypoint_indices_[FACE_DECORATIVE_TYPE_NOSE] = {52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};

    // 自定义装饰使用所有关键点
    keypoint_indices_[FACE_DECORATIVE_TYPE_CUSTOM] = {};
    for (int i = 0; i < 110; i++) {
        keypoint_indices_[FACE_DECORATIVE_TYPE_CUSTOM].push_back(i);
    }
}

void FaceDecorativeFilter::setDecorativeImage(const std::string& imagePath, FaceDecorativeType type) {
    decorative_image_ = SourceImage::create(imagePath);
    current_config_.imagePath = imagePath;
    current_config_.type = type;
}

void FaceDecorativeFilter::setDecorativeConfig(const DecorativeConfig& config) {
    current_config_ = config;
    if (!config.imagePath.empty()) {
        decorative_image_ = SourceImage::create(config.imagePath);
    }
}

void FaceDecorativeFilter::updateFaceLandmarks(const std::vector<float>& landmarks) {
    if (landmarks.empty()) {
        has_face_ = false;
        return;
    }
    
    // 将关键点坐标从[0,1]范围转换到[-1,1]范围
    face_landmarks_.clear();
    for (auto landmark : landmarks) {
        face_landmarks_.push_back(2.0f * landmark - 1.0f);
    }
    has_face_ = true;
    
    // 计算变换矩阵
    calculateTransformMatrix(face_landmarks_, current_config_.type);
}

void FaceDecorativeFilter::calculateTransformMatrix(const std::vector<float>& landmarks, FaceDecorativeType type) {
    if (landmarks.empty()) {
        return;
    }
    
    // 获取相关关键点
    auto keypointIndices = getKeyPointIndices(type);
    if (keypointIndices.empty()) {
        return;
    }
    
    // 计算边界框
    float minX = 1.0f, maxX = -1.0f, minY = 1.0f, maxY = -1.0f;
    for (int idx : keypointIndices) {
        if (idx * 2 + 1 < landmarks.size()) {
            float x = landmarks[idx * 2];
            float y = landmarks[idx * 2 + 1];
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }
    }
    
    // 计算中心点和尺寸
    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float width = maxX - minX;
    float height = maxY - minY;
    
    // 应用配置的缩放、偏移和旋转
    float scaleX = width * current_config_.scale;
    float scaleY = height * current_config_.scale;
    float offsetX = current_config_.offsetX;
    float offsetY = current_config_.offsetY;
    float rotation = current_config_.rotation * M_PI / 180.0f; // 转换为弧度
    
    // 构建变换矩阵
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    
    // 4x4变换矩阵
    transform_matrix_[0] = scaleX * cosR;  // m00
    transform_matrix_[1] = scaleX * sinR;  // m01
    transform_matrix_[4] = -scaleY * sinR; // m10
    transform_matrix_[5] = scaleY * cosR;  // m11
    transform_matrix_[12] = centerX + offsetX; // m30 (translation X)
    transform_matrix_[13] = centerY + offsetY; // m31 (translation Y)
}

std::vector<int> FaceDecorativeFilter::getKeyPointIndices(FaceDecorativeType type) {
    auto it = keypoint_indices_.find(type);
    if (it != keypoint_indices_.end()) {
        return it->second;
    }
    return {};
}

std::vector<GLfloat> FaceDecorativeFilter::calculateDecorativeVertices(const std::vector<float>& landmarks, FaceDecorativeType type) {
    std::vector<GLfloat> vertices;
    
    if (landmarks.empty()) {
        // 默认矩形顶点
        vertices = {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f
        };
    } else {
        // 根据关键点计算顶点
        auto keypointIndices = getKeyPointIndices(type);
        if (!keypointIndices.empty()) {
            // 计算边界框
            float minX = 1.0f, maxX = -1.0f, minY = 1.0f, maxY = -1.0f;
            for (int idx : keypointIndices) {
                if (idx * 2 + 1 < landmarks.size()) {
                    float x = landmarks[idx * 2];
                    float y = landmarks[idx * 2 + 1];
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                    minY = std::min(minY, y);
                    maxY = std::max(maxY, y);
                }
            }
            
            // 创建矩形顶点
            vertices = {
                minX, minY, 0.0f,
                maxX, minY, 0.0f,
                minX, maxY, 0.0f,
                maxX, maxY, 0.0f
            };
        }
    }
    
    return vertices;
}

std::vector<GLfloat> FaceDecorativeFilter::calculateDecorativeTexCoords() {
    // 标准纹理坐标
    return {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };
}

bool FaceDecorativeFilter::proceed(bool bUpdateTargets, int64_t frameTime) {
    if (!enabled_ || !decorative_image_ || !has_face_) {
        // 如果没有启用、没有装饰图片或没有检测到人脸，直接渲染原始图像
        return Filter::proceed(bUpdateTargets, frameTime);
    }
    
    static const GLfloat imageVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };
    
    _framebuffer->active();
    
    // 首先渲染原始图像
    GPUPixelContext::getInstance()->setActiveShaderProgram(_filterProgram);
    CHECK_GL(glClearColor(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, _backgroundColor.a));
    CHECK_GL(glClear(GL_COLOR_BUFFER_BIT));
    
    CHECK_GL(glActiveTexture(GL_TEXTURE0));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, _inputFramebuffers[0].frameBuffer->getTexture()));
    _filterProgram->setUniformValue("inputImageTexture", 0);
    
    CHECK_GL(glEnableVertexAttribArray(_filterPositionAttribute));
    CHECK_GL(glVertexAttribPointer(_filterPositionAttribute, 2, GL_FLOAT, 0, 0, imageVertices));
    
    GLuint filterTexCoordAttribute = _filterProgram->getAttribLocation("inputTextureCoordinate");
    if (filterTexCoordAttribute != (GLuint)-1) {
        CHECK_GL(glEnableVertexAttribArray(filterTexCoordAttribute));
        CHECK_GL(glVertexAttribPointer(filterTexCoordAttribute, 2, GL_FLOAT, 0, 0, _getTexureCoordinate(NoRotation)));
    }
    
    CHECK_GL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    
    // 然后渲染装饰图像
    GPUPixelContext::getInstance()->setActiveShaderProgram(decorative_program_);
    
    // 计算装饰顶点
    decorative_vertices_ = calculateDecorativeVertices(face_landmarks_, current_config_.type);
    decorative_texcoords_ = calculateDecorativeTexCoords();
    
    // 设置变换矩阵
    Matrix4 matrix;
    memcpy(&matrix, transform_matrix_, sizeof(float) * 16);
    decorative_program_->setUniformValue("transformMatrix", matrix);
    decorative_program_->setUniformValue("alpha", current_config_.alpha);
    decorative_program_->setUniformValue("intensity", intensity_);
    
    // 绑定装饰纹理
    CHECK_GL(glActiveTexture(GL_TEXTURE0));
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, decorative_image_->getFramebuffer()->getTexture()));
    decorative_program_->setUniformValue("decorativeTexture", 0);
    
    // 设置顶点属性
    CHECK_GL(glEnableVertexAttribArray(position_attribute_));
    CHECK_GL(glVertexAttribPointer(position_attribute_, 3, GL_FLOAT, 0, 0, decorative_vertices_.data()));
    
    CHECK_GL(glEnableVertexAttribArray(texcoord_attribute_));
    CHECK_GL(glVertexAttribPointer(texcoord_attribute_, 2, GL_FLOAT, 0, 0, decorative_texcoords_.data()));
    
    // 绘制装饰
    CHECK_GL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    
    _framebuffer->inactive();
    
    return Source::proceed(bUpdateTargets, frameTime);
}

NS_GPUPIXEL_END
