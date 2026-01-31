# gen_dispatch_stubs.py
import os

type_tags = [
    "none", "int", "float", "string", "bool", "double", "long", "long_long", "long_double",
    "uint", "ulong", "ulong_long", "list", "set", "dict", "orderedset", "ordereddict", "graph"
]

cpp_types = {
    "int": "int", "float": "float", "string": "std::string", "bool": "bool", 
    "double": "double", "long": "long", "long_long": "long long", 
    "long_double": "long double", "uint": "unsigned int", "ulong": "unsigned long", 
    "ulong_long": "unsigned long long", "list": "List", "set": "Set", 
    "dict": "Dict", "orderedset": "OrderedSet", "ordereddict": "OrderedDict", "graph": "Graph"
}

# Fully-qualified names for container types used inside generated .cpp
qualified = {}
for k, v in cpp_types.items():
    if v in ("List", "Set", "Dict", "OrderedSet", "OrderedDict", "Graph"):
        qualified[k] = f"pythonic::vars::{v}"
    else:
        qualified[k] = v

ops = {
    "add": "add", "sub": "sub", "mul": "mul", "div": "div", "mod": "mod",
    "eq": "eq", "ne": "ne", "gt": "gt", "ge": "ge", "lt": "lt", "le": "le",
    "band": "band", "bor": "bor", "bxor": "bxor", "shl": "shl", "shr": "shr",
    "land": "land", "lor": "lor"
}

print("#include \"pythonic/pythonicDispatchForwardDecls.hpp\"")
print("#include \"pythonic/pythonicVars.hpp\"")
print("#include \"pythonic/pythonicError.hpp\"")
print("#include <stdexcept>\n")
print("namespace pythonic {")
print("namespace dispatch {")
print("// generated stubs live in pythonic::dispatch")

def is_numeric(t):
    return t in ["int", "float", "double", "long", "long_long", "long_double", "uint", "ulong", "ulong_long"]

def get_common_type(t1, t2):
    if t1 == t2:
        return cpp_types[t1]
    if t1 == "long_double" or t2 == "long_double": return "long double"
    if t1 == "double" or t2 == "double": return "double"
    if t1 == "float" or t2 == "float": return "float"
    if t1 == "long_long" or t2 == "long_long": return "long long"
    if t1 == "ulong_long" or t2 == "ulong_long": return "unsigned long long"
    if t1 == "long" or t2 == "long": return "long"
    if t1 == "ulong" or t2 == "ulong": return "ulong"
    if t1 == "int" or t2 == "int": return "int"
    if t1 == "uint" or t2 == "uint": return "uint"

    return "long long" # Default promotion

for opname in ops.values():
    print(f"\n// Stub definitions for {opname}")
    for left in type_tags:
        for right in type_tags:
            # Use double underscore to avoid ambiguity
            print(f"var {opname}__{left}__{right}(const var& a, const var& b, pythonic::overflow::Overflow policy, bool smallest_fit) {{")
            
            # Implementation Logic
            if opname == "add":
                if left == "string" and right == "string":
                    print(f"    return var(a.var_get<std::string>() + b.var_get<std::string>());")
                elif left == "list" and right == "list":
                    print(f"    const auto& al = a.var_get<{qualified['list']}>();")
                    print(f"    const auto& bl = b.var_get<{qualified['list']}>();")
                    print(f"    {qualified['list']} res; res.reserve(al.size() + bl.size());")
                    print("    res.insert(res.end(), al.begin(), al.end());")
                    print("    res.insert(res.end(), bl.begin(), bl.end());")
                    print("    return var(std::move(res));")
                elif is_numeric(left) and is_numeric(right):
                    ctype = get_common_type(left, right)
                    print(f"    return var(({ctype})a.var_get<{cpp_types[left]}>() + ({ctype})b.var_get<{cpp_types[right]}>());")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for +: '{left}' and '{right}'\");")
            
            elif opname == "sub":
                if is_numeric(left) and is_numeric(right):
                    ctype = get_common_type(left, right)
                    print(f"    return var(({ctype})a.var_get<{cpp_types[left]}>() - ({ctype})b.var_get<{cpp_types[right]}>());")
                elif left == "set" and right == "set":
                    print(f"    const auto& as = a.var_get<{qualified['set']}>();")
                    print(f"    const auto& bs = b.var_get<{qualified['set']}>();")
                    print(f"    {qualified['set']} res;")
                    print("    for(const auto& item : as) {")
                    print("        if(bs.find(item) == bs.end()) res.insert(item);")
                    print("    }")
                    print("    return var(std::move(res));")
                elif left == "dict" and right == "dict":
                    print(f"    const auto& ad = a.var_get<{qualified['dict']}>();")
                    print(f"    const auto& bd = b.var_get<{qualified['dict']}>();")
                    print(f"    {qualified['dict']} res;")
                    print("    for(const auto& [k, v] : ad) {")
                    print("        if(bd.find(k) == bd.end()) res[k] = v;")
                    print("    }")
                    print("    return var(std::move(res));")
                elif left == "orderedset" and right == "orderedset":
                    print(f"    const auto& a_cont = a.var_get<{qualified['orderedset']}>();")
                    print(f"    const auto& b_cont = b.var_get<{qualified['orderedset']}>();")
                    print(f"    {qualified['orderedset']} res;")
                    print("    // Merge-like difference preserving order")
                    print("    auto it_a = a_cont.begin();")
                    print("    auto it_b = b_cont.begin();")
                    print("    while (it_a != a_cont.end() && it_b != b_cont.end()) {")
                    print("        if (*it_a < *it_b) {")
                    print("            res.insert(*it_a);")
                    print("            ++it_a;")
                    print("        } else if (*it_b < *it_a) {")
                    print("            ++it_b;")
                    print("        } else {")
                    print("            // Equal, skip")
                    print("            ++it_a; ++it_b;")
                    print("        }")
                    print("    }")
                    print("    while (it_a != a_cont.end()) {")
                    print("        res.insert(*it_a);")
                    print("        ++it_a;")
                    print("    }")
                    print("    return var(std::move(res));")
                elif left == "ordereddict" and right == "ordereddict":
                    print(f"    const auto& ad = a.var_get<{qualified['ordereddict']}>();")
                    print(f"    const auto& bd = b.var_get<{qualified['ordereddict']}>();")
                    print(f"    {qualified['ordereddict']} res;")
                    print("    for(const auto& val : ad) {")
                    print("        if(bd.find(val.first) == bd.end()) res.insert(val);")
                    print("    }")
                    print("    return var(std::move(res));")
                elif left == "list" and right == "list":
                    # List difference (remove items in A that are in B)
                    print(f"    const auto& al = a.var_get<{qualified['list']}>();")
                    print(f"    const auto& bl = b.var_get<{qualified['list']}>();")
                    print(f"    {qualified['list']} res;")
                    print(f"    {qualified['set']} bs(bl.begin(), bl.end());")
                    print("    for(const auto& item : al) {")
                    print("        if(bs.find(item) == bs.end()) res.push_back(item);")
                    print("    }")
                    print("    return var(std::move(res));")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for -: '{left}' and '{right}'\");")
            
            elif opname == "mul":
                if left == "string" and right in ["int", "long", "long_long"]:
                     print("    std::string s = a.var_get<std::string>();")
                     print(f"    long long n = (long long)b.var_get<{cpp_types[right]}>();")
                     print("    if(n <= 0) return var(std::string(\"\"));")
                     print("    std::string res; res.reserve(s.size() * n);")
                     print("    for(long long i=0; i<n; ++i) res += s;")
                     print("    return var(res);")
                elif right == "string" and left in ["int", "long", "long_long"]:
                     print("    std::string s = b.var_get<std::string>();")
                     print(f"    long long n = (long long)a.var_get<{cpp_types[left]}>();")
                     print("    if(n <= 0) return var(std::string(\"\"));")
                     print("    std::string res; res.reserve(s.size() * n);")
                     print("    for(long long i=0; i<n; ++i) res += s;")
                     print("    return var(res);")
                elif left == "list" and right in ["int", "long", "long_long"]:
                     print(f"    const auto& lst = a.var_get<{qualified['list']}>();")
                     print(f"    long long n = (long long)b.var_get<{cpp_types[right]}>();")
                     print(f"    if(n <= 0) return var({qualified['list']}{{}});")
                     print(f"    {qualified['list']} res; res.reserve(lst.size() * n);")
                     print("    for(long long i=0; i<n; ++i) res.insert(res.end(), lst.begin(), lst.end());")
                     print("    return var(std::move(res));")
                elif right == "list" and left in ["int", "long", "long_long"]:
                     print(f"    const auto& lst = b.var_get<{qualified['list']}>();")
                     print(f"    long long n = (long long)a.var_get<{cpp_types[left]}>();")
                     print(f"    if(n <= 0) return var({qualified['list']}{{}});")
                     print(f"    {qualified['list']} res; res.reserve(lst.size() * n);")
                     print("    for(long long i=0; i<n; ++i) res.insert(res.end(), lst.begin(), lst.end());")
                     print("    return var(std::move(res));")
                elif is_numeric(left) and is_numeric(right):
                    ctype = get_common_type(left, right)
                    print(f"    return var(({ctype})a.var_get<{cpp_types[left]}>() * ({ctype})b.var_get<{cpp_types[right]}>());")
                else:
                     print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for *: '{left}' and '{right}'\");")

            elif opname == "div":
                 if is_numeric(left) and is_numeric(right):
                    print(f"    double val_b = (double)b.var_get<{cpp_types[right]}>();")
                    print(f"    if (val_b == 0.0) throw pythonic::PythonicZeroDivisionError(\"float division by zero\");")
                    print(f"    return var((double)a.var_get<{cpp_types[left]}>() / val_b);")
                 else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for /: '{left}' and '{right}'\");")
            
            elif opname == "mod":
                 if is_numeric(left) and is_numeric(right):
                    if left in ["int", "long", "long_long"] and right in ["int", "long", "long_long"]:
                         print(f"    auto val_b = b.var_get<{cpp_types[right]}>();")
                         print(f"    if (val_b == 0) throw pythonic::PythonicZeroDivisionError(\"integer division or modulo by zero\");")
                         print(f"    return var(a.var_get<{cpp_types[left]}>() % val_b);")
                    else:
                         print(f"    throw pythonic::PythonicTypeError(\"TypeError: modulo not supported for float types here yet\");")
                 else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for %: '{left}' and '{right}'\");")

            else:
                print(f"    throw std::runtime_error(\"Not implemented: {opname} for {left} and {right}\");")
            
            print("}")

# OpTable map to Struct names
op_struct_map = {
    "add": "Add", "sub": "Sub", "mul": "Mul", "div": "Div", "mod": "Mod",
    "eq": "Eq", "ne": "Ne", "gt": "Gt", "ge": "Ge", "lt": "Lt", "le": "Le",
    "band": "BitAnd", "bor": "BitOr", "bxor": "BitXor",
    "shl": "ShiftLeft", "shr": "ShiftRight",
    "land": "LogicalAnd", "lor": "LogicalOr"
}

# Generate OpTable initializations
for opname, opstruct_name in op_struct_map.items():
    actual_op_func = ops[opname] # e.g. "add"
    print(f"\n// OpTable initialization for {opstruct_name}")
    print(f"template <>")
    print(f"const std::array<std::array<pythonic::dispatch::BinaryOpFunc, pythonic::dispatch::TypeTagCount>, pythonic::dispatch::TypeTagCount> pythonic::dispatch::OpTable<pythonic::dispatch::{opstruct_name}>::table = []() {{")
    print(f"    std::array<std::array<pythonic::dispatch::BinaryOpFunc, pythonic::dispatch::TypeTagCount>, pythonic::dispatch::TypeTagCount> t{{}};")
    for i, left in enumerate(type_tags):
        # build row
        entries = ", ".join([f"pythonic::dispatch::{actual_op_func}__{left}__{right}" for right in type_tags])
        print(f"    t[{i}] = {{{entries}}};")
    print(f"    return t;")
    print(f"}}();")

print("\n} // namespace dispatch")
print("} // namespace pythonic")
