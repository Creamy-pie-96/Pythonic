/**
 * @file test_braille_pattern.cpp
 * @brief Debug test to verify braille patterns are correct
 *
 * Build: g++ -std=c++20 -Iinclude -o test_braille_pattern test_braille_pattern.cpp
 * Run: ./test_braille_pattern
 */

#include <pythonic/pythonic.hpp>
#include <iostream>
#include <fstream>

using namespace Pythonic;

int main()
{
    std::cout << "=== Braille Pattern Debug Test ===" << std::endl;

    // Test with a simple known pattern
    std::string test_image = "media/oyshee.png";

    std::ifstream test(test_image);
    if (!test.good())
    {
        std::cout << "Test image not found: " << test_image << std::endl;
        return 1;
    }
    test.close();

    // Render in bw_dot mode (braille, no colors)
    std::string bw_dot = pythonic::print::render_image_to_string(test_image, Mode::bw_dot, 40, 128);

    std::cout << "First few lines of bw_dot output:" << std::endl;
    std::cout << "---" << std::endl;

    // Print first 5 lines
    int line_count = 0;
    for (size_t i = 0; i < bw_dot.size() && line_count < 5; i++)
    {
        char c = bw_dot[i];
        std::cout << c;
        if (c == '\n')
            line_count++;
    }
    std::cout << "---" << std::endl;

    // Count unique patterns
    std::map<std::string, int> pattern_counts;
    std::string current_char;
    for (size_t i = 0; i < bw_dot.size(); i++)
    {
        unsigned char c = bw_dot[i];

        // Check for UTF-8 braille (starts with 0xE2)
        if (c == 0xE2 && i + 2 < bw_dot.size())
        {
            current_char = bw_dot.substr(i, 3);
            pattern_counts[current_char]++;
            i += 2;
        }
        else if (c == '\n')
        {
            // newline
        }
    }

    std::cout << "\nUnique braille patterns found: " << pattern_counts.size() << std::endl;
    std::cout << "Pattern distribution (first 10):" << std::endl;

    int count = 0;
    for (const auto &p : pattern_counts)
    {
        if (count >= 10)
            break;
        // Decode the pattern
        unsigned char b1 = p.first[0];
        unsigned char b2 = p.first[1];
        unsigned char b3 = p.first[2];

        // Braille is U+2800-U+28FF
        // UTF-8: E2 A0 80 to E2 A3 BF
        uint32_t codepoint = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        uint8_t pattern = codepoint - 0x2800;

        std::cout << "  Pattern 0x" << std::hex << (int)pattern << std::dec
                  << " (" << p.first << "): " << p.second << " times" << std::endl;
        count++;
    }

    // Check if we have empty braille (0x2800 = ⠀)
    std::string empty_braille = "\xe2\xa0\x80";
    if (pattern_counts.find(empty_braille) != pattern_counts.end())
    {
        std::cout << "\nEmpty braille (⠀) count: " << pattern_counts[empty_braille] << std::endl;
    }

    // Check for full braille (0x28FF = ⣿)
    std::string full_braille = "\xe2\xa3\xbf";
    if (pattern_counts.find(full_braille) != pattern_counts.end())
    {
        std::cout << "Full braille (⣿) count: " << pattern_counts[full_braille] << std::endl;
    }

    // Now let's test the export
    std::cout << "\n--- Exporting to PNG ---" << std::endl;

    // Create a simple test with known patterns
    std::string test_art = "⠀⣿⠀⣿⠀\n⣿⠀⣿⠀⣿\n⠀⣿⠀⣿⠀\n";
    std::cout << "Test art (checkerboard):" << std::endl;
    std::cout << test_art << std::endl;

    pythonic::ex::export_art_to_png(test_art, "test_braille_checkerboard.png", 4, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Exported: test_braille_checkerboard.png" << std::endl;

    // Also export the actual image
    pythonic::ex::export_art_to_png(bw_dot, "test_braille_oyshee.png", 2, pythonic::ex::RGB(0, 0, 0));
    std::cout << "Exported: test_braille_oyshee.png" << std::endl;

    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}
