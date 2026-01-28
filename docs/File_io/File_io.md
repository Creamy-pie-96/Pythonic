[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Functional](../Functional/functional.md)

# File I/O

This page documents all user-facing file I/O helpers in Pythonic, including the File class, open modes, quick helpers, and idiomatic usage, with concise examples.

---

## File Modes

| Mode String | Enum Value              | Description               |
| ----------- | ----------------------- | ------------------------- |
| "r"         | FileMode::READ          | Read only                 |
| "w"         | FileMode::WRITE         | Write only (truncate)     |
| "a"         | FileMode::APPEND        | Append only               |
| "r+"        | FileMode::READ_WRITE    | Read and write            |
| "w+"        | FileMode::WRITE_READ    | Write and read (truncate) |
| "a+"        | FileMode::APPEND_READ   | Append and read           |
| "rb"        | FileMode::READ_BINARY   | Read binary               |
| "wb"        | FileMode::WRITE_BINARY  | Write binary (truncate)   |
| "ab"        | FileMode::APPEND_BINARY | Append binary             |

---

## File Class

| Method / Property      | Description                        | Example                       |
| ---------------------- | ---------------------------------- | ----------------------------- |
| `File(filename, mode)` | Open file with mode (default "r")  | `File("data.txt", "w")`       |
| `open(filename, mode)` | Global open function (like Python) | `open("data.txt", "r")`       |
| `read()`               | Read entire file as string         | `f.read()`                    |
| `read(n)`              | Read n characters                  | `f.read(10)`                  |
| `readline()`           | Read one line                      | `f.readline()`                |
| `readlines()`          | Read all lines as list             | `f.readlines()`               |
| `write(content)`       | Write string to file               | `f.write("hello")`            |
| `writeln(content)`     | Write string with newline          | `f.writeln("line")`           |
| `writelines(lines)`    | Write list of lines                | `f.writelines(list("a","b"))` |
| `flush()`              | Flush buffer                       | `f.flush()`                   |
| `seek(pos)`            | Seek to position                   | `f.seek(0)`                   |
| `tell()`               | Get current position               | `f.tell()`                    |
| `eof()`                | Check if at end of file            | `f.eof()`                     |
| `name()`               | Get filename                       | `f.name()`                    |
| `mode()`               | Get mode as string                 | `f.mode()`                    |
| `is_open()`            | Check if file is open              | `f.is_open()`                 |
| `close()`              | Close the file                     | `f.close()`                   |
| `operator bool()`      | True if file is open               | `if (f) { ... }`              |
| `begin()/end()`        | Iterator for reading lines         | `for (auto line : f) { ... }` |

---

## Quick File Helpers

| Function                         | Description                | Example                                  |
| -------------------------------- | -------------------------- | ---------------------------------------- |
| `read_file(filename)`            | Read entire file as string | `read_file("data.txt")`                  |
| `read_lines(filename)`           | Read all lines as list     | `read_lines("data.txt")`                 |
| `write_file(filename, content)`  | Write string to file       | `write_file("data.txt", "hi")`           |
| `append_file(filename, content)` | Append string to file      | `append_file("data.txt", "hi")`          |
| `write_lines(filename, lines)`   | Write list of lines        | `write_lines("data.txt", list("a","b"))` |
| `file_exists(filename)`          | Check if file exists       | `file_exists("data.txt")`                |

---

## Idiomatic Usage

### Basic File Reading

```cpp
using namespace pythonic::file;
File f("data.txt", "r");
var content = f.read();
f.close();
```

### With Statement (RAII)

```cpp
using namespace pythonic::file;
with_open("data.txt", "r", f) {
    var content = f.read();
}
```

### Quick Helpers

```cpp
using namespace pythonic::file;
var content = read_file("data.txt");
var lines = read_lines("data.txt");
write_file("out.txt", "hello");
append_file("out.txt", "world\n");
write_lines("out.txt", list("a","b"));
```

### Iterating Over Lines

```cpp
using namespace pythonic::file;
File f("data.txt");
for (auto line : f) {
    print(line);
}
```

---

## Notes

- The File class automatically closes files on destruction (RAII).
- Use with_open for Python-like with statement semantics.
- All helpers accept std::string, const char\*, or var for filenames.
- File modes match Python's open() for easy migration.
- File errors throw PythonicFileError exceptions on failure.

---

## Next check

- [QuickStart](../examples/quickstart.md)