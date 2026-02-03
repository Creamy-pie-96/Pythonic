# pythonicMedia Module

The `pythonicMedia.hpp` module provides a proprietary encrypted media format system for protecting images and videos. Files are encrypted using XOR with a rotating key and random salt, making them unreadable by standard media tools.

## Include

```cpp
#include <pythonic/pythonicMedia.hpp>
using namespace pythonic::media;
```

## File Extensions

| Extension | Type           |
| --------- | -------------- |
| `.pi`     | Pythonic Image |
| `.pv`     | Pythonic Video |

## Quick Start

```cpp
#include <pythonic/pythonicMedia.hpp>
using namespace pythonic::media;

// Convert image to encrypted format
std::string encrypted_path = convert("photo.jpg");  // Creates photo.pi

// Convert video to encrypted format
std::string encrypted_path = convert("video.mp4");  // Creates video.pv

// Revert back to original format
std::string original_path = revert("photo.pi");     // Creates photo_restored.jpg
std::string original_path = revert("video.pv");     // Creates video_restored.mp4
```

## API Reference

### convert()

Convert a standard image or video file to Pythonic encrypted format.

```cpp
/**
 * @param input_path Path to the source media file
 * @param type Type hint (auto_detect, image, video) - default: auto_detect
 * @return Path to the created encrypted file
 * @throws std::runtime_error on file I/O errors
 */
std::string convert(const std::string& input_path, Type type = Type::auto_detect);
```

**Examples:**

```cpp
// Auto-detect type from extension
convert("photo.png");      // Creates photo.pi
convert("movie.mp4");      // Creates movie.pv

// Force type (useful for unusual extensions)
convert("data.bin", Type::image);  // Creates data.pi
convert("data.bin", Type::video);  // Creates data.pv
```

### revert()

Restore an encrypted Pythonic file back to its original format.

```cpp
/**
 * @param input_path Path to the Pythonic encrypted file (.pi or .pv)
 * @return Path to the restored file
 * @throws std::runtime_error on file I/O errors or invalid format
 */
std::string revert(const std::string& input_path);
```

**Examples:**

```cpp
// Restore files
revert("photo.pi");   // Creates photo_restored.png (original was .png)
revert("video.pv");   // Creates video_restored.mp4 (original was .mp4)
```

### get_info()

Get information about a Pythonic encrypted file without extracting it.

```cpp
/**
 * @param filepath Path to the Pythonic file
 * @return Tuple of (is_image, original_extension, original_size)
 * @throws std::runtime_error on invalid file
 */
std::tuple<bool, std::string, uint64_t> get_info(const std::string& filepath);
```

**Example:**

```cpp
auto [is_image, ext, size] = get_info("photo.pi");
std::cout << "Type: " << (is_image ? "Image" : "Video") << std::endl;
std::cout << "Original extension: " << ext << std::endl;
std::cout << "Original size: " << size << " bytes" << std::endl;
```

### extract_to_temp()

Extract a Pythonic file to a temporary file for processing.

```cpp
/**
 * @param filepath Path to the Pythonic file
 * @return Path to the temporary extracted file
 * @note Caller is responsible for deleting the temp file
 */
std::string extract_to_temp(const std::string& filepath);
```

**Example:**

```cpp
std::string temp_path = extract_to_temp("video.pv");
// Use temp_path for processing...
std::remove(temp_path.c_str());  // Clean up
```

### read_pythonic()

Read the raw decrypted data from a Pythonic file.

```cpp
/**
 * @param filepath Path to the Pythonic file
 * @return Pair of (header, decrypted_data)
 * @throws std::runtime_error on invalid file
 */
std::pair<PythonicMediaHeader, std::vector<uint8_t>> read_pythonic(const std::string& filepath);
```

## Format Detection

```cpp
// Check if file has Pythonic extension
bool is_image = is_pythonic_image("photo.pi");   // true
bool is_video = is_pythonic_video("video.pv");   // true
bool is_any = is_pythonic_format("data.pi");     // true

// Case insensitive
is_pythonic_image("PHOTO.PI");  // true
is_pythonic_video("VIDEO.PV");  // true
```

## Header Structure

The Pythonic format uses a 64-byte header:

```cpp
struct PythonicMediaHeader {
    uint8_t magic[8];       // "PYTHIMG" or "PYTHVID"
    uint8_t version;        // Format version (currently 1)
    uint8_t ext_length;     // Length of original extension
    char original_ext[16];  // Original file extension
    uint32_t salt;          // Random salt for encryption
    uint32_t reserved1;     // Reserved
    uint64_t original_size; // Original file size
    uint8_t reserved[22];   // Reserved for future use
};
```

## Integration with Print

Pythonic encrypted files are automatically detected and decrypted by the `print()` function:

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Print encrypted files directly (auto-detected)
print("photo.pi");                        // Renders the encrypted image
print("video.pv", Type::video);           // Plays the encrypted video
print("video.pv", Type::video, Mode::colored, Parser::default_parser, Audio::on);
```

## Security Notes

⚠️ **This is NOT cryptographically secure encryption!**

The Pythonic media format uses XOR-based obfuscation for:

- Preventing casual access to media files
- Making files unreadable by standard media tools
- Simple DRM-like protection for demonstration purposes

For actual security, use proper encryption libraries (OpenSSL, libsodium, etc.).

## Complete Example

```cpp
#include <pythonic/pythonic.hpp>
#include <pythonic/pythonicMedia.hpp>

using namespace Pythonic;
using namespace pythonic::media;

int main() {
    // Convert a photo to encrypted format
    std::string encrypted = convert("vacation.jpg");
    std::cout << "Created: " << encrypted << std::endl;

    // Get info about the encrypted file
    auto [is_img, ext, size] = get_info(encrypted);
    std::cout << "Original was: " << ext << " (" << size << " bytes)" << std::endl;

    // Display the encrypted image
    print(encrypted, Type::image, Mode::colored);

    // Restore to original format
    std::string restored = revert(encrypted);
    std::cout << "Restored to: " << restored << std::endl;

    return 0;
}
```

## Type Enum

```cpp
enum class Type {
    auto_detect,  // Detect from file extension
    image,        // Force treat as image
    video         // Force treat as video
};
```

Note: This is `pythonic::media::Type`, not to be confused with `pythonic::print::Type` which has additional values like `webcam` and `video_info`.
