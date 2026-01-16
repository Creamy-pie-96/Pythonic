#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <optional>
#include <tuple>
#include "pythonicVars.hpp"
#include "pythonicLoop.hpp"

namespace pythonic
{
    namespace func
    {

        using namespace pythonic::vars;
        using namespace pythonic::loop;

        // ============ Map ============
        // Python-like map(function, iterable) - OPTIMIZED

        // Specialized overload for var containers - uses direct list access
        template <typename Func>
        var map(Func &&func, var &container)
        {
            if (container.is_list())
            {
                List &lst = container.as_list_unchecked();
                List result;
                result.reserve(lst.size());
                for (var &item : lst)
                {
                    result.emplace_back(func(item));
                }
                return var(std::move(result));
            }
            // Fallback for other container types
            List result;
            for (auto &&item : container)
            {
                result.emplace_back(func(item));
            }
            return var(std::move(result));
        }

        template <typename Func>
        var map(Func &&func, const var &container)
        {
            if (container.is_list())
            {
                const List &lst = container.as_list_unchecked();
                List result;
                result.reserve(lst.size());
                for (const var &item : lst)
                {
                    result.emplace_back(func(item));
                }
                return var(std::move(result));
            }
            // Fallback for other container types
            List result;
            for (auto &&item : container)
            {
                result.emplace_back(func(item));
            }
            return var(std::move(result));
        }

        // Generic overload for non-var iterables
        template <typename Func, typename Iterable,
                  typename = std::enable_if_t<!std::is_same_v<std::decay_t<Iterable>, var>>>
        var map(Func &&func, Iterable &&iterable)
        {
            List result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result.emplace_back(func(item));
                }
                else
                {
                    result.emplace_back(func(var(item)));
                }
            }
            return var(std::move(result));
        }

        // Map with index (like enumerate + map) - OPTIMIZED
        template <typename Func, typename Iterable>
        var map_indexed(Func &&func, Iterable &&iterable)
        {
            List result;
            size_t idx = 0;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result.emplace_back(func(idx, item));
                }
                else
                {
                    result.emplace_back(func(idx, var(item)));
                }
                ++idx;
            }
            return var(std::move(result));
        }

        // ============ Filter ============
        // Python-like filter(function, iterable) - OPTIMIZED

        // Specialized overload for var containers
        template <typename Func>
        var filter(Func &&func, var &container)
        {
            if (container.is_list())
            {
                List &lst = container.as_list_unchecked();
                List result;
                for (var &item : lst)
                {
                    if (static_cast<bool>(func(item)))
                    {
                        result.emplace_back(item);
                    }
                }
                return var(std::move(result));
            }
            // Fallback for other container types
            List result;
            for (auto &&item : container)
            {
                if (static_cast<bool>(func(item)))
                {
                    result.emplace_back(item);
                }
            }
            return var(std::move(result));
        }

        template <typename Func>
        var filter(Func &&func, const var &container)
        {
            if (container.is_list())
            {
                const List &lst = container.as_list_unchecked();
                List result;
                for (const var &item : lst)
                {
                    if (static_cast<bool>(func(item)))
                    {
                        result.emplace_back(item);
                    }
                }
                return var(std::move(result));
            }
            // Fallback for other container types
            List result;
            for (auto &&item : container)
            {
                if (static_cast<bool>(func(item)))
                {
                    result.emplace_back(item);
                }
            }
            return var(std::move(result));
        }

        // Generic overload for non-var iterables
        template <typename Func, typename Iterable,
                  typename = std::enable_if_t<!std::is_same_v<std::decay_t<Iterable>, var>>>
        var filter(Func &&func, Iterable &&iterable)
        {
            List result;
            for (auto &&item : iterable)
            {
                bool keep = false;
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    keep = static_cast<bool>(func(item));
                    if (keep)
                        result.emplace_back(item);
                }
                else
                {
                    keep = static_cast<bool>(func(var(item)));
                    if (keep)
                        result.emplace_back(item);
                }
            }
            return var(std::move(result));
        }

        // ============ Reduce ============
        // Python-like reduce(function, iterable, initializer) - OPTIMIZED

        // Specialized overload for var containers
        template <typename Func>
        var reduce(Func &&func, var &container)
        {
            if (container.is_list())
            {
                List &lst = container.as_list_unchecked();
                if (lst.empty())
                {
                    throw std::runtime_error("reduce() of empty sequence with no initial value");
                }
                auto it = lst.begin();
                var result = *it;
                ++it;
                for (; it != lst.end(); ++it)
                {
                    result = func(result, *it);
                }
                return result;
            }
            // Fallback
            auto it = container.begin();
            if (it == container.end())
            {
                throw std::runtime_error("reduce() of empty sequence with no initial value");
            }
            var result = *it;
            ++it;
            for (; it != container.end(); ++it)
            {
                result = func(result, *it);
            }
            return result;
        }

        template <typename Func>
        var reduce(Func &&func, const var &container)
        {
            if (container.is_list())
            {
                const List &lst = container.as_list_unchecked();
                if (lst.empty())
                {
                    throw std::runtime_error("reduce() of empty sequence with no initial value");
                }
                auto it = lst.begin();
                var result = *it;
                ++it;
                for (; it != lst.end(); ++it)
                {
                    result = func(result, *it);
                }
                return result;
            }
            // Fallback
            auto it = container.begin();
            if (it == container.end())
            {
                throw std::runtime_error("reduce() of empty sequence with no initial value");
            }
            var result = *it;
            ++it;
            for (; it != container.end(); ++it)
            {
                result = func(result, *it);
            }
            return result;
        }

        // Generic overload for non-var iterables
        template <typename Func, typename Iterable,
                  typename = std::enable_if_t<!std::is_same_v<std::decay_t<Iterable>, var>>>
        var reduce(Func &&func, Iterable &&iterable)
        {
            auto it = iterable.begin();
            if (it == iterable.end())
            {
                throw std::runtime_error("reduce() of empty sequence with no initial value");
            }
            var result = *it;
            ++it;
            for (; it != iterable.end(); ++it)
            {
                result = func(result, *it);
            }
            return result;
        }

        template <typename Func, typename Iterable>
        var reduce(Func &&func, Iterable &&iterable, var initial)
        {
            var result = initial;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result = func(result, item);
                }
                else
                {
                    result = func(result, var(item));
                }
            }
            return result;
        }

        // ============ List Comprehension ============
        // list_comp(expression, iterable) - [expr(x) for x in iterable]
        // list_comp(expression, iterable, condition) - [expr(x) for x in iterable if cond(x)]

        template <typename Expr, typename Iterable>
        var list_comp(Expr &&expr, Iterable &&iterable)
        {
            return map(std::forward<Expr>(expr), std::forward<Iterable>(iterable));
        }

        template <typename Expr, typename Iterable, typename Cond>
        var list_comp(Expr &&expr, Iterable &&iterable, Cond &&cond)
        {
            List result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (static_cast<bool>(cond(item)))
                    {
                        result.emplace_back(expr(item));
                    }
                }
                else
                {
                    var v(item);
                    if (static_cast<bool>(cond(v)))
                    {
                        result.emplace_back(expr(v));
                    }
                }
            }
            return var(result);
        }

        // ============ Set Comprehension ============
        // set_comp(expression, iterable) - {expr(x) for x in iterable}
        // set_comp(expression, iterable, condition) - {expr(x) for x in iterable if cond(x)}

        template <typename Expr, typename Iterable>
        var set_comp(Expr &&expr, Iterable &&iterable)
        {
            Set result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result.insert(var(expr(item)));
                }
                else
                {
                    result.insert(var(expr(var(item))));
                }
            }
            return var(result);
        }

        template <typename Expr, typename Iterable, typename Cond>
        var set_comp(Expr &&expr, Iterable &&iterable, Cond &&cond)
        {
            Set result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (static_cast<bool>(cond(item)))
                    {
                        result.insert(var(expr(item)));
                    }
                }
                else
                {
                    var v(item);
                    if (static_cast<bool>(cond(v)))
                    {
                        result.insert(var(expr(v)));
                    }
                }
            }
            return var(result);
        }

        // ============ Dict Comprehension ============
        // dict_comp(key_expr, value_expr, iterable) - {key(x): val(x) for x in iterable}
        // dict_comp(key_expr, value_expr, iterable, condition) - {key(x): val(x) for x in iterable if cond(x)}

        template <typename KeyExpr, typename ValExpr, typename Iterable>
        var dict_comp(KeyExpr &&key_expr, ValExpr &&val_expr, Iterable &&iterable)
        {
            Dict result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    std::string key = key_expr(item).template get<std::string>();
                    result[key] = val_expr(item);
                }
                else
                {
                    var v(item);
                    std::string key = key_expr(v).template get<std::string>();
                    result[key] = val_expr(v);
                }
            }
            return var(result);
        }

        template <typename KeyExpr, typename ValExpr, typename Iterable, typename Cond>
        var dict_comp(KeyExpr &&key_expr, ValExpr &&val_expr, Iterable &&iterable, Cond &&cond)
        {
            Dict result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (static_cast<bool>(cond(item)))
                    {
                        std::string key = key_expr(item).template get<std::string>();
                        result[key] = val_expr(item);
                    }
                }
                else
                {
                    var v(item);
                    if (static_cast<bool>(cond(v)))
                    {
                        std::string key = key_expr(v).template get<std::string>();
                        result[key] = val_expr(v);
                    }
                }
            }
            return var(result);
        }

        // ============ Sorted ============
        // Python-like sorted(iterable, key=None, reverse=False) - OPTIMIZED

        inline var sorted(const var &iterable, bool reverse = false)
        {
            List result;
            for (const auto &item : iterable)
            {
                result.emplace_back(item);
            }
            if (reverse)
            {
                std::sort(result.begin(), result.end(), [](const var &a, const var &b)
                          { return b < a; });
            }
            else
            {
                std::sort(result.begin(), result.end());
            }
            return var(result);
        }

        template <typename KeyFunc>
        var sorted(const var &iterable, KeyFunc &&key, bool reverse = false)
        {
            List result;
            for (const auto &item : iterable)
            {
                result.emplace_back(item);
            }
            if (reverse)
            {
                std::sort(result.begin(), result.end(), [&key](const var &a, const var &b)
                          { return key(b) < key(a); });
            }
            else
            {
                std::sort(result.begin(), result.end(), [&key](const var &a, const var &b)
                          { return key(a) < key(b); });
            }
            return var(result);
        }

// ============ Lambda Helpers ============
// Helper macros for creating lambdas more Pythonically

// lambda_(params, body) - create a lambda
// Usage: auto square = lambda_(x, x * x);
#define lambda_(param, body) [](auto param) { return body; }

// lambda2_(p1, p2, body) - create a lambda with 2 params
// Usage: auto add = lambda2_(x, y, x + y);
#define lambda2_(p1, p2, body) [](auto p1, auto p2) { return body; }

// lambda3_(p1, p2, p3, body) - create a lambda with 3 params
#define lambda3_(p1, p2, p3, body) [](auto p1, auto p2, auto p3) { return body; }

// Capture lambdas (with [&] capture)
#define clambda_(param, body) [&](auto param) { return body; }
#define clambda2_(p1, p2, body) [&](auto p1, auto p2) { return body; }
#define clambda3_(p1, p2, p3, body) [&](auto p1, auto p2, auto p3) { return body; }

        // ============ Apply ============
        // Apply a function to arguments from a list/tuple

        template <typename Func>
        var apply(Func &&func, const var &args)
        {
            if (!args.is<List>())
            {
                throw std::runtime_error("apply() requires a list of arguments");
            }
            const auto &list = args.get<List>();
            switch (list.size())
            {
            case 0:
                return var(func());
            case 1:
                return var(func(list[0]));
            case 2:
                return var(func(list[0], list[1]));
            case 3:
                return var(func(list[0], list[1], list[2]));
            case 4:
                return var(func(list[0], list[1], list[2], list[3]));
            case 5:
                return var(func(list[0], list[1], list[2], list[3], list[4]));
            default:
                throw std::runtime_error("apply() supports up to 5 arguments");
            }
        }

        // ============ Partial ============
        // Partial function application (like functools.partial)

        template <typename Func, typename... Args>
        auto partial(Func &&func, Args &&...args)
        {
            return [func = std::forward<Func>(func),
                    bound_args = std::make_tuple(std::forward<Args>(args)...)](auto &&...remaining_args)
            {
                return std::apply([&](auto &&...bound)
                                  { return func(bound..., std::forward<decltype(remaining_args)>(remaining_args)...); }, bound_args);
            };
        }

        // ============ Compose ============
        // Function composition (f ∘ g)(x) = f(g(x))

        template <typename F, typename G>
        auto compose(F &&f, G &&g)
        {
            return [f = std::forward<F>(f), g = std::forward<G>(g)](auto &&x)
            {
                return f(g(std::forward<decltype(x)>(x)));
            };
        }

        // ============ Find / Find_if ============

        template <typename Iterable, typename Pred>
        std::optional<var> find_if(Iterable &&iterable, Pred &&pred)
        {
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (static_cast<bool>(pred(item)))
                    {
                        return item;
                    }
                }
                else
                {
                    var v(item);
                    if (static_cast<bool>(pred(v)))
                    {
                        return v;
                    }
                }
            }
            return std::nullopt;
        }

        template <typename Iterable>
        std::optional<size_t> index(Iterable &&iterable, const var &value)
        {
            size_t idx = 0;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (static_cast<bool>(item == value))
                    {
                        return idx;
                    }
                }
                else
                {
                    if (static_cast<bool>(var(item) == value))
                    {
                        return idx;
                    }
                }
                ++idx;
            }
            return std::nullopt;
        }

        // ============ Count ============

        template <typename Iterable, typename Pred>
        size_t count_if(Iterable &&iterable, Pred &&pred)
        {
            size_t count = 0;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (static_cast<bool>(pred(item)))
                        ++count;
                }
                else
                {
                    if (static_cast<bool>(pred(var(item))))
                        ++count;
                }
            }
            return count;
        }

        template <typename Iterable>
        size_t count(Iterable &&iterable, const var &value)
        {
            return count_if(std::forward<Iterable>(iterable), [&value](const var &v)
                            { return static_cast<bool>(v == value); });
        }

        // ============ Take / Drop ============
        // OPTIMIZED: reserve + emplace_back

        template <typename Iterable>
        var take(size_t n, Iterable &&iterable)
        {
            List result;
            result.reserve(n);
            size_t i = 0;
            for (auto &&item : iterable)
            {
                if (i >= n)
                    break;
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result.emplace_back(item);
                }
                else
                {
                    result.emplace_back(item);
                }
                ++i;
            }
            return var(result);
        }

        // OPTIMIZED: use emplace_back
        template <typename Iterable>
        var drop(size_t n, Iterable &&iterable)
        {
            List result;
            size_t i = 0;
            for (auto &&item : iterable)
            {
                if (i >= n)
                {
                    if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                    {
                        result.emplace_back(item);
                    }
                    else
                    {
                        result.emplace_back(item);
                    }
                }
                ++i;
            }
            return var(result);
        }

        // ============ TakeWhile / DropWhile ============
        // OPTIMIZED: use emplace_back

        template <typename Iterable, typename Pred>
        var take_while(Pred &&pred, Iterable &&iterable)
        {
            List result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (!static_cast<bool>(pred(item)))
                        break;
                    result.emplace_back(item);
                }
                else
                {
                    var v(item);
                    if (!static_cast<bool>(pred(v)))
                        break;
                    result.emplace_back(v);
                }
            }
            return var(result);
        }

        template <typename Iterable, typename Pred>
        var drop_while(Pred &&pred, Iterable &&iterable)
        {
            List result;
            bool dropping = true;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    if (dropping && static_cast<bool>(pred(item)))
                        continue;
                    dropping = false;
                    result.emplace_back(item);
                }
                else
                {
                    var v(item);
                    if (dropping && static_cast<bool>(pred(v)))
                        continue;
                    dropping = false;
                    result.emplace_back(v);
                }
            }
            return var(result);
        }

        // ============ Flatten ============
        // Flatten nested lists one level - OPTIMIZED

        inline var flatten(const var &nested)
        {
            List result;
            for (const auto &item : nested)
            {
                if (item.is<List>())
                {
                    for (const auto &inner : item)
                    {
                        result.emplace_back(inner);
                    }
                }
                else
                {
                    result.emplace_back(item);
                }
            }
            return var(result);
        }

        // ============ Unique ============
        // Return unique elements preserving order - OPTIMIZED

        inline var unique(const var &iterable)
        {
            List result;
            Set seen;
            for (const auto &item : iterable)
            {
                if (seen.find(item) == seen.end())
                {
                    result.emplace_back(item);
                    seen.insert(item);
                }
            }
            return var(result);
        }

        // ============ GroupBy ============
        // Group elements by key function - OPTIMIZED

        template <typename KeyFunc>
        var group_by(KeyFunc &&key_func, const var &iterable)
        {
            Dict result;
            for (const auto &item : iterable)
            {
                var key = key_func(item);
                std::string key_str = key.str();
                if (result.find(key_str) == result.end())
                {
                    result[key_str] = vars::list();
                }
                result[key_str].get<List>().emplace_back(item);
            }
            return var(result);
        }

        // ============ Slice ============
        // Python-like slicing: slice(list, start, end, step)

        inline var slice(const var &container, long long start = 0,
                         std::optional<long long> end_opt = std::nullopt,
                         long long step = 1)
        {
            if (!container.is<List>() && !container.is<std::string>())
            {
                throw std::runtime_error("slice() requires a list or string");
            }

            List result;
            long long size = static_cast<long long>(container.len());

            // Handle negative indices
            if (start < 0)
                start = std::max(0LL, size + start);
            if (start > size)
                start = size;

            long long end = end_opt.value_or(step > 0 ? size : -size - 1);
            if (end < 0)
                end = std::max(0LL, size + end);
            if (end > size)
                end = size;

            if (step == 0)
            {
                throw std::runtime_error("slice step cannot be zero");
            }

            if (container.is<List>())
            {
                const auto &lst = container.get<List>();
                if (step > 0)
                {
                    for (long long i = start; i < end; i += step)
                    {
                        result.push_back(lst[static_cast<size_t>(i)]);
                    }
                }
                else
                {
                    for (long long i = start; i > end; i += step)
                    {
                        result.push_back(lst[static_cast<size_t>(i)]);
                    }
                }
            }
            else
            {
                const auto &str = container.get<std::string>();
                if (step > 0)
                {
                    for (long long i = start; i < end; i += step)
                    {
                        result.push_back(var(std::string(1, str[static_cast<size_t>(i)])));
                    }
                }
                else
                {
                    for (long long i = start; i > end; i += step)
                    {
                        result.push_back(var(std::string(1, str[static_cast<size_t>(i)])));
                    }
                }
            }

            return var(result);
        }

        // ============ Join ============
        // Python-like str.join(iterable)

        inline var join(const var &separator, const var &iterable)
        {
            std::string sep = separator.is<std::string>() ? separator.get<std::string>() : separator.str();
            std::string result;
            bool first = true;
            for (auto item : iterable)
            {
                if (!first)
                    result += sep;
                result += item.str();
                first = false;
            }
            return var(result);
        }

        // ============ Split ============
        // Python-like str.split(separator)

        inline var split(const var &str_var, const var &separator = var(" "))
        {
            std::string str = str_var.is<std::string>() ? str_var.get<std::string>() : str_var.str();
            std::string sep = separator.is<std::string>() ? separator.get<std::string>() : separator.str();

            List result;
            size_t pos = 0;
            size_t found;
            while ((found = str.find(sep, pos)) != std::string::npos)
            {
                result.push_back(var(str.substr(pos, found - pos)));
                pos = found + sep.length();
            }
            result.push_back(var(str.substr(pos)));
            return var(result);
        }

        // ============ Product ============
        // Product of all elements

        template <typename Iterable>
        var product(Iterable &&iterable, var start = var(1))
        {
            var result = start;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result = result * item;
                }
                else
                {
                    result = result * var(item);
                }
            }
            return result;
        }

    } // namespace func
} // namespace pythonic
