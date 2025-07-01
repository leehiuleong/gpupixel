/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/face_reshape_filter.h"
#include "core/gpupixel_context.h"

namespace gpupixel {

#if defined(GPUPIXEL_GLES_SHADER)
const std::string kGPUPixelThinFaceFragmentShaderString = R"(
 precision highp float;
 varying highp vec2 textureCoordinate;
 uniform sampler2D inputImageTexture;

 uniform int faceCount; 
 uniform float facePoints[4*222]; 

 uniform highp float aspectRatio;
 uniform float thinFaceDelta;
 uniform float bigEyeDelta;
 
 vec2 enlargeEye(vec2 textureCoord, vec2 originPosition, float radius, float delta) {

     float weight = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     weight = 1.0 - (1.0 - weight * weight) * delta;
     weight = clamp(weight,0.0,1.0);
     textureCoord = originPosition + (textureCoord - originPosition) * weight;
     return textureCoord;
 }
 
 vec2 curveWarp(vec2 textureCoord, vec2 originPosition, vec2 targetPosition, float delta) {

     vec2 offset = vec2(0.0);
     vec2 result = vec2(0.0);
     vec2 direction = (targetPosition - originPosition) * delta;

     float radius = distance(vec2(targetPosition.x, targetPosition.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio));
     float ratio = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     ratio = 1.0 - ratio;
     ratio = clamp(ratio, 0.0, 1.0);
     offset = direction * ratio;

     result = textureCoord - offset;

     return result;
 }

 vec2 thinFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;

     vec2 faceIndexs[9];
     faceIndexs[0] = vec2(3., 44.);
     faceIndexs[1] = vec2(29., 44.);
     faceIndexs[2] = vec2(7., 45.);
     faceIndexs[3] = vec2(25., 45.);
     faceIndexs[4] = vec2(10., 46.);
     faceIndexs[5] = vec2(22., 46.);
     faceIndexs[6] = vec2(14., 49.);
     faceIndexs[7] = vec2(18., 49.);
     faceIndexs[8] = vec2(16., 49.);

     for(int i = 0; i < 9; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

 vec2 bigEye(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;

     vec2 faceIndexs[2];
     faceIndexs[0] = vec2(74., 72.);
     faceIndexs[1] = vec2(77., 75.);

     for(int i = 0; i < 2; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);

         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);

         float radius = distance(vec2(targetPoint.x, targetPoint.y / aspectRatio), vec2(originPoint.x, originPoint.y / aspectRatio));
         radius = radius * 5.;
         currentCoordinate = enlargeEye(currentCoordinate, originPoint, radius, bigEyeDelta);
     }
     return currentCoordinate;
 }

 void main()
 {
     vec2 positionToUse = textureCoordinate;

     for(int i = 0; i < faceCount; i++) {
         positionToUse = thinFace(positionToUse, i);
         positionToUse = bigEye(positionToUse, i);
     }

     gl_FragColor = texture2D(inputImageTexture, positionToUse);
 }
 )";
#elif defined(GPUPIXEL_GL_SHADER)
const std::string kGPUPixelThinFaceFragmentShaderString = R"(
 varying vec2 textureCoordinate;
 uniform sampler2D inputImageTexture;

 uniform int faceCount; 
 uniform float facePoints[4*222]; 
 uniform float aspectRatio;
 uniform float thinFaceDelta;
 uniform float bigEyeDelta;
 uniform float vShapeDelta;
 uniform float narrowFaceDelta;
 uniform float shortFaceDelta;
 uniform float cheekbonesDelta;
 uniform float jawboneDelta;
 uniform float chinDelta;
 uniform float noseDelta;
 uniform float mouthSizeDelta;
 uniform float eyeDistanceDelta;
 uniform float mouthPositionDelta;
 
 // 基础矩形区域像素平移函数
 vec2 rectangleTranslate(vec2 textureCoord, vec2 rectTopLeft, vec2 rectBottomRight, vec2 translateOffset) {
     // 检查当前像素是否在矩形框内
     bool inRect = (textureCoord.x >= rectTopLeft.x && textureCoord.x <= rectBottomRight.x) &&
                   (textureCoord.y >= rectTopLeft.y && textureCoord.y <= rectBottomRight.y);
     
     if (inRect) {
         // 在矩形内，应用平移偏移
         return textureCoord + translateOffset;
     } else {
         // 在矩形外，保持原坐标
         return textureCoord;
     }
 }


 // 带平滑边缘的矩形区域像素平移函数
 vec2 rectangleTranslateSmooth(vec2 textureCoord, vec2 rectCenter, vec2 rectSize, vec2 translateOffset, float smoothEdge) {
     // 计算到矩形中心的相对坐标
     vec2 relativePos = abs(textureCoord - rectCenter);
     vec2 halfSize = rectSize * 0.5;
     
     // 计算距离矩形边界的距离
     vec2 distToEdge = halfSize - relativePos;
     float minDistToEdge = min(distToEdge.x, distToEdge.y);
     
     // 计算权重：在矩形内部权重为1，边缘平滑过渡
     float weight = smoothstep(0.0, smoothEdge, minDistToEdge);
     weight = clamp(weight, 0.0, 1.0);
     
     // 应用加权的平移偏移
     return textureCoord + translateOffset * weight;
 }
 
 // 改进方案1：使用高斯函数的椭圆形区域平移（推荐用于嘴巴）
 vec2 ellipseTranslateGaussian(vec2 textureCoord, vec2 center, vec2 size, vec2 translateOffset, float intensity) {
     // 计算标准化距离
     vec2 relativePos = (textureCoord - center) / size;
     float distance = length(relativePos);
     
     // 使用高斯函数计算权重，产生更自然的过渡
     float weight = exp(-distance * distance * 4.0) * intensity;
     weight = clamp(weight, 0.0, 1.0);
     
     return textureCoord + translateOffset * weight;
 }
 
 // 改进方案2：使用余弦插值的圆形区域平移
 vec2 circleTranslateCosine(vec2 textureCoord, vec2 center, float radius, vec2 translateOffset, float intensity) {
     float dist = distance(textureCoord, center);
     
     if (dist > radius) {
         return textureCoord;
     }
     
     // 使用余弦插值产生S型曲线过渡
     float normalizedDist = dist / radius;
     float weight = 0.5 * (1.0 + cos(3.14159 * normalizedDist)) * intensity;
     weight = clamp(weight, 0.0, 1.0);
     
     return textureCoord + translateOffset * weight;
 }
 
 // 改进方案3：多层次衰减的椭圆形平移（最自然的效果）
 vec2 ellipseTranslateMultiLayer(vec2 textureCoord, vec2 center, vec2 size, vec2 translateOffset, float intensity) {
     vec2 relativePos = textureCoord - center;
     vec2 normalizedPos = relativePos / size;
     float ellipseDist = length(normalizedPos);
     
     if (ellipseDist > 1.0) {
         return textureCoord;
     }
     
     // 多层次衰减：内核区域+过渡区域+边缘区域
     float weight = 0.0;
     if (ellipseDist < 0.3) {
         // 内核区域：几乎完全平移
         weight = intensity * 0.95;
     } else if (ellipseDist < 0.7) {
         // 主要过渡区域：平滑衰减
         float t = (ellipseDist - 0.3) / 0.4;
         weight = intensity * (0.95 - 0.7 * smoothstep(0.0, 1.0, t));
     } else {
         // 边缘过渡区域：快速衰减到0
         float t = (ellipseDist - 0.7) / 0.3;
         weight = intensity * 0.25 * (1.0 - smoothstep(0.0, 1.0, t));
     }
     
     weight = clamp(weight, 0.0, 1.0);
     return textureCoord + translateOffset * weight;
 }

 vec2 enlargeEye(vec2 textureCoord, vec2 originPosition, float radius, float delta) {

     float weight = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     weight = 1.0 - (1.0 - weight * weight) * delta;
     weight = clamp(weight,0.0,1.0);
     textureCoord = originPosition + (textureCoord - originPosition) * weight;
     return textureCoord;
 }
 
 vec2 curveWarp(vec2 textureCoord, vec2 originPosition, vec2 targetPosition, float delta) {

     vec2 offset = vec2(0.0);
     vec2 result = vec2(0.0);
     vec2 direction = (targetPosition - originPosition) * delta;

     float radius = distance(vec2(targetPosition.x, targetPosition.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio));
     float ratio = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     ratio = 1.0 - ratio;
     ratio = clamp(ratio, 0.0, 1.0);
     offset = direction * ratio;

     result = textureCoord - offset;

     return result;
 }

// 瘦脸功能
 vec2 thinFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
     vec2 faceIndexs[9];
     faceIndexs[0] = vec2(3., 44.);
     faceIndexs[1] = vec2(29., 44.);
     faceIndexs[2] = vec2(7., 45.);
     faceIndexs[3] = vec2(25., 45.);
     faceIndexs[4] = vec2(10., 46.);
     faceIndexs[5] = vec2(22., 46.);
     faceIndexs[6] = vec2(14., 49.);
     faceIndexs[7] = vec2(18., 49.);
     faceIndexs[8] = vec2(16., 49.);

     for(int i = 0; i < 9; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// V脸调整
  vec2 vShapeFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
     vec2 faceIndexs[4];
     faceIndexs[0] = vec2(10., 95.);
     faceIndexs[1] = vec2(22., 91.);
     faceIndexs[2] = vec2(14., 20.);
     faceIndexs[3] = vec2(18., 13.);

     for(int i = 0; i < 4; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, vShapeDelta);
     }
     return currentCoordinate;
 }

// 窄脸调整
   vec2 narrowFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
    
     vec2 faceIndexs[6];
    faceIndexs[0] = vec2(0., 33.);
     faceIndexs[1] = vec2(33., 0.);
     faceIndexs[2] = vec2(3., 29.);
     faceIndexs[3] = vec2(29., 3.);
     faceIndexs[4] = vec2(7., 25.);
     faceIndexs[5] = vec2(25., 7.);
 

     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, narrowFaceDelta);
     }
     return currentCoordinate;
 }

// 短脸调整
   vec2 shortFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
     vec2 faceIndexs[5];
     faceIndexs[0] = vec2(10., 52.);
     faceIndexs[1] = vec2(22., 61.);
     faceIndexs[2] = vec2(14., 82.);
     faceIndexs[3] = vec2(18., 83.);
     faceIndexs[4] = vec2(16., 49.);
     
     for(int i = 0; i < 5; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, shortFaceDelta);
     }
     return currentCoordinate;
 }

// 颧骨调整
    vec2 adjustCheekbones(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[6];
      faceIndexs[0] = vec2(3., 44.);
      faceIndexs[1] = vec2(29., 44.);
      faceIndexs[2] = vec2(4., 80.);
      faceIndexs[3] = vec2(28., 81.);
      faceIndexs[4] = vec2(5., 80.);
      faceIndexs[5] = vec2(27., 81.);

     
     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, cheekbonesDelta);
     }
     return currentCoordinate;
 }

// 下颌骨调整
  vec2 adjustJawbone(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[8];
      faceIndexs[0] = vec2(7., 49.);
      faceIndexs[1] = vec2(25., 49.);
      faceIndexs[2] = vec2(8., 49.);
      faceIndexs[3] = vec2(24., 49.);
      faceIndexs[4] = vec2(9., 87.);
      faceIndexs[5] = vec2(23., 87.);
      faceIndexs[6] = vec2(10., 87.);
      faceIndexs[7] = vec2(22., 87.);
     
     for(int i = 0; i < 8; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, jawboneDelta);
     }
     return currentCoordinate;
 }


 // 下巴调整
  vec2 adjustChin(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[7];
      faceIndexs[0] = vec2(13., 82.);
      faceIndexs[1] = vec2(19., 83.);
      faceIndexs[2] = vec2(14., 47.);
      faceIndexs[3] = vec2(18., 51.);
      faceIndexs[4] = vec2(15., 48.);
      faceIndexs[5] = vec2(17., 50.);
      faceIndexs[6] = vec2(16., 49.);
     
     for(int i = 0; i < 7; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, chinDelta);
     }
     return currentCoordinate;
 }

  // 瘦鼻功能
  vec2 slimNose(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[6];
      faceIndexs[0] = vec2(80., 81.);
      faceIndexs[1] = vec2(81., 80.);
      faceIndexs[2] = vec2(82., 83.);
      faceIndexs[3] = vec2(83., 82.);
      faceIndexs[4] = vec2(47., 51.);
      faceIndexs[5] = vec2(51., 47.);
     
     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, noseDelta);
     }
     return currentCoordinate;
 }

   // 嘴型大小调整
  vec2 adjustMouthSize(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[6];
      faceIndexs[0] = vec2(80., 81.);
      faceIndexs[1] = vec2(81., 80.);
      faceIndexs[2] = vec2(82., 83.);
      faceIndexs[3] = vec2(83., 82.);
      faceIndexs[4] = vec2(47., 51.);
      faceIndexs[5] = vec2(51., 47.);
     
     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, mouthSizeDelta);
     }
     return currentCoordinate;
 }

    // 眼距调整 - 使用改进的椭圆形平移函数
  vec2 adjustEyeDistance(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;     
    
    // 左眼 - 使用椭圆形高斯平移
    vec2 leftCenter = vec2(facePoints[baseIndex + 74 * 2], facePoints[baseIndex + 74 * 2 + 1]);
    vec2 eyeSize = vec2(0.08, 0.04);  // 缩小影响区域
    currentCoordinate = ellipseTranslateGaussian(currentCoordinate, leftCenter, eyeSize, vec2(0.15, 0.0), eyeDistanceDelta);
    
    // 右眼 - 使用椭圆形高斯平移
    vec2 rightCenter = vec2(facePoints[baseIndex + 77 * 2], facePoints[baseIndex + 77 * 2 + 1]);
    currentCoordinate = ellipseTranslateGaussian(currentCoordinate, rightCenter, eyeSize, vec2(-0.15, 0.0), eyeDistanceDelta);
    
    return currentCoordinate;
 }

 // 嘴巴位置调整 - 使用改进的椭圆形平移函数
 vec2 adjustMouthPosition(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;     
 
    vec2 center = vec2(facePoints[baseIndex + 93 * 2], facePoints[baseIndex + 93 * 2 + 1]);
    
    // 方案1：使用高斯椭圆形平移（推荐）
    // vec2 size = vec2(0.1, 0.1);  // 稍微缩小影响区域
    // currentCoordinate = ellipseTranslateGaussian(currentCoordinate, center, size, vec2(0.0, -0.15), thinFaceDelta);
    
         // 方案2：使用多层次椭圆形平移（最自然，可选）
     vec2 size = vec2(0.15, 0.12);  // 微调椭圆大小以配合93号landmark
     currentCoordinate = ellipseTranslateMultiLayer(currentCoordinate, center, size, vec2(0.0, -0.10), mouthPositionDelta);
    
    // 方案3：使用圆形余弦平移（简单有效，可选）
    // float radius = 0.08;
    // currentCoordinate = circleTranslateCosine(currentCoordinate, center, radius, vec2(0.0, -0.12), thinFaceDelta);
 
    return currentCoordinate;
 }
 
// 大眼功能
 vec2 bigEye(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;

     vec2 faceIndexs[2];
     faceIndexs[0] = vec2(74., 72.);
     faceIndexs[1] = vec2(77., 75.);

     for(int i = 0; i < 2; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);

         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);

         float radius = distance(vec2(targetPoint.x, targetPoint.y / aspectRatio), vec2(originPoint.x, originPoint.y / aspectRatio));
         radius = radius * 5.;
         currentCoordinate = enlargeEye(currentCoordinate, originPoint, radius, bigEyeDelta);
     }
     return currentCoordinate;
 }

 void main()
 {
     vec2 positionToUse = textureCoordinate;

     for(int i = 0; i < faceCount; i++) {
         // 脸型调整
         positionToUse = thinFace(positionToUse, i);
         positionToUse = vShapeFace(positionToUse, i);
         positionToUse = narrowFace(positionToUse, i);
         positionToUse = shortFace(positionToUse, i);
         
         // 面部特征调整
         positionToUse = adjustCheekbones(positionToUse, i);
         positionToUse = adjustJawbone(positionToUse, i);
         positionToUse = adjustChin(positionToUse, i);
         positionToUse = slimNose(positionToUse, i);
         
         // 嘴巴和眼部调整
         positionToUse = adjustMouthSize(positionToUse, i);
         positionToUse = adjustMouthPosition(positionToUse, i);
         positionToUse = adjustEyeDistance(positionToUse, i);
         positionToUse = bigEye(positionToUse, i);
     }

     gl_FragColor = texture2D(inputImageTexture, positionToUse);
 }
 )";
#endif

FaceReshapeFilter::FaceReshapeFilter() {}

FaceReshapeFilter::~FaceReshapeFilter() {}

std::shared_ptr<FaceReshapeFilter> FaceReshapeFilter::Create() {
  auto ret = std::shared_ptr<FaceReshapeFilter>(new FaceReshapeFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool FaceReshapeFilter::Init() {
  if (!InitWithFragmentShaderString(kGPUPixelThinFaceFragmentShaderString)) {
    return false;
  }
  RegisterProperty("thin_face", 0,
                   "The face slimming level with range between -1 and 1.",
                   [this](float& val) { SetFaceSlimLevel(val); });

  RegisterProperty("big_eye", 0,
                   "The eye enlargement level with range between -1 and 1.",
                   [this](float& val) { SetEyeZoomLevel(val); });

  RegisterProperty("v_shape", 0,
                   "The V-shape face level with range between -1 and 1.",
                   [this](float& val) { SetVShapeLevel(val); });

  RegisterProperty("narrow_face", 0,
                   "The narrow face level with range between -1 and 1.",
                   [this](float& val) { SetNarrowFaceLevel(val); });

  RegisterProperty("short_face", 0,
                   "The short face level with range between -1 and 1.",
                   [this](float& val) { SetShortFaceLevel(val); });

  RegisterProperty(
      "cheekbones", 0,
      "The cheekbones adjustment level with range between -1 and 1.",
      [this](float& val) { SetCheekbonesLevel(val); });

  RegisterProperty("jawbone", 0,
                   "The jawbone adjustment level with range between -1 and 1.",
                   [this](float& val) { SetJawboneLevel(val); });

  RegisterProperty("chin", 0,
                   "The chin adjustment level with range between -1 and 1.",
                   [this](float& val) { SetChinLevel(val); });

  RegisterProperty("nose", 0,
                   "The nose slimming level with range between -1 and 1.",
                   [this](float& val) { SetNoseLevel(val); });

  RegisterProperty(
      "mouth_size", 0,
      "The mouth size adjustment level with range between -1 and 1.",
      [this](float& val) { SetMouthSizeLevel(val); });

  RegisterProperty(
      "eye_distance", 0,
      "The eye distance adjustment level with range between -1 and 1.",
      [this](float& val) { SetEyeDistanceLevel(val); });

  RegisterProperty(
      "mouth_position", 0,
      "The mouth position adjustment level with range between -1 and 1.",
      [this](float& val) { SetMouthPositionLevel(val); });

  std::vector<float> defaut;
  RegisterProperty("face_landmark", defaut,
                   "The face landmark of filter with range between -1 and 1.",
                   [this](std::vector<float> val) { SetFaceLandmarks(val); });

  this->thin_face_delta_ = 0.0;
  this->big_eye_delta_ = 0.0;
  this->vshape_delta_ = 0.0;
  this->narrow_face_delta_ = 0.0;
  this->short_face_delta_ = 0.0;
  this->cheekbones_delta_ = 0.0;
  this->jawbone_delta_ = 0.0;
  this->chin_delta_ = 0.0;
  this->nose_delta_ = 0.0;
  this->mouth_size_delta_ = 0.0;
  this->eye_distance_delta_ = 0.0;
  this->mouth_position_delta_ = 0.0;
  return true;
}

// 处理单个或多个人脸的关键点数据（每个人脸有222个点，包含x,y坐标）
void FaceReshapeFilter::SetFaceLandmarks(std::vector<float> landmarks) {
  face_landmarks_.clear();

  if (landmarks.empty()) {
    return;
  }
  face_landmarks_ = landmarks;
  face_count_ = static_cast<int>(face_landmarks_.size() / 222);
}

// 使用当前人脸关键点和效果参数渲染滤镜
bool FaceReshapeFilter::DoRender(bool updateSinks) {
  float aspect = (float)framebuffer_->GetWidth() / framebuffer_->GetHeight();
  filter_program_->SetUniformValue("aspectRatio", aspect);
  filter_program_->SetUniformValue("thinFaceDelta", thin_face_delta_);
  filter_program_->SetUniformValue("bigEyeDelta", big_eye_delta_);
  filter_program_->SetUniformValue("vShapeDelta", vshape_delta_);
  filter_program_->SetUniformValue("narrowFaceDelta", narrow_face_delta_);
  filter_program_->SetUniformValue("shortFaceDelta", short_face_delta_);
  filter_program_->SetUniformValue("cheekbonesDelta", cheekbones_delta_);
  filter_program_->SetUniformValue("jawboneDelta", jawbone_delta_);
  filter_program_->SetUniformValue("chinDelta", chin_delta_);
  filter_program_->SetUniformValue("noseDelta", nose_delta_);
  filter_program_->SetUniformValue("mouthSizeDelta", mouth_size_delta_);
  filter_program_->SetUniformValue("eyeDistanceDelta", eye_distance_delta_);
  filter_program_->SetUniformValue("mouthPositionDelta", mouth_position_delta_);

  filter_program_->SetUniformValue("faceCount", face_count_);

  if (!face_landmarks_.empty()) {
    filter_program_->SetUniformValue("facePoints", face_landmarks_.data(),
                                     static_cast<int>(face_landmarks_.size()));
  }

  return Filter::DoRender(updateSinks);
}

#pragma mark - 瘦脸
void FaceReshapeFilter::SetFaceSlimLevel(float level) {
  thin_face_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 大眼
void FaceReshapeFilter::SetEyeZoomLevel(float level) {
  // 推荐范围: [0, 0.15]
  big_eye_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - V脸
void FaceReshapeFilter::SetVShapeLevel(float level) {
  vshape_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 窄脸
void FaceReshapeFilter::SetNarrowFaceLevel(float level) {
  narrow_face_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 短脸
void FaceReshapeFilter::SetShortFaceLevel(float level) {
  short_face_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 颧骨
void FaceReshapeFilter::SetCheekbonesLevel(float level) {
  cheekbones_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 下颌骨
void FaceReshapeFilter::SetJawboneLevel(float level) {
  jawbone_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 下巴
void FaceReshapeFilter::SetChinLevel(float level) {
  chin_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 瘦鼻
void FaceReshapeFilter::SetNoseLevel(float level) {
  nose_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 嘴型大小
void FaceReshapeFilter::SetMouthSizeLevel(float level) {
  mouth_size_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 眼距
void FaceReshapeFilter::SetEyeDistanceLevel(float level) {
  eye_distance_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - 嘴巴位置
void FaceReshapeFilter::SetMouthPositionLevel(float level) {
  mouth_position_delta_ = std::clamp(level, -1.0f, 1.0f);
}

}  // namespace gpupixel
