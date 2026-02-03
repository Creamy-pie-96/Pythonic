[⬅ Back to Table of Contents](../index.md)

# 🔐 pythonicMedia Tutorial - Proprietary Format Encryption

**pythonicMedia.hpp** provides functions to convert standard media files into encrypted Pythonic formats (.pi, .pv). This tutorial explains the format, encryption, compression, and practical workflows.

---

## 📚 Table of Contents

1. [What is Pythonic Format?](#1-what-is-pythonic-format)
2. [File Structure & Encryption](#2-file-structure--encryption)
3. [RLE Compression](#3-rle-compression)
4. [Core Workflow: convert() & revert()](#4-core-workflow-convert--revert)
5. [Integration with Print/Export](#5-integration-with-printexport)
6. [Practical Examples](#6-practical-examples)

---

## 1. What is Pythonic Format?

Pythonic format provides **encrypted, optionally compressed** media files that can only be read by this library.

### File Extensions

| Extension | Type  | Description                      |
| --------- | ----- | -------------------------------- |
| `.pi`     | Image | Pythonic Image (encrypted)       |
| `.pv`     | Video | Pythonic Video (encrypted)       |

### Why Use Pythonic Format?

| Feature           | Benefit                                  |
| ----------------- | ---------------------------------------- |
| **Obfuscation**   | XOR encryption with salt-based key       |
| **Compression**   | RLE compression for terminal graphics    |
| **Round-trip**    | Lossless conversion back to original     |
| **Auto-integration** | Works seamlessly with `print()` and `export_media()` |
| **Metadata**      | Preserves original format for perfect restoration |

---

## 2. File Structure & Encryption

### File Layout

```
┌─────────────────────────────────────────┐
│ Header (64 bytes)                       │
│  - Magic bytes (8 bytes)                │
│  - Version (1 byte)                     │
│  - Original extension (16 bytes)        │
│  - Salt (4 bytes)                       │
│  - Compression info (8 bytes)           │
│  - Size metadata (16 bytes)             │
│  - Reserved (14 bytes)                  │
├─────────────────────────────────────────┤
│ Encrypted Data (variable size)          │
│  - XOR encrypted with rotating key      │
│  - Bit-rotated for additional obfusc.   │
│  - Optionally RLE compressed            │
└─────────────────────────────────────────┘
```

### Header Breakdown

| Offset | Field                | Size | Purpose                                  |
| ------ | -------------------- | ---- | ---------------------------------------- |
| 0-7    | Magic bytes          | 8    | 'PYTHIMG\x01' or 'PYTHVID\x01'          |
| 8      | Version              | 1    | Format version (v1 = current)            |
| 9      | Ext length           | 1    | Length of original extension             |
| 10-25  | Original extension   | 16   | `.jpg`, `.png`, `.mp4`, etc.            |
| 26-29  | Salt                 | 4    | Random salt for key derivation           |
| 30     | Compression type     | 1    | 0=none, 1=RLE                           |
| 34-41  | Original size        | 8    | Size before encryption/compression       |
| 42-49  | Compressed size      | 8    | Size after compression (0 if none)       |

**Magic Bytes Identification:**

```cpp
// Image file
'P' 'Y' 'T' 'H' 'I' 'M' 'G' 0x01

// Video file
'P' 'Y' 'T' 'H' 'V' 'I' 'D' 0x01
```

### Encryption Algorithm

**Step 1: Key Derivation**
```cpp
// Base key (32 bytes): "Pythonic" + random hex values
ENCRYPT_KEY = {0x50, 0x79, 0x74, 0x68, 0x6F, 0x6E, 0x69, 0x63, ...}

// File-specific key (XOR with salt)
for (size_t i = 0; i < 32; ++i) {
    file_key[i] = ENCRYPT_KEY[i] ^ (salt >> (i % 4 * 8));
}
```

**Step 2: Byte Transformation**
```cpp
for (size_t i = 0; i < data.size(); ++i) {
    // 1. XOR with rotating key
    size_t key_idx = (i + (i / 32)) % 32;  // Position-dependent
    data[i] ^= file_key[key_idx];
    
    // 2. Bit rotation (left 3, right 5)
    data[i] = (data[i] << 3) | (data[i] >> 5);
}
```

**Decryption:**
```cpp
// Reverse order: bit rotation first, then XOR
data[i] = (data[i] >> 3) | (data[i] << 5);  // Reverse rotation
data[i] ^= file_key[key_idx];               // XOR again
```

**Security Note:** This is **obfuscation**, not cryptographic security. XOR is symmetric and reversible. Use for privacy, not military-grade secrets!

---

## 3. RLE Compression

**Run-Length Encoding (RLE)** compresses runs of identical bytes. Perfect for terminal graphics with many repeated values (e.g., 0x00 for empty braille cells).

### RLE Format

```
For runs of 3+ identical bytes:
  [ESCAPE=0xFF] [count] [value]

For other bytes:
  [literal byte]

Special case (if byte is 0xFF):
  [0xFF] [0x01] [0xFF]
```

### Example Compression

**Input:**
```
0x00 0x00 0x00 0x00 0x00 0x41 0x42 0xFF 0xFF 0xFF
```

**Output:**
```
0xFF 0x05 0x00     // 5 zeros
0x41                // literal 'A'
0x42                // literal 'B'
0xFF 0x03 0xFF     // 3 escape bytes
```

**Compression Ratio:**
```
Original: 10 bytes
Compressed: 7 bytes
Ratio: 70%
```

### Compression Effectiveness

| Content Type           | Typical Ratio | Reason                              |
| ---------------------- | ------------- | ----------------------------------- |
| Terminal graphics      | **20-40%**    | Many repeated 0x00 (empty cells)    |
| Already compressed (.png, .jpg, .mp4) | **98-102%**   | No benefit (already compressed)     |
| Raw bitmap data        | **60-80%**    | Some repeated pixels                |
| Text files             | **50-70%**    | Repeated spaces, newlines           |

---

## 4. Core Workflow: convert() & revert()

### convert() - Encrypt Media

**Process Flow:**

```
1. Read source file into memory
   ↓
2. Auto-detect type (image vs video)
   ↓
3. Apply RLE compression (if enabled)
   ↓
4. XOR encrypt with file-specific key
   ↓
5. Create header with metadata
   ↓
6. Write header + encrypted data to .pi/.pv
```

**Usage:**

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::media;

// Basic usage (auto-detect, with compression)
std::string output = convert("photo.jpg");
// Creates: photo.pi

// Force type
convert("data.bin", MediaType::image);

// Disable compression
convert("video.mp4", MediaType::auto_detect, false);
```

**When to disable compression:**
- Already compressed media (.jpg, .png, .mp4): No benefit
- Speed-critical conversions: Skip compression step
- Small files (< 100 KB): Overhead not worth it

### revert() - Decrypt Media

**Process Flow:**

```
1. Read .pi/.pv file
   ↓
2. Parse header, validate magic bytes
   ↓
3. Read encrypted data
   ↓
4. XOR decrypt with salt-derived key
   ↓
5. Decompress RLE (if compression flag set)
   ↓
6. Write original file with saved extension
```

**Usage:**

```cpp
// Default naming: basename_reverted.ext
revert("photo.pi");  // Creates: photo_reverted.jpg

// Custom output name
revert("photo.pi", "restored_photo");  // Creates: restored_photo.jpg

// With path
revert("secret.pi", "output/final");  // Creates: output/final.jpg
```

---

## 5. Integration with Print/Export

**Seamless Decryption:** `print()` and `export_media()` automatically decrypt .pi/.pv files!

### Example 1: Print Encrypted Media

```cpp
using namespace Pythonic;
using namespace pythonic::media;

// Encrypt
convert("photo.jpg");  // Creates photo.pi

// Print (auto-decrypts!)
print("photo.pi");  // Displays as if it were photo.jpg

// Works with videos too
convert("movie.mp4");  // Creates movie.pv
print("movie.pv");     // Plays video with audio
```

### Example 2: Export Encrypted Media

```cpp
// Encrypt source
convert("photo.jpg");  // photo.pi

// Export encrypted file (auto-decrypts, then exports)
export_media("photo.pi", "output", RenderConfig()
    .set_format(Format::image)
    .set_mode(Mode::colored));
// Creates: output.png (rendered from decrypted photo.pi)
```

### Example 3: Workflow Integration

```cpp
// 1. Encrypt source media
std::string encrypted_img = convert("sensitive_photo.jpg");
std::string encrypted_vid = convert("private_video.mp4");

// 2. Use encrypted files everywhere
print(encrypted_img);  // Display
export_media(encrypted_vid, "clip", RenderConfig()
    .set_format(Format::video)
    .set_start_time(10)
    .set_end_time(20));

// 3. Revert when needed
revert(encrypted_img, "final_output");
```

---

## 6. Practical Examples

### Example 1: Batch Encrypt Directory

```cpp
#include <pythonic/pythonic.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using namespace pythonic::media;

void encrypt_directory(const std::string &dir) {
    for (const auto &entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        
        std::string path = entry.path().string();
        std::string ext = get_extension(path);
        
        if (is_image_extension(ext) || is_video_extension(ext)) {
            try {
                std::string output = convert(path);
                std::cout << "✓ " << path << " → " << output << "\n";
            } catch (const std::exception &e) {
                std::cerr << "✗ " << path << ": " << e.what() << "\n";
            }
        }
    }
}
```

### Example 2: Inspect Encrypted Files

```cpp
#include <pythonic/pythonic.hpp>
#include <iomanip>

using namespace pythonic::media;

void analyze_pythonic_file(const std::string &filepath) {
    auto [is_image, ext, orig_size, comp_size, comp_type] = 
        get_info_detailed(filepath);
    
    std::cout << "════════════════════════════════════\n";
    std::cout << "File: " << filepath << "\n";
    std::cout << "────────────────────────────────────\n";
    std::cout << "Type: " << (is_image ? "Image" : "Video") << "\n";
    std::cout << "Original format: " << ext << "\n";
    std::cout << "Original size: " << orig_size << " bytes ("
              << (orig_size / 1024.0) << " KB)\n";
    
    if (comp_type == Compression::rle && comp_size > 0) {
        double ratio = 100.0 * comp_size / orig_size;
        double savings = orig_size - comp_size;
        
        std::cout << "Compressed: " << comp_size << " bytes ("
                  << std::fixed << std::setprecision(1) << ratio << "%)\n";
        std::cout << "Saved: " << savings << " bytes ("
                  << (savings / 1024.0) << " KB)\n";
    } else {
        std::cout << "Compression: None\n";
    }
    
    std::cout << "════════════════════════════════════\n";
}
```

**Output:**

```
════════════════════════════════════
File: terminal_art.pi
────────────────────────────────────
Type: Image
Original format: .txt
Original size: 1024000 bytes (1000.0 KB)
Compressed: 256000 bytes (25.0%)
Saved: 768000 bytes (750.0 KB)
════════════════════════════════════
```

### Example 3: Smart Compression Decision

```cpp
#include <pythonic/pythonic.hpp>
#include <filesystem>

using namespace pythonic::media;

std::string smart_convert(const std::string &filepath) {
    std::string ext = get_extension(filepath);
    size_t size = std::filesystem::file_size(filepath);
    
    // Heuristics for compression
    bool should_compress = false;
    
    // Text/raw formats: Always compress
    if (ext == ".txt" || ext == ".csv" || ext == ".bmp") {
        should_compress = true;
    }
    // Already compressed: Never compress
    else if (ext == ".jpg" || ext == ".png" || ext == ".mp4") {
        should_compress = false;
    }
    // Large files: Compress if > 1MB
    else {
        should_compress = (size > 1024 * 1024);
    }
    
    std::string output = convert(filepath, MediaType::auto_detect, should_compress);
    
    std::cout << filepath << " → " << output 
              << (should_compress ? " (compressed)" : " (uncompressed)") << "\n";
    
    return output;
}
```

---

## Summary Table

| Function              | Purpose                          | Input           | Output                |
| --------------------- | -------------------------------- | --------------- | --------------------- |
| `convert()`           | Encrypt media to .pi/.pv         | .jpg, .mp4, etc | .pi or .pv            |
| `revert()`            | Decrypt .pi/.pv back to original | .pi or .pv      | Original format       |
| `get_info()`          | Get metadata (type, ext, size)   | .pi or .pv      | Tuple (bool, str, uint64) |
| `get_info_detailed()` | Get detailed info + compression  | .pi or .pv      | Tuple (5 values)      |
| `is_pythonic_format()`| Check if file is .pi/.pv         | Any filename    | bool                  |
| `extract_to_temp()`   | Decrypt to temp file             | .pi or .pv      | Temp file path        |

---

## Recommendations

✅ **Enable compression for:**
- Terminal graphics (.txt with Braille/ASCII art)
- Raw bitmap data (.bmp)
- Text-based formats (.csv, .log)

❌ **Disable compression for:**
- Already compressed formats (.jpg, .png, .mp4)
- Real-time conversion scenarios
- Very small files (< 100 KB)

📦 **Storage savings:**
- Terminal graphics: 60-80% reduction
- Videos/Images: Minimal (already compressed)

🔐 **Security level:**
- Obfuscation: ✅ (good for privacy)
- Cryptographic security: ❌ (not for sensitive secrets)
- Use case: Protecting media in public repositories, preventing casual viewing

---

# Next Check

- [Back to Table of Contents](../index.md)
- [Media Documentation](../Media/media.md)
