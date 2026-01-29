#pragma once

#include <iterator>
#include <tuple>
#include <vector>
#include <utility>
#include <stdexcept>
#include "pythonicError.hpp"
#include <type_traits>
#include <algorithm>
#include <concepts>
#include <ranges>
#include "pythonicVars.hpp"

namespace pythonic
{
    // ============================================================================
    // TODOs for future improvements:
    // - enumerate and for_enumerate macro are not safe for dynamic containers (pythonic::vars::var/list). Consider a class-based or type-trait-based solution for future.
    // - reverse_view and reversed are not supported for pythonic::vars::var/list. Consider adding bidirectional iterator support or a custom reverse adaptor for dynamic containers.
    // - Refactor all loop macros (for_each, for_index, for_enumerate, etc.) to class-based or constexpr function-based implementations for better type safety and IDE support.
    // ============================================================================

    // ============================================================================
    // C++20 Concepts for Pythonic Containers and Iterables
    // ============================================================================
    // These concepts enable user-defined containers to work seamlessly with
    // pythonic functions like map(), filter(), reduce(), enumerate(), zip().
    //
    // Concept Hierarchy:
    //   Iterable      - Has begin()/end(), can be iterated
    //   Sized         - Has size(), can query length
    //   Container     - Iterable + Sized
    //   Reversible    - Has rbegin()/rend(), can iterate backwards
    //   RandomAccess  - Has operator[], supports index access
    //
    // Also provides std::ranges integration for C++20 range adaptors.
    // ============================================================================

    namespace traits
    {
        // ============ Core Iteration Concepts ============

        /**
         * @brief Concept for types that can be iterated with begin()/end()
         *
         * This uses ADL-friendly std::begin/std::end to support:
         * - Standard containers (vector, list, set, map, etc.)
         * - C-style arrays
         * - User-defined types with begin()/end() methods
         * - Types with free-function begin()/end() via ADL
         */
        template <typename T>
        concept Iterable = requires(T &t) {
            { std::begin(t) } -> std::input_or_output_iterator;
            { std::end(t) } -> std::sentinel_for<decltype(std::begin(t))>;
        };

        /**
         * @brief Concept for types that have a size() method
         *
         * Separate from Iterable because some iterables (generators, views)
         * may not have a knowable size.
         */
        template <typename T>
        concept Sized = requires(T &t) {
            { t.size() } -> std::integral;
        };

        /**
         * @brief Concept for full containers (Iterable + Sized)
         *
         * This is the most common case for standard containers.
         */
        template <typename T>
        concept Container = Iterable<T> && Sized<T>;

        /**
         * @brief Concept for types that can be iterated in reverse
         *
         * Supports reversed() function and reverse iteration patterns.
         */
        template <typename T>
        concept Reversible = requires(T &t) {
            { std::rbegin(t) } -> std::bidirectional_iterator;
            { std::rend(t) } -> std::sentinel_for<decltype(std::rbegin(t))>;
        };

        /**
         * @brief Concept for types supporting index access
         */
        template <typename T>
        concept RandomAccess = requires(T &t, size_t i) {
            { t[i] };
        };

        /**
         * @brief Full-featured container: Iterable + Sized + Reversible + RandomAccess
         */
        template <typename T>
        concept FullContainer = Container<T> && Reversible<T> && RandomAccess<T>;

        // ============ Numeric Concepts ============

        /**
         * @brief Concept for numeric types (integral or floating point)
         */
        template <typename T>
        concept Numeric = std::integral<T> || std::floating_point<T>;

        /**
         * @brief Concept for signed numeric types
         */
        template <typename T>
        concept SignedNumeric = Numeric<T> && std::is_signed_v<T>;

        /**
         * @brief Concept for unsigned numeric types
         */
        template <typename T>
        concept UnsignedNumeric = Numeric<T> && std::is_unsigned_v<T>;

        // ============ Callable Concepts ============

        /**
         * @brief Concept for nullary callables (no arguments)
         */
        template <typename T>
        concept Callable = requires(T &&t) {
            { t() };
        };

        /**
         * @brief Concept for unary callables (one argument)
         */
        template <typename F, typename Arg>
        concept UnaryCallable = requires(F &&f, Arg &&arg) {
            { f(std::forward<Arg>(arg)) };
        };

        /**
         * @brief Concept for binary callables (two arguments)
         */
        template <typename F, typename Arg1, typename Arg2>
        concept BinaryCallable = requires(F &&f, Arg1 &&a1, Arg2 &&a2) {
            { f(std::forward<Arg1>(a1), std::forward<Arg2>(a2)) };
        };

        /**
         * @brief Concept for predicates (returns bool-convertible)
         */
        template <typename F, typename... Args>
        concept Predicate = requires(F &&f, Args &&...args) {
            { f(std::forward<Args>(args)...) } -> std::convertible_to<bool>;
        };

        // ============ std::ranges Integration ============

        /**
         * @brief Concept for C++20 ranges (std::ranges::range)
         */
        template <typename T>
        concept Range = std::ranges::range<T>;

        /**
         * @brief Concept for sized ranges
         */
        template <typename T>
        concept SizedRange = std::ranges::sized_range<T>;

        /**
         * @brief Concept for bidirectional ranges
         */
        template <typename T>
        concept BidirectionalRange = std::ranges::bidirectional_range<T>;

        /**
         * @brief Concept for random access ranges
         */
        template <typename T>
        concept RandomAccessRange = std::ranges::random_access_range<T>;

        /**
         * @brief Concept for contiguous ranges (like std::vector, std::array, std::span)
         */
        template <typename T>
        concept ContiguousRange = std::ranges::contiguous_range<T>;

        /**
         * @brief Concept for viewable ranges (can be converted to a view)
         */
        template <typename T>
        concept ViewableRange = std::ranges::viewable_range<T>;

    } // namespace traits

    // ============================================================================
    // Range Adaptor Helpers (using std::views)
    // ============================================================================
    // These provide pythonic-style wrappers around C++20 range adaptors

    namespace views
    {
        using namespace std::views;

        /**
         * @brief Create a view that takes the first n elements
         *
         * Usage: for (auto x : pythonic::views::take_n(container, 5)) { ... }
         */
        template <traits::Range R>
        auto take_n(R &&r, size_t n)
        {
            return std::forward<R>(r) | std::views::take(n);
        }

        /**
         * @brief Create a view that drops the first n elements
         */
        template <traits::Range R>
        auto drop_n(R &&r, size_t n)
        {
            return std::forward<R>(r) | std::views::drop(n);
        }

        /**
         * @brief Create a filtered view
         */
        template <traits::Range R, typename Pred>
        auto filter_view(R &&r, Pred &&pred)
        {
            return std::forward<R>(r) | std::views::filter(std::forward<Pred>(pred));
        }

        /**
         * @brief Create a transformed view
         */
        template <traits::Range R, typename Func>
        auto transform_view(R &&r, Func &&func)
        {
            return std::forward<R>(r) | std::views::transform(std::forward<Func>(func));
        }

        /**
         * @brief Reverse a range (lazy view, no copy)
         */
        template <traits::BidirectionalRange R>
        auto reverse_view(R &&r)
        {
            return std::forward<R>(r) | std::views::reverse;
        }

        // Note: enumerate_view is defined after the loop namespace (see below)
        // because it depends on pythonic::loop::enumerate

        /**
         * @brief Create a view over a range of integers (like Python's range)
         */
        inline auto iota_view(int start, int end)
        {
            return std::views::iota(start, end);
        }

        inline auto iota_view(int end)
        {
            return std::views::iota(0, end);
        }

    } // namespace views

    namespace loop
    {

        // ============ Range Class ============
        // Python-like range(start, end, step) with support for forward and backward iteration

        class range
        {
        public:
            using value_type = long long;

        private:
            value_type start_;
            value_type end_;
            value_type step_;

        public:
            // range(end) - 0 to end-1
            explicit range(value_type end)
                : start_(0), end_(end), step_(end >= 0 ? 1 : -1) {}

            // range(start, end) - start to end-1 (or end+1 if start > end)
            range(value_type start, value_type end)
                : start_(start), end_(end), step_(start <= end ? 1 : -1) {}

            // range(start, end, step) - with explicit step
            range(value_type start, value_type end, value_type step)
                : start_(start), end_(end), step_(step)
            {
                if (step == 0)
                {
                    throw pythonic::PythonicValueError("range() step argument must not be zero");
                }
            }

            // Iterator class - OPTIMIZED: removed forward_ bool, simplified comparison
            class iterator
            {
            public:
                using difference_type = std::ptrdiff_t;
                using value_type = long long;
                using pointer = const value_type *;
                using reference = value_type;
                using iterator_category = std::forward_iterator_tag;

            private:
                value_type current_;
                value_type step_;

            public:
                iterator() : current_(0), step_(1) {}
                iterator(value_type current, value_type step, value_type /*end*/)
                    : current_(current), step_(step) {}

                reference operator*() const { return current_; }
                pointer operator->() const { return &current_; }

                iterator &operator++()
                {
                    current_ += step_;
                    return *this;
                }

                iterator operator++(int)
                {
                    iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }

                // OPTIMIZED: Simple value comparison - end iterator is pre-computed
                bool operator==(const iterator &other) const
                {
                    return current_ == other.current_;
                }

                bool operator!=(const iterator &other) const
                {
                    return current_ != other.current_;
                }
            };

            iterator begin() const
            {
                return iterator(start_, step_, end_);
            }

            iterator end() const
            {
                // Calculate the "past-the-end" value
                if (step_ > 0)
                {
                    if (start_ >= end_)
                    {
                        return iterator(start_, step_, end_); // Empty range
                    }
                    // Calculate how many steps and the end position
                    value_type steps = (end_ - start_ + step_ - 1) / step_;
                    return iterator(start_ + steps * step_, step_, end_);
                }
                else
                {
                    if (start_ <= end_)
                    {
                        return iterator(start_, step_, end_); // Empty range
                    }
                    // For negative step
                    value_type steps = (start_ - end_ - step_ - 1) / (-step_);
                    return iterator(start_ + steps * step_, step_, end_);
                }
            }

            // Size of range
            size_t size() const
            {
                if (step_ > 0)
                {
                    if (start_ >= end_)
                        return 0;
                    return static_cast<size_t>((end_ - start_ + step_ - 1) / step_);
                }
                else
                {
                    if (start_ <= end_)
                        return 0;
                    return static_cast<size_t>((start_ - end_ - step_ - 1) / (-step_));
                }
            }

            bool empty() const { return size() == 0; }

            // Convert to var list - OPTIMIZED: reserve capacity
            vars::var to_list() const
            {
                vars::List result;
                result.reserve(size());
                for (auto val : *this)
                {
                    result.emplace_back(static_cast<int>(val));
                }
                return vars::var(result);
            }
        };

        // ============ Enumerate ============
        // Python-like enumerate(iterable, start=0)

        template <typename Container>
        class enumerate_wrapper
        {
        private:
            Container &container_;
            size_t start_;

        public:
            enumerate_wrapper(Container &container, size_t start = 0)
                : container_(container), start_(start) {}

            class iterator
            {
            private:
                using ContainerIter = decltype(std::declval<Container>().begin());
                using difference_type = std::ptrdiff_t;
                using value_type = std::pair<size_t, typename std::iterator_traits<ContainerIter>::value_type>;
                using pointer = void; // Input iterator
                using reference = value_type;
                using iterator_category = std::input_iterator_tag;

                ContainerIter it_;
                size_t index_;

            public:
                iterator() : index_(0) {}
                iterator(ContainerIter it, size_t index) : it_(it), index_(index) {}

                auto operator*() const
                {
                    return std::make_pair(index_, *it_);
                }

                iterator &operator++()
                {
                    ++it_;
                    ++index_;
                    return *this;
                }

                bool operator!=(const iterator &other) const
                {
                    return it_ != other.it_;
                }

                bool operator==(const iterator &other) const
                {
                    return it_ == other.it_;
                }
            };

            iterator begin() { return iterator(container_.begin(), start_); }
            // OPTIMIZED: end index not used, avoid std::distance
            iterator end() { return iterator(container_.end(), 0); }
        };

        template <typename Container>
        enumerate_wrapper<Container> enumerate(Container &container, size_t start = 0)
        {
            return enumerate_wrapper<Container>(container, start);
        }

        // For const containers
        template <typename Container>
        class enumerate_wrapper_const
        {
        private:
            const Container &container_;
            size_t start_;

        public:
            enumerate_wrapper_const(const Container &container, size_t start = 0)
                : container_(container), start_(start) {}

            class iterator
            {
            private:
                using ContainerIter = decltype(std::declval<const Container>().begin());
                using difference_type = std::ptrdiff_t;
                using value_type = std::pair<size_t, typename std::iterator_traits<ContainerIter>::value_type>;
                using pointer = void;
                using reference = value_type;
                using iterator_category = std::input_iterator_tag;

                ContainerIter it_;
                size_t index_;

            public:
                iterator() : index_(0) {}
                iterator(ContainerIter it, size_t index) : it_(it), index_(index) {}

                auto operator*() const
                {
                    return std::make_pair(index_, *it_);
                }

                iterator &operator++()
                {
                    ++it_;
                    ++index_;
                    return *this;
                }

                bool operator!=(const iterator &other) const
                {
                    return it_ != other.it_;
                }

                bool operator==(const iterator &other) const
                {
                    return it_ == other.it_;
                }
            };

            iterator begin() const { return iterator(container_.begin(), start_); }
            // OPTIMIZED: end index not used, avoid std::distance
            iterator end() const { return iterator(container_.end(), 0); }
        };

        template <typename Container>
        enumerate_wrapper_const<Container> enumerate(const Container &container, size_t start = 0)
        {
            return enumerate_wrapper_const<Container>(container, start);
        }

        // ============ Zip ============
        // Python-like zip(iterable1, iterable2, ...)

        template <typename... Containers>
        class zip_wrapper
        {
        private:
            std::tuple<Containers &...> containers_;

        public:
            zip_wrapper(Containers &...containers) : containers_(containers...) {}

            class iterator
            {
            public:
                using difference_type = std::ptrdiff_t;
                using value_type = std::tuple<typename std::iterator_traits<decltype(std::declval<Containers>().begin())>::value_type...>;
                using pointer = void;
                using reference = value_type;
                using iterator_category = std::input_iterator_tag;

            private:
                std::tuple<decltype(std::declval<Containers>().begin())...> iterators_;

                template <size_t... Is>
                auto deref_impl(std::index_sequence<Is...>) const
                {
                    return std::make_tuple(*std::get<Is>(iterators_)...);
                }

                template <size_t... Is>
                void increment_impl(std::index_sequence<Is...>)
                {
                    ((++std::get<Is>(iterators_)), ...);
                }

            public:
                iterator() = default;
                iterator(decltype(std::declval<Containers>().begin())... its)
                    : iterators_(its...) {}

                auto operator*() const
                {
                    return deref_impl(std::index_sequence_for<Containers...>{});
                }

                iterator &operator++()
                {
                    increment_impl(std::index_sequence_for<Containers...>{});
                    return *this;
                }

                bool operator!=(const iterator &other) const
                {
                    return iterators_ != other.iterators_;
                }

                bool operator==(const iterator &other) const
                {
                    return iterators_ == other.iterators_;
                }
            };

            auto begin()
            {
                return std::apply([](auto &...containers)
                                  { return iterator(containers.begin()...); }, containers_);
            }

            auto end()
            {
                return std::apply([](auto &...containers)
                                  { return iterator(containers.end()...); }, containers_);
            }
        };

        template <typename... Containers>
        zip_wrapper<Containers...> zip(Containers &...containers)
        {
            return zip_wrapper<Containers...>(containers...);
        }

        // ============ Reversed ============
        // Python-like reversed(iterable)

        template <typename Container>
        class reversed_wrapper
        {
        private:
            Container &container_;

        public:
            reversed_wrapper(Container &container) : container_(container) {}

            auto begin() { return container_.rbegin(); }
            auto end() { return container_.rend(); }
        };

        template <typename Container>
        reversed_wrapper<Container> reversed(Container &container)
        {
            return reversed_wrapper<Container>(container);
        }

        // For const containers
        template <typename Container>
        class reversed_wrapper_const
        {
        private:
            const Container &container_;

        public:
            reversed_wrapper_const(const Container &container) : container_(container) {}

            auto begin() const { return container_.rbegin(); }
            auto end() const { return container_.rend(); }
        };

        template <typename Container>
        reversed_wrapper_const<Container> reversed(const Container &container)
        {
            return reversed_wrapper_const<Container>(container);
        }

        // ============ Iteration helpers for var ============

        // enumerate for var type
        inline enumerate_wrapper<vars::var> enumerate(vars::var &v, size_t start = 0)
        {
            return enumerate_wrapper<vars::var>(v, start);
        }

        inline enumerate_wrapper_const<vars::var> enumerate(const vars::var &v, size_t start = 0)
        {
            return enumerate_wrapper_const<vars::var>(v, start);
        }

// ============ Macros for Python-like syntax ============

// for_each(item, container) - cleaner syntax for range-based for
// Usage: for_each(x, my_list) { ... }
#define for_each(var, container) for (auto var : container)

// for_index(i, container) - loop with index
// Usage: for_index(i, my_list) { ... }
#define for_index(idx, container) \
    for (size_t idx = 0; idx < (container).len(); ++idx)

// for_enumerate(idx, val, container) - enumerate style
// Usage: for_enumerate(i, x, my_list) { ... }
#define for_enumerate(idx, val, container) \
    for (auto [idx, val] : pythonic::loop::enumerate(container))

// for_range(var, ...) - Python-like for i in range(...)
// Usage: for_range(i, 10) { ... }
//        for_range(i, 1, 10) { ... }
//        for_range(i, 1, 10, 2) { ... }
#define for_range(var, ...) for (auto var : pythonic::loop::range(__VA_ARGS__))

// while_true - infinite loop (like Python's while True)
#define while_true while (true)

        // ============ Utility Functions ============

        // len() for range
        inline size_t len(const range &r)
        {
            return r.size();
        }

        // Convert any iterable to a list - OPTIMIZED: use emplace_back
        template <typename Iterable>
        vars::var to_list(Iterable &&iterable)
        {
            vars::List result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    result.emplace_back(item);
                }
                else
                {
                    result.emplace_back(item);
                }
            }
            return vars::var(result);
        }

        // sum() - Python-like sum
        // OPTIMIZED: Use += for in-place update
        template <typename Iterable>
        vars::var sum(Iterable &&iterable, vars::var start = vars::var(0))
        {
            vars::var result = start;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    result += item;
                }
                else
                {
                    result += vars::var(item);
                }
            }
            return result;
        }

        // min() for iterables
        template <typename Iterable>
        vars::var min(Iterable &&iterable)
        {
            auto it = iterable.begin();
            if (it == iterable.end())
            {
                throw pythonic::PythonicValueError("min() arg is an empty sequence");
            }
            vars::var result = *it;
            ++it;
            for (; it != iterable.end(); ++it)
            {
                if (*it < result)
                {
                    result = *it;
                }
            }
            return result;
        }

        // max() for iterables
        template <typename Iterable>
        vars::var max(Iterable &&iterable)
        {
            auto it = iterable.begin();
            if (it == iterable.end())
            {
                throw pythonic::PythonicValueError("max() arg is an empty sequence");
            }
            vars::var result = *it;
            ++it;
            for (; it != iterable.end(); ++it)
            {
                if (result < *it)
                {
                    result = *it;
                }
            }
            return result;
        }

        // any() - True if any element is truthy
        template <typename Iterable>
        vars::var any(Iterable &&iterable)
        {
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    if (static_cast<bool>(item))
                        return vars::var(true);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(item)>, bool>)
                {
                    if (item)
                        return vars::var(true);
                }
                else
                {
                    if (item)
                        return vars::var(true);
                }
            }
            return vars::var(false);
        }

        // all() - True if all elements are truthy
        template <typename Iterable>
        vars::var all(Iterable &&iterable)
        {
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    if (!static_cast<bool>(item))
                        return vars::var(false);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(item)>, bool>)
                {
                    if (!item)
                        return vars::var(false);
                }
                else
                {
                    if (!item)
                        return vars::var(false);
                }
            }
            return vars::var(true);
        }

    } // namespace loop

    // ============================================================================
    // enumerate_view (must be defined after loop::enumerate)
    // ============================================================================
    namespace views
    {
        /**
         * @brief Create an enumerated view (index, value pairs)
         *
         * Note: std::views::enumerate is C++23, so we provide our own for C++20
         */
        template <traits::Range R>
        auto enumerate_view(R &&r, size_t start = 0)
        {
            return pythonic::loop::enumerate(std::forward<R>(r), start);
        }
    } // namespace views

} // namespace pythonic
