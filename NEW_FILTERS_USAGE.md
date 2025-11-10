# 三个新滤镜使用说明

本文档介绍如何使用新创建的三个滤镜：
1. **MaskOverlayFilter** - 面罩滤镜
2. **NoseDeroFilter** - 鼻子贴纸滤镜
3. **EyeDeroFilter** - 眼镜滤镜

## 1. MaskOverlayFilter（面罩滤镜）

使用人脸关键点 2 到 30 的闭合路径作为蒙层，实现面罩效果。

### C++ 使用示例

```cpp
#include <gpupixel/gpupixel.h>
using namespace gpupixel;

// 创建滤镜
std::shared_ptr<MaskOverlayFilter> mask_filter = MaskOverlayFilter::Create();

// 加载面罩图片纹理
std::shared_ptr<SourceImage> mask_texture = SourceImage::Create("/path/to/mask.png");
mask_filter->SetImageTexture(mask_texture);

// 设置参数
mask_filter->SetOpacity(1.0f);      // 透明度 [0.0, 1.0]
mask_filter->SetBlendLevel(1.0f);   // 混合强度 [0.0, 1.0]

// 检测人脸并设置关键点
std::shared_ptr<FaceDetector> detector = FaceDetector::Create();
std::vector<float> landmarks = detector->Detect(
    buffer, width, height, width * 4,
    GPUPIXEL_MODE_FMT_PICTURE,
    GPUPIXEL_FRAME_TYPE_RGBA);

if (!landmarks.empty()) {
    mask_filter->SetFaceLandmarks(landmarks);
}

// 构建管道并渲染
source_image->AddSink(mask_filter)->AddSink(sink);
source_image->Render();
```

### Objective-C++ 使用示例

```objc
#import <gpupixel/gpupixel.h>
using namespace gpupixel;

// 创建滤镜
std::shared_ptr<MaskOverlayFilter> maskFilter = MaskOverlayFilter::Create();

// 加载面罩图片
NSString *maskPath = [[NSBundle mainBundle] pathForResource:@"mask" ofType:@"png"];
std::shared_ptr<SourceImage> maskTexture = SourceImage::Create([maskPath UTF8String]);
maskFilter->SetImageTexture(maskTexture);

// 设置参数
maskFilter->SetOpacity(1.0f);
maskFilter->SetBlendLevel(1.0f);

// 设置人脸关键点
maskFilter->SetFaceLandmarks(landmarks);
```

## 2. NoseDeroFilter（鼻子贴纸滤镜）

使用人脸关键点 45 作为中心点，图片跟随鼻子移动。

### C++ 使用示例

```cpp
#include <gpupixel/gpupixel.h>
using namespace gpupixel;

// 创建滤镜
std::shared_ptr<NoseDeroFilter> nose_filter = NoseDeroFilter::Create();

// 加载鼻子贴纸图片纹理
std::shared_ptr<SourceImage> nose_texture = SourceImage::Create("/path/to/nose_sticker.png");
nose_filter->SetImageTexture(nose_texture);

// 设置参数
nose_filter->SetOpacity(1.0f);
nose_filter->SetBlendLevel(1.0f);
nose_filter->SetImageSize(0.2f, 0.2f);  // 设置图片大小（归一化坐标，相对于人脸大小）

// 检测人脸并设置关键点
std::shared_ptr<FaceDetector> detector = FaceDetector::Create();
std::vector<float> landmarks = detector->Detect(
    buffer, width, height, width * 4,
    GPUPIXEL_MODE_FMT_PICTURE,
    GPUPIXEL_FRAME_TYPE_RGBA);

if (!landmarks.empty()) {
    nose_filter->SetFaceLandmarks(landmarks);
}

// 构建管道并渲染
source_image->AddSink(nose_filter)->AddSink(sink);
source_image->Render();
```

### Objective-C++ 使用示例

```objc
#import <gpupixel/gpupixel.h>
using namespace gpupixel;

// 创建滤镜
std::shared_ptr<NoseDeroFilter> noseFilter = NoseDeroFilter::Create();

// 加载鼻子贴纸图片
NSString *nosePath = [[NSBundle mainBundle] pathForResource:@"nose_sticker" ofType:@"png"];
std::shared_ptr<SourceImage> noseTexture = SourceImage::Create([nosePath UTF8String]);
noseFilter->SetImageTexture(noseTexture);

// 设置参数
noseFilter->SetOpacity(1.0f);
noseFilter->SetBlendLevel(1.0f);
noseFilter->SetImageSize(0.2f, 0.2f);  // 设置贴纸大小

// 设置人脸关键点
noseFilter->SetFaceLandmarks(landmarks);
```

## 3. EyeDeroFilter（眼镜滤镜）

使用人脸关键点 33、32、30、2 作为图片矩阵，实现眼镜效果。

### C++ 使用示例

```cpp
#include <gpupixel/gpupixel.h>
using namespace gpupixel;

// 创建滤镜
std::shared_ptr<EyeDeroFilter> eye_filter = EyeDeroFilter::Create();

// 加载眼镜图片纹理
std::shared_ptr<SourceImage> eye_texture = SourceImage::Create("/path/to/glasses.png");
eye_filter->SetImageTexture(eye_texture);

// 设置参数
eye_filter->SetOpacity(1.0f);
eye_filter->SetBlendLevel(1.0f);

// 检测人脸并设置关键点
std::shared_ptr<FaceDetector> detector = FaceDetector::Create();
std::vector<float> landmarks = detector->Detect(
    buffer, width, height, width * 4,
    GPUPIXEL_MODE_FMT_PICTURE,
    GPUPIXEL_FRAME_TYPE_RGBA);

if (!landmarks.empty()) {
    eye_filter->SetFaceLandmarks(landmarks);
}

// 构建管道并渲染
source_image->AddSink(eye_filter)->AddSink(sink);
source_image->Render();
```

### Objective-C++ 使用示例

```objc
#import <gpupixel/gpupixel.h>
using namespace gpupixel;

// 创建滤镜
std::shared_ptr<EyeDeroFilter> eyeFilter = EyeDeroFilter::Create();

// 加载眼镜图片
NSString *eyePath = [[NSBundle mainBundle] pathForResource:@"glasses" ofType:@"png"];
std::shared_ptr<SourceImage> eyeTexture = SourceImage::Create([eyePath UTF8String]);
eyeFilter->SetImageTexture(eyeTexture);

// 设置参数
eyeFilter->SetOpacity(1.0f);
eyeFilter->SetBlendLevel(1.0f);

// 设置人脸关键点
eyeFilter->SetFaceLandmarks(landmarks);
```

## 完整示例：同时使用三个滤镜

### C++ 完整示例

```cpp
#include <gpupixel/gpupixel.h>
using namespace gpupixel;

void SetupAllFilters() {
    // 1. 初始化资源路径
    GPUPixel::SetResourcePath("/path/to/resource");
    
    // 2. 创建人脸检测器
    std::shared_ptr<FaceDetector> face_detector = FaceDetector::Create();
    
    // 3. 创建源图片
    std::shared_ptr<SourceImage> source_image = SourceImage::Create("/path/to/image.png");
    
    // 4. 创建三个滤镜
    std::shared_ptr<MaskOverlayFilter> mask_filter = MaskOverlayFilter::Create();
    std::shared_ptr<NoseDeroFilter> nose_filter = NoseDeroFilter::Create();
    std::shared_ptr<EyeDeroFilter> eye_filter = EyeDeroFilter::Create();
    
    // 5. 加载叠加图片纹理
    std::shared_ptr<SourceImage> mask_texture = SourceImage::Create("/path/to/mask.png");
    mask_filter->SetImageTexture(mask_texture);
    
    std::shared_ptr<SourceImage> nose_texture = SourceImage::Create("/path/to/nose_sticker.png");
    nose_filter->SetImageTexture(nose_texture);
    
    std::shared_ptr<SourceImage> eye_texture = SourceImage::Create("/path/to/glasses.png");
    eye_filter->SetImageTexture(eye_texture);
    
    // 6. 设置滤镜参数
    mask_filter->SetOpacity(1.0f);
    mask_filter->SetBlendLevel(1.0f);
    
    nose_filter->SetOpacity(1.0f);
    nose_filter->SetBlendLevel(1.0f);
    nose_filter->SetImageSize(0.2f, 0.2f);
    
    eye_filter->SetOpacity(1.0f);
    eye_filter->SetBlendLevel(1.0f);
    
    // 7. 创建输出
    std::shared_ptr<SinkRawData> sink_raw_data = SinkRawData::Create();
    
    // 8. 构建滤镜管道
    source_image->AddSink(mask_filter)
        ->AddSink(nose_filter)
        ->AddSink(eye_filter)
        ->AddSink(sink_raw_data);
    
    // 9. 检测人脸关键点
    int width = source_image->GetWidth();
    int height = source_image->GetHeight();
    const unsigned char* buffer = source_image->GetRgbaImageBuffer();
    
    std::vector<float> landmarks = face_detector->Detect(
        buffer, width, height, width * 4,
        GPUPIXEL_MODE_FMT_PICTURE,
        GPUPIXEL_FRAME_TYPE_RGBA);
    
    // 10. 设置人脸关键点
    if (!landmarks.empty()) {
        mask_filter->SetFaceLandmarks(landmarks);
        nose_filter->SetFaceLandmarks(landmarks);
        eye_filter->SetFaceLandmarks(landmarks);
    }
    
    // 11. 渲染处理后的图片
    source_image->Render();
    
    // 12. 获取处理结果
    const uint8_t* processed_data = sink_raw_data->GetRgbaBuffer();
    int processed_width = sink_raw_data->GetWidth();
    int processed_height = sink_raw_data->GetHeight();
}
```

## API 说明

### MaskOverlayFilter

| 方法 | 说明 | 参数 |
|------|------|------|
| `SetImageTexture()` | 设置面罩图片纹理 | `std::shared_ptr<SourceImage>` |
| `SetFaceLandmarks()` | 设置人脸关键点（归一化坐标 0-1） | `std::vector<float>` |
| `SetOpacity()` | 设置透明度 | `float` [0.0, 1.0] |
| `SetBlendLevel()` | 设置混合强度 | `float` [0.0, 1.0] |

### NoseDeroFilter

| 方法 | 说明 | 参数 |
|------|------|------|
| `SetImageTexture()` | 设置鼻子贴纸图片纹理 | `std::shared_ptr<SourceImage>` |
| `SetFaceLandmarks()` | 设置人脸关键点（归一化坐标 0-1） | `std::vector<float>` |
| `SetOpacity()` | 设置透明度 | `float` [0.0, 1.0] |
| `SetBlendLevel()` | 设置混合强度 | `float` [0.0, 1.0] |
| `SetImageSize()` | 设置图片大小（归一化坐标） | `float width, float height` |

### EyeDeroFilter

| 方法 | 说明 | 参数 |
|------|------|------|
| `SetImageTexture()` | 设置眼镜图片纹理 | `std::shared_ptr<SourceImage>` |
| `SetFaceLandmarks()` | 设置人脸关键点（归一化坐标 0-1） | `std::vector<float>` |
| `SetOpacity()` | 设置透明度 | `float` [0.0, 1.0] |
| `SetBlendLevel()` | 设置混合强度 | `float` [0.0, 1.0] |

## 注意事项

1. **人脸关键点格式**：`SetFaceLandmarks()` 接受的参数是归一化坐标（0-1），每个点由两个 float 值表示（x, y）。

2. **图片纹理**：叠加图片需要是 RGBA 格式，支持透明通道。

3. **滤镜顺序**：多个滤镜可以串联使用，处理顺序会影响最终效果。

4. **实时处理**：在视频处理中，需要每帧都更新人脸关键点。

5. **资源路径**：确保图片资源路径正确，iOS 中可以使用 `NSBundle` 获取资源路径。

## 示例图片要求

- **面罩图片**：建议使用透明背景的 PNG 图片，覆盖人脸区域
- **鼻子贴纸**：建议使用透明背景的 PNG 图片，尺寸适中
- **眼镜图片**：建议使用透明背景的 PNG 图片，适合人脸宽度

