/**
 * @file test_video_export_debug.cpp
 * @brief Debug test for video export issue - reproduces the exact export pipeline
 *
 * Build: g++ -std=c++20 -Iinclude -o test_video_export_debug test_video_export_debug.cpp
 * Run: ./test_video_export_debug
 */

#include <pythonic/pythonic.hpp>
#include <iostream>
#include <fstream>

using namespace Pythonic;

int main()
{
    std::cout << "=== Video Export Debug Test ===" << std::endl;

    std::string test_video = "media/video.mp4";

    std::ifstream test(test_video);
    if (!test.good())
    {
        std::cout << "Test video not found: " << test_video << std::endl;
        return 1;
    }
    test.close();

    // Test 1: Extract a single frame and compare terminal vs export
    std::cout << "\n--- Step 1: Extracting single frame from video ---" << std::endl;
    std::string temp_frame = "/tmp/debug_video_frame.png";
    std::string cmd = "ffmpeg -y -i \"" + test_video + "\" -vframes 1 \"" + temp_frame + "\" 2>/dev/null";
    int result = std::system(cmd.c_str());

    if (result != 0)
    {
        std::cout << "Failed to extract frame" << std::endl;
        return 1;
    }
    std::cout << "Frame extracted to: " << temp_frame << std::endl;

    // Test 2: Render using different modes
    std::cout << "\n--- Step 2: Testing different render modes ---" << std::endl;

    // Mode::colored (what produces the second screenshot in terminal)
    std::string colored_str = pythonic::print::render_image_to_string(temp_frame, Mode::colored, 80, 128);
    std::cout << "Colored mode string length: " << colored_str.size() << " bytes" << std::endl;

    // Mode::bw_dot (braille)
    std::string bw_dot_str = pythonic::print::render_image_to_string(temp_frame, Mode::bw_dot, 80, 128);
    std::cout << "BW dot mode string length: " << bw_dot_str.size() << " bytes" << std::endl;

    // Mode::colored_dot
    std::string colored_dot_str = pythonic::print::render_image_to_string(temp_frame, Mode::colored_dot, 80, 128);
    std::cout << "Colored dot mode string length: " << colored_dot_str.size() << " bytes" << std::endl;

    // Test 3: Export each mode to PNG
    std::cout << "\n--- Step 3: Exporting to PNG using export_art_to_png ---" << std::endl;

    pythonic::ex::export_art_to_png(colored_str, "test_video_frame_colored.png", 2, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Exported: test_video_frame_colored.png" << std::endl;

    pythonic::ex::export_art_to_png(bw_dot_str, "test_video_frame_bw_dot.png", 2, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Exported: test_video_frame_bw_dot.png" << std::endl;

    pythonic::ex::export_art_to_png(colored_dot_str, "test_video_frame_colored_dot.png", 2, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Exported: test_video_frame_colored_dot.png" << std::endl;

    // Test 4: Show terminal render for comparison
    std::cout << "\n--- Step 4: Terminal render (colored mode) ---" << std::endl;
    std::cout << colored_str;

    // Test 5: Full video export with different modes
    std::cout << "\n--- Step 5: Full video export ---" << std::endl;

    // Export using Mode::colored (should match terminal)
    std::cout << "Exporting video with Mode::colored..." << std::endl;
    bool export_result = pythonic::print::export_media(
        test_video,
        "test_video_export_colored",
        Type::video,
        Format::video,
        Mode::colored, // This should produce the same as terminal
        80,
        128,
        Audio::off);
    std::cout << "Result: " << (export_result ? "SUCCESS" : "FAILED") << std::endl;

    // Clean up temp frame
    std::remove(temp_frame.c_str());

    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "\nPlease compare:" << std::endl;
    std::cout << "  Terminal output above (colored mode)" << std::endl;
    std::cout << "  test_video_frame_colored.png (single frame export)" << std::endl;
    std::cout << "  test_video_export_colored.mp4 (full video export)" << std::endl;

    return 0;
}
