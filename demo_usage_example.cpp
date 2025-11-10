/*
 * 三个新滤镜的使用示例代码
 * 
 * 1. MaskOverlayFilter - 面罩滤镜
 * 2. NoseDeroFilter - 鼻子贴纸滤镜
 * 3. EyeDeroFilter - 眼镜滤镜
 */

#include <gpupixel/gpupixel.h>
#include <string>
#include <vector>

using namespace gpupixel;

// 示例：使用三个新滤镜的完整代码
void ExampleUsage() {
  // ========== 1. 初始化资源路径 ==========
  GPUPixel::SetResourcePath("/path/to/resource");
  
  // ========== 2. 创建人脸检测器 ==========
  std::shared_ptr<FaceDetector> face_detector = FaceDetector::Create();
  
  // ========== 3. 创建源图片 ==========
  std::shared_ptr<SourceImage> source_image = SourceImage::Create("/path/to/image.png");
  
  // ========== 4. 创建三个新滤镜 ==========
  // 面罩滤镜
  std::shared_ptr<MaskOverlayFilter> mask_filter = MaskOverlayFilter::Create();
  
  // 鼻子贴纸滤镜
  std::shared_ptr<NoseDeroFilter> nose_filter = NoseDeroFilter::Create();
  
  // 眼镜滤镜
  std::shared_ptr<EyeDeroFilter> eye_filter = EyeDeroFilter::Create();
  
  // ========== 5. 加载叠加图片纹理 ==========
  // 面罩图片
  std::shared_ptr<SourceImage> mask_texture = SourceImage::Create("/path/to/mask.png");
  mask_filter->SetImageTexture(mask_texture);
  
  // 鼻子贴纸图片
  std::shared_ptr<SourceImage> nose_texture = SourceImage::Create("/path/to/nose_sticker.png");
  nose_filter->SetImageTexture(nose_texture);
  
  // 眼镜图片
  std::shared_ptr<SourceImage> eye_texture = SourceImage::Create("/path/to/glasses.png");
  eye_filter->SetImageTexture(eye_texture);
  
  // ========== 6. 设置滤镜参数 ==========
  // 面罩滤镜参数
  mask_filter->SetOpacity(1.0f);      // 透明度 [0.0, 1.0]
  mask_filter->SetBlendLevel(1.0f);   // 混合强度 [0.0, 1.0]
  
  // 鼻子贴纸滤镜参数
  nose_filter->SetOpacity(1.0f);
  nose_filter->SetBlendLevel(1.0f);
  nose_filter->SetImageSize(0.2f, 0.2f);  // 设置图片大小（归一化坐标）
  
  // 眼镜滤镜参数
  eye_filter->SetOpacity(1.0f);
  eye_filter->SetBlendLevel(1.0f);
  
  // ========== 7. 创建输出 ==========
  std::shared_ptr<SinkRawData> sink_raw_data = SinkRawData::Create();
  
  // ========== 8. 构建滤镜管道 ==========
  source_image->AddSink(mask_filter)
      ->AddSink(nose_filter)
      ->AddSink(eye_filter)
      ->AddSink(sink_raw_data);
  
  // ========== 9. 处理图片 ==========
  // 获取图片数据
  int width = source_image->GetWidth();
  int height = source_image->GetHeight();
  const unsigned char* buffer = source_image->GetRgbaImageBuffer();
  
  // 检测人脸关键点
  std::vector<float> landmarks = face_detector->Detect(
      buffer, width, height, width * 4,
      GPUPIXEL_MODE_FMT_PICTURE,
      GPUPIXEL_FRAME_TYPE_RGBA);
  
  // 设置人脸关键点到滤镜
  if (!landmarks.empty()) {
    mask_filter->SetFaceLandmarks(landmarks);
    nose_filter->SetFaceLandmarks(landmarks);
    eye_filter->SetFaceLandmarks(landmarks);
  }
  
  // 渲染处理后的图片
  source_image->Render();
  
  // ========== 10. 获取处理结果 ==========
  const uint8_t* processed_data = sink_raw_data->GetRgbaBuffer();
  int processed_width = sink_raw_data->GetWidth();
  int processed_height = sink_raw_data->GetHeight();
  
  // 使用处理后的数据...
}

// ========== 单独使用每个滤镜的示例 ==========

// 示例1：只使用面罩滤镜
void ExampleMaskFilter() {
  std::shared_ptr<SourceImage> source = SourceImage::Create("/path/to/image.png");
  std::shared_ptr<MaskOverlayFilter> mask_filter = MaskOverlayFilter::Create();
  std::shared_ptr<SourceImage> mask_texture = SourceImage::Create("/path/to/mask.png");
  std::shared_ptr<SinkRawData> sink = SinkRawData::Create();
  
  mask_filter->SetImageTexture(mask_texture);
  mask_filter->SetOpacity(0.8f);
  mask_filter->SetBlendLevel(1.0f);
  
  source->AddSink(mask_filter)->AddSink(sink);
  
  // 检测人脸并设置关键点
  std::shared_ptr<FaceDetector> detector = FaceDetector::Create();
  int width = source->GetWidth();
  int height = source->GetHeight();
  const unsigned char* buffer = source->GetRgbaImageBuffer();
  
  std::vector<float> landmarks = detector->Detect(
      buffer, width, height, width * 4,
      GPUPIXEL_MODE_FMT_PICTURE,
      GPUPIXEL_FRAME_TYPE_RGBA);
  
  if (!landmarks.empty()) {
    mask_filter->SetFaceLandmarks(landmarks);
  }
  
  source->Render();
}

// 示例2：只使用鼻子贴纸滤镜
void ExampleNoseFilter() {
  std::shared_ptr<SourceImage> source = SourceImage::Create("/path/to/image.png");
  std::shared_ptr<NoseDeroFilter> nose_filter = NoseDeroFilter::Create();
  std::shared_ptr<SourceImage> nose_texture = SourceImage::Create("/path/to/nose_sticker.png");
  std::shared_ptr<SinkRawData> sink = SinkRawData::Create();
  
  nose_filter->SetImageTexture(nose_texture);
  nose_filter->SetOpacity(1.0f);
  nose_filter->SetBlendLevel(1.0f);
  nose_filter->SetImageSize(0.15f, 0.15f);  // 设置贴纸大小
  
  source->AddSink(nose_filter)->AddSink(sink);
  
  // 检测人脸并设置关键点
  std::shared_ptr<FaceDetector> detector = FaceDetector::Create();
  int width = source->GetWidth();
  int height = source->GetHeight();
  const unsigned char* buffer = source->GetRgbaImageBuffer();
  
  std::vector<float> landmarks = detector->Detect(
      buffer, width, height, width * 4,
      GPUPIXEL_MODE_FMT_PICTURE,
      GPUPIXEL_FRAME_TYPE_RGBA);
  
  if (!landmarks.empty()) {
    nose_filter->SetFaceLandmarks(landmarks);
  }
  
  source->Render();
}

// 示例3：只使用眼镜滤镜
void ExampleEyeFilter() {
  std::shared_ptr<SourceImage> source = SourceImage::Create("/path/to/image.png");
  std::shared_ptr<EyeDeroFilter> eye_filter = EyeDeroFilter::Create();
  std::shared_ptr<SourceImage> eye_texture = SourceImage::Create("/path/to/glasses.png");
  std::shared_ptr<SinkRawData> sink = SinkRawData::Create();
  
  eye_filter->SetImageTexture(eye_texture);
  eye_filter->SetOpacity(1.0f);
  eye_filter->SetBlendLevel(1.0f);
  
  source->AddSink(eye_filter)->AddSink(sink);
  
  // 检测人脸并设置关键点
  std::shared_ptr<FaceDetector> detector = FaceDetector::Create();
  int width = source->GetWidth();
  int height = source->GetHeight();
  const unsigned char* buffer = source->GetRgbaImageBuffer();
  
  std::vector<float> landmarks = detector->Detect(
      buffer, width, height, width * 4,
      GPUPIXEL_MODE_FMT_PICTURE,
      GPUPIXEL_FRAME_TYPE_RGBA);
  
  if (!landmarks.empty()) {
    eye_filter->SetFaceLandmarks(landmarks);
  }
  
  source->Render();
}

