/*
 * GPUPixel LookupFilter Example
 *
 * This example demonstrates how to use the LookupFilter class
 * to apply color lookup table effects to images.
 */

#include <iostream>
#include <memory>
#include <string>
#include "gpupixel.h"
#include "filter/lookup_filter.h"

using namespace gpupixel;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_image> <lookup_image> [output_image]" << std::endl;
        std::cout << "Example: " << argv[0] << " input.jpg lookup.png output.jpg" << std::endl;
        return 1;
    }

    std::string inputImagePath = argv[1];
    std::string lookupImagePath = argv[2];
    std::string outputImagePath = (argc > 3) ? argv[3] : "output.jpg";

    try {
        // Initialize GPUPixel
        auto context = GPUPixelContext::getInstance();
        
        // Create source image
        auto sourceImage = SourceImage::create(inputImagePath);
        if (!sourceImage) {
            std::cerr << "Failed to load input image: " << inputImagePath << std::endl;
            return 1;
        }

        // Create lookup filter
        auto lookupFilter = LookupFilter::create(lookupImagePath);
        if (!lookupFilter) {
            std::cerr << "Failed to create lookup filter" << std::endl;
            return 1;
        }

        // Set filter intensity (0.0 to 1.0)
        lookupFilter->setIntensity(0.8f);

        // Connect source to filter
        sourceImage->addTarget(lookupFilter);

        // Create target for output
        auto target = Target::create();
        lookupFilter->addTarget(target);

        // Process the image
        sourceImage->proceed();

        // Save output (this would need to be implemented based on your needs)
        std::cout << "Successfully processed image with lookup filter!" << std::endl;
        std::cout << "Input: " << inputImagePath << std::endl;
        std::cout << "Lookup: " << lookupImagePath << std::endl;
        std::cout << "Output: " << outputImagePath << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 