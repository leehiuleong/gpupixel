# LookupFilter 使用说明

## 概述

LookupFilter 是一个基于查找表（LUT）的图像滤镜，它可以通过预定义的颜色映射来改变图像的整体色调和风格。这种滤镜常用于电影调色、照片风格化等应用场景。

## 工作原理

LookupFilter 使用一个 512x512 像素的查找表图像，其中：
- 图像的蓝色通道值被用来索引查找表的行
- 红色和绿色通道值被用来索引查找表的列
- 通过双线性插值计算最终的颜色值

## 使用方法

### 1. 基本用法

```cpp
#include "filter/lookup_filter.h"

// 创建滤镜实例
auto lookupFilter = LookupFilter::create("path/to/lookup.png");

// 设置滤镜强度 (0.0 到 1.0)
lookupFilter->setIntensity(0.8f);

// 连接到图像源
sourceImage->addTarget(lookupFilter);
```

### 2. 动态更改查找表

```cpp
// 更换查找表图像
lookupFilter->setLookupImagePath("path/to/new_lookup.png");

// 调整滤镜强度
lookupFilter->setIntensity(0.5f);
```

### 3. 属性系统

LookupFilter 支持通过属性系统进行配置：

```cpp
// 注册属性（在滤镜初始化时自动完成）
lookupFilter->registerProperty("intensity", 0.8f, "滤镜强度");

// 设置属性值
lookupFilter->setProperty("intensity", 0.6f);

// 获取属性值
float intensity;
if (lookupFilter->getProperty("intensity", intensity)) {
    std::cout << "Current intensity: " << intensity << std::endl;
}
```

## 查找表图像要求

### 格式要求
- 支持 PNG、JPG 等常见图像格式
- 推荐使用 512x512 像素的尺寸
- 支持 RGB 和 RGBA 格式

### 标准查找表结构
标准的查找表图像通常是一个 8x8 的网格，每个网格包含 64x64 像素：
- 水平方向：红色通道值 (0-255)
- 垂直方向：绿色通道值 (0-255)
- 蓝色通道值通过网格位置确定

## 性能考虑

- 查找表图像会在滤镜初始化时加载到 GPU 内存
- 更换查找表会重新加载纹理，有一定性能开销
- 建议预加载常用的查找表图像

## 错误处理

LookupFilter 包含完善的错误处理机制：
- 图像加载失败时会记录错误日志
- 不支持的图像格式会被拒绝
- 滤镜会在没有有效查找表时显示原图

## 示例代码

完整的示例代码请参考 `examples/lookup_filter_example.cpp`。

## 注意事项

1. 确保查找表图像路径正确且文件存在
2. 查找表图像应该是标准的 512x512 像素格式
3. 滤镜强度值应在 0.0 到 1.0 范围内
4. 在移动设备上使用时注意内存管理

## 故障排除

### 常见问题

1. **滤镜没有效果**
   - 检查查找表图像是否正确加载
   - 确认滤镜强度是否大于 0

2. **图像显示异常**
   - 验证查找表图像格式是否正确
   - 检查图像尺寸是否为 512x512

3. **性能问题**
   - 避免频繁更换查找表
   - 使用适当尺寸的查找表图像 