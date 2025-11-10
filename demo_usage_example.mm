/*
 * iOS 使用示例代码 (Objective-C++)
 * 
 * 三个新滤镜的使用示例：
 * 1. MaskOverlayFilter - 面罩滤镜
 * 2. NoseDeroFilter - 鼻子贴纸滤镜
 * 3. EyeDeroFilter - 眼镜滤镜
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <gpupixel/gpupixel.h>

using namespace gpupixel;

@interface NewFiltersExample : NSObject {
  // 滤镜
  std::shared_ptr<MaskOverlayFilter> _maskFilter;
  std::shared_ptr<NoseDeroFilter> _noseFilter;
  std::shared_ptr<EyeDeroFilter> _eyeFilter;
  
  // 源图片和纹理
  std::shared_ptr<SourceImage> _sourceImage;
  std::shared_ptr<SourceImage> _maskTexture;
  std::shared_ptr<SourceImage> _noseTexture;
  std::shared_ptr<SourceImage> _eyeTexture;
  
  // 人脸检测器
  std::shared_ptr<FaceDetector> _faceDetector;
  
  // 输出
  std::shared_ptr<SinkRawData> _sinkRawData;
  std::shared_ptr<SinkView> _sinkView;
}

- (void)setupFilters;
- (void)processImage:(UIImage *)image;
- (void)updateWithLandmarks:(std::vector<float>)landmarks;

@end

@implementation NewFiltersExample

- (void)setupFilters {
  // 1. 创建滤镜
  _maskFilter = MaskOverlayFilter::Create();
  _noseFilter = NoseDeroFilter::Create();
  _eyeFilter = EyeDeroFilter::Create();
  _faceDetector = FaceDetector::Create();
  _sinkRawData = SinkRawData::Create();
  
  // 2. 加载叠加图片纹理
  NSString *maskPath = [[NSBundle mainBundle] pathForResource:@"mask" ofType:@"png"];
  if (maskPath) {
    _maskTexture = SourceImage::Create([maskPath UTF8String]);
    _maskFilter->SetImageTexture(_maskTexture);
  }
  
  NSString *nosePath = [[NSBundle mainBundle] pathForResource:@"nose_sticker" ofType:@"png"];
  if (nosePath) {
    _noseTexture = SourceImage::Create([nosePath UTF8String]);
    _noseFilter->SetImageTexture(_noseTexture);
  }
  
  NSString *eyePath = [[NSBundle mainBundle] pathForResource:@"glasses" ofType:@"png"];
  if (eyePath) {
    _eyeTexture = SourceImage::Create([eyePath UTF8String]);
    _eyeFilter->SetImageTexture(_eyeTexture);
  }
  
  // 3. 设置滤镜参数
  _maskFilter->SetOpacity(1.0f);
  _maskFilter->SetBlendLevel(1.0f);
  
  _noseFilter->SetOpacity(1.0f);
  _noseFilter->SetBlendLevel(1.0f);
  _noseFilter->SetImageSize(0.2f, 0.2f);  // 设置鼻子贴纸大小
  
  _eyeFilter->SetOpacity(1.0f);
  _eyeFilter->SetBlendLevel(1.0f);
}

- (void)processImage:(UIImage *)image {
  // 1. 创建源图片
  NSString *tempPath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"temp_image.png"];
  NSData *imageData = UIImagePNGRepresentation(image);
  [imageData writeToFile:tempPath atomically:YES];
  
  _sourceImage = SourceImage::Create([tempPath UTF8String]);
  
  // 2. 构建滤镜管道
  _sourceImage->AddSink(_maskFilter)
      ->AddSink(_noseFilter)
      ->AddSink(_eyeFilter)
      ->AddSink(_sinkRawData);
  
  // 3. 检测人脸关键点
  int width = _sourceImage->GetWidth();
  int height = _sourceImage->GetHeight();
  const unsigned char *buffer = _sourceImage->GetRgbaImageBuffer();
  
  std::vector<float> landmarks = _faceDetector->Detect(
      buffer, width, height, width * 4,
      GPUPIXEL_MODE_FMT_PICTURE,
      GPUPIXEL_FRAME_TYPE_RGBA);
  
  // 4. 设置人脸关键点
  if (!landmarks.empty()) {
    [self updateWithLandmarks:landmarks];
  }
  
  // 5. 渲染处理后的图片
  _sourceImage->Render();
  
  // 6. 获取处理结果
  const uint8_t *processedData = _sinkRawData->GetRgbaBuffer();
  int processedWidth = _sinkRawData->GetWidth();
  int processedHeight = _sinkRawData->GetHeight();
  
  // 转换为 UIImage（需要 ImageConverter 工具类）
  // UIImage *resultImage = [ImageConverter imageFromRGBAData:processedData
  //                                                    width:processedWidth
  //                                                   height:processedHeight];
}

- (void)updateWithLandmarks:(std::vector<float>)landmarks {
  // 更新所有滤镜的人脸关键点
  _maskFilter->SetFaceLandmarks(landmarks);
  _noseFilter->SetFaceLandmarks(landmarks);
  _eyeFilter->SetFaceLandmarks(landmarks);
}

// 实时视频处理示例
- (void)processVideoFrame:(CMSampleBufferRef)sampleBuffer {
  CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (!imageBuffer) {
    return;
  }
  
  CVPixelBufferLockBaseAddress(imageBuffer, 0);
  auto width = CVPixelBufferGetWidth(imageBuffer);
  auto height = CVPixelBufferGetHeight(imageBuffer);
  auto stride = CVPixelBufferGetBytesPerRow(imageBuffer);
  auto pixels = (const uint8_t *)CVPixelBufferGetBaseAddress(imageBuffer);
  
  // 检测人脸关键点
  std::vector<float> landmarks = _faceDetector->Detect(
      pixels, width, height, stride,
      GPUPIXEL_MODE_FMT_VIDEO,
      GPUPIXEL_FRAME_TYPE_BGRA);
  
  // 更新滤镜的人脸关键点
  if (!landmarks.empty()) {
    _maskFilter->SetFaceLandmarks(landmarks);
    _noseFilter->SetFaceLandmarks(landmarks);
    _eyeFilter->SetFaceLandmarks(landmarks);
  }
  
  CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
  
  // 处理视频帧（需要 SourceRawData）
  // _sourceRawData->ProcessData(pixels, stride, height, stride, GPUPIXEL_FRAME_TYPE_BGRA);
}

@end

// ========== 在 ViewController 中使用示例 ==========

/*
@interface MyViewController : UIViewController {
  NewFiltersExample *_filterExample;
  std::shared_ptr<SinkView> _gpuPixelView;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  
  // 创建 GPU Pixel View
  _gpuPixelView = SinkView::Create((__bridge void *)self.view);
  
  // 初始化滤镜示例
  _filterExample = [[NewFiltersExample alloc] init];
  [_filterExample setupFilters];
  
  // 加载图片并处理
  UIImage *image = [UIImage imageNamed:@"sample_face"];
  [_filterExample processImage:image];
}
@end
*/

