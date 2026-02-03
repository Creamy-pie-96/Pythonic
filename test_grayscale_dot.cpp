/**
 * @brief Test grayscale_dot rendering mode
 */
#include "pythonic/pythonic.hpp"

int main()
{
    using namespace pythonic;
    using namespace pythonic::draw;
    using namespace pythonic::print;

    print("=== Testing grayscale_dot mode ===\n");
    
    // Test image exists (use any image in the project)
    std::string test_image = "/home/DATA/CODE/code/pythonic/media/oyshee.txt";
    
    // Check if we have a test image
    std::ifstream test(test_image);
    if (!test.good()) {
        // Try to find or create a test image
        print("Creating test gradient image...\n");
        
        // Create a simple gradient PPM
        std::string ppm_path = "/tmp/test_gradient.ppm";
        std::ofstream ppm(ppm_path);
        if (ppm) {
            int w = 160, h = 96;
            ppm << "P6\n" << w << " " << h << "\n255\n";
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    // Gradient from black (left) to white (right)
                    uint8_t v = static_cast<uint8_t>((x * 255) / w);
                    ppm.put(v).put(v).put(v);
                }
            }
            ppm.close();
            test_image = ppm_path;
        }
    }
    test.close();

    print("\n--- Standard bw_dot (threshold) ---\n");
    print(render_image(test_image, 80, 128));
    
    print("\n--- bw_dithered (ordered dithering) ---\n");
    print(render_image_dithered(test_image, 80));
    
    print("\n--- grayscale_dot (ANSI grayscale colored) ---\n");
    print(render_image_grayscale(test_image, 80, 128, true));
    
    print("\n=== Tests complete ===\n");
    
    return 0;
}
