#pragma once
// scriptit_methods.hpp — Dtype-dependent dot-method dispatch for ScriptIt v2
// Architecture: map of dtype → supported methods, dispatched by arg count.

#include "scriptit_types.hpp"

// ─── Method Signature Types ──────────────────────────────

using Method0 = std::function<var(var &)>;
using Method1 = std::function<var(var &, const var &)>;
using Method2 = std::function<var(var &, const var &, const var &)>;
using Method3 = std::function<var(var &, const var &, const var &, const var &)>;

struct MethodTable
{
    std::unordered_map<std::string, Method0> m0; // zero-arg methods
    std::unordered_map<std::string, Method1> m1; // one-arg methods
    std::unordered_map<std::string, Method2> m2; // two-arg methods
    std::unordered_map<std::string, Method3> m3; // three-arg methods
};

// ─── Universal Methods (all dtypes) ─────────────────────

inline MethodTable make_universal_methods()
{
    MethodTable t;

    // Type introspection — zero arg
    t.m0["type"] = [](var &s) -> var
    { return var(s.type()); };
    t.m0["str"] = [](var &s) -> var
    { return var(s.str()); };
    t.m0["pretty_str"] = [](var &s) -> var
    { return var(s.pretty_str()); };
    t.m0["len"] = [](var &s) -> var
    { return s.len(); };
    t.m0["hash"] = [](var &s) -> var
    { return var((long long)s.hash()); };

    // Type checks
    t.m0["is_none"] = [](var &s) -> var
    { return var(s.is_none() ? 1 : 0); };
    t.m0["is_bool"] = [](var &s) -> var
    { return var(s.is_bool() ? 1 : 0); };
    t.m0["is_int"] = [](var &s) -> var
    { return var(s.is_int() ? 1 : 0); };
    t.m0["is_uint"] = [](var &s) -> var
    { return var(s.is_uint() ? 1 : 0); };
    t.m0["is_long"] = [](var &s) -> var
    { return var(s.is_long() ? 1 : 0); };
    t.m0["is_ulong"] = [](var &s) -> var
    { return var(s.is_ulong() ? 1 : 0); };
    t.m0["is_long_long"] = [](var &s) -> var
    { return var(s.is_long_long() ? 1 : 0); };
    t.m0["is_ulong_long"] = [](var &s) -> var
    { return var(s.is_ulong_long() ? 1 : 0); };
    t.m0["is_float"] = [](var &s) -> var
    { return var(s.is_float() ? 1 : 0); };
    t.m0["is_double"] = [](var &s) -> var
    { return var(s.is_double() ? 1 : 0); };
    t.m0["is_long_double"] = [](var &s) -> var
    { return var(s.is_long_double() ? 1 : 0); };
    t.m0["is_string"] = [](var &s) -> var
    { return var(s.is_string() ? 1 : 0); };
    t.m0["is_list"] = [](var &s) -> var
    { return var(s.is_list() ? 1 : 0); };
    t.m0["is_dict"] = [](var &s) -> var
    { return var(s.is_dict() ? 1 : 0); };
    t.m0["is_set"] = [](var &s) -> var
    { return var(s.is_set() ? 1 : 0); };
    t.m0["is_ordered_set"] = [](var &s) -> var
    { return var(s.is_ordered_set() ? 1 : 0); };
    t.m0["is_ordered_dict"] = [](var &s) -> var
    { return var(s.is_ordered_dict() ? 1 : 0); };
    t.m0["is_graph"] = [](var &s) -> var
    { return var(s.is_graph() ? 1 : 0); };
    t.m0["is_any_integral"] = [](var &s) -> var
    { return var(s.is_any_integral() ? 1 : 0); };
    t.m0["is_any_floating"] = [](var &s) -> var
    { return var(s.is_any_floating() ? 1 : 0); };
    t.m0["is_any_numeric"] = [](var &s) -> var
    { return var(s.is_any_numeric() ? 1 : 0); };
    t.m0["isNone"] = [](var &s) -> var
    { return var(s.isNone() ? 1 : 0); };
    t.m0["isNumeric"] = [](var &s) -> var
    { return var(s.isNumeric() ? 1 : 0); };
    t.m0["isIntegral"] = [](var &s) -> var
    { return var(s.isIntegral() ? 1 : 0); };

    // Conversion — works on numeric+string
    t.m0["toInt"] = [](var &s) -> var
    { return var(s.toInt()); };
    t.m0["toDouble"] = [](var &s) -> var
    { return var(s.toDouble()); };
    t.m0["toFloat"] = [](var &s) -> var
    { return var(s.toFloat()); };
    t.m0["toLong"] = [](var &s) -> var
    { return var(s.toLong()); };
    t.m0["toLongLong"] = [](var &s) -> var
    { return var(s.toLongLong()); };
    t.m0["toLongDouble"] = [](var &s) -> var
    { return var(s.toLongDouble()); };
    t.m0["toBool"] = [](var &s) -> var
    { return var(static_cast<bool>(s) ? 1 : 0); };
    t.m0["toString"] = [](var &s) -> var
    { return var(s.toString()); };

    return t;
}

// ─── String Methods ─────────────────────────────────────

inline MethodTable make_string_methods()
{
    MethodTable t;

    // 0-arg
    t.m0["upper"] = [](var &s) -> var
    { return s.upper(); };
    t.m0["lower"] = [](var &s) -> var
    { return s.lower(); };
    t.m0["strip"] = [](var &s) -> var
    { return s.strip(); };
    t.m0["lstrip"] = [](var &s) -> var
    { return s.lstrip(); };
    t.m0["rstrip"] = [](var &s) -> var
    { return s.rstrip(); };
    t.m0["capitalize"] = [](var &s) -> var
    { return s.capitalize(); };
    t.m0["sentence_case"] = [](var &s) -> var
    { return s.sentence_case(); };
    t.m0["title"] = [](var &s) -> var
    { return s.title(); };
    t.m0["reverse"] = [](var &s) -> var
    { return s.reverse(); };
    t.m0["isdigit"] = [](var &s) -> var
    { return var(s.isdigit() ? 1 : 0); };
    t.m0["isalpha"] = [](var &s) -> var
    { return var(s.isalpha() ? 1 : 0); };
    t.m0["isalnum"] = [](var &s) -> var
    { return var(s.isalnum() ? 1 : 0); };
    t.m0["isspace"] = [](var &s) -> var
    { return var(s.isspace() ? 1 : 0); };
    t.m0["empty"] = [](var &s) -> var
    { return var(s.empty() ? 1 : 0); };
    t.m0["size"] = [](var &s) -> var
    { return s.len(); };
    // split() with no args → split on whitespace
    t.m0["split"] = [](var &s) -> var
    { return s.split(var(" ")); };

    // 1-arg
    t.m1["find"] = [](var &s, const var &a) -> var
    { return s.find(a); };
    t.m1["count"] = [](var &s, const var &a) -> var
    { return s.count(a); };
    t.m1["startswith"] = [](var &s, const var &a) -> var
    { return s.startswith(a); };
    t.m1["endswith"] = [](var &s, const var &a) -> var
    { return s.endswith(a); };
    t.m1["contains"] = [](var &s, const var &a) -> var
    { return s.contains(a); };
    t.m1["has"] = [](var &s, const var &a) -> var
    { return s.has(a); };
    t.m1["split"] = [](var &s, const var &a) -> var
    { return s.split(a); };
    t.m1["join"] = [](var &s, const var &a) -> var
    { return s.join(a); };
    t.m1["zfill"] = [](var &s, const var &a) -> var
    { return s.zfill(a.toInt()); };
    t.m1["at"] = [](var &s, const var &a) -> var
    { return s.at(a.toInt()); };

    // 2-arg
    t.m2["replace"] = [](var &s, const var &a, const var &b) -> var
    { return s.replace(a, b); };
    t.m2["center"] = [](var &s, const var &a, const var &b) -> var
    { return s.center(a.toInt(), b); };
    t.m2["slice"] = [](var &s, const var &a, const var &b) -> var
    { return s.slice(a, b); };

    // 3-arg
    t.m3["slice"] = [](var &s, const var &a, const var &b, const var &c) -> var
    { return s.slice(a, b, c); };

    return t;
}

// ─── List Methods ───────────────────────────────────────

inline MethodTable make_list_methods()
{
    MethodTable t;

    // 0-arg
    t.m0["front"] = [](var &s) -> var
    { return s.front(); };
    t.m0["back"] = [](var &s) -> var
    { return s.back(); };
    t.m0["pop"] = [](var &s) -> var
    { return s.pop(); };
    t.m0["clear"] = [](var &s) -> var
    { s.clear(); return var(NoneType{}); };
    t.m0["empty"] = [](var &s) -> var
    { return var(s.empty() ? 1 : 0); };
    t.m0["size"] = [](var &s) -> var
    { return s.len(); };
    t.m0["sort"] = [](var &s) -> var
    {
        auto &lst = s.var_get<List>();
        std::sort(lst.begin(), lst.end(), [](const var &a, const var &b)
                  { return a < b; });
        return s;
    };
    t.m0["reverse"] = [](var &s) -> var
    { return s.reverse(); };
    t.m0["keys"] = [](var &s) -> var
    {
        // List "keys" = indices [0, 1, 2, ...]
        List indices;
        int sz = s.len().toInt();
        for (int i = 0; i < sz; ++i)
            indices.push_back(var(i));
        return var(std::move(indices));
    };

    // 1-arg
    t.m1["append"] = [](var &s, const var &a) -> var
    { s.append(a); return s; };
    t.m1["extend"] = [](var &s, const var &a) -> var
    { s.extend(a); return s; };
    t.m1["remove"] = [](var &s, const var &a) -> var
    { s.remove(a); return s; };
    t.m1["contains"] = [](var &s, const var &a) -> var
    { return s.contains(a); };
    t.m1["has"] = [](var &s, const var &a) -> var
    { return s.has(a); };
    t.m1["count"] = [](var &s, const var &a) -> var
    { return s.count(a); };
    t.m1["index"] = [](var &s, const var &a) -> var
    {
        // Manual index search — find first occurrence
        auto &lst = s.var_get<List>();
        for (size_t i = 0; i < lst.size(); ++i)
            if (lst[i] == a)
                return var((int)i);
        return var(-1);
    };
    t.m1["at"] = [](var &s, const var &a) -> var
    { return s.at(a.toInt()); };

    // 2-arg
    t.m2["slice"] = [](var &s, const var &a, const var &b) -> var
    { return s.slice(a, b); };
    t.m2["insert"] = [](var &s, const var &a, const var &b) -> var
    {
        // insert(index, value)
        auto &lst = s.var_get<List>();
        int sz = (int)lst.size();
        int idx = a.toInt();
        if (idx < 0)
            idx += sz;
        if (idx < 0)
            idx = 0;
        if (idx > sz)
            idx = sz;
        lst.insert(lst.begin() + idx, b);
        return s;
    };

    // 3-arg
    t.m3["slice"] = [](var &s, const var &a, const var &b, const var &c) -> var
    { return s.slice(a, b, c); };

    return t;
}

// ─── Set Methods ────────────────────────────────────────

inline MethodTable make_set_methods()
{
    MethodTable t;

    // 0-arg
    t.m0["clear"] = [](var &s) -> var
    { s.clear(); return var(NoneType{}); };
    t.m0["empty"] = [](var &s) -> var
    { return var(s.empty() ? 1 : 0); };
    t.m0["size"] = [](var &s) -> var
    { return s.len(); };

    // 1-arg
    t.m1["add"] = [](var &s, const var &a) -> var
    { s.add(a); return s; };
    t.m1["remove"] = [](var &s, const var &a) -> var
    { s.remove(a); return s; };
    t.m1["contains"] = [](var &s, const var &a) -> var
    { return s.contains(a); };
    t.m1["has"] = [](var &s, const var &a) -> var
    { return s.has(a); };
    t.m1["extend"] = [](var &s, const var &a) -> var
    { s.extend(a); return s; };
    t.m1["update"] = [](var &s, const var &a) -> var
    { s.update(a); return s; };

    return t;
}

// ─── Dict Methods ───────────────────────────────────────

inline MethodTable make_dict_methods()
{
    MethodTable t;

    // 0-arg
    t.m0["keys"] = [](var &s) -> var
    { return s.keys(); };
    t.m0["values"] = [](var &s) -> var
    { return s.values(); };
    t.m0["items"] = [](var &s) -> var
    { return s.items(); };
    t.m0["clear"] = [](var &s) -> var
    { s.clear(); return var(NoneType{}); };
    t.m0["empty"] = [](var &s) -> var
    { return var(s.empty() ? 1 : 0); };
    t.m0["size"] = [](var &s) -> var
    { return s.len(); };

    // 1-arg
    t.m1["contains"] = [](var &s, const var &a) -> var
    { return s.contains(a); };
    t.m1["has"] = [](var &s, const var &a) -> var
    { return s.has(a); };
    t.m1["update"] = [](var &s, const var &a) -> var
    { s.update(a); return s; };
    t.m1["get"] = [](var &s, const var &a) -> var
    {
        // dict.get(key) → value or None
        if (s.contains(a))
            return s[a];
        return var(NoneType{});
    };

    // 2-arg
    t.m2["get"] = [](var &s, const var &a, const var &b) -> var
    {
        // dict.get(key, default) → value or default
        if (s.contains(a))
            return s[a];
        return b;
    };

    return t;
}

// ─── Numeric Methods (int, float, double, long, etc.) ──

inline MethodTable make_numeric_methods()
{
    MethodTable t;
    // Numeric types mostly use universal methods (type checks, conversions)
    // But we can add numeric-specific helpers here
    return t;
}

// ─── Dispatch Entry Point ───────────────────────────────

// Holds all dtype method tables. Initialized once (lazy singleton).
struct MethodDispatch
{
    MethodTable universal;
    MethodTable string_m;
    MethodTable list_m;
    MethodTable set_m;
    MethodTable dict_m;
    MethodTable numeric_m;

    static const MethodDispatch &instance()
    {
        static MethodDispatch inst = []()
        {
            MethodDispatch d;
            d.universal = make_universal_methods();
            d.string_m = make_string_methods();
            d.list_m = make_list_methods();
            d.set_m = make_set_methods();
            d.dict_m = make_dict_methods();
            d.numeric_m = make_numeric_methods();
            return d;
        }();
        return inst;
    }

    // Get the dtype-specific table for a var
    const MethodTable *dtype_table(const var &v) const
    {
        if (v.is_string())
            return &string_m;
        if (v.is_list())
            return &list_m;
        if (v.is_set() || v.is_ordered_set())
            return &set_m;
        if (v.is_dict() || v.is_ordered_dict())
            return &dict_m;
        if (v.is_any_numeric() || v.is_bool())
            return &numeric_m;
        return nullptr; // none, graph, etc.
    }
};

// ─── Top-level dispatch function ────────────────────────

// Dispatches obj.method(args...) → result
// Tries: 1) dtype-specific table  2) universal table
// Handles overloaded arity (e.g. split with 0 or 1 arg)
inline var dispatch_method(var &self, const std::string &method, const std::vector<var> &args)
{
    const auto &md = MethodDispatch::instance();
    int argc = (int)args.size();

    // Helper: try to find method in a table at given arity
    auto try_table = [&](const MethodTable &t) -> std::pair<bool, var>
    {
        if (argc == 0)
        {
            auto it = t.m0.find(method);
            if (it != t.m0.end())
                return {true, it->second(self)};
        }
        else if (argc == 1)
        {
            auto it = t.m1.find(method);
            if (it != t.m1.end())
                return {true, it->second(self, args[0])};
            // Fallback: if called with 1 arg but only 0-arg exists, error
        }
        else if (argc == 2)
        {
            auto it = t.m2.find(method);
            if (it != t.m2.end())
                return {true, it->second(self, args[0], args[1])};
        }
        else if (argc == 3)
        {
            auto it = t.m3.find(method);
            if (it != t.m3.end())
                return {true, it->second(self, args[0], args[1], args[2])};
        }
        return {false, var(NoneType{})};
    };

    // 1) Try dtype-specific table
    const MethodTable *dtable = md.dtype_table(self);
    if (dtable)
    {
        auto [found, result] = try_table(*dtable);
        if (found)
            return result;
    }

    // 2) Try universal table
    {
        auto [found, result] = try_table(md.universal);
        if (found)
            return result;
    }

    // 3) Check if method exists at a different arity (better error message)
    auto method_exists_somewhere = [&](const MethodTable &t, const std::string &m) -> bool
    {
        return t.m0.count(m) || t.m1.count(m) || t.m2.count(m) || t.m3.count(m);
    };

    bool exists = method_exists_somewhere(md.universal, method);
    if (dtable)
        exists = exists || method_exists_somewhere(*dtable, method);

    if (exists)
    {
        throw std::runtime_error("Method '" + method + "' on " + self.type() +
                                 " does not accept " + std::to_string(argc) + " argument(s)");
    }

    throw std::runtime_error("Unknown method '" + method + "' on type '" + self.type() + "'");
}
