# gen_dispatch_stubs.py
import os
import sys

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

# TODO: For now I am keeping land and lor in the var class and simple. if someday i feel like adding complex and fun features with those operators overloads that will need type context then i will add logics for them.
ops = {
    "add": "add", "sub": "sub", "mul": "mul", "div": "div", "mod": "mod",
    "eq": "eq", "ne": "ne", "gt": "gt", "ge": "ge", "lt": "lt", "le": "le",
    "band": "band", "bor": "bor", "bxor": "bxor", "shl": "shl", "shr": "shr",
    "land": "land", "lor": "lor"
}

op_symbols = {
    "add": "+", "sub": "-", "mul": "*", "div": "/", "mod": "%",
    "eq": "==", "ne": "!=", "gt": ">", "ge": ">=", "lt": "<", "le": "<=",
    "band": "&", "bor": "|", "bxor": "^", "shl": "<<", "shr": ">>",
    "land": "and", "lor": "or"
}

# OpTable map to Struct names
op_struct_map = {
    "add": "Add", "sub": "Sub", "mul": "Mul", "div": "Div", "mod": "Mod",
    "eq": "Eq", "ne": "Ne", "gt": "Gt", "ge": "Ge", "lt": "Lt", "le": "Le",
    "band": "BitAnd", "bor": "BitOr", "bxor": "BitXor",
    "shl": "ShiftLeft", "shr": "ShiftRight",
    "land": "LogicalAnd", "lor": "LogicalOr"
}

if len(sys.argv) > 1 and sys.argv[1] == 'declarations':
    print("// Generated declarations for OpTable specializations")
    for opname, opstruct_name in op_struct_map.items():
        print(f"template<> const std::array<std::array<BinaryOpFunc, TypeTagCount>, TypeTagCount> OpTable<{opstruct_name}>::table;")
    sys.exit(0)

print("#include \"pythonic/pythonicDispatchForwardDecls.hpp\"")
print("#include \"pythonic/pythonicVars.hpp\"")
print("#include \"pythonic/pythonicError.hpp\"")
print("#include \"pythonic/pythonicOverflow.hpp\"")
print("#include \"pythonic/pythonicPromotion.hpp\"")
print("#include <stdexcept>\n")
print("#include <algorithm>\n")
print("namespace pythonic {")
print("namespace dispatch {")
print("// generated stubs live in pythonic::dispatch")

def is_numeric(t):
    return t in ["int", "float", "double", "long", "long_long", "long_double", "uint", "ulong", "ulong_long", "bool"]

def is_integral(t):
    return t in ["int", "long", "long_long", "uint", "ulong", "ulong_long", "bool"]

def get_common_type(t1, t2):
    # Treat bool as integer (0/1) for promotion purposes
    if t1 == 'bool' and t2 == 'bool':
        return 'int'
    if t1 == 'bool':
        return get_common_type('int', t2)
    if t2 == 'bool':
        return get_common_type(t1, 'int')
    if t1 == t2:
        return cpp_types[t1]
    if t1 == "long_double" or t2 == "long_double": return "long double"
    if t1 == "double" or t2 == "double": return "double"
    if t1 == "float" or t2 == "float": return "float"
    if t1 == "long_long" or t2 == "long_long": return "long long"
    if t1 == "ulong_long" or t2 == "ulong_long": return "unsigned long long"
    if t1 == "long" or t2 == "long": return "long"
    if t1 == "ulong" or t2 == "ulong": return "unsigned long"
    if t1 == "int" or t2 == "int": return "int"
    if t1 == "uint" or t2 == "uint": return "unsigned int"

    return "long long" # Default promotion

def get_common_integral(t1, t2):
    # Treat bool as int
    if t1 == 'bool': t1 = 'int'
    if t2 == 'bool': t2 = 'int'
    if t1 == t2:
        return cpp_types[t1]
    
    if t1 == "ulong_long" and t2 == "ulong_long": return "unsigned long long"
    if t1 == "ulong" and t2 == "ulong": return "unsigned long"
    if t1 == "uint" and t2 == "uint": return "unsigned int"

    if t1 == "long_long" or t2 == "long_long": return "long long"
    if t1 == "ulong_long" or t2 == "ulong_long": return "long long"
    if t1 == "long" or t2 == "long": return "long"
    if t1 == "ulong" or t2 == "ulong": return "long"
    if t1 == "int" or t2 == "int": return "int"
    if t1 == "uint" or t2 == "uint": return "int"
    return "long long" # Default

def get_common_integral_bitwise(t1, t2):
    # Treat bool as int
    if t1 == 'bool': t1 = 'int'
    if t2 == 'bool': t2 = 'int'

    unsigned_types = ['uint', 'ulong', 'ulong_long']
    signed_types   = ['int', 'long', 'long_long']

    # If either operand is unsigned → use widest unsigned type
    if t1 in unsigned_types or t2 in unsigned_types:
        if 'ulong_long' in (t1, t2): return 'unsigned long long'
        if 'ulong'      in (t1, t2): return 'unsigned long'
        if 'uint'       in (t1, t2): return 'unsigned int'

    # Both operands signed → use widest signed type
    if 'long_long' in (t1, t2): return 'long long'
    if 'long'      in (t1, t2): return 'long'
    if 'int'       in (t1, t2): return 'int'

    # fallback
    return 'long long'


# Rank map for promotion min_rank calculation
rank_map = {
    'int':'pythonic::promotion::RANK_INT', 'uint':'pythonic::promotion::RANK_UINT',
    'long':'pythonic::promotion::RANK_LONG', 'ulong':'pythonic::promotion::RANK_ULONG',
    'long_long':'pythonic::promotion::RANK_LONG_LONG', 'ulong_long':'pythonic::promotion::RANK_ULONG_LONG',
    'float':'pythonic::promotion::RANK_FLOAT', 'double':'pythonic::promotion::RANK_DOUBLE', 'long_double':'pythonic::promotion::RANK_LONG_DOUBLE',
    'bool':'pythonic::promotion::RANK_INT'
}

# Helper to emit cast/load lines for generated stubs. When a source type is `bool`
# generate a `var_get<bool>()` load so the bool becomes 0/1 when cast to integral types.
def print_cast(operand, varname, t, ctype):
    if t == 'bool':
        print(f"    {ctype} {varname} = static_cast<{ctype}>({operand}.var_get<bool>());")
    else:
        print(f"    {ctype} {varname} = static_cast<{ctype}>({operand}.var_get<{cpp_types[t]}>());")

for opname in ops.values():
    print(f"\n// Stub definitions for {opname}")
    for left in type_tags:
        for right in type_tags:
            # Use double underscore to avoid ambiguity
            print(f"var {opname}__{left}__{right}(const var& a, const var& b, pythonic::overflow::Overflow policy, bool smallest_fit) {{")
            
            # Implementation Logic
            # Simple comparison operators: direct cast to common type for numeric and string
            if opname in ("eq", "ne", "gt", "ge", "lt", "le"):
                # string compare
                if left == "string" and right == "string":
                    if opname == "eq":
                        print(f"    return var(a.var_get<std::string>() == b.var_get<std::string>());")
                    elif opname == "ne":
                        print(f"    return var(a.var_get<std::string>() != b.var_get<std::string>());")
                    elif opname == "gt":
                        print(f"    return var(a.var_get<std::string>() > b.var_get<std::string>());")
                    elif opname == "ge":
                        print(f"    return var(a.var_get<std::string>() >= b.var_get<std::string>());")
                    elif opname == "lt":
                        print(f"    return var(a.var_get<std::string>() < b.var_get<std::string>());")
                    elif opname == "le":
                        print(f"    return var(a.var_get<std::string>() <= b.var_get<std::string>());")
                # container compare (lists, sets, orderedset, dicts, ordereddict)
                elif left == "list" and right == "list":
                    print(f"    const auto &lst1 = a.var_get<{qualified['list']}>();")
                    print(f"    const auto &lst2 = b.var_get<{qualified['list']}>();")
                    if opname == "eq":
                        print(f"    if (lst1.size() != lst2.size()) return var(false);")
                        print(f"    return var(std::equal(lst1.begin(), lst1.end(), lst2.begin(), lst2.end()));")
                    elif opname == "ne":
                        print(f"    if (lst1.size() != lst2.size()) return var(true);")
                        print(f"    return var(!std::equal(lst1.begin(), lst1.end(), lst2.begin(), lst2.end()));")
                    elif opname == "lt":
                        print(f"    return var(std::lexicographical_compare(lst1.begin(), lst1.end(), lst2.begin(), lst2.end()));")
                    elif opname == "le":
                        print(f"    return var(!std::lexicographical_compare(lst2.begin(), lst2.end(), lst1.begin(), lst1.end()));")
                    elif opname == "gt":
                        print(f"    return var(std::lexicographical_compare(lst2.begin(), lst2.end(), lst1.begin(), lst1.end()));")
                    elif opname == "ge":
                        print(f"    return var(!std::lexicographical_compare(lst1.begin(), lst1.end(), lst2.begin(), lst2.end()));")
                elif left == "set" and right == "set":
                    print(f"    const auto &set1 = a.var_get<{qualified['set']}>();")
                    print(f"    const auto &set2 = b.var_get<{qualified['set']}>();")
                    if opname == "eq":
                        print(f"    if (set1.size() != set2.size()) return var(false);")
                        print("    for (const auto &elem : set1) {")
                        print("        if (set2.find(elem) == set2.end()) return var(false);")
                        print("    }")
                        print(f"    return var(true);")
                    elif opname == "ne":
                        print(f"    if (set1.size() != set2.size()) return var(true);")
                        print("    for (const auto &elem : set1) {")
                        print("        if (set2.find(elem) == set2.end()) return var(true);")
                        print("    }")
                        print(f"    return var(false);")
                    elif opname == "lt":
                        print(f"    if (set1.size() >= set2.size()) return var(false);")
                        print("    for (const auto &elem : set1) {")
                        print("        if (set2.find(elem) == set2.end()) return var(false);")
                        print("    }")
                        print(f"    return var(true);")
                    elif opname == "le":
                        print("    for (const auto &elem : set1) {")
                        print("        if (set2.find(elem) == set2.end()) return var(false);")
                        print("    }")
                        print(f"    return var(true);")
                    elif opname == "gt":
                        print(f"    if (set2.size() >= set1.size()) return var(false);")
                        print("    for (const auto &elem : set2) {")
                        print("        if (set1.find(elem) == set1.end()) return var(false);")
                        print("    }")
                        print(f"    return var(true);")
                    elif opname == "ge":
                        print("    for (const auto &elem : set2) {")
                        print("        if (set1.find(elem) == set1.end()) return var(false);")
                        print("    }")
                        print(f"    return var(true);")
                elif left == "orderedset" and right == "orderedset":
                    print(f"    const auto &set1 = a.var_get<{qualified['orderedset']}>();")
                    print(f"    const auto &set2 = b.var_get<{qualified['orderedset']}>();")
                    if opname == "eq":
                        print(f"    return var(std::equal(set1.begin(), set1.end(), set2.begin(), set2.end()));")
                    elif opname == "ne":
                        print(f"    return var(!std::equal(set1.begin(), set1.end(), set2.begin(), set2.end()));")
                    elif opname == "lt":
                        print(f"    return var(std::lexicographical_compare(set1.begin(), set1.end(), set2.begin(), set2.end()));")
                    elif opname == "le":
                        print(f"    return var(!std::lexicographical_compare(set2.begin(), set2.end(), set1.begin(), set1.end()));")
                    elif opname == "gt":
                        print(f"    return var(std::lexicographical_compare(set2.begin(), set2.end(), set1.begin(), set1.end()));")
                    elif opname == "ge":
                        print(f"    return var(!std::lexicographical_compare(set1.begin(), set1.end(), set2.begin(), set2.end()));")
                elif left == "dict" and right == "dict":
                    if opname in ("eq", "ne"):
                        print(f"    const auto &dict1 = a.var_get<{qualified['dict']}>();")
                        print(f"    const auto &dict2 = b.var_get<{qualified['dict']}>();")
                        if opname == "eq":
                            print(f"    if (dict1.size() != dict2.size()) return var(false);")
                            print("    for (const auto &kv : dict1) {")
                            print("        const auto &key = kv.first; const auto &val = kv.second;")
                            print("        auto it = dict2.find(key);")
                            print("        if (it == dict2.end() || !static_cast<bool>(val == it->second)) return var(false);")
                            print("    }")
                            print(f"    return var(true);")
                        else:
                            print(f"    if (dict1.size() != dict2.size()) return var(true);")
                            print("    for (const auto &kv : dict1) {")
                            print("        const auto &key = kv.first; const auto &val = kv.second;")
                            print("        auto it = dict2.find(key);")
                            print("        if (it == dict2.end() || static_cast<bool>(val != it->second)) return var(true);")
                            print("    }")
                            print(f"    return var(false);")
                    else:
                        print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for {op_symbols[opname]}: 'dict' and 'dict'\");")
                elif left == "ordereddict" and right == "ordereddict":
                    print(f"    const auto &dict1 = a.var_get<{qualified['ordereddict']}>();")
                    print(f"    const auto &dict2 = b.var_get<{qualified['ordereddict']}>();")
                    if opname == "eq":
                        print(f"    return var(std::equal(dict1.begin(), dict1.end(), dict2.begin(), dict2.end()));")
                    elif opname == "ne":
                        print(f"    return var(!std::equal(dict1.begin(), dict1.end(), dict2.begin(), dict2.end()));")
                    elif opname == "lt":
                        print(f"    return var(std::lexicographical_compare(dict1.begin(), dict1.end(), dict2.begin(), dict2.end()));")
                    elif opname == "le":
                        print(f"    return var(!std::lexicographical_compare(dict2.begin(), dict2.end(), dict1.begin(), dict1.end()));")
                    elif opname == "gt":
                        print(f"    return var(std::lexicographical_compare(dict2.begin(), dict2.end(), dict1.begin(), dict1.end()));")
                    elif opname == "ge":
                        print(f"    return var(!std::lexicographical_compare(dict1.begin(), dict1.end(), dict2.begin(), dict2.end()));")
                # numeric compare
                elif is_numeric(left) and is_numeric(right):
                    ctype = get_common_type(left, right)
                    print_cast('a', 'la', left, ctype)
                    print_cast('b', 'lb', right, ctype)
                    if opname == "eq":
                        print(f"    return var(la == lb);")
                    elif opname == "ne":
                        print(f"    return var(la != lb);")
                    elif opname == "gt":
                        print(f"    return var(la > lb);")
                    elif opname == "ge":
                        print(f"    return var(la >= lb);")
                    elif opname == "lt":
                        print(f"    return var(la < lb);")
                    elif opname == "le":
                        print(f"    return var(la <= lb);")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for {op_symbols[opname]}: '{left}' and '{right}'\");")
                print("}")
                continue

            elif opname in ("band", "bor", "bxor"):
                if left == 'bool' and right == 'bool':
                    if opname == "band":
                        print(f"    return var(static_cast<bool>(a.var_get<bool>() & b.var_get<bool>()));")
                    elif opname == "bor":
                        print(f"    return var(static_cast<bool>(a.var_get<bool>() | b.var_get<bool>()));")
                    elif opname == "bxor":
                        print(f"    return var(static_cast<bool>(a.var_get<bool>() ^ b.var_get<bool>()));")
                elif is_integral(left) and is_integral(right):
                    ctype = get_common_integral_bitwise(left, right)
                    print_cast('a', 'la', left, ctype)
                    print_cast('b', 'lb', right, ctype)
                    if opname == "band":
                        print(f"    return var(la & lb);")
                    elif opname == "bor":
                        print(f"    return var(la | lb);")
                    elif opname == "bxor":
                        print(f"    return var(la ^ lb);")
                elif left == right:
                    if left == "set":
                        print(f"    const auto &lhs = a.var_get<{qualified['set']}>();")
                        print(f"    const auto &rhs = b.var_get<{qualified['set']}>();")
                        if opname == "band":
                            print(f"    {qualified['set']} result;")
                            print("    for (const auto &item : lhs) {")
                            print("        if (rhs.find(item) != rhs.end()) {")
                            print("            result.insert(item);")
                            print("        }")
                            print("    }")
                            print("    return var(std::move(result));")
                        elif opname == "bor":
                            print(f"    {qualified['set']} result = lhs;")
                            print("    result.insert(rhs.begin(), rhs.end());")
                            print("    return var(std::move(result));")
                        elif opname == "bxor":
                            print(f"    {qualified['set']} result;")
                            print("    for (const auto &item : lhs) {")
                            print("        if (rhs.find(item) == rhs.end()) {")
                            print("            result.insert(item);")
                            print("        }")
                            print("    }")
                            print("    for (const auto &item : rhs) {")
                            print("        if (lhs.find(item) == lhs.end()) {")
                            print("            result.insert(item);")
                            print("        }")
                            print("    }")
                            print("    return var(std::move(result));")
                    elif left == "orderedset":
                        print(f"    const auto &lhs = a.var_get<{qualified['orderedset']}>();")
                        print(f"    const auto &rhs = b.var_get<{qualified['orderedset']}>();")
                        if opname == "band":
                            print(f"    {qualified['orderedset']} result;")
                            print("    auto it_lhs = lhs.begin();")
                            print("    auto it_rhs = rhs.begin();")
                            print("    while (it_lhs != lhs.end() && it_rhs != rhs.end()) {")
                            print("        if (*it_lhs < *it_rhs) {")
                            print("            ++it_lhs;")
                            print("        } else if (*it_rhs < *it_lhs) {")
                            print("            ++it_rhs;")
                            print("        } else {")
                            print("            result.insert(*it_lhs);")
                            print("            ++it_lhs;")
                            print("            ++it_rhs;")
                            print("        }")
                            print("    }")
                            print("    return var(std::move(result));")
                        elif opname == "bor":
                            print(f"    {qualified['orderedset']} result;")
                            print("    auto it_lhs = lhs.begin();")
                            print("    auto it_rhs = rhs.begin();")
                            print("    while (it_lhs != lhs.end() && it_rhs != rhs.end()) {")
                            print("        if (*it_lhs < *it_rhs) {")
                            print("            result.insert(*it_lhs);")
                            print("            ++it_lhs;")
                            print("        } else if (*it_rhs < *it_lhs) {")
                            print("            result.insert(*it_rhs);")
                            print("            ++it_rhs;")
                            print("        } else {")
                            print("            result.insert(*it_lhs);")
                            print("            ++it_lhs;")
                            print("            ++it_rhs;")
                            print("        }")
                            print("    }")
                            print("    while (it_lhs != lhs.end()) {")
                            print("        result.insert(*it_lhs);")
                            print("        ++it_lhs;")
                            print("    }")
                            print("    while (it_rhs != rhs.end()) {")
                            print("        result.insert(*it_rhs);")
                            print("        ++it_rhs;")
                            print("    }")
                            print("    return var(std::move(result));")
                        elif opname == "bxor":
                            print(f"    {qualified['orderedset']} result;")
                            print("    auto it_lhs = lhs.begin();")
                            print("    auto it_rhs = rhs.begin();")
                            print("    while (it_lhs != lhs.end() && it_rhs != rhs.end()) {")
                            print("        if (*it_lhs < *it_rhs) {")
                            print("            result.insert(*it_lhs);")
                            print("            ++it_lhs;")
                            print("        } else if (*it_rhs < *it_lhs) {")
                            print("            result.insert(*it_rhs);")
                            print("            ++it_rhs;")
                            print("        } else {")
                            print("            ++it_lhs;")
                            print("            ++it_rhs;")
                            print("        }")
                            print("    }")
                            print("    while (it_lhs != lhs.end()) {")
                            print("        result.insert(*it_lhs);")
                            print("        ++it_lhs;")
                            print("    }")
                            print("    while (it_rhs != rhs.end()) {")
                            print("        result.insert(*it_rhs);")
                            print("        ++it_rhs;")
                            print("    }")
                            print("    return var(std::move(result));")
                    elif left == "list":
                        print(f"    const auto &lhs = a.var_get<{qualified['list']}>();")
                        print(f"    const auto &rhs = b.var_get<{qualified['list']}>();")
                        if opname == "band":
                            print(f"    {qualified['list']} result;")
                            print(f"    {qualified['set']} rhs_set(rhs.begin(), rhs.end());")
                            print("    for (const auto &item : lhs) {")
                            print("        if (rhs_set.find(item) != rhs_set.end()) {")
                            print("            result.push_back(item);")
                            print("        }")
                            print("    }")
                            print("    return var(std::move(result));")
                        elif opname == "bor":
                            print(f"    {qualified['list']} result;")
                            print("    result.reserve(lhs.size() + rhs.size());")
                            print("    result.insert(result.end(), lhs.begin(), lhs.end());")
                            print("    result.insert(result.end(), rhs.begin(), rhs.end());")
                            print("    return var(std::move(result));")
                        elif opname == "bxor":
                            print(f"    {qualified['list']} result;")
                            print(f"    {qualified['set']} lhs_set(lhs.begin(), lhs.end());")
                            print(f"    {qualified['set']} rhs_set(rhs.begin(), rhs.end());")
                            print("    for (const auto &item : lhs) {")
                            print("        if (rhs_set.find(item) == rhs_set.end()) {")
                            print("            result.push_back(item);")
                            print("        }")
                            print("    }")
                            print("    for (const auto &item : rhs) {")
                            print("        if (lhs_set.find(item) == lhs_set.end()) {")
                            print("            result.push_back(item);")
                            print("        }")
                            print("    }")
                            print("    return var(std::move(result));")
                    elif left == "dict":
                        if opname in ("band", "bor"):
                            print(f"    const auto &lhs = a.var_get<{qualified['dict']}>();")
                            print(f"    const auto &rhs = b.var_get<{qualified['dict']}>();")
                            if opname == "band":
                                print(f"    {qualified['dict']} result;")
                                print("    for (const auto &[key, val] : lhs) {")
                                print("        if (rhs.find(key) != rhs.end()) {")
                                print("            result[key] = val;")
                                print("        }")
                                print("    }")
                                print("    return var(std::move(result));")
                            elif opname == "bor":
                                print(f"    {qualified['dict']} result = lhs;")
                                print("    for (const auto &[key, val] : rhs) {")
                                print("        result[key] = val;")
                                print("    }")
                                print("    return var(std::move(result));")
                        else:
                            print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for {op_symbols[opname]}: '{left}' and '{right}'\");")
                    elif left == "ordereddict":
                        if opname in ("band", "bor"):
                            print(f"    const auto &lhs = a.var_get<{qualified['ordereddict']}>();")
                            print(f"    const auto &rhs = b.var_get<{qualified['ordereddict']}>();")
                            if opname == "band":
                                print(f"    {qualified['ordereddict']} result;")
                                print("    for (const auto &[key, val] : lhs) {")
                                print("        if (rhs.find(key) != rhs.end()) {")
                                print("            result[key] = val;")
                                print("        }")
                                print("    }")
                                print("    return var(std::move(result));")
                            elif opname == "bor":
                                print(f"    {qualified['ordereddict']} result = lhs;")
                                print("    for (const auto &[key, val] : rhs) {")
                                print("        result[key] = val;")
                                print("    }")
                                print("    return var(std::move(result));")
                        else:
                            print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for {op_symbols[opname]}: '{left}' and '{right}'\");")
                    else:
                        print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for {op_symbols[opname]}: '{left}' and '{right}'\");")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for {op_symbols[opname]}: '{left}' and '{right}'\");")
                print("}")
                continue

            elif opname in ("shl", "shr"):
                if is_integral(left) and is_integral(right):
                    ctype = get_common_integral_bitwise(left, right)
                    print_cast('a', 'la', left, ctype)
                    print_cast('b', 'lb', right, ctype)
                    if opname == "shl":
                        print(f"    return var(la << lb);")
                    elif opname == "shr":
                        print(f"    return var(la >> lb);")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for {op_symbols[opname]}: '{left}' and '{right}'\");")
                print("}")
                continue

            if opname == "add":
                if left == "string" and right == "string":
                    print(f"    return var(a.var_get<std::string>() + b.var_get<std::string>());")
                elif left == "string" and right == "bool":
                    print(f"    return var(a.var_get<std::string>() + (b.var_get<bool>() ? std::string(\"true\") : std::string(\"false\")));")
                elif left == "bool" and right == "string":
                    print(f"    return var((a.var_get<bool>() ? std::string(\"true\") : std::string(\"false\")) + b.var_get<std::string>());")
                elif left == "list" and right == "list":
                    print(f"    const auto& al = a.var_get<{qualified['list']}>();")
                    print(f"    const auto& bl = b.var_get<{qualified['list']}>();")
                    print(f"    {qualified['list']} res; res.reserve(al.size() + bl.size());")
                    print("    res.insert(res.end(), al.begin(), al.end());")
                    print("    res.insert(res.end(), bl.begin(), bl.end());")
                    print("    return var(std::move(res));")
                elif is_numeric(left) and is_numeric(right):
                    ctype = get_common_type(left, right)
                    # Numeric: handle by policy
                    print(f"    // Numeric add with policy-aware handling")
                    print_cast('a', 'la', left, ctype)
                    print_cast('b', 'lb', right, ctype)
                    print(f"    if (policy == pythonic::overflow::Overflow::None_of_them) {{")
                    print(f"        return var(la + lb);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Throw) {{")
                    print(f"        auto res = pythonic::overflow::add_throw(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Wrap) {{")
                    print(f"        auto res = pythonic::overflow::add_wrap(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else {{")
                    print(f"        // Promote: compute in long double then smart-promote")
                    print(f"        long double result = static_cast<long double>(la) + static_cast<long double>(lb);")
                    # determine type enum
                    is_float = left in ["float", "double", "long_double"] or right in ["float", "double", "long_double"]
                    both_unsigned = left in ["uint","ulong","ulong_long"] and right in ["uint","ulong","ulong_long"]
                    if is_float:
                        print(f"        auto ptype = pythonic::promotion::Has_float;")
                    elif both_unsigned:
                        print(f"        auto ptype = pythonic::promotion::Both_unsigned;")
                    else:
                        print(f"        auto ptype = pythonic::promotion::Signed;")
                    # min_rank as max of input ranks
                    rank_map = {
                        'int':'pythonic::promotion::RANK_INT', 'uint':'pythonic::promotion::RANK_UINT',
                        'long':'pythonic::promotion::RANK_LONG', 'ulong':'pythonic::promotion::RANK_ULONG',
                        'long_long':'pythonic::promotion::RANK_LONG_LONG', 'ulong_long':'pythonic::promotion::RANK_ULONG_LONG',
                        'float':'pythonic::promotion::RANK_FLOAT', 'double':'pythonic::promotion::RANK_DOUBLE', 'long_double':'pythonic::promotion::RANK_LONG_DOUBLE'
                    }
                    left_rank = rank_map.get(left, '0')
                    right_rank = rank_map.get(right, '0')
                    print(f"        int min_rank = std::max({left_rank}, {right_rank});")
                    print(f"        return pythonic::promotion::smart_promote(result, ptype, smallest_fit, min_rank, false);")
                    print("    }")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for +: '{left}' and '{right}'\");")
            
            elif opname == "sub":
                if is_numeric(left) and is_numeric(right):
                    ctype = get_common_type(left, right)
                    print(f"    // Numeric sub with policy-aware handling")
                    print_cast('a', 'la', left, ctype)
                    print_cast('b', 'lb', right, ctype)
                    print(f"    if (policy == pythonic::overflow::Overflow::None_of_them) {{")
                    print(f"        return var(la - lb);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Throw) {{")
                    print(f"        auto res = pythonic::overflow::sub_throw(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Wrap) {{")
                    print(f"        auto res = pythonic::overflow::sub_wrap(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else {{")
                    print(f"        long double result = static_cast<long double>(la) - static_cast<long double>(lb);")
                    is_float = left in ["float", "double", "long_double"] or right in ["float", "double", "long_double"]
                    both_unsigned = left in ["uint","ulong","ulong_long"] and right in ["uint","ulong","ulong_long"]
                    if is_float:
                        print(f"        auto ptype = pythonic::promotion::Has_float;")
                    elif both_unsigned:
                        print(f"        auto ptype = pythonic::promotion::Both_unsigned;")
                    else:
                        print(f"        auto ptype = pythonic::promotion::Signed;")
                    left_rank = rank_map.get(left, '0')
                    right_rank = rank_map.get(right, '0')
                    print(f"        int min_rank = std::max({left_rank}, {right_rank});")
                    print(f"        return pythonic::promotion::smart_promote(result, ptype, smallest_fit, min_rank, true);")
                    print("    }")
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
                    print(f"    // Numeric mul with policy-aware handling")
                    print_cast('a', 'la', left, ctype)
                    print_cast('b', 'lb', right, ctype)
                    print(f"    if (policy == pythonic::overflow::Overflow::None_of_them) {{")
                    print(f"        return var(la * lb);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Throw) {{")
                    print(f"        auto res = pythonic::overflow::mul_throw(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Wrap) {{")
                    print(f"        auto res = pythonic::overflow::mul_wrap(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else {{")
                    print(f"        long double result = static_cast<long double>(la) * static_cast<long double>(lb);")
                    is_float = left in ["float", "double", "long_double"] or right in ["float", "double", "long_double"]
                    both_unsigned = left in ["uint","ulong","ulong_long"] and right in ["uint","ulong","ulong_long"]
                    if is_float:
                        print(f"        auto ptype = pythonic::promotion::Has_float;")
                    elif both_unsigned:
                        print(f"        auto ptype = pythonic::promotion::Both_unsigned;")
                    else:
                        print(f"        auto ptype = pythonic::promotion::Signed;")
                    left_rank = rank_map.get(left, '0')
                    right_rank = rank_map.get(right, '0')
                    print(f"        int min_rank = std::max({left_rank}, {right_rank});")
                    print(f"        return pythonic::promotion::smart_promote(result, ptype, smallest_fit, min_rank, false);")
                    print("    }")
                else:
                     print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for *: '{left}' and '{right}'\");")

            elif opname == "div":
                if is_numeric(left) and is_numeric(right):
                    ctype = get_common_type(left, right)
                    print(f"    // Numeric div with policy-aware handling")
                    print_cast('a', 'la', left, ctype)
                    print_cast('b', 'lb', right, ctype)

                    print(f"    if (static_cast<long double>(lb) == 0.0L) throw pythonic::PythonicZeroDivisionError(\"float division by zero\");")
                    print(f"    if (policy == pythonic::overflow::Overflow::None_of_them) {{")
                    print(f"        return var(la / lb);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Throw) {{")
                    print(f"        auto res = pythonic::overflow::div_throw(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else if (policy == pythonic::overflow::Overflow::Wrap) {{")
                    print(f"        auto res = pythonic::overflow::div_wrap(la, lb);")
                    print(f"        return var(res);")
                    print(f"    }} else {{")
                    print(f"        long double result = static_cast<long double>(la) / static_cast<long double>(lb);")
                    is_float = left in ["float", "double", "long_double"] or right in ["float", "double", "long_double"]
                    both_unsigned = left in ["uint","ulong","ulong_long"] and right in ["uint","ulong","ulong_long"]
                    
                    print(f"        auto ptype = pythonic::promotion::Has_float;")
                    left_rank = rank_map.get(left, '0')
                    right_rank = rank_map.get(right, '0')
                    print(f"        int min_rank = std::max({left_rank}, {right_rank});")
                    print(f"        return pythonic::promotion::smart_promote(result, ptype, smallest_fit, min_rank, false);")
                    print("    }")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for /: '{left}' and '{right}'\");")

            elif opname == "mod":
                if is_numeric(left) and is_numeric(right):
                    # If both are integer-like, use integral modulo helpers
                    int_types = ["int", "long", "long_long", "uint", "ulong", "ulong_long", "bool"]
                    float_types = ["float", "double", "long_double"]
                    if left in int_types and right in int_types:
                        ctype = get_common_type(left, right)
                        print_cast('a', 'la', left, ctype)
                        print_cast('b', 'lb', right, ctype)
                        print(f"    if (lb == 0) throw pythonic::PythonicZeroDivisionError(\"integer division or modulo by zero\");")
                        print(f"    if (policy == pythonic::overflow::Overflow::None_of_them) {{")
                        print(f"        return var(la % lb);")
                        print(f"    }} else if (policy == pythonic::overflow::Overflow::Throw) {{")
                        print(f"        auto res = pythonic::overflow::mod_throw(la, lb);")
                        print(f"        return var(res);")
                        print(f"    }} else if (policy == pythonic::overflow::Overflow::Wrap) {{")
                        print(f"        auto res = pythonic::overflow::mod_wrap(la, lb);")
                        print(f"        return var(res);")
                        print(f"    }} else {{")
                        print(f"        long double result = std::fmod(static_cast<long double>(la), static_cast<long double>(lb));")
                        is_float = left in float_types or right in float_types
                        both_unsigned = left in ["uint","ulong","ulong_long"] and right in ["uint","ulong","ulong_long"]
                        if is_float:
                            print(f"        auto ptype = pythonic::promotion::Has_float;")
                        elif both_unsigned:
                            print(f"        auto ptype = pythonic::promotion::Both_unsigned;")
                        else:
                            print(f"        auto ptype = pythonic::promotion::Signed;")
                        left_rank = rank_map.get(left, '0')
                        right_rank = rank_map.get(right, '0')
                        print(f"        int min_rank = std::max({left_rank}, {right_rank});")
                        print(f"        return pythonic::promotion::smart_promote(result, ptype, smallest_fit, min_rank, false);")
                        print("    }")
                    # If either operand is floating, perform floating modulo (fmod)
                    elif left in float_types or right in float_types:
                        # choose compute type based on presence of long_double/double/float
                        compute = None
                        if left == 'long_double' or right == 'long_double':
                            compute = 'long double'
                        elif left == 'double' or right == 'double':
                            compute = 'double'
                        else:
                            compute = 'float'
                        # Use print_cast here too; it will call var_get<bool>() correctly if needed.
                        # For float compute types we still want to cast from the original stored type.
                        if left == 'bool':
                            print(f"    {compute} la = static_cast<{compute}>(a.var_get<bool>());")
                        else:
                            print(f"    {compute} la = static_cast<{compute}>({{}}.var_get<{cpp_types[left]}>());".format('a'))
                        if right == 'bool':
                            print(f"    {compute} lb = static_cast<{compute}>(b.var_get<bool>());")
                        else:
                            print(f"    {compute} lb = static_cast<{compute}>({{}}.var_get<{cpp_types[right]}>());".format('b'))
                        print(f"    if (static_cast<long double>(lb) == 0.0L) throw pythonic::PythonicZeroDivisionError(\"float division by zero\");")
                        print(f"    if (policy == pythonic::overflow::Overflow::None_of_them || policy == pythonic::overflow::Overflow::Throw || policy == pythonic::overflow::Overflow::Wrap) {{")
                        print(f"        {compute} res = std::fmod(la, lb);")
                        print(f"        return var(res);")
                        print(f"    }} else {{")
                        print(f"        long double result = std::fmod(static_cast<long double>(la), static_cast<long double>(lb));")
                        is_float = True
                        both_unsigned = False
                        print(f"        auto ptype = pythonic::promotion::Has_float;")
                        left_rank = rank_map.get(left, '0')
                        right_rank = rank_map.get(right, '0')
                        print(f"        int min_rank = std::max({left_rank}, {right_rank});")
                        print(f"        return pythonic::promotion::smart_promote(result, ptype, smallest_fit, min_rank, false);")
                        print("    }")
                    else:
                        print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for %: '{left}' and '{right}'\");")
                else:
                    print(f"    throw pythonic::PythonicTypeError(\"TypeError: unsupported operand type(s) for %: '{left}' and '{right}'\");")

            else:
                print(f"    throw std::runtime_error(\"Not implemented: {opname} for {left} and {right}\");")
            
            print("}")

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
