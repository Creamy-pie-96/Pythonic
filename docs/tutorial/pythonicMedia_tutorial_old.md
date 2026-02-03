[⬅ Back to Table of Contents](../index.md)

# 🔐 pythonicMedia Tutorial - Understanding the File Format

**pythonicMedia.hpp** provides a proprietary format for storing images and videos. This tutorial explains how the encryption and compression work under the hood.

---

## 📚 Table of Contents

1. [File Format Overview](#1-file-format-overview)
2. [The Header Structure](#2-the-header-structure)
3. [XOR Encryption](#3-xor-encryption)
4. [RLE Compression](#4-rle-compression)
5. [Converting Files](#5-converting-files)
6. [Reverting Files](#6-reverting-files)
7. [Integration with Draw Module](#7-integration-with-draw-module)

---

## 1. File Format Overview

Pythonic uses two proprietary file extensions:

| Extension | Purpose        | Magic Bytes   |
| --------- | -------------- | ------------- |
| `.pi`     | Pythonic Image | `PYTHIMG\x01` |
| `.pv`     | Pythonic Video | `PYTHVID\x01` |

**File structure:**

```
┌────────────────────────────────────┐
│ Header (64 bytes)                  │
├────────────────────────────────────┤
│ Encrypted/Compressed Data          │
│ (variable length)                  │
└────────────────────────────────────┘
```

---

## 2. The Header Structure

```cpp
#pragma pack(push, 1)  // Pack tightly, no padding
struct PythonicMediaHeader
{
    uint8_t magic[8];           // Bytes 0-7:  "PYTHIMG\x01" or "PYTHVID\x01"
    uint8_t version;            // Byte 8:    Format version (currently 1)
    uint8_t ext_length;         // Byte 9:    Length of original extension
    char original_ext[16];      // Bytes 10-25: Original extension (e.g., "jpg")
    uint32_t salt;              // Bytes 26-29: Random salt for encryption
    uint8_t compression;        // Byte 30:   0=none, 1=RLE
    uint8_t reserved_flags[3];  // Bytes 31-33: Reserved
    uint64_t original_size;     // Bytes 34-41: Original file size
    uint64_t compressed_size;   // Bytes 42-49: Compressed size (0 if uncompressed)
    uint8_t reserved[14];       // Bytes 50-63: Reserved for future use
};
#pragma pack(pop)
```

**Visual layout:**

```
Offset:  0    8   10      26  30 34       42       50      64
         ┌────┬───┬───────┬───┬──┬────────┬────────┬───────┐
         │MAGIC│V│E│EXT    │SALT│C│ORIG_SZ │COMP_SZ │RSVD   │
         └────┴───┴───────┴───┴──┴────────┴────────┴───────┘

V = Version (1 byte)
E = Extension length (1 byte)
C = Compression type (1 byte)
```

**Example header for "photo.jpg" converted to "photo.pi":**

```
Bytes 0-7:   50 59 54 48 49 4D 47 01  ("PYTHIMG" + version 1)
Byte 8:      01                        (format version 1)
Byte 9:      03                        (extension length = 3)
Bytes 10-25: "jpg" + zeros             (original extension)
Bytes 26-29: Random salt               (unique per file)
Byte 30:     01                        (RLE compression)
...
```

---

## 3. XOR Encryption

The encryption is XOR-based with a rotating key. It's obfuscation, not cryptographic security.

```cpp
constexpr uint8_t ENCRYPT_KEY[32] = {
    0x50, 0x79, 0x74, 0x68, 0x6F, 0x6E, 0x69, 0x63,  // "Pythonic"
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x13, 0x37, 0x42, 0x69, 0x88, 0x99, 0xAA, 0xBB,
    0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44
};
```

**How it works:**

```cpp
void xor_transform(std::vector<uint8_t> &data, uint32_t salt)
{
    // Step 1: Create file-specific key by XORing base key with salt
    uint8_t file_key[32];
    for (size_t i = 0; i < 32; ++i) {
        // Salt is split into 4 bytes, rotated through
        file_key[i] = ENCRYPT_KEY[i] ^ static_cast<uint8_t>(
            (salt >> (i % 4 * 8)) & 0xFF
        );
    }

    // Step 2: Transform each byte
    for (size_t i = 0; i < data.size(); ++i) {
        // Key index rotates and shifts
        size_t key_idx = (i + (i / 32)) % 32;

        // XOR with key byte
        data[i] ^= file_key[key_idx];

        // Bit rotation for additional scrambling
        data[i] = ((data[i] << 3) | (data[i] >> 5));
    }
}
```

**Example transformation:**

```
Original byte:  0x41 ('A')
Salt:           0x12345678
Key byte:       0x50 ^ 0x12 = 0x42
After XOR:      0x41 ^ 0x42 = 0x03
After rotate:   0x03 << 3 | 0x03 >> 5 = 0x18

Result: 'A' (0x41) → 0x18
```

**Why salt matters:**

```
File 1: photo.jpg  → salt=0xAABBCCDD → photo.pi (encrypted data A)
File 2: photo.jpg  → salt=0x11223344 → photo_copy.pi (encrypted data B)

Same source file, different salts = different encrypted output!
This prevents identical files from having identical encrypted forms.
```

---

## 4. RLE Compression

Run-Length Encoding compresses repeated bytes, common in terminal graphics (many 0x00 for empty space).

```cpp
inline std::vector<uint8_t> rle_compress(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> compressed;
    constexpr uint8_t ESCAPE = 0xFF;  // Escape byte

    size_t i = 0;
    while (i < data.size()) {
        uint8_t current = data[i];

        // Count consecutive identical bytes
        size_t run_length = 1;
        while (i + run_length < data.size() &&
               data[i + run_length] == current &&
               run_length < 256) {
            ++run_length;
        }

        if (run_length >= 3 || current == ESCAPE) {
            // Encode as: ESCAPE, count, value
            compressed.push_back(ESCAPE);
            compressed.push_back(run_length == 256 ? 0 : run_length);
            compressed.push_back(current);
            i += run_length;
        } else {
            // Emit bytes directly
            for (size_t j = 0; j < run_length; ++j) {
                compressed.push_back(current);
            }
            i += run_length;
        }
    }
    return compressed;
}
```

**Format:**

```
For runs of 3+ identical bytes:
  [0xFF] [count] [value]

  count = 1-255 for that many bytes
  count = 0 means 256 bytes

For other bytes:
  [byte] (literal)

Special case: if byte is 0xFF:
  [0xFF] [0x01] [0xFF] (encode as run of 1)
```

**Example:**

```
Input:  00 00 00 00 00 41 42 43 00 00 00
        └─── 5 zeros ───┘ A B C └3 zeros┘

Output: FF 05 00 41 42 43 FF 03 00
        └RLE: 5×0x00┘ literals └RLE: 3×0x00┘

Compression: 11 bytes → 9 bytes (82%)
```

**Decompression:**

```cpp
inline std::vector<uint8_t> rle_decompress(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> decompressed;
    constexpr uint8_t ESCAPE = 0xFF;

    size_t i = 0;
    while (i < data.size()) {
        uint8_t byte = data[i++];

        if (byte == ESCAPE) {
            // RLE sequence
            uint8_t count_byte = data[i++];
            uint8_t value = data[i++];

            size_t count = (count_byte == 0) ? 256 : count_byte;
            for (size_t j = 0; j < count; ++j) {
                decompressed.push_back(value);
            }
        } else {
            // Literal byte
            decompressed.push_back(byte);
        }
    }
    return decompressed;
}
```

---

## 5. Converting Files

```cpp
std::string convert(const std::string &input_path,
                    Type type = Type::auto_detect)
{
    // Step 1: Detect file type
    std::string ext = get_extension(input_path);
    bool is_image = (type == Type::image) ||
                    (type == Type::auto_detect && is_image_extension(ext));

    // Step 2: Read original file
    std::vector<uint8_t> data = read_file(input_path);

    // Step 3: Prepare header
    PythonicMediaHeader header;
    if (is_image)
        header.set_magic_image();
    else
        header.set_magic_video();

    header.set_extension(ext);
    header.original_size = data.size();

    // Generate random salt
    std::random_device rd;
    header.salt = rd();

    // Step 4: Compress (optional, controlled by flag)
    std::vector<uint8_t> processed = rle_compress(data);
    header.compression = static_cast<uint8_t>(Compression::rle);
    header.compressed_size = processed.size();

    // Step 5: Encrypt
    xor_transform(processed, header.salt);

    // Step 6: Write output file
    std::string output_path = remove_extension(input_path) +
                              (is_image ? ".pi" : ".pv");

    std::ofstream out(output_path, std::ios::binary);
    out.write(reinterpret_cast<char*>(&header), sizeof(header));
    out.write(reinterpret_cast<char*>(processed.data()), processed.size());

    return output_path;
}
```

**Pipeline visualization:**

```
photo.jpg (100KB)
    │
    ▼ Read file
[Raw bytes: 100KB]
    │
    ▼ RLE Compress
[Compressed: ~95KB]
    │
    ▼ XOR Encrypt (with random salt)
[Encrypted: ~95KB]
    │
    ▼ Add header (64 bytes)
photo.pi (~95KB + 64 bytes)
```

---

## 6. Reverting Files

```cpp
std::string revert(const std::string &input_path)
{
    // Step 1: Read header
    std::ifstream in(input_path, std::ios::binary);
    PythonicMediaHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Step 2: Validate header
    if (!header.is_valid()) {
        throw std::runtime_error("Invalid Pythonic file format");
    }

    // Step 3: Read encrypted data
    std::vector<uint8_t> data(header.compressed_size > 0 ?
                              header.compressed_size :
                              header.original_size);
    in.read(reinterpret_cast<char*>(data.data()), data.size());

    // Step 4: Decrypt (same function, XOR is symmetric)
    xor_untransform(data, header.salt);

    // Step 5: Decompress if needed
    if (header.compression == static_cast<uint8_t>(Compression::rle)) {
        data = rle_decompress(data, header.original_size);
    }

    // Step 6: Write restored file
    std::string base = remove_extension(input_path);
    std::string output_path = base + "_restored." + header.get_extension();

    std::ofstream out(output_path, std::ios::binary);
    out.write(reinterpret_cast<char*>(data.data()), data.size());

    return output_path;
}
```

---

## 7. Integration with Draw Module

The media module integrates seamlessly with pythonicDraw:

```cpp
// In pythonicDraw.hpp
void print_image(const std::string &path, ...)
{
    std::string actual_path = path;

    // Check if this is a Pythonic format
    if (pythonic::media::is_pythonic_image(path)) {
        // Extract to temp file, get actual image path
        actual_path = pythonic::media::extract_to_temp(path);
    }

    // Now render using normal image loading
    // (FFmpeg/ImageMagick can read the temp file)
    ...
}
```

**Example usage:**

```cpp
#include "pythonic/pythonic.hpp"
using namespace Pythonic;

int main() {
    // Convert image to Pythonic format
    pythonic::media::convert("photo.jpg");  // Creates photo.pi

    // Display it directly (auto-detects .pi format)
    print("photo.pi");  // Works seamlessly!

    // Or explicitly
    print("photo.pi", Type::image, Mode::colored);

    return 0;
}
```

---

## 📊 Compression Statistics

For typical terminal graphics (lots of empty space):

| Content Type   | Original | Compressed | Ratio |
| -------------- | -------- | ---------- | ----- |
| Empty canvas   | 10KB     | 0.5KB      | 5%    |
| Simple drawing | 10KB     | 2KB        | 20%   |
| Complex image  | 100KB    | 90KB       | 90%   |
| Video frame    | 50KB     | 45KB       | 90%   |

RLE works best when there are many repeated bytes!

---

## 🔒 Security Note

**This is NOT cryptographic encryption!**

The XOR obfuscation is designed to:

- ✅ Prevent casual viewing of media files
- ✅ Make files unreadable by standard tools
- ✅ Provide unique output for each conversion (salt)

It is NOT designed to:

- ❌ Protect against determined attackers
- ❌ Provide secure encryption
- ❌ Meet compliance requirements

For actual security, use proper encryption libraries.

---

## 📚 Next Steps

- [pythonicDraw Tutorial](../LiveDraw/pythonicDraw_tutorial.md) - Learn how to render media
- [LiveDraw Tutorial](../LiveDraw/livedraw.md) - Create drawings that can be saved to .pi
- [Print Documentation](../Print/print.md) - Display media files
