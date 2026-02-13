#pragma once
// scriptit_builtins.hpp — Free-function builtins and math dispatch for ScriptIt v2

#include "scriptit_types.hpp"
#include "scriptit_methods.hpp"
#include <fstream>
#include <sstream>
#include <memory>

// ─── File Handle Registry ───────────────────────────────
// Stores open fstream pointers keyed by integer ID.
// File objects in ScriptIt are Dicts with __type__="file", __id__=<handle_id>.

struct FileRegistry
{
    static FileRegistry &instance()
    {
        static FileRegistry inst;
        return inst;
    }

    int open(const std::string &path, const std::string &mode)
    {
        auto flags = std::ios::in; // default
        bool truncate = false;
        bool appendMode = false;

        if (mode == "r")
            flags = std::ios::in;
        else if (mode == "w")
        {
            flags = std::ios::out;
            truncate = true;
        }
        else if (mode == "a")
        {
            flags = std::ios::out | std::ios::app;
            appendMode = true;
        }
        else if (mode == "rw" || mode == "r+")
            flags = std::ios::in | std::ios::out;
        else if (mode == "w+")
        {
            flags = std::ios::in | std::ios::out;
            truncate = true;
        }
        else if (mode == "a+")
        {
            flags = std::ios::in | std::ios::out | std::ios::app;
            appendMode = true;
        }
        else
            throw std::runtime_error("open(): invalid mode '" + mode + "'");

        if (truncate)
            flags |= std::ios::trunc;

        auto fs = std::make_unique<std::fstream>(path, flags);
        if (!fs->is_open())
            throw std::runtime_error("open(): cannot open file '" + path + "'");

        int id = nextId_++;
        files_[id] = std::move(fs);
        paths_[id] = path;
        modes_[id] = mode;
        // Register raw pointer in FileStore so method dispatch can access it
        scriptit_file_internal::FileStore::instance().streams[id] = files_[id].get();
        return id;
    }

    std::fstream &get(int id)
    {
        auto it = files_.find(id);
        if (it == files_.end() || !it->second)
            throw std::runtime_error("File handle " + std::to_string(id) + " is not open");
        return *it->second;
    }

    std::string getPath(int id)
    {
        auto it = paths_.find(id);
        return (it != paths_.end()) ? it->second : "<unknown>";
    }

    std::string getMode(int id)
    {
        auto it = modes_.find(id);
        return (it != modes_.end()) ? it->second : "?";
    }

    void close(int id)
    {
        auto it = files_.find(id);
        if (it != files_.end() && it->second)
        {
            it->second->close();
            it->second.reset();
        }
        files_.erase(id);
        paths_.erase(id);
        modes_.erase(id);
        scriptit_file_internal::FileStore::instance().streams.erase(id);
    }

    bool isOpen(int id)
    {
        auto it = files_.find(id);
        return it != files_.end() && it->second && it->second->is_open();
    }

    void closeAll()
    {
        for (auto &[id, fs] : files_)
        {
            if (fs && fs->is_open())
                fs->close();
        }
        files_.clear();
        paths_.clear();
        modes_.clear();
        scriptit_file_internal::FileStore::instance().streams.clear();
    }

private:
    int nextId_ = 1;
    std::unordered_map<int, std::unique_ptr<std::fstream>> files_;
    std::unordered_map<int, std::string> paths_;
    std::unordered_map<int, std::string> modes_;
};

// Helper: create a file-handle var (Dict with __type__="file", __id__=N)
inline var make_file_var(int id)
{
    Dict d;
    d["__type__"] = var("file");
    d["__id__"] = var(id);
    d["path"] = var(FileRegistry::instance().getPath(id));
    d["mode"] = var(FileRegistry::instance().getMode(id));
    return var(std::move(d));
}

// Helper: check if a var is a file handle and extract the id
inline bool is_file_var(const var &v, int &outId)
{
    if (!v.is_dict())
        return false;
    try
    {
        // var operator[] with string key works on dict/ordered_dict
        // But it's non-const — we need to work around this
        var &mv = const_cast<var &>(v);
        var typeVal = mv["__type__"];
        if (!typeVal.is_string() || typeVal.as_string_unchecked() != "file")
            return false;
        outId = mv["__id__"].toInt();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// ─── Math Function Dispatch ─────────────────────────────

inline var dispatch_math(const std::string &fname, std::stack<var> &stk)
{
    if (fname == "min" || fname == "max")
    {
        if (stk.size() < 2)
            throw std::runtime_error("Missing args for " + fname);
        var b = stk.top();
        stk.pop();
        var a = stk.top();
        stk.pop();
        return fname == "min" ? pythonic::math::min(a, b) : pythonic::math::max(a, b);
    }

    using MathFn = std::function<var(const var &)>;
    static const std::unordered_map<std::string, MathFn> mathOps = {
        {"sin", [](const var &x)
         { return pythonic::math::sin(x); }},
        {"cos", [](const var &x)
         { return pythonic::math::cos(x); }},
        {"tan", [](const var &x)
         { return pythonic::math::tan(x); }},
        {"asin", [](const var &x)
         { return pythonic::math::asin(x); }},
        {"acos", [](const var &x)
         { return pythonic::math::acos(x); }},
        {"atan", [](const var &x)
         { return pythonic::math::atan(x); }},
        {"log", [](const var &x)
         { return pythonic::math::log(x); }},
        {"log2", [](const var &x)
         { return pythonic::math::log2(x); }},
        {"log10", [](const var &x)
         { return pythonic::math::log10(x); }},
        {"sqrt", [](const var &x)
         { return pythonic::math::sqrt(x); }},
        {"abs", [](const var &x)
         { return pythonic::math::fabs(x); }},
        {"ceil", [](const var &x)
         { return pythonic::math::ceil(x); }},
        {"floor", [](const var &x)
         { return pythonic::math::floor(x); }},
        {"round", [](const var &x)
         { return pythonic::math::round(x); }},
        {"cot", [](const var &x)
         { return pythonic::math::cot(x); }},
        {"sec", [](const var &x)
         { return pythonic::math::sec(x); }},
        {"csc", [](const var &x)
         { return pythonic::math::csc(x); }},
    };

    if (stk.empty())
        throw std::runtime_error("Missing arg for " + fname);
    var arg = stk.top();
    stk.pop();
    auto it = mathOps.find(fname);
    if (it == mathOps.end())
        throw std::runtime_error("Unknown math function: " + fname);
    return it->second(arg);
}

// ─── Built-in Free Functions ────────────────────────────

using BuiltinFn = std::function<void(std::stack<var> &, int)>;

inline const std::unordered_map<std::string, BuiltinFn> &get_builtins()
{
    static const std::unordered_map<std::string, BuiltinFn> builtins = {

        // ── I/O ──────────────────────────────────────

        {"print", [](std::stack<var> &s, int argc)
         {
             std::vector<var> args;
             for (int i = 0; i < argc; ++i)
             {
                 if (s.empty())
                     throw std::runtime_error("Stack underflow for print");
                 args.push_back(s.top());
                 s.pop();
             }
             std::reverse(args.begin(), args.end());
             for (size_t i = 0; i < args.size(); ++i)
             {
                 if (i > 0)
                     std::cout << " ";
                 std::cout << (args[i].is_string() ? args[i].as_string_unchecked() : args[i].str());
             }
             std::cout << std::endl;
             s.push(var(NoneType{}));
         }},

        {"pprint", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("pprint() takes 1 argument");
             var a = s.top();
             s.pop();
             std::cout << a.pretty_str() << std::endl;
             s.push(var(NoneType{}));
         }},

        {"input", [](std::stack<var> &s, int argc)
         {
             std::string prompt;
             if (argc >= 1)
             {
                 var pv = s.top();
                 s.pop();
                 prompt = pv.is_string() ? pv.as_string_unchecked() : pv.str();
             }
             std::cout << prompt;
             std::cout.flush();
             std::string inputLine;
             std::getline(std::cin, inputLine);
             s.push(var(inputLine));
         }},

        {"read", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("read(filename) takes exactly 1 argument");
             var fn = s.top();
             s.pop();
             if (!fn.is_string())
                 throw std::runtime_error("read() expects a string filename");
             std::ifstream f(fn.as_string_unchecked());
             if (!f.is_open())
                 throw std::runtime_error("Cannot open file: " + fn.as_string_unchecked());
             std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
             s.push(var(content));
         }},

        {"readLine", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("readLine(filename) takes exactly 1 argument");
             var fn = s.top();
             s.pop();
             if (!fn.is_string())
                 throw std::runtime_error("readLine() expects a string filename");
             std::ifstream f(fn.as_string_unchecked());
             if (!f.is_open())
                 throw std::runtime_error("Cannot open file: " + fn.as_string_unchecked());
             List lines;
             std::string line;
             while (std::getline(f, line))
                 lines.push_back(var(line));
             s.push(var(std::move(lines)));
         }},

        {"write", [](std::stack<var> &s, int argc)
         {
             if (argc < 2 || argc > 3)
                 throw std::runtime_error("write(filename, data [, mode]) takes 2-3 arguments");
             std::string mode = "w";
             if (argc == 3)
             {
                 mode = s.top().is_string() ? s.top().as_string_unchecked() : "w";
                 s.pop();
             }
             var data = s.top();
             s.pop();
             var fn = s.top();
             s.pop();
             if (!fn.is_string())
                 throw std::runtime_error("write() expects a string filename");
             std::ios_base::openmode om = std::ios_base::out;
             if (mode == "a")
                 om |= std::ios_base::app;
             std::ofstream f(fn.as_string_unchecked(), om);
             if (!f.is_open())
                 throw std::runtime_error("Cannot open file for writing: " + fn.as_string_unchecked());
             f << (data.is_string() ? data.as_string_unchecked() : data.str());
             s.push(var(NoneType{}));
         }},

        // ── Type / Conversion ────────────────────────

        {"len", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("len() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(a.len());
         }},

        {"type", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("type() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(var(a.type()));
         }},

        {"str", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("str() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(var(a.str()));
         }},

        {"int", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("int() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(var(a.toInt()));
         }},

        {"float", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("float() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(var(a.toDouble()));
         }},

        {"bool", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("bool() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(var(static_cast<bool>(a) ? 1 : 0));
         }},

        {"repr", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("repr() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(var(a.pretty_str()));
         }},

        {"isinstance", [](std::stack<var> &s, int argc)
         {
             if (argc != 2)
                 throw std::runtime_error("isinstance(obj, type_name) takes exactly 2 arguments");
             var typeName = s.top();
             s.pop();
             var obj = s.top();
             s.pop();
             std::string tn = typeName.is_string() ? typeName.as_string_unchecked() : typeName.str();
             s.push(var(obj.type() == tn ? 1 : 0));
         }},

        // ── Container Constructors ───────────────────

        {"list", [](std::stack<var> &s, int argc)
         {
             if (argc == 0)
             {
                 s.push(var(List{}));
                 return;
             }
             var a = s.top();
             s.pop();
             s.push(a);
         }},

        {"set", [](std::stack<var> &s, int argc)
         {
             if (argc == 0)
             {
                 s.push(var(Set{}));
                 return;
             }
             var a = s.top();
             s.pop();
             s.push(a);
         }},

        {"dict", [](std::stack<var> &s, int argc)
         {
             if (argc == 0)
             {
                 s.push(var(Dict{}));
                 return;
             }
             var a = s.top();
             s.pop();
             s.push(a);
         }},

        {"range_list", [](std::stack<var> &s, int argc)
         {
             if (argc != 2)
                 throw std::runtime_error("range_list(start, end) takes exactly 2 arguments");
             var endVal = s.top();
             s.pop();
             var startVal = s.top();
             s.pop();
             int sv = startVal.toInt(), ev = endVal.toInt();
             List result;
             if (sv <= ev)
                 for (int i = sv; i <= ev; ++i)
                     result.push_back(var(i));
             else
                 for (int i = sv; i >= ev; --i)
                     result.push_back(var(i));
             s.push(var(std::move(result)));
         }},

        // ── Container free functions ─────────────────

        {"append", [](std::stack<var> &s, int argc)
         {
             if (argc != 2)
                 throw std::runtime_error("append(list, item) takes exactly 2 arguments");
             var item = s.top();
             s.pop();
             var lst = s.top();
             s.pop();
             if (!lst.is_list())
                 throw std::runtime_error("append() requires a list as first argument");
             lst.append(item);
             s.push(lst);
         }},

        {"pop", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("pop() takes exactly 1 argument");
             var lst = s.top();
             s.pop();
             if (!lst.is_list())
                 throw std::runtime_error("pop() requires a list");
             s.push(lst.pop());
         }},

        // ── Functional / Iteration ───────────────────

        {"sum", [](std::stack<var> &s, int argc)
         {
             if (argc < 1 || argc > 2)
                 throw std::runtime_error("sum(iterable[, start]) takes 1-2 arguments");
             var start = var(0);
             if (argc == 2)
             {
                 start = s.top();
                 s.pop();
             }
             var lst = s.top();
             s.pop();
             if (!lst.is_list())
                 throw std::runtime_error("sum() requires a list");
             var total = start;
             for (const auto &item : lst)
                 total = pythonic::math::add(total, item, Overflow::Promote);
             s.push(total);
         }},

        {"sorted", [](std::stack<var> &s, int argc)
         {
             if (argc < 1 || argc > 2)
                 throw std::runtime_error("sorted(iterable[, reverse]) takes 1-2 arguments");
             bool rev = false;
             if (argc == 2)
             {
                 rev = static_cast<bool>(s.top());
                 s.pop();
             }
             var lst = s.top();
             s.pop();
             if (!lst.is_list())
                 throw std::runtime_error("sorted() requires a list");
             auto &src = lst.var_get<List>();
             List sorted_list(src.begin(), src.end());
             std::sort(sorted_list.begin(), sorted_list.end(), [](const var &a, const var &b)
                       { return a < b; });
             if (rev)
                 std::reverse(sorted_list.begin(), sorted_list.end());
             s.push(var(std::move(sorted_list)));
         }},

        {"reversed", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("reversed() takes exactly 1 argument");
             var lst = s.top();
             s.pop();
             s.push(lst.reverse());
         }},

        {"all", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("all() takes exactly 1 argument");
             var lst = s.top();
             s.pop();
             if (!lst.is_list())
                 throw std::runtime_error("all() requires a list");
             for (const auto &item : lst)
                 if (!static_cast<bool>(item))
                 {
                     s.push(var(0));
                     return;
                 }
             s.push(var(1));
         }},

        {"any", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("any() takes exactly 1 argument");
             var lst = s.top();
             s.pop();
             if (!lst.is_list())
                 throw std::runtime_error("any() requires a list");
             for (const auto &item : lst)
                 if (static_cast<bool>(item))
                 {
                     s.push(var(1));
                     return;
                 }
             s.push(var(0));
         }},

        {"enumerate", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("enumerate() takes exactly 1 argument");
             var lst = s.top();
             s.pop();
             List result;
             int idx = 0;
             for (const auto &item : lst)
             {
                 List pair;
                 pair.push_back(var(idx));
                 pair.push_back(item);
                 result.push_back(var(std::move(pair)));
                 idx++;
             }
             s.push(var(std::move(result)));
         }},

        {"zip", [](std::stack<var> &s, int argc)
         {
             if (argc != 2)
                 throw std::runtime_error("zip() takes exactly 2 arguments");
             var lst2 = s.top();
             s.pop();
             var lst1 = s.top();
             s.pop();
             if (!lst1.is_list() || !lst2.is_list())
                 throw std::runtime_error("zip() requires two lists");
             List result;
             size_t minLen = std::min((size_t)lst1.len().toInt(), (size_t)lst2.len().toInt());
             for (size_t i = 0; i < minLen; ++i)
             {
                 List pair;
                 pair.push_back(lst1[var((int)i)]);
                 pair.push_back(lst2[var((int)i)]);
                 result.push_back(var(std::move(pair)));
             }
             s.push(var(std::move(result)));
         }},

        {"map", [](std::stack<var> &s, int argc)
         {
             // map(func_name, list) — limited: placeholder for now
             if (argc != 2)
                 throw std::runtime_error("map() takes exactly 2 arguments");
             var lst = s.top();
             s.pop();
             var funcName = s.top();
             s.pop();
             s.push(lst); // placeholder — use for-in loops
         }},

        // ── Math free functions ──────────────────────

        {"abs", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("abs() takes exactly 1 argument");
             var a = s.top();
             s.pop();
             s.push(pythonic::math::fabs(a));
         }},

        // ── File I/O free functions ──────────────────

        {"open", [](std::stack<var> &s, int argc)
         {
             if (argc < 1 || argc > 2)
                 throw std::runtime_error("open(path[, mode]) takes 1-2 arguments");
             std::string mode = "r";
             if (argc == 2)
             {
                 var modeArg = s.top();
                 s.pop();
                 mode = modeArg.is_string() ? modeArg.as_string_unchecked() : modeArg.str();
             }
             var pathArg = s.top();
             s.pop();
             std::string path = pathArg.is_string() ? pathArg.as_string_unchecked() : pathArg.str();
             int id = FileRegistry::instance().open(path, mode);
             s.push(make_file_var(id));
         }},

        {"close", [](std::stack<var> &s, int argc)
         {
             if (argc != 1)
                 throw std::runtime_error("close(file) takes exactly 1 argument");
             var fileArg = s.top();
             s.pop();
             int id;
             if (!is_file_var(fileArg, id))
                 throw std::runtime_error("close() requires a file handle");
             FileRegistry::instance().close(id);
             s.push(var(NoneType{}));
         }},

    };
    return builtins;
}
