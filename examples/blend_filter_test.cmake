# BlendFilter 测试程序 CMake 配置
# 将此文件内容添加到主 CMakeLists.txt 中

# 添加 BlendFilter 测试程序
add_executable(blend_filter_test
    blend_filter_test.cpp
)

# 链接 GPUPixel 库
target_link_libraries(blend_filter_test
    gpupixel
)

# 设置包含目录
target_include_directories(blend_filter_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/output/include
)

# 添加 BlendFilter 示例程序
add_executable(blend_filter_example
    blend_filter_example.cpp
)

# 链接 GPUPixel 库
target_link_libraries(blend_filter_example
    gpupixel
)

# 设置包含目录
target_include_directories(blend_filter_example PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/output/include
)

# 如果是 iOS 平台，设置正确的框架
if(IOS)
    set_target_properties(blend_filter_test PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "BlendFilterTest"
    )
    
    set_target_properties(blend_filter_example PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "BlendFilterExample"
    )
endif()
