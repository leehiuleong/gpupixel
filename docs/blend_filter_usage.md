# BlendFilter 图层叠加滤镜使用指南

## 概述

BlendFilter 是一个强大的图层叠加滤镜，支持类似 Photoshop 的图层混合模式。它允许你动态添加多个图片图层，每个图层可以设置不同的混合模式、不透明度和强度，从而实现复杂的视觉效果。

## 主要特性

- **多种混合模式**: 支持 18 种混合模式，包括正片叠底、滤色、叠加、柔光等
- **动态图层管理**: 可以动态添加、移除、修改图层
- **独立图层控制**: 每个图层可以独立设置不透明度、强度和混合模式
- **全局控制**: 支持全局不透明度和强度调整
- **实时更新**: 支持实时修改图层属性，无需重新创建滤镜

## 支持的混合模式

| 模式 | 枚举值 | 描述 | 效果 |
|------|--------|------|------|
| 正常 | NORMAL | 直接覆盖 | 上层完全覆盖下层 |
| 正片叠底 | MULTIPLY | 相乘混合 | 使图像变暗，适合阴影效果 |
| 滤色 | SCREEN | 滤色混合 | 使图像变亮，适合高光效果 |
| 叠加 | OVERLAY | 叠加混合 | 增强对比度，保持高光和阴影 |
| 柔光 | SOFT_LIGHT | 柔光混合 | 温和的对比度调整 |
| 强光 | HARD_LIGHT | 强光混合 | 强烈的对比度调整 |
| 颜色减淡 | COLOR_DODGE | 颜色减淡 | 使图像变亮，适合发光效果 |
| 颜色加深 | COLOR_BURN | 颜色加深 | 使图像变暗，适合阴影效果 |
| 变暗 | DARKEN | 变暗混合 | 选择较暗的颜色 |
| 变亮 | LIGHTEN | 变亮混合 | 选择较亮的颜色 |
| 差值 | DIFFERENCE | 差值混合 | 产生反色效果 |
| 排除 | EXCLUSION | 排除混合 | 类似差值但对比度较低 |
| 线性减淡 | LINEAR_DODGE | 线性减淡 | 线性增亮效果 |
| 线性加深 | LINEAR_BURN | 线性加深 | 线性变暗效果 |
| 亮光 | VIVID_LIGHT | 亮光混合 | 强烈的亮光效果 |
| 线性光 | LINEAR_LIGHT | 线性光混合 | 线性光效果 |
| 点光 | PIN_LIGHT | 点光混合 | 点光源效果 |
| 实色混合 | HARD_MIX | 实色混合 | 产生纯色效果 |

## 基本使用方法

### 1. 创建 BlendFilter

```cpp
#include "blend_filter.h"

// 创建滤镜实例
auto blendFilter = BlendFilter::create();
if (!blendFilter) {
    // 处理创建失败
    return;
}
```

### 2. 添加图层

```cpp
// 添加正片叠底图层
int layer1 = blendFilter->addLayer(
    "texture1.png",           // 图片路径
    BlendMode::MULTIPLY,      // 混合模式
    0.8f,                     // 不透明度 (0.0-1.0)
    1.0f                      // 强度 (0.0-1.0)
);

// 添加滤色图层
int layer2 = blendFilter->addLayer(
    "texture2.png",
    BlendMode::SCREEN,
    0.6f,
    1.0f
);
```

### 3. 修改图层属性

```cpp
// 修改图层不透明度
blendFilter->setLayerOpacity(layer1, 0.5f);

// 修改图层混合模式
blendFilter->setLayerBlendMode(layer2, BlendMode::OVERLAY);

// 修改图层强度
blendFilter->setLayerIntensity(layer1, 0.8f);

// 启用/禁用图层
blendFilter->setLayerEnabled(layer1, false);
```

### 4. 设置全局属性

```cpp
// 设置全局不透明度
blendFilter->setGlobalOpacity(0.9f);

// 设置全局强度
blendFilter->setGlobalIntensity(1.0f);
```

### 5. 图层管理

```cpp
// 获取图层数量
int layerCount = blendFilter->getLayerCount();

// 获取图层信息
const BlendLayer* layer = blendFilter->getLayer(0);
if (layer) {
    std::cout << "图层路径: " << layer->imagePath << std::endl;
    std::cout << "混合模式: " << (int)layer->blendMode << std::endl;
    std::cout << "不透明度: " << layer->opacity << std::endl;
}

// 移除图层
blendFilter->removeLayer(0);

// 清空所有图层
blendFilter->removeAllLayers();
```

## 高级使用技巧

### 1. 创建复杂叠加效果

```cpp
// 基础纹理层 (正片叠底)
int baseLayer = blendFilter->addLayer("base.png", BlendMode::MULTIPLY, 0.8f, 1.0f);

// 高光层 (滤色)
int highlightLayer = blendFilter->addLayer("highlight.png", BlendMode::SCREEN, 0.4f, 1.0f);

// 细节层 (叠加)
int detailLayer = blendFilter->addLayer("detail.png", BlendMode::OVERLAY, 0.6f, 1.0f);

// 色彩调整层 (柔光)
int colorLayer = blendFilter->addLayer("color.png", BlendMode::SOFT_LIGHT, 0.5f, 1.0f);
```

### 2. 动态调整效果

```cpp
// 根据时间或用户输入动态调整
float time = getCurrentTime();
float intensity = 0.5f + 0.5f * sin(time * 0.01f);

blendFilter->setLayerIntensity(highlightLayer, intensity);
blendFilter->setGlobalOpacity(0.8f + 0.2f * cos(time * 0.005f));
```

### 3. 条件性图层控制

```cpp
// 根据条件启用/禁用图层
if (userWantsHighlight) {
    blendFilter->setLayerEnabled(highlightLayer, true);
    blendFilter->setLayerOpacity(highlightLayer, highlightIntensity);
} else {
    blendFilter->setLayerEnabled(highlightLayer, false);
}
```

## 性能优化建议

1. **纹理管理**: 尽量重用纹理，避免频繁加载/卸载
2. **图层数量**: 控制图层数量，过多图层会影响性能
3. **纹理尺寸**: 使用合适尺寸的纹理，过大的纹理会消耗更多内存
4. **动态更新**: 避免每帧都修改图层属性，只在需要时更新

## 注意事项

1. **图片格式**: 支持 PNG、JPG 等常见格式，建议使用 PNG 以获得更好的透明度支持
2. **纹理限制**: 受 OpenGL 纹理单元数量限制，建议同时使用的图层不超过 16 个
3. **内存管理**: 图层纹理会自动管理，但建议及时移除不需要的图层
4. **线程安全**: 当前实现不是线程安全的，请在主线程中使用

## 示例项目

参考 `examples/blend_filter_example.cpp` 获取完整的使用示例。

## 常见问题

**Q: 如何实现类似 Photoshop 的图层组效果？**
A: 可以创建多个 BlendFilter 实例，每个实例作为一个图层组，然后将它们串联使用。

**Q: 支持哪些图片格式？**
A: 支持 stb_image 库支持的所有格式，包括 PNG、JPG、BMP、TGA 等。

**Q: 如何实现渐变遮罩效果？**
A: 可以创建一个渐变纹理作为图层，使用适当的混合模式和不透明度。

**Q: 性能如何优化？**
A: 控制图层数量，使用合适尺寸的纹理，避免频繁的属性修改。
