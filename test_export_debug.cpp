/**
 * @file test_export_debug.cpp
 * @brief Debug test for video export issue
 *
 * Build: g++ -std=c++20 -Iinclude -o test_export_debug test_export_debug.cpp
 * Run: ./test_export_debug
 */

#include <pythonic/pythonic.hpp>
#include <iostream>
#include <fstream>

using namespace Pythonic;

int main()
{
    std::cout << "=== Export Debug Test ===" << std::endl;

    // Test 1: Render an image frame and show its structure
    std::string test_image = "media/oyshee.png";

    std::ifstream test(test_image);
    if (!test.good())
    {
        std::cout << "Test image not found: " << test_image << std::endl;
        return 1;
    }
    test.close();

    // Test different render modes and their ANSI output
    std::cout << "\n--- Testing Mode::colored (half-blocks) ---" << std::endl;
    std::string colored = pythonic::print::render_image_to_string(test_image, Mode::colored, 40, 128);

    // Print first few characters to see ANSI structure
    std::cout << "First 500 bytes of colored output:" << std::endl;
    std::cout << "---" << std::endl;
    for (size_t i = 0; i < std::min((size_t)500, colored.size()); i++)
    {
        char c = colored[i];
        if (c == '\033')
        {
            std::cout << "\\033";
        }
        else if (c < 32 && c != '\n')
        {
            std::cout << "\\x" << std::hex << (int)(unsigned char)c << std::dec;
        }
        else
        {
            std::cout << c;
        }
    }
    std::cout << "\n---" << std::endl;

    // Export colored mode to PNG
    std::cout << "\nExporting colored mode to test_debug_colored.png..." << std::endl;
    bool result1 = pythonic::ex::export_art_to_png(colored, "test_debug_colored.png", 2, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Result: " << (result1 ? "SUCCESS" : "FAILED") << std::endl;

    // Test with bw_dot mode (braille)
    std::cout << "\n--- Testing Mode::bw_dot (braille) ---" << std::endl;
    std::string bw_dot = pythonic::print::render_image_to_string(test_image, Mode::bw_dot, 40, 128);

    // Print first few characters
    std::cout << "First 500 bytes of bw_dot output:" << std::endl;
    std::cout << "---" << std::endl;
    for (size_t i = 0; i < std::min((size_t)500, bw_dot.size()); i++)
    {
        char c = bw_dot[i];
        if (c == '\033')
        {
            std::cout << "\\033";
        }
        else if (c < 32 && c != '\n')
        {
            std::cout << "\\x" << std::hex << (int)(unsigned char)c << std::dec;
        }
        else
        {
            std::cout << c;
        }
    }
    std::cout << "\n---" << std::endl;

    // Export bw_dot mode to PNG
    std::cout << "\nExporting bw_dot mode to test_debug_bw_dot.png..." << std::endl;
    bool result2 = pythonic::ex::export_art_to_png(bw_dot, "test_debug_bw_dot.png", 2, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Result: " << (result2 ? "SUCCESS" : "FAILED") << std::endl;

    // Test with colored_dot mode
    std::cout << "\n--- Testing Mode::colored_dot (colored braille) ---" << std::endl;
    std::string colored_dot = pythonic::print::render_image_to_string(test_image, Mode::colored_dot, 40, 128);

    std::cout << "First 500 bytes of colored_dot output:" << std::endl;
    std::cout << "---" << std::endl;
    for (size_t i = 0; i < std::min((size_t)500, colored_dot.size()); i++)
    {
        char c = colored_dot[i];
        if (c == '\033')
        {
            std::cout << "\\033";
        }
        else if (c < 32 && c != '\n')
        {
            std::cout << "\\x" << std::hex << (int)(unsigned char)c << std::dec;
        }
        else
        {
            std::cout << c;
        }
    }
    std::cout << "\n---" << std::endl;

    // Export colored_dot mode to PNG
    std::cout << "\nExporting colored_dot mode to test_debug_colored_dot.png..." << std::endl;
    bool result3 = pythonic::ex::export_art_to_png(colored_dot, "test_debug_colored_dot.png", 2, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Result: " << (result3 ? "SUCCESS" : "FAILED") << std::endl;

    // Also render the terminal output for comparison
    std::cout << "\n--- Terminal render (colored mode) ---" << std::endl;
    std::cout << colored << std::endl;

    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}
