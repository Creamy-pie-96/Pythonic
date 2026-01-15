#pragma once

#include <iterator>
#include <tuple>
#include <vector>
#include <utility>
#include <stdexcept>
#include <type_traits>
#include <algorithm>
#include "pythonicVars.hpp"

namespace pythonic
{
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
                    throw std::invalid_argument("range() step argument must not be zero");
                }
            }

            // Iterator class
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
                value_type end_;
                bool forward_;

            public:
                iterator(value_type current, value_type step, value_type end)
                    : current_(current), step_(step), end_(end), forward_(step > 0) {}

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

                bool operator==(const iterator &other) const
                {
                    // Check if we've passed the end
                    if (forward_)
                    {
                        return (current_ >= end_ && other.current_ >= other.end_) ||
                               (current_ == other.current_);
                    }
                    else
                    {
                        return (current_ <= end_ && other.current_ <= other.end_) ||
                               (current_ == other.current_);
                    }
                }

                bool operator!=(const iterator &other) const
                {
                    return !(*this == other);
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

            // Convert to var list
            vars::var to_list() const
            {
                vars::List result;
                for (auto val : *this)
                {
                    result.push_back(vars::var(static_cast<int>(val)));
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
                ContainerIter it_;
                size_t index_;

            public:
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
            iterator end() { return iterator(container_.end(), start_ + std::distance(container_.begin(), container_.end())); }
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
                ContainerIter it_;
                size_t index_;

            public:
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
            iterator end() const { return iterator(container_.end(), start_ + std::distance(container_.begin(), container_.end())); }
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

// for_in(item, container) - cleaner syntax for range-based for
// Usage: for_in(x, my_list) { ... }
#define for_in(var, container) for (auto var : container)

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

        // Convert any iterable to a list
        template <typename Iterable>
        vars::var to_list(Iterable &&iterable)
        {
            vars::List result;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    result.push_back(item);
                }
                else
                {
                    result.push_back(vars::var(item));
                }
            }
            return vars::var(result);
        }

        // sum() - Python-like sum
        template <typename Iterable>
        vars::var sum(Iterable &&iterable, vars::var start = vars::var(0))
        {
            vars::var result = start;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    result = result + item;
                }
                else
                {
                    result = result + vars::var(item);
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
                throw std::runtime_error("min() arg is an empty sequence");
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
                throw std::runtime_error("max() arg is an empty sequence");
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
        bool any(Iterable &&iterable)
        {
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    if (static_cast<bool>(item))
                        return true;
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(item)>, bool>)
                {
                    if (item)
                        return true;
                }
                else
                {
                    if (item)
                        return true;
                }
            }
            return false;
        }

        // all() - True if all elements are truthy
        template <typename Iterable>
        bool all(Iterable &&iterable)
        {
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, vars::var>)
                {
                    if (!static_cast<bool>(item))
                        return false;
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(item)>, bool>)
                {
                    if (!item)
                        return false;
                }
                else
                {
                    if (!item)
                        return false;
                }
            }
            return true;
        }

    } // namespace loop
} // namespace pythonic
