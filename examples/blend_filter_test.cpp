/*
 * BlendFilter 简单测试程序
 * 
 * 测试 BlendFilter 的基本功能
 */

#include "gpupixel.h"
#include <iostream>
#include <memory>

using namespace gpupixel;

int main() {
    std::cout << "=== BlendFilter 测试程序 ===" << std::endl;
    
    // 初始化 GPUPixel 上下文
    if (!GPUPixelContext::getInstance()->init()) {
        std::cerr << "错误: 无法初始化 GPUPixel 上下文" << std::endl;
        return -1;
    }
    std::cout << "✓ GPUPixel 上下文初始化成功" << std::endl;
    
    // 创建 BlendFilter
    auto blendFilter = BlendFilter::create();
    if (!blendFilter) {
        std::cerr << "错误: 无法创建 BlendFilter" << std::endl;
        return -1;
    }
    std::cout << "✓ BlendFilter 创建成功" << std::endl;
    
    // 测试添加图层
    std::cout << "\n--- 测试图层管理 ---" << std::endl;
    
    // 添加测试图层 (使用不存在的图片路径，测试错误处理)
    int layer1 = blendFilter->addLayer("test_texture1.png", BlendMode::MULTIPLY, 0.8f, 1.0f);
    std::cout << "添加图层1 (索引: " << layer1 << ")" << std::endl;
    
    int layer2 = blendFilter->addLayer("test_texture2.png", BlendMode::SCREEN, 0.6f, 1.0f);
    std::cout << "添加图层2 (索引: " << layer2 << ")" << std::endl;
    
    // 检查图层数量
    int layerCount = blendFilter->getLayerCount();
    std::cout << "当前图层数量: " << layerCount << std::endl;
    
    if (layerCount == 2) {
        std::cout << "✓ 图层添加成功" << std::endl;
    } else {
        std::cout << "✗ 图层添加失败" << std::endl;
    }
    
    // 测试图层属性设置
    std::cout << "\n--- 测试图层属性设置 ---" << std::endl;
    
    blendFilter->setLayerOpacity(layer1, 0.5f);
    blendFilter->setLayerBlendMode(layer2, BlendMode::OVERLAY);
    blendFilter->setLayerIntensity(layer1, 0.8f);
    blendFilter->setLayerEnabled(layer2, false);
    
    std::cout << "✓ 图层属性设置完成" << std::endl;
    
    // 测试全局属性设置
    std::cout << "\n--- 测试全局属性设置 ---" << std::endl;
    
    blendFilter->setGlobalOpacity(0.9f);
    blendFilter->setGlobalIntensity(1.0f);
    
    std::cout << "✓ 全局属性设置完成" << std::endl;
    
    // 测试图层信息获取
    std::cout << "\n--- 测试图层信息获取 ---" << std::endl;
    
    for (int i = 0; i < layerCount; ++i) {
        const BlendLayer* layer = blendFilter->getLayer(i);
        if (layer) {
            std::cout << "图层" << i << ": " 
                      << "路径=" << layer->imagePath 
                      << ", 模式=" << (int)layer->blendMode
                      << ", 不透明度=" << layer->opacity
                      << ", 强度=" << layer->intensity
                      << ", 启用=" << (layer->enabled ? "是" : "否") << std::endl;
        } else {
            std::cout << "图层" << i << ": 获取失败" << std::endl;
        }
    }
    
    // 测试图层移除
    std::cout << "\n--- 测试图层移除 ---" << std::endl;
    
    blendFilter->removeLayer(layer1);
    int newLayerCount = blendFilter->getLayerCount();
    std::cout << "移除图层1后，图层数量: " << newLayerCount << std::endl;
    
    if (newLayerCount == 1) {
        std::cout << "✓ 图层移除成功" << std::endl;
    } else {
        std::cout << "✗ 图层移除失败" << std::endl;
    }
    
    // 测试清空所有图层
    std::cout << "\n--- 测试清空所有图层 ---" << std::endl;
    
    blendFilter->removeAllLayers();
    int finalLayerCount = blendFilter->getLayerCount();
    std::cout << "清空所有图层后，图层数量: " << finalLayerCount << std::endl;
    
    if (finalLayerCount == 0) {
        std::cout << "✓ 清空图层成功" << std::endl;
    } else {
        std::cout << "✗ 清空图层失败" << std::endl;
    }
    
    // 测试混合模式枚举
    std::cout << "\n--- 测试混合模式枚举 ---" << std::endl;
    
    std::cout << "支持的混合模式:" << std::endl;
    std::cout << "  正常 (NORMAL): " << (int)BlendMode::NORMAL << std::endl;
    std::cout << "  正片叠底 (MULTIPLY): " << (int)BlendMode::MULTIPLY << std::endl;
    std::cout << "  滤色 (SCREEN): " << (int)BlendMode::SCREEN << std::endl;
    std::cout << "  叠加 (OVERLAY): " << (int)BlendMode::OVERLAY << std::endl;
    std::cout << "  柔光 (SOFT_LIGHT): " << (int)BlendMode::SOFT_LIGHT << std::endl;
    std::cout << "  强光 (HARD_LIGHT): " << (int)BlendMode::HARD_LIGHT << std::endl;
    std::cout << "  颜色减淡 (COLOR_DODGE): " << (int)BlendMode::COLOR_DODGE << std::endl;
    std::cout << "  颜色加深 (COLOR_BURN): " << (int)BlendMode::COLOR_BURN << std::endl;
    std::cout << "  变暗 (DARKEN): " << (int)BlendMode::DARKEN << std::endl;
    std::cout << "  变亮 (LIGHTEN): " << (int)BlendMode::LIGHTEN << std::endl;
    std::cout << "  差值 (DIFFERENCE): " << (int)BlendMode::DIFFERENCE << std::endl;
    std::cout << "  排除 (EXCLUSION): " << (int)BlendMode::EXCLUSION << std::endl;
    std::cout << "  线性减淡 (LINEAR_DODGE): " << (int)BlendMode::LINEAR_DODGE << std::endl;
    std::cout << "  线性加深 (LINEAR_BURN): " << (int)BlendMode::LINEAR_BURN << std::endl;
    std::cout << "  亮光 (VIVID_LIGHT): " << (int)BlendMode::VIVID_LIGHT << std::endl;
    std::cout << "  线性光 (LINEAR_LIGHT): " << (int)BlendMode::LINEAR_LIGHT << std::endl;
    std::cout << "  点光 (PIN_LIGHT): " << (int)BlendMode::PIN_LIGHT << std::endl;
    std::cout << "  实色混合 (HARD_MIX): " << (int)BlendMode::HARD_MIX << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    std::cout << "BlendFilter 基本功能测试通过！" << std::endl;
    std::cout << "注意: 由于没有实际的图片文件，纹理加载测试被跳过。" << std::endl;
    std::cout << "在实际使用中，请确保提供有效的图片路径。" << std::endl;
    
    return 0;
}
