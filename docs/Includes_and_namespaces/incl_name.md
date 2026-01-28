[⬅ Back to Table of Contents](../index.md)

# Includes and Namespaces

## The Easy Way (Recommended)

Just include the main header—this pulls in everything under the `Pythonic` (or `py`) namespace:

```cpp
#include "pythonic/pythonic.hpp"

using namespace Pythonic;
// or
using namespace py;
```

## The Manual Way

Or, pick exactly what you need:

```cpp
#include "pythonicVars.hpp"      // var, list, dict, set
#include "pythonicPrint.hpp"     // print(), pprint()
#include "pythonicLoop.hpp"      // range(), enumerate(), zip() + macros
#include "pythonicFunction.hpp"  // map, filter, comprehensions
#include "pythonicFile.hpp"      // File I/O
#include "pythonicMath.hpp"      // Math functions (trig, logarithms, etc.)

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::math;
using namespace pythonic::file;
using namespace pythonic::func;
using namespace pythonic::graph;
using namespace pythonic::fast;
using namespace pythonic::overflow;
using namespace pythonic::error;
```

> **Caution:**  
> If you import namespaces globally, be careful with `std` and other namespaces—they might clash or cause ambiguity. We recommend using `std::` explicitly if you make the `pythonic` namespace global.

---

## About Namespaces

- `pythonic::vars` – Core `var` type, containers, type conversion
- `pythonic::print` – Printing and formatting (`print()`, `pprint()`)
- `pythonic::loop` – Iteration helpers (`range()`, `enumerate()`, `zip()`, `reversed()`)
- `pythonic::func` – Functional programming (`map()`, `filter()`, comprehensions)
- `pythonic::file` – File I/O
- `pythonic::math` – Comprehensive math library (`trig`, `logarithms`, `random`, etc.)
- `pythonic::graph` – Graph data structure and algorithms
- `pythonic::error` – Exception hierarchy and error handling
- `pythonic::fast` – Hot-loop optimizations
- `pythonic::overflow` – Checked arithmetic helpers

You can use them all at once with:

```cpp
using namespace Pythonic;
// or
using namespace py;
```

---

> **Note:**  
> We highly recommend including the whole library at once, as many parts have dependencies on others (e.g., the math library depends on `var`). If you try to use the math library without including `pythonicVar.hpp` and the `var` namespace, your code may not work as expected.

---

## Next

- [Print](../Print/print.md)
