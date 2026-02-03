[⬅ Back to Table of Contents](../index.md)

# Media (Pythonic Format)

This page documents the Pythonic proprietary media format conversion functions. These functions convert standard media files (.png, .jpg, .mp4, etc.) to encrypted Pythonic formats (.pi, .pv) and back.

---

## Quick Start

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::media;

// Convert image to encrypted .pi format
convert("photo.jpg");  // Creates photo.pi

// Convert video to encrypted .pv format
convert("video.mp4");  // Creates video.pv

// Revert back to original format
revert("photo.pi");    // Creates photo_reverted.jpg
revert("video.pv");    // Creates video_reverted.mp4

// Get file information
auto [is_image, ext, size] = get_info("photo.pi");
std::cout << "Type: " << (is_image ? "Image" : "Video") << "\n";
std::cout << "Original format: " << ext << "\n";
std::cout << "Size: " << size << " bytes\n";
```

---

## Why Pythonic Format?

| Feature              | Description                                |
| -------------------- | ------------------------------------------ |
| **Encryption**       | XOR-based obfuscation with rotating key    |
| **Compression**      | Optional RLE compression for smaller files |
| **Format Detection** | Auto-detects image vs video from extension |
| **Metadata Storage** | Preserves original file type and size      |
| **Round-trip**       | Lossless conversion back to original       |

**File Extensions:**

- `.pi` - Pythonic Image (encrypted image data)
- `.pv` - Pythonic Video (encrypted video data)

---

## Core Functions

### convert()

Convert a media file to Pythonic encrypted format.

**Signature:**

```cpp
std::string convert(const std::string &filepath,
                   MediaType type = MediaType::auto_detect,
                   bool compress = true)
```

**Parameters:**

| Parameter  | Type          | Default                  | Description                             |
| ---------- | ------------- | ------------------------ | --------------------------------------- |
| `filepath` | `std::string` | Required                 | Path to source media file               |
| `type`     | `MediaType`   | `MediaType::auto_detect` | Force type (image/video) or auto-detect |
| `compress` | `bool`        | `true`                   | Enable RLE compression (saves space)    |

**Returns:** Path to created Pythonic file (.pi or .pv)

**Throws:** `std::runtime_error` if file cannot be read or type cannot be determined

**Examples:**

```cpp
// Auto-detect type from extension
convert("photo.png");         // Creates photo.pi
convert("video.mp4");         // Creates video.pv

// Force image type
convert("data.bin", MediaType::image);  // Creates data.pi

// Disable compression (faster but larger file)
convert("video.mp4", MediaType::auto_detect, false);

// Full control
std::string output = convert("photo.jpg", MediaType::image, true);
std::cout << "Created: " << output << std::endl;
```

---

### revert()

Revert a Pythonic format file back to its original format.

**Signature:**

```cpp
std::string revert(const std::string &filepath,
                  const std::string &output_name = "")
```

**Parameters:**

| Parameter     | Type          | Default  | Description                                         |
| ------------- | ------------- | -------- | --------------------------------------------------- |
| `filepath`    | `std::string` | Required | Path to Pythonic file (.pi or .pv)                  |
| `output_name` | `std::string` | `""`     | Custom output name (default: basename_reverted.ext) |

**Returns:** Path to restored file

**Throws:** `std::runtime_error` if file is invalid or cannot be read

**Examples:**

```cpp
// Default naming: photo_reverted.jpg
revert("photo.pi");

// Custom output name
revert("photo.pi", "original_photo");  // Creates original_photo.jpg

// Custom with path
revert("secret.pi", "output/restored");  // Creates output/restored.jpg

// Video example
revert("video.pv", "final_video");  // Creates final_video.mp4
```

---

### get_info()

Get metadata about a Pythonic file without decrypting it.

**Signature:**

```cpp
std::tuple<bool, std::string, uint64_t> get_info(const std::string &filepath)
```

**Returns:** `(is_image, original_extension, original_size)`

**Examples:**

```cpp
auto [is_image, ext, size] = get_info("file.pi");

if (is_image) {
    std::cout << "Image file" << std::endl;
} else {
    std::cout << "Video file" << std::endl;
}

std::cout << "Original format: " << ext << std::endl;
std::cout << "Original size: " << size << " bytes" << std::endl;
```

---

### get_info_detailed()

Get detailed metadata including compression information.

**Signature:**

```cpp
std::tuple<bool, std::string, uint64_t, uint64_t, Compression>
get_info_detailed(const std::string &filepath)
```

**Returns:** `(is_image, original_extension, original_size, compressed_size, compression_type)`

**Compression Enum:**

- `Compression::none` - No compression applied
- `Compression::rle` - Run-Length Encoding compression

**Examples:**

```cpp
auto [is_image, ext, orig_size, comp_size, comp_type] =
    get_info_detailed("file.pi");

std::cout << "Type: " << (is_image ? "Image" : "Video") << "\n";
std::cout << "Format: " << ext << "\n";
std::cout << "Original: " << orig_size << " bytes\n";

if (comp_type == Compression::rle) {
    double ratio = (double)comp_size / orig_size;
    std::cout << "Compressed: " << comp_size << " bytes ("
              << (ratio * 100) << "%)\n";
} else {
    std::cout << "Not compressed\n";
}
```

---

## Utility Functions

### File Type Detection

| Function                       | Description                        | Example                             |
| ------------------------------ | ---------------------------------- | ----------------------------------- |
| `is_pythonic_image(filename)`  | Check if file is .pi               | `if (is_pythonic_image("file.pi"))` |
| `is_pythonic_video(filename)`  | Check if file is .pv               | `if (is_pythonic_video("file.pv"))` |
| `is_pythonic_format(filename)` | Check if file is .pi or .pv        | `if (is_pythonic_format(filename))` |
| `is_image_extension(ext)`      | Check if extension is image format | `if (is_image_extension(".png"))`   |
| `is_video_extension(ext)`      | Check if extension is video format | `if (is_video_extension(".mp4"))`   |

### File Name Manipulation

| Function                  | Description                | Example                                 |
| ------------------------- | -------------------------- | --------------------------------------- |
| `get_extension(filename)` | Extract file extension     | `get_extension("photo.jpg")` → `".jpg"` |
| `get_basename(filename)`  | Get name without extension | `get_basename("photo.jpg")` → `"photo"` |

### Advanced Functions

| Function                                | Description                       | Use Case                          |
| --------------------------------------- | --------------------------------- | --------------------------------- |
| `extract_to_temp(filepath)`             | Decrypt to temporary file         | For FFmpeg/ImageMagick processing |
| `read_pythonic(filepath, original_ext)` | Read and decrypt file into memory | For custom processing             |

---

## MediaType Enum

Controls type detection behavior:

| MediaType                | Description                          | Use Case                       |
| ------------------------ | ------------------------------------ | ------------------------------ |
| `MediaType::auto_detect` | Detect from file extension (default) | Most cases                     |
| `MediaType::image`       | Force treat as image                 | Binary files with no extension |
| `MediaType::video`       | Force treat as video                 | Non-standard video formats     |

---

## Format Specifications

### File Structure

```
[Header: 64 bytes]
[Encrypted Data: variable]
```

### Header Layout (64 bytes)

| Offset | Size | Field              | Description                         |
| ------ | ---- | ------------------ | ----------------------------------- |
| 0-7    | 8    | Magic bytes        | 'PYTHIMG\x01' or 'PYTHVID\x01'      |
| 8      | 1    | Version            | Format version (currently 1)        |
| 9      | 1    | Extension length   | Length of original extension string |
| 10-25  | 16   | Original extension | Null-padded original file extension |
| 26-29  | 4    | Salt               | Random salt for encryption key      |
| 30     | 1    | Compression type   | 0=none, 1=RLE                       |
| 31-33  | 3    | Reserved flags     | For future use                      |
| 34-41  | 8    | Original size      | Original file size (little-endian)  |
| 42-49  | 8    | Compressed size    | Compressed size (0 if uncompressed) |
| 50-63  | 14   | Reserved           | For future use (zero-filled)        |

### Encryption

- **Algorithm:** XOR with rotating key + bit rotation
- **Key:** 32-byte key derived from ENCRYPT_KEY XOR salt
- **Obfuscation:** Position-dependent key rotation + 3-bit left rotation per byte
- **Note:** Not cryptographically secure - designed for obfuscation, not strong encryption

### Compression (RLE)

- **Algorithm:** Run-Length Encoding
- **Format:** Escape byte (0xFF) + count + value for runs ≥ 3
- **Optimization:** Only compresses if result is smaller than original
- **Best for:** Terminal graphics with many repeated values (e.g., empty braille cells)

---

## Examples

### Example 1: Basic Conversion Workflow

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::media;

// Convert image
std::string encrypted = convert("photo.jpg");
std::cout << "Encrypted: " << encrypted << std::endl;  // photo.pi

// Later, revert it back
std::string restored = revert(encrypted);
std::cout << "Restored: " << restored << std::endl;  // photo_reverted.jpg
```

### Example 2: Batch Conversion

```cpp
#include <pythonic/pythonic.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using namespace pythonic::media;

void encrypt_all_images(const std::string &directory) {
    for (const auto &entry : fs::directory_iterator(directory)) {
        std::string path = entry.path().string();
        std::string ext = get_extension(path);

        if (is_image_extension(ext)) {
            try {
                std::string output = convert(path);
                std::cout << "Encrypted: " << output << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
}
```

### Example 3: Inspect File Before Reverting

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::media;

void inspect_and_revert(const std::string &filepath) {
    // Get detailed info
    auto [is_image, ext, orig_size, comp_size, comp] =
        get_info_detailed(filepath);

    std::cout << "File: " << filepath << "\n";
    std::cout << "Type: " << (is_image ? "Image" : "Video") << "\n";
    std::cout << "Original format: " << ext << "\n";
    std::cout << "Original size: " << orig_size << " bytes\n";

    if (comp == Compression::rle && comp_size > 0) {
        double ratio = 100.0 * comp_size / orig_size;
        std::cout << "Compressed: " << comp_size << " bytes ("
                  << std::fixed << std::setprecision(1) << ratio << "%)\n";
    }

    // Revert
    std::string output = revert(filepath);
    std::cout << "Reverted to: " << output << "\n";
}
```

### Example 4: Conditional Compression

```cpp
#include <pythonic/pythonic.hpp>
#include <filesystem>

using namespace pythonic::media;

std::string smart_convert(const std::string &filepath) {
    size_t file_size = std::filesystem::file_size(filepath);

    // Only compress large files (> 1MB)
    bool should_compress = (file_size > 1024 * 1024);

    return convert(filepath, MediaType::auto_detect, should_compress);
}
```

### Example 5: Integration with Print API

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;
using namespace pythonic::media;

// Encrypt, then display
std::string encrypted = convert("photo.jpg");  // Creates photo.pi
print(encrypted);  // Automatically decrypts and displays!

// Encrypt video, then play
std::string encrypted_video = convert("movie.mp4");  // Creates movie.pv
print(encrypted_video);  // Automatically decrypts and plays!
```

---

## Error Handling

All functions throw `std::runtime_error` on failure:

```cpp
try {
    convert("nonexistent.jpg");
} catch (const std::runtime_error &e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

**Common errors:**

- `"Cannot open file"` - File doesn't exist or no read permission
- `"Cannot determine media type"` - Unknown file extension and type not specified
- `"Invalid Pythonic format header"` - Corrupted .pi/.pv file or wrong file type
- `"Data size mismatch"` - Corrupted file or wrong format version
- `"RLE: Size mismatch"` - Compression corruption

---

## Performance Notes

**Conversion Speed:**

- Small files (< 1MB): Instant
- Large videos (> 1GB): 1-5 seconds depending on compression

**Compression Ratio:**

- Terminal graphics (.txt with Braille): **20-40%** (very effective!)
- Images (.png, .jpg): **95-105%** (already compressed, minimal benefit)
- Videos (.mp4): **98-102%** (already compressed, no benefit)

**Recommendation:** Enable compression for text/terminal graphics, disable for already-compressed media.

---

# Next Check

- [Back to Table of Contents](../index.md)
- [Media Tutorial](../tutorial/pythonicMedia_tutorial.md)
