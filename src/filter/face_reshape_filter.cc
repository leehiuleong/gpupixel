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
 
 // Basic rectangle area pixel translation function
 vec2 rectangleTranslate(vec2 textureCoord, vec2 rectTopLeft, vec2 rectBottomRight, vec2 translateOffset) {
     // Check if current pixel is within rectangle bounds
     bool inRect = (textureCoord.x >= rectTopLeft.x && textureCoord.x <= rectBottomRight.x) &&
                   (textureCoord.y >= rectTopLeft.y && textureCoord.y <= rectBottomRight.y);
     
     if (inRect) {
         // Inside rectangle, apply translation offset
         return textureCoord + translateOffset;
     } else {
         // Outside rectangle, keep original coordinates
         return textureCoord;
     }
 }


 // Rectangle area pixel translation function with smooth edges
 vec2 rectangleTranslateSmooth(vec2 textureCoord, vec2 rectCenter, vec2 rectSize, vec2 translateOffset, float smoothEdge) {
     // Calculate relative coordinates to rectangle center
     vec2 relativePos = abs(textureCoord - rectCenter);
     vec2 halfSize = rectSize * 0.5;
     
     // Calculate distance to rectangle boundary
     vec2 distToEdge = halfSize - relativePos;
     float minDistToEdge = min(distToEdge.x, distToEdge.y);
     
     // Calculate weight: weight is 1 inside rectangle, smooth transition at edges
     float weight = smoothstep(0.0, smoothEdge, minDistToEdge);
     weight = clamp(weight, 0.0, 1.0);
     
     // Apply weighted translation offset
     return textureCoord + translateOffset * weight;
 }
 
 // Improved solution 1: Elliptical area translation using Gaussian function (recommended for mouth)
 vec2 ellipseTranslateGaussian(vec2 textureCoord, vec2 center, vec2 size, vec2 translateOffset, float intensity) {
     // Calculate normalized distance
     vec2 relativePos = (textureCoord - center) / size;
     float distance = length(relativePos);
     
     // Use Gaussian function to calculate weight, producing more natural transition
     float weight = exp(-distance * distance * 4.0) * intensity;
     weight = clamp(weight, 0.0, 1.0);
     
     return textureCoord + translateOffset * weight;
 }
 
 // Improved solution 2: Circular area translation using cosine interpolation
 vec2 circleTranslateCosine(vec2 textureCoord, vec2 center, float radius, vec2 translateOffset, float intensity) {
     float dist = distance(textureCoord, center);
     
     if (dist > radius) {
         return textureCoord;
     }
     
     // Use cosine interpolation to produce S-curve transition
     float normalizedDist = dist / radius;
     float weight = 0.5 * (1.0 + cos(3.14159 * normalizedDist)) * intensity;
     weight = clamp(weight, 0.0, 1.0);
     
     return textureCoord + translateOffset * weight;
 }
 
 // Improved solution 3: Multi-layer decay elliptical translation (most natural effect)
 vec2 ellipseTranslateMultiLayer(vec2 textureCoord, vec2 center, vec2 size, vec2 translateOffset, float intensity) {
     vec2 relativePos = textureCoord - center;
     vec2 normalizedPos = relativePos / size;
     float ellipseDist = length(normalizedPos);
     
     if (ellipseDist > 1.0) {
         return textureCoord;
     }
     
     // Multi-layer decay: core area + transition area + edge area
     float weight = 0.0;
     if (ellipseDist < 0.3) {
         // Core area: almost complete translation
         weight = intensity * 0.95;
     } else if (ellipseDist < 0.7) {
         // Main transition area: smooth decay
         float t = (ellipseDist - 0.3) / 0.4;
         weight = intensity * (0.95 - 0.7 * smoothstep(0.0, 1.0, t));
     } else {
         // Edge transition area: rapid decay to 0
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

// Face slimming
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

// V-shape face adjustment
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// Narrow face adjustment
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// Short face adjustment
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// Cheekbone adjustment
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// Jawbone adjustment
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }


 // Chin adjustment
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

  // Nose slimming
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

   // Mouth size adjustment
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
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

    // Eye distance adjustment - using improved elliptical translation function
  vec2 adjustEyeDistance(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;     
    
    // Left eye - using elliptical Gaussian translation
    vec2 leftCenter = vec2(facePoints[baseIndex + 74 * 2], facePoints[baseIndex + 74 * 2 + 1]);
    vec2 eyeSize = vec2(0.08, 0.04);  // Reduce influence area
    currentCoordinate = ellipseTranslateGaussian(currentCoordinate, leftCenter, eyeSize, vec2(0.15, 0.0), thinFaceDelta);
    
    // Right eye - using elliptical Gaussian translation
    vec2 rightCenter = vec2(facePoints[baseIndex + 77 * 2], facePoints[baseIndex + 77 * 2 + 1]);
    currentCoordinate = ellipseTranslateGaussian(currentCoordinate, rightCenter, eyeSize, vec2(-0.15, 0.0), thinFaceDelta);
    
    return currentCoordinate;
 }

 // Mouth position adjustment - using improved elliptical translation function
 vec2 adjustMouthPosition(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;     
 
    vec2 center = vec2(facePoints[baseIndex + 93 * 2], facePoints[baseIndex + 93 * 2 + 1]);
    
    // Option 1: Using Gaussian elliptical translation (recommended)
    // vec2 size = vec2(0.1, 0.1);  // Slightly reduce influence area
    // currentCoordinate = ellipseTranslateGaussian(currentCoordinate, center, size, vec2(0.0, -0.15), thinFaceDelta);
    
         // Option 2: Using multi-layer elliptical translation (most natural, optional)
     vec2 size = vec2(0.15, 0.12);  // Fine-tune ellipse size to match landmark 93
     currentCoordinate = ellipseTranslateMultiLayer(currentCoordinate, center, size, vec2(0.0, -0.10), thinFaceDelta);
    
    // Option 3: Using circular cosine translation (simple and effective, optional)
    // float radius = 0.08;
    // currentCoordinate = circleTranslateCosine(currentCoordinate, center, radius, vec2(0.0, -0.12), thinFaceDelta);
 
    return currentCoordinate;
 }
 
// Eye enlargement
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
        //  positionToUse = thinFace(positionToUse, i);
         positionToUse = adjustMouthPosition(positionToUse, i);
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
                   "The smoothing of filter with range between -1 and 1.",
                   [this](float& val) { SetFaceSlimLevel(val); });

  RegisterProperty("big_eye", 0,
                   "The smoothing of filter with range between -1 and 1.",
                   [this](float& val) { SetEyeZoomLevel(val); });

  std::vector<float> defaut;
  RegisterProperty("face_landmark", defaut,
                   "The face landmark of filter with range between -1 and 1.",
                   [this](std::vector<float> val) { SetFaceLandmarks(val); });

  this->thin_face_delta_ = 0.0;
  // [0, 0.15]
  this->big_eye_delta_ = 0.0;
  return true;
}

// Process landmarks for single or multiple faces (each face has 478 points with
// x,y coordinates)
void FaceReshapeFilter::SetFaceLandmarks(std::vector<float> landmarks) {
  face_landmarks_.clear();

  if (landmarks.empty()) {
    return;
  }
  face_landmarks_ = landmarks;
  face_count_ = static_cast<int>(face_landmarks_.size() / 222);
}

// Render the filter with current face landmarks and effect parameters
bool FaceReshapeFilter::DoRender(bool updateSinks) {
  float aspect = (float)framebuffer_->GetWidth() / framebuffer_->GetHeight();
  filter_program_->SetUniformValue("aspectRatio", aspect);
  filter_program_->SetUniformValue("thinFaceDelta", thin_face_delta_);
  filter_program_->SetUniformValue("bigEyeDelta", big_eye_delta_);

  filter_program_->SetUniformValue("faceCount", face_count_);

  if (!face_landmarks_.empty()) {
    filter_program_->SetUniformValue("facePoints", face_landmarks_.data(),
                                     static_cast<int>(face_landmarks_.size()));
  }

  return Filter::DoRender(updateSinks);
}

#pragma mark - face slim
void FaceReshapeFilter::SetFaceSlimLevel(float level) {
  thin_face_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - eye zoom
void FaceReshapeFilter::SetEyeZoomLevel(float level) {
  // Recommended range: [0, 0.15]
  big_eye_delta_ = std::clamp(level, -1.0f, 1.0f);
}

}  // namespace gpupixel
