/*
 * BlendFilter使用示例
 * 
 * 演示如何使用BlendFilter进行图层叠加效果
 * 支持类似Photoshop的图层混合模式
 */

#include "gpupixel.h"
#include <iostream>
#include <memory>

using namespace gpupixel;

int main() {
    // 初始化GPUPixel上下文
    if (!GPUPixelContext::getInstance()->init()) {
        std::cerr << "Failed to initialize GPUPixel context" << std::endl;
        return -1;
    }
    
    // 创建BlendFilter
    auto blendFilter = BlendFilter::create();
    if (!blendFilter) {
        std::cerr << "Failed to create BlendFilter" << std::endl;
        return -1;
    }
    
    // 示例1: 添加正片叠底图层
    std::cout << "=== 示例1: 正片叠底效果 ===" << std::endl;
    int layer1 = blendFilter->addLayer("overlay_texture.png", BlendMode::MULTIPLY, 0.8f, 1.0f);
    std::cout << "添加图层1 (正片叠底): " << layer1 << std::endl;
    
    // 示例2: 添加滤色图层
    std::cout << "=== 示例2: 滤色效果 ===" << std::endl;
    int layer2 = blendFilter->addLayer("screen_texture.png", BlendMode::SCREEN, 0.6f, 1.0f);
    std::cout << "添加图层2 (滤色): " << layer2 << std::endl;
    
    // 示例3: 添加叠加图层
    std::cout << "=== 示例3: 叠加效果 ===" << std::endl;
    int layer3 = blendFilter->addLayer("overlay_texture2.png", BlendMode::OVERLAY, 0.7f, 1.0f);
    std::cout << "添加图层3 (叠加): " << layer3 << std::endl;
    
    // 示例4: 动态调整图层属性
    std::cout << "=== 示例4: 动态调整图层属性 ===" << std::endl;
    
    // 调整图层1的不透明度
    blendFilter->setLayerOpacity(layer1, 0.5f);
    std::cout << "调整图层1不透明度为0.5" << std::endl;
    
    // 调整图层2的混合模式
    blendFilter->setLayerBlendMode(layer2, BlendMode::SOFT_LIGHT);
    std::cout << "调整图层2混合模式为柔光" << std::endl;
    
    // 禁用图层3
    blendFilter->setLayerEnabled(layer3, false);
    std::cout << "禁用图层3" << std::endl;
    
    // 设置全局属性
    blendFilter->setGlobalOpacity(0.9f);
    blendFilter->setGlobalIntensity(1.0f);
    std::cout << "设置全局不透明度为0.9" << std::endl;
    
    // 示例5: 图层管理
    std::cout << "=== 示例5: 图层管理 ===" << std::endl;
    std::cout << "当前图层数量: " << blendFilter->getLayerCount() << std::endl;
    
    // 获取图层信息
    for (int i = 0; i < blendFilter->getLayerCount(); ++i) {
        const BlendLayer* layer = blendFilter->getLayer(i);
        if (layer) {
            std::cout << "图层" << i << ": " 
                      << "路径=" << layer->imagePath 
                      << ", 模式=" << (int)layer->blendMode
                      << ", 不透明度=" << layer->opacity
                      << ", 强度=" << layer->intensity
                      << ", 启用=" << (layer->enabled ? "是" : "否") << std::endl;
        }
    }
    
    // 示例6: 移除图层
    std::cout << "=== 示例6: 移除图层 ===" << std::endl;
    blendFilter->removeLayer(layer2);
    std::cout << "移除图层2，当前图层数量: " << blendFilter->getLayerCount() << std::endl;
    
    // 示例7: 清空所有图层
    std::cout << "=== 示例7: 清空所有图层 ===" << std::endl;
    blendFilter->removeAllLayers();
    std::cout << "清空所有图层，当前图层数量: " << blendFilter->getLayerCount() << std::endl;
    
    // 示例8: 创建复杂的多层叠加效果
    std::cout << "=== 示例8: 复杂多层叠加效果 ===" << std::endl;
    
    // 添加基础纹理图层 (正片叠底)
    int baseLayer = blendFilter->addLayer("base_texture.png", BlendMode::MULTIPLY, 0.8f, 1.0f);
    
    // 添加高光图层 (滤色)
    int highlightLayer = blendFilter->addLayer("highlight.png", BlendMode::SCREEN, 0.4f, 1.0f);
    
    // 添加细节图层 (叠加)
    int detailLayer = blendFilter->addLayer("detail.png", BlendMode::OVERLAY, 0.6f, 1.0f);
    
    // 添加色彩调整图层 (柔光)
    int colorLayer = blendFilter->addLayer("color_adjust.png", BlendMode::SOFT_LIGHT, 0.5f, 1.0f);
    
    std::cout << "创建了4个图层的复杂叠加效果:" << std::endl;
    std::cout << "- 基础纹理 (正片叠底, 不透明度0.8)" << std::endl;
    std::cout << "- 高光效果 (滤色, 不透明度0.4)" << std::endl;
    std::cout << "- 细节增强 (叠加, 不透明度0.6)" << std::endl;
    std::cout << "- 色彩调整 (柔光, 不透明度0.5)" << std::endl;
    
    // 示例9: 支持的混合模式列表
    std::cout << "=== 示例9: 支持的混合模式 ===" << std::endl;
    std::cout << "0  - 正常 (NORMAL)" << std::endl;
    std::cout << "1  - 正片叠底 (MULTIPLY)" << std::endl;
    std::cout << "2  - 滤色 (SCREEN)" << std::endl;
    std::cout << "3  - 叠加 (OVERLAY)" << std::endl;
    std::cout << "4  - 柔光 (SOFT_LIGHT)" << std::endl;
    std::cout << "5  - 强光 (HARD_LIGHT)" << std::endl;
    std::cout << "6  - 颜色减淡 (COLOR_DODGE)" << std::endl;
    std::cout << "7  - 颜色加深 (COLOR_BURN)" << std::endl;
    std::cout << "8  - 变暗 (DARKEN)" << std::endl;
    std::cout << "9  - 变亮 (LIGHTEN)" << std::endl;
    std::cout << "10 - 差值 (DIFFERENCE)" << std::endl;
    std::cout << "11 - 排除 (EXCLUSION)" << std::endl;
    std::cout << "12 - 线性减淡 (LINEAR_DODGE)" << std::endl;
    std::cout << "13 - 线性加深 (LINEAR_BURN)" << std::endl;
    std::cout << "14 - 亮光 (VIVID_LIGHT)" << std::endl;
    std::cout << "15 - 线性光 (LINEAR_LIGHT)" << std::endl;
    std::cout << "16 - 点光 (PIN_LIGHT)" << std::endl;
    std::cout << "17 - 实色混合 (HARD_MIX)" << std::endl;
    
    std::cout << "\nBlendFilter使用示例完成！" << std::endl;
    std::cout << "BlendFilter支持动态添加多个图层，每个图层可以设置不同的混合模式、不透明度和强度。" << std::endl;
    std::cout << "这样可以实现类似Photoshop的复杂图层叠加效果。" << std::endl;
    
    return 0;
}
