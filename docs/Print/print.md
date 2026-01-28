[⬅ Back to Table of Contents](../index.md)

# Print and Pretty Print

This page documents the `print` and `pprint` helpers in Pythonic, using a clear format with detailed examples.

---

## Overview

The `print` and `pprint` helpers in Pythonic provide a convenient way to output `var` objects and other types to the console or files. These functions are designed to handle a wide range of data types, including primitive values, containers, and nested structures, with a focus on readability and ease of use.

---

## `print`

The `print` function is used for standard output of `var` objects and other types. It supports multiple arguments, custom separators, and end characters.

### Features

- Outputs `var` objects and other types using `str()` or `operator<<`.
- Supports multiple arguments, separated by a customizable separator (default is a space).
- Allows customization of the end character (default is a newline `\n`).

### Example

```cpp
#include "pythonicPrint.hpp"
#include "pythonicVars.hpp"

int main() {
    var a = 42;
    var b = "Hello, World!";
    var c = {1, 2, 3};

    print(a); // Output: 42
    print(b, c); // Output: Hello, World! [1, 2, 3]
    print(a, b, c, "\n", "---"); // Output: 42 Hello, World! [1, 2, 3] ---
    return 0;
}
```

---

## `pprint`

The `pprint` function is designed for pretty-printing complex or nested structures. It uses `pretty_str()` to format the output with indentation and line breaks for better readability.

### Features

- Outputs nested structures in a human-readable format.
- Automatically applies indentation and line breaks for clarity.
- Ideal for debugging and inspecting large or deeply nested data.

### Example

```cpp
#include "pythonicPrint.hpp"
#include "pythonicVars.hpp"

int main() {
    var nested = {"key1": {"subkey1": 1, "subkey2": 2}, "key2": [1, 2, 3]};

    pprint(nested);
    // Output:
    // {
    //     "key1": {
    //         "subkey1": 1,
    //         "subkey2": 2
    //     },
    //     "key2": [
    //         1,
    //         2,
    //         3
    //     ]
    // }

    return 0;
}
```

---

## Integration with `str()`, `toString()`, `pretty_str()`, and `operator<<`

- `print` relies on `str()` and `operator<<` to format output. This ensures that even large containers are displayed in a readable format.
- `pprint` uses `pretty_str()` to format nested structures with indentation and line breaks.
- Custom types can implement `toString()` or `operator<<` to define their own string representation for `print` and `pprint`.

### Example

```cpp
#include "pythonicPrint.hpp"
#include "pythonicVars.hpp"

struct CustomType {
    int x;
    int y;

    friend std::ostream& operator<<(std::ostream& os, const CustomType& obj) {
        return os << "CustomType(" << obj.x << ", " << obj.y << ")";
    }
};

int main() {
    CustomType obj = {10, 20};

    print(obj); // Output: CustomType(10, 20)

    return 0;
}
```

---

## File Output Helpers

Both `print` and `pprint` can redirect their output to files or streams. This is useful for logging or saving data for later analysis.

### Example

```cpp
#include "pythonicPrint.hpp"
#include "pythonicVars.hpp"
#include <fstream>

int main() {
    var data = {"name": "Alice", "age": 30, "hobbies": {"reading", "hiking"}};

    std::ofstream file("output.txt");
    if (file.is_open()) {
        print(file, data); // Writes to file
        pprint(file, data); // Pretty-prints to file
        file.close();
    }

    return 0;
}
```

---

## Performance Considerations

While `print` and `pprint` are convenient for debugging and output, they may not be suitable for performance-sensitive logging. For large `var` objects or containers, consider serializing only the necessary parts to minimize overhead.

# Next Check
- [Var](../Var/var.md)