#pragma once
// scriptit_methods.hpp — Dtype-dependent dot-method dispatch for ScriptIt v2
// Architecture: map of dtype → supported methods, dispatched by arg count.

#include "scriptit_types.hpp"
#include <fstream>
#include <sstream>

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
    { return var(s.is_none()); };
    t.m0["is_bool"] = [](var &s) -> var
    { return var(s.is_bool()); };
    t.m0["is_int"] = [](var &s) -> var
    { return var(s.is_int()); };
    t.m0["is_uint"] = [](var &s) -> var
    { return var(s.is_uint()); };
    t.m0["is_long"] = [](var &s) -> var
    { return var(s.is_long()); };
    t.m0["is_ulong"] = [](var &s) -> var
    { return var(s.is_ulong()); };
    t.m0["is_long_long"] = [](var &s) -> var
    { return var(s.is_long_long()); };
    t.m0["is_ulong_long"] = [](var &s) -> var
    { return var(s.is_ulong_long()); };
    t.m0["is_float"] = [](var &s) -> var
    { return var(s.is_float()); };
    t.m0["is_double"] = [](var &s) -> var
    { return var(s.is_double()); };
    t.m0["is_long_double"] = [](var &s) -> var
    { return var(s.is_long_double()); };
    t.m0["is_string"] = [](var &s) -> var
    { return var(s.is_string()); };
    t.m0["is_list"] = [](var &s) -> var
    { return var(s.is_list()); };
    t.m0["is_dict"] = [](var &s) -> var
    { return var(s.is_dict()); };
    t.m0["is_set"] = [](var &s) -> var
    { return var(s.is_set()); };
    t.m0["is_ordered_set"] = [](var &s) -> var
    { return var(s.is_ordered_set()); };
    t.m0["is_ordered_dict"] = [](var &s) -> var
    { return var(s.is_ordered_dict()); };
    t.m0["is_graph"] = [](var &s) -> var
    { return var(s.is_graph()); };
    t.m0["is_any_integral"] = [](var &s) -> var
    { return var(s.is_any_integral()); };
    t.m0["is_any_floating"] = [](var &s) -> var
    { return var(s.is_any_floating()); };
    t.m0["is_any_numeric"] = [](var &s) -> var
    { return var(s.is_any_numeric()); };
    t.m0["isNone"] = [](var &s) -> var
    { return var(s.isNone()); };
    t.m0["isNumeric"] = [](var &s) -> var
    { return var(s.isNumeric()); };
    t.m0["isIntegral"] = [](var &s) -> var
    { return var(s.isIntegral()); };

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
    { return var(static_cast<bool>(s)); };
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
    { return var(s.isdigit()); };
    t.m0["isalpha"] = [](var &s) -> var
    { return var(s.isalpha()); };
    t.m0["isalnum"] = [](var &s) -> var
    { return var(s.isalnum()); };
    t.m0["isspace"] = [](var &s) -> var
    { return var(s.isspace()); };
    t.m0["empty"] = [](var &s) -> var
    { return var(s.empty()); };
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
    { return var(s.empty()); };
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
    { return var(s.empty()); };
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
    { return var(s.empty()); };
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
        std::string key = a.is_string() ? a.as_string_unchecked() : a.str();
        if (s.contains(var(key)))
            return s[key];
        return var(NoneType{});
    };

    // 2-arg
    t.m2["get"] = [](var &s, const var &a, const var &b) -> var
    {
        // dict.get(key, default) → value or default
        std::string key = a.is_string() ? a.as_string_unchecked() : a.str();
        if (s.contains(var(key)))
            return s[key];
        return b;
    };

    return t;
}

// ─── Helper: resolve a var to a graph node ID ───────────
// Supports: int → direct ID, or search by node data value
inline size_t resolve_node_id(VarGraphWrapper &g, const var &v)
{
    if (v.is_any_integral())
    {
        size_t id = (size_t)var_to_double(v);
        if (id >= g.node_count())
            throw std::runtime_error("Node ID " + std::to_string(id) + " out of range (graph has " + std::to_string(g.node_count()) + " nodes)");
        return id;
    }
    // Search by node data
    for (size_t i = 0; i < g.node_count(); ++i)
    {
        try
        {
            if (g.get_node_data(i) == v)
                return i;
        }
        catch (...)
        {
        }
    }
    throw std::runtime_error("Node not found in graph: " + v.str());
}

// Helper: check if a var is an edge spec dict (created by -> or <->)
inline bool is_edge_spec(const var &v)
{
    if (!v.is_dict())
        return false;
    var &mv = const_cast<var &>(v);
    try
    {
        var t = mv["__dir__"];
        return t.is_string() && (t.as_string_unchecked() == "directed" || t.as_string_unchecked() == "bidirectional" || t.as_string_unchecked() == "undirected");
    }
    catch (...)
    {
        return false;
    }
}

// ─── Graph Methods ──────────────────────────────────────

inline MethodTable make_graph_methods()
{
    MethodTable t;

    // ── 0-arg methods ──
    t.m0["node_count"] = [](var &s) -> var
    {
        return var((long long)s.as_graph_unchecked()->node_count());
    };
    t.m0["edge_count"] = [](var &s) -> var
    {
        return var((long long)s.as_graph_unchecked()->edge_count());
    };
    t.m0["size"] = [](var &s) -> var
    {
        return var((long long)s.as_graph_unchecked()->size());
    };
    t.m0["is_connected"] = [](var &s) -> var
    {
        return var(s.as_graph_unchecked()->is_connected());
    };
    t.m0["has_cycle"] = [](var &s) -> var
    {
        return var(s.as_graph_unchecked()->has_cycle());
    };
    t.m0["nodes"] = [](var &s) -> var
    {
        auto &g = *s.as_graph_unchecked();
        List result;
        for (size_t i = 0; i < g.node_count(); ++i)
            result.push_back(var((long long)i));
        return var(std::move(result));
    };
    t.m0["add_node"] = [](var &s) -> var
    {
        return var((long long)s.as_graph_unchecked()->add_node());
    };
    t.m0["topological_sort"] = [](var &s) -> var
    {
        auto order = s.as_graph_unchecked()->topological_sort();
        List result;
        for (auto id : order)
            result.push_back(var((long long)id));
        return var(std::move(result));
    };
    t.m0["connected_components"] = [](var &s) -> var
    {
        auto comps = s.as_graph_unchecked()->connected_components();
        List result;
        for (auto &comp : comps)
        {
            List inner;
            for (auto id : comp)
                inner.push_back(var((long long)id));
            result.push_back(var(std::move(inner)));
        }
        return var(std::move(result));
    };
    t.m0["strongly_connected_components"] = [](var &s) -> var
    {
        auto comps = s.as_graph_unchecked()->strongly_connected_components();
        List result;
        for (auto &comp : comps)
        {
            List inner;
            for (auto id : comp)
                inner.push_back(var((long long)id));
            result.push_back(var(std::move(inner)));
        }
        return var(std::move(result));
    };
    t.m0["prim_mst"] = [](var &s) -> var
    {
        auto [cost, edges] = s.as_graph_unchecked()->prim_mst();
        Dict result;
        result["cost"] = var(cost);
        List edge_list;
        for (auto &[u, v, w] : edges)
        {
            List e;
            e.push_back(var((long long)u));
            e.push_back(var((long long)v));
            e.push_back(var(w));
            edge_list.push_back(var(std::move(e)));
        }
        result["edges"] = var(std::move(edge_list));
        return var(std::move(result));
    };
    t.m0["pretty_str"] = [](var &s) -> var
    {
        return var(s.as_graph_unchecked()->pretty_str());
    };
    t.m0["show"] = [](var &s) -> var
    {
#ifdef PYTHONIC_ENABLE_GRAPH_VIEWER
        s.show(true);
        return var(NoneType{});
#else
        throw std::runtime_error("Graph viewer not available. Build with PYTHONIC_ENABLE_GRAPH_VIEWER=ON (requires ImGui).\nUse .to_dot(filename) for Graphviz export or .pretty_str() for terminal output.");
#endif
    };
    t.m0["draw"] = [](var &s) -> var
    {
        // 2D ASCII drawing of the graph
        return var(s.as_graph_unchecked()->pretty_str());
    };

    // ── 1-arg methods ──

    // add_node(data) — add a node with data
    t.m1["add_node"] = [](var &s, const var &a) -> var
    {
        return var((long long)s.as_graph_unchecked()->add_node(a));
    };

    // add_edge(edge_spec) — add edge from edge spec (A -> B or A <-> B)
    t.m1["add_edge"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        if (is_edge_spec(a))
        {
            var &ma = const_cast<var &>(a);
            var from_v = ma["__from__"];
            var to_v = ma["__to__"];
            std::string dir = ma["__dir__"].as_string_unchecked();
            size_t from_id = resolve_node_id(g, from_v);
            size_t to_id = resolve_node_id(g, to_v);
            bool directed = (dir == "directed");
            g.add_edge(from_id, to_id, directed);
        }
        else
        {
            throw std::runtime_error("add_edge expects an edge spec (use A -> B or A <-> B) or two node arguments");
        }
        return var(NoneType{});
    };

    // neighbors(node) — get neighbors of a node
    t.m1["neighbors"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        size_t id = resolve_node_id(g, a);
        auto nbrs = g.neighbors(id);
        List result;
        for (auto nid : nbrs)
            result.push_back(var((long long)nid));
        return var(std::move(result));
    };

    // out_degree(node)
    t.m1["out_degree"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        return var((long long)g.out_degree(resolve_node_id(g, a)));
    };

    // in_degree(node)
    t.m1["in_degree"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        return var((long long)g.in_degree(resolve_node_id(g, a)));
    };

    // remove_node(node)
    t.m1["remove_node"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        g.remove_node(resolve_node_id(g, a));
        return var(NoneType{});
    };

    // dfs(start) — depth-first search from node
    t.m1["dfs"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        auto order = g.dfs(resolve_node_id(g, a));
        List result;
        for (auto id : order)
            result.push_back(var((long long)id));
        return var(std::move(result));
    };

    // bfs(start) — breadth-first search from node
    t.m1["bfs"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        auto order = g.bfs(resolve_node_id(g, a));
        List result;
        for (auto id : order)
            result.push_back(var((long long)id));
        return var(std::move(result));
    };

    // bellman_ford(src) — returns dict with "distances" and "predecessors"
    t.m1["bellman_ford"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        auto [distances, predecessors] = g.bellman_ford(resolve_node_id(g, a));
        Dict result;
        List dist_list, pred_list;
        for (auto d : distances)
            dist_list.push_back(var(d));
        for (auto p : predecessors)
            pred_list.push_back(var((long long)p));
        result["distances"] = var(std::move(dist_list));
        result["predecessors"] = var(std::move(pred_list));
        return var(std::move(result));
    };

    // floyd_warshall() is 0-arg but returns a 2D list
    t.m0["floyd_warshall"] = [](var &s) -> var
    {
        auto matrix = s.as_graph_unchecked()->floyd_warshall();
        List result;
        for (auto &row : matrix)
        {
            List inner;
            for (auto d : row)
                inner.push_back(var(d));
            result.push_back(var(std::move(inner)));
        }
        return var(std::move(result));
    };

    // has_edge(node) — check if an edge spec or node pair exists
    t.m1["has_edge"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        if (is_edge_spec(a))
        {
            var &ma = const_cast<var &>(a);
            size_t from_id = resolve_node_id(g, ma["__from__"]);
            size_t to_id = resolve_node_id(g, ma["__to__"]);
            return var(g.has_edge(from_id, to_id));
        }
        throw std::runtime_error("has_edge expects an edge spec (A -> B) or two arguments");
    };

    // get_edge_weight(edge_spec)
    t.m1["get_edge_weight"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        if (is_edge_spec(a))
        {
            var &ma = const_cast<var &>(a);
            size_t from_id = resolve_node_id(g, ma["__from__"]);
            size_t to_id = resolve_node_id(g, ma["__to__"]);
            auto w = g.get_edge_weight(from_id, to_id);
            if (w.has_value())
                return var(w.value());
            return var(NoneType{});
        }
        throw std::runtime_error("get_edge_weight expects an edge spec (A -> B) or two arguments");
    };

    // set_node_data(node, data)
    t.m1["set_node_data"] = [](var &s, const var &a) -> var
    {
        throw std::runtime_error("set_node_data requires 2 arguments: node and data");
    };

    // get_node_data(node)
    t.m1["get_node_data"] = [](var &s, const var &a) -> var
    {
        auto &g = *s.as_graph_unchecked();
        return g.get_node_data(resolve_node_id(g, a));
    };

    // save(filename) — save graph
    t.m1["save"] = [](var &s, const var &a) -> var
    {
        s.as_graph_unchecked()->save(a.as_string_unchecked());
        return var(NoneType{});
    };

    // to_dot(filename)
    t.m1["to_dot"] = [](var &s, const var &a) -> var
    {
        s.as_graph_unchecked()->to_dot(a.as_string_unchecked());
        return var(NoneType{});
    };

    // ── 2-arg methods ──

    // add_edge(from, to) — undirected edge
    t.m2["add_edge"] = [](var &s, const var &a, const var &b) -> var
    {
        auto &g = *s.as_graph_unchecked();
        if (is_edge_spec(a))
        {
            // add_edge(A -> B, weight)
            var &ma = const_cast<var &>(a);
            size_t from_id = resolve_node_id(g, ma["__from__"]);
            size_t to_id = resolve_node_id(g, ma["__to__"]);
            std::string dir = ma["__dir__"].as_string_unchecked();
            bool directed = (dir == "directed");
            double weight = var_to_double(b);
            g.add_edge(from_id, to_id, directed, weight);
        }
        else
        {
            // add_edge(from, to) — undirected, no weight
            size_t from_id = resolve_node_id(g, a);
            size_t to_id = resolve_node_id(g, b);
            g.add_edge(from_id, to_id, false);
        }
        return var(NoneType{});
    };

    // has_edge(from, to)
    t.m2["has_edge"] = [](var &s, const var &a, const var &b) -> var
    {
        auto &g = *s.as_graph_unchecked();
        return var(g.has_edge(resolve_node_id(g, a), resolve_node_id(g, b)));
    };

    // get_edge_weight(from, to)
    t.m2["get_edge_weight"] = [](var &s, const var &a, const var &b) -> var
    {
        auto &g = *s.as_graph_unchecked();
        auto w = g.get_edge_weight(resolve_node_id(g, a), resolve_node_id(g, b));
        if (w.has_value())
            return var(w.value());
        return var(NoneType{});
    };

    // remove_edge(from, to)
    t.m2["remove_edge"] = [](var &s, const var &a, const var &b) -> var
    {
        auto &g = *s.as_graph_unchecked();
        return var(g.remove_edge(resolve_node_id(g, a), resolve_node_id(g, b)));
    };

    // get_shortest_path(src, dest)
    t.m2["get_shortest_path"] = [](var &s, const var &a, const var &b) -> var
    {
        auto &g = *s.as_graph_unchecked();
        auto [path, cost] = g.get_shortest_path(resolve_node_id(g, a), resolve_node_id(g, b));
        Dict result;
        List path_list;
        for (auto id : path)
            path_list.push_back(var((long long)id));
        result["path"] = var(std::move(path_list));
        result["cost"] = var(cost);
        return var(std::move(result));
    };

    // set_node_data(node, data)
    t.m2["set_node_data"] = [](var &s, const var &a, const var &b) -> var
    {
        auto &g = *s.as_graph_unchecked();
        g.set_node_data(resolve_node_id(g, a), b);
        return var(NoneType{});
    };

    // set_edge_weight(edge_spec, weight) — using edge spec
    t.m2["set_edge_weight"] = [](var &s, const var &a, const var &b) -> var
    {
        auto &g = *s.as_graph_unchecked();
        if (is_edge_spec(a))
        {
            var &ma = const_cast<var &>(a);
            size_t from_id = resolve_node_id(g, ma["__from__"]);
            size_t to_id = resolve_node_id(g, ma["__to__"]);
            g.set_edge_weight(from_id, to_id, var_to_double(b));
        }
        else
        {
            // set_edge_weight(from, to) — need 3 args
            throw std::runtime_error("set_edge_weight needs 3 args (from, to, weight) or (edge_spec, weight)");
        }
        return var(NoneType{});
    };

    // to_dot(filename, show_weights)
    t.m2["to_dot"] = [](var &s, const var &a, const var &b) -> var
    {
        s.as_graph_unchecked()->to_dot(a.as_string_unchecked(), static_cast<bool>(b));
        return var(NoneType{});
    };

    // ── 3-arg methods ──

    // add_edge(from, to, directed)
    t.m3["add_edge"] = [](var &s, const var &a, const var &b, const var &c) -> var
    {
        auto &g = *s.as_graph_unchecked();
        if (is_edge_spec(a))
        {
            // add_edge(A -> B, weight, reverse_weight) — directed with two weights
            var &ma = const_cast<var &>(a);
            size_t from_id = resolve_node_id(g, ma["__from__"]);
            size_t to_id = resolve_node_id(g, ma["__to__"]);
            std::string dir = ma["__dir__"].as_string_unchecked();
            bool directed = (dir == "directed");
            double w1 = var_to_double(b);
            double w2 = var_to_double(c);
            g.add_edge(from_id, to_id, directed, w1, w2);
        }
        else
        {
            // add_edge(from, to, weight_or_directed)
            size_t from_id = resolve_node_id(g, a);
            size_t to_id = resolve_node_id(g, b);
            if (c.is_bool())
            {
                g.add_edge(from_id, to_id, c.as_bool_unchecked());
            }
            else
            {
                double weight = var_to_double(c);
                g.add_edge(from_id, to_id, false, weight);
            }
        }
        return var(NoneType{});
    };

    // set_edge_weight(from, to, weight)
    t.m3["set_edge_weight"] = [](var &s, const var &a, const var &b, const var &c) -> var
    {
        auto &g = *s.as_graph_unchecked();
        g.set_edge_weight(resolve_node_id(g, a), resolve_node_id(g, b), var_to_double(c));
        return var(NoneType{});
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
    MethodTable graph_m;

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
            d.graph_m = make_graph_methods();
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
        if (v.is_graph())
            return &graph_m;
        if (v.is_any_numeric() || v.is_bool())
            return &numeric_m;
        return nullptr; // none, etc.
    }
};

// ─── Forward declarations for file I/O ──────────────────
// FileRegistry is defined in scriptit_builtins.hpp (included after this header).
// We use a function pointer pattern to break the circular dependency.
// The actual file dispatch is implemented via dispatch_file_method() below,
// which is defined inline here but uses lazy-linked FileRegistry access.

// We need direct fstream access. FileRegistry stores them, but we can't
// include scriptit_builtins.hpp here. Instead, we store a global function
// pointer that ScriptIt.cpp sets after including all headers.

// Actually, let's use a different approach: file method dispatch is done
// inline using the fstream headers we already have, with a global map.

namespace scriptit_file_internal
{
    struct FileStore
    {
        static FileStore &instance()
        {
            static FileStore inst;
            return inst;
        }
        std::unordered_map<int, std::fstream *> streams; // non-owning — owned by FileRegistry
    };
}

// Helper: check if a var is a file dict
inline bool is_file_dict(const var &v, int &outId)
{
    if (!v.is_dict())
        return false;
    try
    {
        // Use const-safe access to avoid auto-inserting keys into the dict
        const auto *dp = v.var_get_if<Dict>();
        if (!dp)
            return false;
        auto it = dp->find("__type__");
        if (it == dp->end() || !it->second.is_string() || it->second.as_string_unchecked() != "file")
            return false;
        auto it2 = dp->find("__id__");
        if (it2 == dp->end())
            return false;
        outId = it2->second.toInt();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Dispatch file-specific methods. Returns {true, result} if handled.
inline std::pair<bool, var> dispatch_file_method(var &self, const std::string &method, const std::vector<var> &args)
{
    int fid;
    if (!is_file_dict(self, fid))
        return {false, var(NoneType{})};

    auto &store = scriptit_file_internal::FileStore::instance();
    auto it = store.streams.find(fid);
    if (it == store.streams.end() || !it->second || !it->second->is_open())
        throw std::runtime_error("File handle " + std::to_string(fid) + " is not open");

    std::fstream &fs = *it->second;
    int argc = (int)args.size();

    if (method == "read" && argc == 0)
    {
        // Read entire file content
        std::ostringstream oss;
        // Seek to beginning for full read
        fs.clear();
        fs.seekg(0, std::ios::beg);
        oss << fs.rdbuf();
        return {true, var(oss.str())};
    }
    if (method == "readline" && argc == 0)
    {
        std::string line;
        if (std::getline(fs, line))
            return {true, var(line)};
        return {true, var("")};
    }
    if (method == "readlines" && argc == 0)
    {
        fs.clear();
        fs.seekg(0, std::ios::beg);
        List lines;
        std::string line;
        while (std::getline(fs, line))
            lines.push_back(var(line));
        return {true, var(std::move(lines))};
    }
    if (method == "write" && argc == 1)
    {
        std::string data = args[0].is_string() ? args[0].as_string_unchecked() : args[0].str();
        fs << data;
        fs.flush();
        return {true, var((int)data.size())};
    }
    if (method == "writelines" && argc == 1)
    {
        if (!args[0].is_list())
            throw std::runtime_error("writelines() requires a list");
        int total = 0;
        for (const auto &item : args[0])
        {
            std::string line = item.is_string() ? item.as_string_unchecked() : item.str();
            fs << line << "\n";
            total += (int)line.size() + 1;
        }
        fs.flush();
        return {true, var(total)};
    }
    if (method == "close" && argc == 0)
    {
        fs.close();
        store.streams.erase(fid);
        return {true, var(NoneType{})};
    }
    if (method == "is_open" && argc == 0)
    {
        return {true, var(fs.is_open())};
    }
    if (method == "flush" && argc == 0)
    {
        fs.flush();
        return {true, var(NoneType{})};
    }

    // Not a file method — fall through to normal dispatch (dict methods)
    return {false, var(NoneType{})};
}

// ─── Top-level dispatch function ────────────────────────

// Dispatches obj.method(args...) → result
// Tries: 1) dtype-specific table  2) universal table
// Handles overloaded arity (e.g. split with 0 or 1 arg)
inline var dispatch_method(var &self, const std::string &method, const std::vector<var> &args)
{
    // 0) Try file-specific methods first (file handles are Dicts internally)
    {
        auto [found, result] = dispatch_file_method(self, method, args);
        if (found)
            return result;
    }

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
