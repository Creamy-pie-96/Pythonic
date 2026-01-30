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
print("#include \"pythonicVars.hpp\"\n")
print("namespace pythonic {")
print("namespace dispatch {")

for opname in ops.values():
    print(f"\n// Forward declarations for all (TypeTag, TypeTag) combinations for {opname}")
    for left in type_tags:
        for right in type_tags:
            print(f"var {opname}_{left}_{right}(const var&, const var&);")

print("\n} // namespace dispatch")
print("} // namespace pythonic")