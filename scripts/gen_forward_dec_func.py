# generate_dispatch_decls.py

type_tags = [
    "none", "int", "float", "string", "bool", "double", "long", "long_long", "long_double",
    "uint", "ulong", "ulong_long", "list", "set", "dict", "orderedset", "ordereddict", "graph"
]

# Map operator to function prefix
ops = {
    "add": "add",
    "sub": "sub",
    "mul": "mul",
    "div": "div",
    "mod": "mod",
    "eq": "eq",
    "ne": "ne",
    "gt": "gt",
    "ge": "ge",
    "lt": "lt",
    "le": "le",
    "band": "band",
    "bor": "bor",
    "bxor": "bxor",
    "shl": "shl",
    "shr": "shr",
    "land": "land",
    "lor": "lor"
}

print("#pragma once\n")
print("#include <cstdint>\n")
print("#include \"pythonicOverflow.hpp\"\n")
print("// Minimal forward-declarations to avoid heavy includes\n")
print("namespace pythonic {\n  namespace vars {\n    enum class TypeTag : uint8_t;\n    class var;\n  }\n}\n")
print("namespace pythonic {\nnamespace dispatch {\nusing pythonic::vars::var;\n")
for opname in ops.values():
    print(f"\n// Forward declarations for all (TypeTag, TypeTag) combinations for {opname}")
    for left in type_tags:
        for right in type_tags:
            print(f"var {opname}__{left}__{right}(const var&, const var&, pythonic::overflow::Overflow policy = pythonic::overflow::Overflow::None_of_them, bool smallest_fit = false);")

print("\n} // namespace dispatch")
print("} // namespace pythonic")