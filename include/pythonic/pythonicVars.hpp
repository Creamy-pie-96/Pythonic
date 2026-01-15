#pragma once

#include <string>
#include <variant>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <climits>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace pythonic
{
    namespace vars
    {

        // Forward declaration
        class var;

        // Type aliases for containers
        using List = std::vector<var>;
        using Set = std::set<var>;
        using Dict = std::unordered_map<std::string, var>;

        // Helper to check if type is a container
        template <typename T>
        struct is_container : std::false_type
        {
        };
        template <>
        struct is_container<List> : std::true_type
        {
        };
        template <>
        struct is_container<Set> : std::true_type
        {
        };
        template <>
        struct is_container<Dict> : std::true_type
        {
        };

        // Helper to convert value to string safely
        template <typename T>
        std::string to_str(const T &val)
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                return val;
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return val ? "True" : "False";
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                return std::to_string(val);
            }
            else
            {
                return "[container]";
            }
        }

        // NoneType for Python-like None
        struct NoneType
        {
            bool operator<(const NoneType &) const { return false; }
            bool operator==(const NoneType &) const { return true; }
            bool operator!=(const NoneType &) const { return false; }
        };

        // The main variant type (primitives only, containers added after var is defined)
        using varType = std::variant<
            NoneType,
            int, float, std::string, bool, double,
            long, long long, long double,
            unsigned int, unsigned long, unsigned long long,
            List, Set, Dict>;

        class var
        {
        private:
            varType value;

        public:
            // Constructors
            var() : value(0) {}
            var(const varType &v) : value(v) {}
            var(const char *s) : value(std::string(s)) {}
            var(NoneType) : value(NoneType{}) {} // Constructor for None

            // Template constructor for arithmetic types
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var(T v) : value(v) {}

            // Check if this var is None
            bool isNone() const { return std::holds_alternative<NoneType>(value); }

            // Accessors
            const varType &getValue() const { return value; }
            void setValue(const varType &v) { this->value = v; }

            // Type checking
            template <typename T>
            bool is() const
            {
                return std::holds_alternative<T>(value);
            }

            // Type name - returns string like "int", "str", "list", "dict", "NoneType" etc.
            std::string type() const
            {
                return std::visit([](auto &&arg) -> std::string
                                  {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, NoneType>) {
                        return "NoneType";
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        return "str";
                    } else if constexpr (std::is_same_v<T, bool>) {
                        return "bool";
                    } else if constexpr (std::is_same_v<T, int>) {
                        return "int";
                    } else if constexpr (std::is_same_v<T, float>) {
                        return "float";
                    } else if constexpr (std::is_same_v<T, double>) {
                        return "double";
                    } else if constexpr (std::is_same_v<T, long>) {
                        return "long";
                    } else if constexpr (std::is_same_v<T, long long>) {
                        return "long long";
                    } else if constexpr (std::is_same_v<T, long double>) {
                        return "long double";
                    } else if constexpr (std::is_same_v<T, unsigned int>) {
                        return "unsigned int";
                    } else if constexpr (std::is_same_v<T, unsigned long>) {
                        return "unsigned long";
                    } else if constexpr (std::is_same_v<T, unsigned long long>) {
                        return "unsigned long long";
                    } else if constexpr (std::is_same_v<T, List>) {
                        return "list";
                    } else if constexpr (std::is_same_v<T, Set>) {
                        return "set";
                    } else if constexpr (std::is_same_v<T, Dict>) {
                        return "dict";
                    } else {
                        return "unknown";
                    } }, value);
            }

            // Get value as specific type
            template <typename T>
            T &get()
            {
                return std::get<T>(value);
            }

            template <typename T>
            const T &get() const
            {
                return std::get<T>(value);
            }

            // String conversion
            std::string str() const
            {
                return std::visit([](auto &&arg) -> std::string
                                  {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, NoneType>) {
                        return "None";
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        return arg;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        return arg ? "True" : "False";
                    } else if constexpr (std::is_arithmetic_v<T>) {
                        std::ostringstream ss;
                        ss << arg;
                        return ss.str();
                    } else if constexpr (std::is_same_v<T, List>) {
                        std::ostringstream ss;
                        ss << "[";
                        for (size_t i = 0; i < arg.size(); ++i) {
                            if (i > 0) ss << ", ";
                            ss << arg[i].str();
                        }
                        ss << "]";
                        return ss.str();
                    } else if constexpr (std::is_same_v<T, Set>) {
                        std::ostringstream ss;
                        ss << "{";
                        bool first = true;
                        for (const auto& item : arg) {
                            if (!first) ss << ", ";
                            ss << item.str();
                            first = false;
                        }
                        ss << "}";
                        return ss.str();
                    } else if constexpr (std::is_same_v<T, Dict>) {
                        std::ostringstream ss;
                        ss << "{";
                        bool first = true;
                        for (const auto& [k, v] : arg) {
                            if (!first) ss << ", ";
                            ss << "\"" << k << "\": " << v.str();
                            first = false;
                        }
                        ss << "}";
                        return ss.str();
                    } else {
                        return "[unknown]";
                    } }, value);
            }

            // Pretty string with indentation (for pprint)
            std::string pretty_str(int indent = 0, int indent_step = 2) const
            {
                std::string ind(indent, ' ');
                std::string inner_ind(indent + indent_step, ' ');

                return std::visit([&](auto &&arg) -> std::string
                                  {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        return "\"" + arg + "\"";
                    } else if constexpr (std::is_same_v<T, bool>) {
                        return arg ? "True" : "False";
                    } else if constexpr (std::is_arithmetic_v<T>) {
                        std::ostringstream ss;
                        ss << arg;
                        return ss.str();
                    } else if constexpr (std::is_same_v<T, List>) {
                        if (arg.empty()) return "[]";
                        std::ostringstream ss;
                        ss << "[\n";
                        for (size_t i = 0; i < arg.size(); ++i) {
                            ss << inner_ind << arg[i].pretty_str(indent + indent_step, indent_step);
                            if (i < arg.size() - 1) ss << ",";
                            ss << "\n";
                        }
                        ss << ind << "]";
                        return ss.str();
                    } else if constexpr (std::is_same_v<T, Set>) {
                        if (arg.empty()) return "{}";
                        std::ostringstream ss;
                        ss << "{\n";
                        size_t i = 0;
                        for (const auto& item : arg) {
                            ss << inner_ind << item.pretty_str(indent + indent_step, indent_step);
                            if (i < arg.size() - 1) ss << ",";
                            ss << "\n";
                            ++i;
                        }
                        ss << ind << "}";
                        return ss.str();
                    } else if constexpr (std::is_same_v<T, Dict>) {
                        if (arg.empty()) return "{}";
                        std::ostringstream ss;
                        ss << "{\n";
                        size_t i = 0;
                        for (const auto& [k, v] : arg) {
                            ss << inner_ind << "\"" << k << "\": " 
                               << v.pretty_str(indent + indent_step, indent_step);
                            if (i < arg.size() - 1) ss << ",";
                            ss << "\n";
                            ++i;
                        }
                        ss << ind << "}";
                        return ss.str();
                    } else {
                        return "[unknown]";
                    } }, value);
            }

            friend std::ostream &operator<<(std::ostream &os, const var &v)
            {
                os << v.str();
                return os;
            }

            // Comparison operator for use in containers (std::set)
            bool operator<(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> bool
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_same_v<A, B>) {
                        if constexpr (std::is_arithmetic_v<A> || std::is_same_v<A, std::string>) {
                            return a < b;
                        } else {
                            // Containers: compare by size then element-wise
                            return false; // fallback
                        }
                    } else if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return a < b;
                    } else {
                        // Different types: compare by type index
                        return std::variant<A>(a).index() < std::variant<B>(b).index();
                    } }, value, other.getValue());
            }

            // Arithmetic operators
            var operator+(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    
                    // Both arithmetic
                    if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a + b);
                    }
                    // String concatenation
                    else if constexpr (std::is_same_v<A, std::string> && std::is_same_v<B, std::string>) {
                        return var(a + b);
                    }
                    // String + arithmetic -> string concat
                    else if constexpr (std::is_same_v<A, std::string> && std::is_arithmetic_v<B>) {
                        return var(a + to_str(b));
                    }
                    else if constexpr (std::is_arithmetic_v<A> && std::is_same_v<B, std::string>) {
                        return var(to_str(a) + b);
                    }
                    // List concatenation
                    else if constexpr (std::is_same_v<A, List> && std::is_same_v<B, List>) {
                        List result = a;
                        result.insert(result.end(), b.begin(), b.end());
                        return var(result);
                    }
                    else {
                        throw std::runtime_error("Unsupported types for addition");
                    } }, value, other.getValue());
            }

            var operator-(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    // Arithmetic subtraction
                    if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a - b);
                    }
                    // Set difference
                    else if constexpr (std::is_same_v<A, Set> && std::is_same_v<B, Set>) {
                        Set result;
                        for (const auto& item : a) {
                            if (b.find(item) == b.end()) {
                                result.insert(item);
                            }
                        }
                        return var(result);
                    }
                    // List difference (remove all occurrences of items in second list)
                    else if constexpr (std::is_same_v<A, List> && std::is_same_v<B, List>) {
                        List result;
                        Set b_set(b.begin(), b.end());
                        for (const auto& item : a) {
                            if (b_set.find(item) == b_set.end()) {
                                result.push_back(item);
                            }
                        }
                        return var(result);
                    }
                    // Dict difference (keys in first but not in second)
                    else if constexpr (std::is_same_v<A, Dict> && std::is_same_v<B, Dict>) {
                        Dict result;
                        for (const auto& [key, value] : a) {
                            if (b.find(key) == b.end()) {
                                result[key] = value;
                            }
                        }
                        return var(result);
                    }
                    else {
                        throw std::runtime_error("operator- requires arithmetic types or containers (difference)");
                    } }, value, other.getValue());
            }

            var operator*(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a * b);
                    }
                    // String repetition: "abc" * 3
                    else if constexpr (std::is_same_v<A, std::string> && std::is_integral_v<B> && !std::is_same_v<B, bool>) {
                        std::string result;
                        for (B i = 0; i < b; ++i) result += a;
                        return var(result);
                    }
                    // List repetition: [1,2] * 3
                    else if constexpr (std::is_same_v<A, List> && std::is_integral_v<B> && !std::is_same_v<B, bool>) {
                        List result;
                        for (B i = 0; i < b; ++i) {
                            result.insert(result.end(), a.begin(), a.end());
                        }
                        return var(result);
                    }
                    else {
                        throw std::runtime_error("Unsupported types for multiplication");
                    } }, value, other.getValue());
            }

            var operator/(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        if (b == 0) throw std::runtime_error("Division by zero");
                        return var(a / b);
                    } else {
                        throw std::runtime_error("Unsupported types for division");
                    } }, value, other.getValue());
            }

            var operator%(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
                        if (b == 0) throw std::runtime_error("Modulo by zero");
                        return var(a % b);
                    } else {
                        throw std::runtime_error("Unsupported types for modulo");
                    } }, value, other.getValue());
            }

            // Comparison operators (return var(bool) for Pythonic style)
            var operator==(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_same_v<A, B>) {
                        if constexpr (std::is_arithmetic_v<A> || std::is_same_v<A, std::string>) {
                            return var(a == b);
                        } else {
                            return var(false); // containers
                        }
                    } else if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a == b);
                    } else {
                        return var(false);
                    } }, value, other.getValue());
            }

            var operator!=(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_same_v<A, B>) {
                        if constexpr (std::is_arithmetic_v<A> || std::is_same_v<A, std::string>) {
                            return var(a != b);
                        } else {
                            return var(true); // containers
                        }
                    } else if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a != b);
                    } else {
                        return var(true);
                    } }, value, other.getValue());
            }

            var operator>(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a > b);
                    } else if constexpr (std::is_same_v<A, std::string> && std::is_same_v<B, std::string>) {
                        return var(a > b);
                    } else {
                        throw std::runtime_error("Unsupported types for comparison");
                    } }, value, other.getValue());
            }

            var operator>=(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a >= b);
                    } else if constexpr (std::is_same_v<A, std::string> && std::is_same_v<B, std::string>) {
                        return var(a >= b);
                    } else {
                        throw std::runtime_error("Unsupported types for comparison");
                    } }, value, other.getValue());
            }

            var operator<=(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        return var(a <= b);
                    } else if constexpr (std::is_same_v<A, std::string> && std::is_same_v<B, std::string>) {
                        return var(a <= b);
                    } else {
                        throw std::runtime_error("Unsupported types for comparison");
                    } }, value, other.getValue());
            }

            // Compound assignment operators
            var &operator+=(const var &other)
            {
                *this = *this + other;
                return *this;
            }
            var &operator-=(const var &other)
            {
                *this = *this - other;
                return *this;
            }
            var &operator*=(const var &other)
            {
                *this = *this * other;
                return *this;
            }
            var &operator/=(const var &other)
            {
                *this = *this / other;
                return *this;
            }
            var &operator%=(const var &other)
            {
                *this = *this % other;
                return *this;
            }

            // ============ Implicit Conversion Operators for Arithmetic Types ============
            // These eliminate the need to wrap literals in var() - e.g., var(x) + 2 just works

            // Addition: supports arithmetic and string concatenation
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator+(T other) const { return *this + var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator+(T lhs, const var &rhs) { return var(lhs) + rhs; }

            // String concatenation with const char*
            var operator+(const char *other) const { return *this + var(other); }
            friend var operator+(const char *lhs, const var &rhs) { return var(lhs) + rhs; }

            // Subtraction: arithmetic only
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator-(T other) const { return *this - var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator-(T lhs, const var &rhs) { return var(lhs) - rhs; }

            // Multiplication: arithmetic and string/list repetition with integers
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator*(T other) const { return *this * var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator*(T lhs, const var &rhs) { return var(lhs) * rhs; }

            // Division: arithmetic only
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator/(T other) const { return *this / var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator/(T lhs, const var &rhs) { return var(lhs) / rhs; }

            // Modulo: integral only
            template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
            var operator%(T other) const { return *this % var(other); }

            template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
            friend var operator%(T lhs, const var &rhs) { return var(lhs) % rhs; }

            // Comparison operators with implicit conversion
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator==(T other) const { return *this == var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator==(T lhs, const var &rhs) { return var(lhs) == rhs; }

            var operator==(const char *other) const { return *this == var(other); }
            friend var operator==(const char *lhs, const var &rhs) { return var(lhs) == rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator!=(T other) const { return *this != var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator!=(T lhs, const var &rhs) { return var(lhs) != rhs; }

            var operator!=(const char *other) const { return *this != var(other); }
            friend var operator!=(const char *lhs, const var &rhs) { return var(lhs) != rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator>(T other) const { return *this > var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator>(T lhs, const var &rhs) { return var(lhs) > rhs; }

            var operator>(const char *other) const { return *this > var(other); }
            friend var operator>(const char *lhs, const var &rhs) { return var(lhs) > rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator>=(T other) const { return *this >= var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator>=(T lhs, const var &rhs) { return var(lhs) >= rhs; }

            var operator>=(const char *other) const { return *this >= var(other); }
            friend var operator>=(const char *lhs, const var &rhs) { return var(lhs) >= rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator<(T other) const { return *this < var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator<(T lhs, const var &rhs) { return var(lhs) < rhs; }

            var operator<(const char *other) const { return *this < var(other); }
            friend var operator<(const char *lhs, const var &rhs) { return var(lhs) < rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator<=(T other) const { return *this <= var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator<=(T lhs, const var &rhs) { return var(lhs) <= rhs; }

            var operator<=(const char *other) const { return *this <= var(other); }
            friend var operator<=(const char *lhs, const var &rhs) { return var(lhs) <= rhs; }

            // Compound assignment with implicit conversion
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var &operator+=(T other) { return *this += var(other); }

            var &operator+=(const char *other) { return *this += var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var &operator-=(T other) { return *this -= var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var &operator*=(T other) { return *this *= var(other); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var &operator/=(T other) { return *this /= var(other); }

            template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
            var &operator%=(T other) { return *this %= var(other); }

            // Logical operators
            var operator&&(const var &other) const
            {
                return var(static_cast<bool>(*this) && static_cast<bool>(other));
            }

            var operator||(const var &other) const
            {
                return var(static_cast<bool>(*this) || static_cast<bool>(other));
            }

            var operator!() const
            {
                return var(!static_cast<bool>(*this));
            }

            // Bitwise operators (only for integral types)
            var operator~() const
            {
                return std::visit([](auto &&a) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    if constexpr (std::is_integral_v<A> && !std::is_same_v<A, bool>) {
                        return var(~a);
                    } else {
                        throw std::runtime_error("Bitwise NOT requires integral type");
                    } }, value);
            }

            var operator&(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    // Bitwise AND for integers
                    if constexpr (std::is_integral_v<A> && std::is_integral_v<B> && 
                                  !std::is_same_v<A, bool> && !std::is_same_v<B, bool>) {
                        return var(a & b);
                    }
                    // Set intersection
                    else if constexpr (std::is_same_v<A, Set> && std::is_same_v<B, Set>) {
                        Set result;
                        for (const auto& item : a) {
                            if (b.find(item) != b.end()) {
                                result.insert(item);
                            }
                        }
                        return var(result);
                    }
                    // List intersection (preserves order from first list)
                    else if constexpr (std::is_same_v<A, List> && std::is_same_v<B, List>) {
                        List result;
                        Set b_set(b.begin(), b.end());
                        for (const auto& item : a) {
                            if (b_set.find(item) != b_set.end()) {
                                result.push_back(item);
                            }
                        }
                        return var(result);
                    }
                    // Dict intersection (keys present in both, values from first)
                    else if constexpr (std::is_same_v<A, Dict> && std::is_same_v<B, Dict>) {
                        Dict result;
                        for (const auto& [key, value] : a) {
                            if (b.find(key) != b.end()) {
                                result[key] = value;
                            }
                        }
                        return var(result);
                    }
                    else {
                        throw std::runtime_error("operator& requires integral types (bitwise) or containers (intersection)");
                    } }, value, other.getValue());
            }

            var operator|(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    // Bitwise OR for integers
                    if constexpr (std::is_integral_v<A> && std::is_integral_v<B> && 
                                  !std::is_same_v<A, bool> && !std::is_same_v<B, bool>) {
                        return var(a | b);
                    }
                    // Set union
                    else if constexpr (std::is_same_v<A, Set> && std::is_same_v<B, Set>) {
                        Set result = a;
                        result.insert(b.begin(), b.end());
                        return var(result);
                    }
                    // List concatenation
                    else if constexpr (std::is_same_v<A, List> && std::is_same_v<B, List>) {
                        List result = a;
                        result.insert(result.end(), b.begin(), b.end());
                        return var(result);
                    }
                    // Dict merge (right overwrites left for same keys)
                    else if constexpr (std::is_same_v<A, Dict> && std::is_same_v<B, Dict>) {
                        Dict result = a;
                        for (const auto& [key, value] : b) {
                            result[key] = value;
                        }
                        return var(result);
                    }
                    else {
                        throw std::runtime_error("operator| requires integral types (bitwise) or containers (union/merge)");
                    } }, value, other.getValue());
            }

            var operator^(const var &other) const
            {
                return std::visit([](auto &&a, auto &&b) -> var
                                  {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;
                    // Bitwise XOR for integers
                    if constexpr (std::is_integral_v<A> && std::is_integral_v<B> && 
                                  !std::is_same_v<A, bool> && !std::is_same_v<B, bool>) {
                        return var(a ^ b);
                    }
                    // Set symmetric difference (elements in either but not both)
                    else if constexpr (std::is_same_v<A, Set> && std::is_same_v<B, Set>) {
                        Set result;
                        // Items in a but not b
                        for (const auto& item : a) {
                            if (b.find(item) == b.end()) {
                                result.insert(item);
                            }
                        }
                        // Items in b but not a
                        for (const auto& item : b) {
                            if (a.find(item) == a.end()) {
                                result.insert(item);
                            }
                        }
                        return var(result);
                    }
                    // List symmetric difference
                    else if constexpr (std::is_same_v<A, List> && std::is_same_v<B, List>) {
                        List result;
                        Set a_set(a.begin(), a.end());
                        Set b_set(b.begin(), b.end());
                        // Items in a but not b
                        for (const auto& item : a) {
                            if (b_set.find(item) == b_set.end()) {
                                result.push_back(item);
                            }
                        }
                        // Items in b but not a
                        for (const auto& item : b) {
                            if (a_set.find(item) == a_set.end()) {
                                result.push_back(item);
                            }
                        }
                        return var(result);
                    }
                    else {
                        throw std::runtime_error("operator^ requires integral types (bitwise) or sets/lists (symmetric difference)");
                    } }, value, other.getValue());
            }

            // Bool conversion
            operator bool() const
            {
                return std::visit([](auto &&val) -> bool
                                  {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        return val;
                    } else if constexpr (std::is_arithmetic_v<T>) {
                        return val != 0;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        return !val.empty();
                    } else if constexpr (std::is_same_v<T, List> || std::is_same_v<T, Set> || std::is_same_v<T, Dict>) {
                        return !val.empty();
                    } else {
                        return true;
                    } }, value);
            }

            // Container access - operator[] for list and dict
            var &operator[](size_t index)
            {
                if (auto *lst = std::get_if<List>(&value))
                {
                    if (index >= lst->size())
                    {
                        throw std::out_of_range("List index out of range");
                    }
                    return (*lst)[index];
                }
                throw std::runtime_error("operator[size_t] requires a list");
            }

            const var &operator[](size_t index) const
            {
                if (auto *lst = std::get_if<List>(&value))
                {
                    if (index >= lst->size())
                    {
                        throw std::out_of_range("List index out of range");
                    }
                    return (*lst)[index];
                }
                throw std::runtime_error("operator[size_t] requires a list");
            }

            var &operator[](const std::string &key)
            {
                if (auto *dct = std::get_if<Dict>(&value))
                {
                    return (*dct)[key];
                }
                throw std::runtime_error("operator[string] requires a dict");
            }

            var &operator[](const char *key)
            {
                return (*this)[std::string(key)];
            }

            // len() support
            size_t len() const
            {
                return std::visit([](auto &&val) -> size_t
                                  {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        return val.size();
                    } else if constexpr (std::is_same_v<T, List> || std::is_same_v<T, Set> || std::is_same_v<T, Dict>) {
                        return val.size();
                    } else {
                        throw std::runtime_error("len() not supported for this type");
                    } }, value);
            }

            // append for list
            void append(const var &v)
            {
                if (auto *lst = std::get_if<List>(&value))
                {
                    lst->push_back(v);
                }
                else
                {
                    throw std::runtime_error("append() requires a list");
                }
            }

            // add for set
            void add(const var &v)
            {
                if (auto *st = std::get_if<Set>(&value))
                {
                    st->insert(v);
                }
                else
                {
                    throw std::runtime_error("add() requires a set");
                }
            }

            // extend for list - adds all elements from another iterable
            void extend(const var &other)
            {
                if (auto *lst = std::get_if<List>(&value))
                {
                    if (auto *other_lst = std::get_if<List>(&other.getValue()))
                    {
                        lst->insert(lst->end(), other_lst->begin(), other_lst->end());
                    }
                    else if (auto *other_set = std::get_if<Set>(&other.getValue()))
                    {
                        for (const auto &item : *other_set)
                        {
                            lst->push_back(item);
                        }
                    }
                    else if (auto *other_str = std::get_if<std::string>(&other.getValue()))
                    {
                        // Extend list with characters from string
                        for (char c : *other_str)
                        {
                            lst->push_back(var(std::string(1, c)));
                        }
                    }
                    else
                    {
                        throw std::runtime_error("extend() requires an iterable (list, set, or string)");
                    }
                }
                else
                {
                    throw std::runtime_error("extend() requires a list");
                }
            }

            // update for set - adds all elements from another iterable (like Python's set.update())
            void update(const var &other)
            {
                if (auto *st = std::get_if<Set>(&value))
                {
                    if (auto *other_set = std::get_if<Set>(&other.getValue()))
                    {
                        st->insert(other_set->begin(), other_set->end());
                    }
                    else if (auto *other_lst = std::get_if<List>(&other.getValue()))
                    {
                        for (const auto &item : *other_lst)
                        {
                            st->insert(item);
                        }
                    }
                    else
                    {
                        throw std::runtime_error("update() requires an iterable (set or list)");
                    }
                }
                else
                {
                    throw std::runtime_error("update() requires a set");
                }
            }

            // contains check (in operator simulation)
            bool contains(const var &v) const
            {
                return std::visit([&v](auto &&val) -> bool
                                  {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, List>) {
                        for (const auto& item : val) {
                            if (static_cast<bool>(item == v)) return true;
                        }
                        return false;
                    } else if constexpr (std::is_same_v<T, Set>) {
                        return val.find(v) != val.end();
                    } else if constexpr (std::is_same_v<T, Dict>) {
                        if (v.is<std::string>()) {
                            return val.find(v.get<std::string>()) != val.end();
                        }
                        return false;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        if (v.is<std::string>()) {
                            return val.find(v.get<std::string>()) != std::string::npos;
                        }
                        return false;
                    } else {
                        return false;
                    } }, value);
            }

            // ============ Iterator Support ============

            // Generic iterator wrapper that works with different container types
            class iterator
            {
            public:
                using difference_type = std::ptrdiff_t;
                using value_type = var;
                using pointer = var *;
                using reference = var &;
                using iterator_category = std::forward_iterator_tag;

            private:
                std::variant<
                    List::iterator,
                    Set::iterator,
                    Dict::iterator,
                    std::string::iterator>
                    it;
                enum class IterType
                {
                    LIST,
                    SET,
                    DICT,
                    STRING
                } type;

            public:
                iterator() : type(IterType::LIST) {}
                iterator(List::iterator i) : it(i), type(IterType::LIST) {}
                iterator(Set::iterator i) : it(i), type(IterType::SET) {}
                iterator(Dict::iterator i) : it(i), type(IterType::DICT) {}
                iterator(std::string::iterator i) : it(i), type(IterType::STRING) {}

                var operator*() const
                {
                    switch (type)
                    {
                    case IterType::LIST:
                        return *std::get<List::iterator>(it);
                    case IterType::SET:
                        return *std::get<Set::iterator>(it);
                    case IterType::DICT:
                    {
                        auto &p = *std::get<Dict::iterator>(it);
                        return var(p.first); // Return key for dict iteration
                    }
                    case IterType::STRING:
                        return var(std::string(1, *std::get<std::string::iterator>(it)));
                    }
                    return var();
                }

                iterator &operator++()
                {
                    switch (type)
                    {
                    case IterType::LIST:
                        ++std::get<List::iterator>(it);
                        break;
                    case IterType::SET:
                        ++std::get<Set::iterator>(it);
                        break;
                    case IterType::DICT:
                        ++std::get<Dict::iterator>(it);
                        break;
                    case IterType::STRING:
                        ++std::get<std::string::iterator>(it);
                        break;
                    }
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
                    if (type != other.type)
                        return false;
                    switch (type)
                    {
                    case IterType::LIST:
                        return std::get<List::iterator>(it) == std::get<List::iterator>(other.it);
                    case IterType::SET:
                        return std::get<Set::iterator>(it) == std::get<Set::iterator>(other.it);
                    case IterType::DICT:
                        return std::get<Dict::iterator>(it) == std::get<Dict::iterator>(other.it);
                    case IterType::STRING:
                        return std::get<std::string::iterator>(it) == std::get<std::string::iterator>(other.it);
                    }
                    return false;
                }

                bool operator!=(const iterator &other) const { return !(*this == other); }
            };

            class const_iterator
            {
            public:
                using difference_type = std::ptrdiff_t;
                using value_type = var;
                using pointer = const var *;
                using reference = const var &;
                using iterator_category = std::forward_iterator_tag;

            private:
                std::variant<
                    List::const_iterator,
                    Set::const_iterator,
                    Dict::const_iterator,
                    std::string::const_iterator>
                    it;
                enum class IterType
                {
                    LIST,
                    SET,
                    DICT,
                    STRING
                } type;

            public:
                const_iterator() : type(IterType::LIST) {}
                const_iterator(List::const_iterator i) : it(i), type(IterType::LIST) {}
                const_iterator(Set::const_iterator i) : it(i), type(IterType::SET) {}
                const_iterator(Dict::const_iterator i) : it(i), type(IterType::DICT) {}
                const_iterator(std::string::const_iterator i) : it(i), type(IterType::STRING) {}

                var operator*() const
                {
                    switch (type)
                    {
                    case IterType::LIST:
                        return *std::get<List::const_iterator>(it);
                    case IterType::SET:
                        return *std::get<Set::const_iterator>(it);
                    case IterType::DICT:
                    {
                        auto &p = *std::get<Dict::const_iterator>(it);
                        return var(p.first);
                    }
                    case IterType::STRING:
                        return var(std::string(1, *std::get<std::string::const_iterator>(it)));
                    }
                    return var();
                }

                const_iterator &operator++()
                {
                    switch (type)
                    {
                    case IterType::LIST:
                        ++std::get<List::const_iterator>(it);
                        break;
                    case IterType::SET:
                        ++std::get<Set::const_iterator>(it);
                        break;
                    case IterType::DICT:
                        ++std::get<Dict::const_iterator>(it);
                        break;
                    case IterType::STRING:
                        ++std::get<std::string::const_iterator>(it);
                        break;
                    }
                    return *this;
                }

                const_iterator operator++(int)
                {
                    const_iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }

                bool operator==(const const_iterator &other) const
                {
                    if (type != other.type)
                        return false;
                    switch (type)
                    {
                    case IterType::LIST:
                        return std::get<List::const_iterator>(it) == std::get<List::const_iterator>(other.it);
                    case IterType::SET:
                        return std::get<Set::const_iterator>(it) == std::get<Set::const_iterator>(other.it);
                    case IterType::DICT:
                        return std::get<Dict::const_iterator>(it) == std::get<Dict::const_iterator>(other.it);
                    case IterType::STRING:
                        return std::get<std::string::const_iterator>(it) == std::get<std::string::const_iterator>(other.it);
                    }
                    return false;
                }

                bool operator!=(const const_iterator &other) const { return !(*this == other); }
            };

            // begin/end for range-based for loops
            iterator begin()
            {
                if (auto *lst = std::get_if<List>(&value))
                    return iterator(lst->begin());
                if (auto *st = std::get_if<Set>(&value))
                    return iterator(st->begin());
                if (auto *dct = std::get_if<Dict>(&value))
                    return iterator(dct->begin());
                if (auto *s = std::get_if<std::string>(&value))
                    return iterator(s->begin());
                throw std::runtime_error("Type is not iterable");
            }

            iterator end()
            {
                if (auto *lst = std::get_if<List>(&value))
                    return iterator(lst->end());
                if (auto *st = std::get_if<Set>(&value))
                    return iterator(st->end());
                if (auto *dct = std::get_if<Dict>(&value))
                    return iterator(dct->end());
                if (auto *s = std::get_if<std::string>(&value))
                    return iterator(s->end());
                throw std::runtime_error("Type is not iterable");
            }

            const_iterator begin() const
            {
                if (auto *lst = std::get_if<List>(&value))
                    return const_iterator(lst->begin());
                if (auto *st = std::get_if<Set>(&value))
                    return const_iterator(st->begin());
                if (auto *dct = std::get_if<Dict>(&value))
                    return const_iterator(dct->begin());
                if (auto *s = std::get_if<std::string>(&value))
                    return const_iterator(s->begin());
                throw std::runtime_error("Type is not iterable");
            }

            const_iterator end() const
            {
                if (auto *lst = std::get_if<List>(&value))
                    return const_iterator(lst->end());
                if (auto *st = std::get_if<Set>(&value))
                    return const_iterator(st->end());
                if (auto *dct = std::get_if<Dict>(&value))
                    return const_iterator(dct->end());
                if (auto *s = std::get_if<std::string>(&value))
                    return const_iterator(s->end());
                throw std::runtime_error("Type is not iterable");
            }

            const_iterator cbegin() const { return begin(); }
            const_iterator cend() const { return end(); }

            // items() for dict - returns list of [key, value] pairs
            var items() const
            {
                if (auto *dct = std::get_if<Dict>(&value))
                {
                    List result;
                    for (const auto &[k, v] : *dct)
                    {
                        // Create a [key, value] list pair manually
                        List pair;
                        pair.push_back(var(k));
                        pair.push_back(v);
                        result.push_back(var(pair));
                    }
                    return var(result);
                }
                throw std::runtime_error("items() requires a dict");
            }

            // keys() for dict
            var keys() const
            {
                if (auto *dct = std::get_if<Dict>(&value))
                {
                    List result;
                    for (const auto &[k, v] : *dct)
                    {
                        result.push_back(var(k));
                    }
                    return var(result);
                }
                throw std::runtime_error("keys() requires a dict");
            }

            // values() for dict
            var values() const
            {
                if (auto *dct = std::get_if<Dict>(&value))
                {
                    List result;
                    for (const auto &[k, v] : *dct)
                    {
                        result.push_back(v);
                    }
                    return var(result);
                }
                throw std::runtime_error("values() requires a dict");
            }

            // ============ Slicing Support ============
            // Python-like slicing: var.slice(start, end, step)
            // Supports negative indices like Python

            var slice(long long start = 0, long long end = LLONG_MAX, long long step = 1) const
            {
                if (step == 0)
                {
                    throw std::invalid_argument("slice step cannot be zero");
                }

                // For List
                if (auto *lst = std::get_if<List>(&value))
                {
                    long long size = static_cast<long long>(lst->size());

                    // Handle negative indices
                    if (start < 0)
                        start = std::max(0LL, size + start);
                    if (end < 0)
                        end = std::max(0LL, size + end);
                    if (end == LLONG_MAX)
                        end = size;

                    // Clamp to valid range
                    start = std::max(0LL, std::min(start, size));
                    end = std::max(0LL, std::min(end, size));

                    List result;
                    if (step > 0)
                    {
                        for (long long i = start; i < end; i += step)
                        {
                            result.push_back((*lst)[static_cast<size_t>(i)]);
                        }
                    }
                    else
                    {
                        // Negative step - reverse iteration
                        if (start == 0 && end == size)
                        {
                            start = size - 1;
                            end = -1;
                        }
                        for (long long i = start; i > end; i += step)
                        {
                            if (i >= 0 && i < size)
                            {
                                result.push_back((*lst)[static_cast<size_t>(i)]);
                            }
                        }
                    }
                    return var(result);
                }

                // For String
                if (auto *s = std::get_if<std::string>(&value))
                {
                    long long size = static_cast<long long>(s->size());

                    // Handle negative indices
                    if (start < 0)
                        start = std::max(0LL, size + start);
                    if (end < 0)
                        end = std::max(0LL, size + end);
                    if (end == LLONG_MAX)
                        end = size;

                    // Clamp to valid range
                    start = std::max(0LL, std::min(start, size));
                    end = std::max(0LL, std::min(end, size));

                    std::string result;
                    if (step > 0)
                    {
                        for (long long i = start; i < end; i += step)
                        {
                            result += (*s)[static_cast<size_t>(i)];
                        }
                    }
                    else
                    {
                        // Negative step - reverse iteration
                        if (start == 0 && end == size)
                        {
                            start = size - 1;
                            end = -1;
                        }
                        for (long long i = start; i > end; i += step)
                        {
                            if (i >= 0 && i < size)
                            {
                                result += (*s)[static_cast<size_t>(i)];
                            }
                        }
                    }
                    return var(result);
                }

                throw std::runtime_error("slice() requires a list or string");
            }

            // Slice overload that accepts var parameters (supports None)
            // Python: lst[None:None:-1] to reverse, lst[:None] = lst[:], etc.
            var slice(const var &start_var, const var &end_var, const var &step_var = var(1)) const
            {
                // Convert None to appropriate defaults based on step direction
                long long step = 1;
                if (!step_var.isNone())
                {
                    step = step_var.is<int>() ? step_var.get<int>() : step_var.is<long long>() ? step_var.get<long long>()
                                                                                               : 1;
                }

                long long start, end;
                if (step > 0)
                {
                    start = start_var.isNone() ? 0 : (start_var.is<int>() ? start_var.get<int>() : start_var.is<long long>() ? start_var.get<long long>()
                                                                                                                             : 0);
                    end = end_var.isNone() ? LLONG_MAX : (end_var.is<int>() ? end_var.get<int>() : end_var.is<long long>() ? end_var.get<long long>()
                                                                                                                           : LLONG_MAX);
                }
                else
                {
                    // For negative step, Python uses -1 as end (before beginning)
                    start = start_var.isNone() ? LLONG_MAX : (start_var.is<int>() ? start_var.get<int>() : start_var.is<long long>() ? start_var.get<long long>()
                                                                                                                                     : LLONG_MAX);
                    end = end_var.isNone() ? LLONG_MIN : (end_var.is<int>() ? end_var.get<int>() : end_var.is<long long>() ? end_var.get<long long>()
                                                                                                                           : LLONG_MIN);
                }

                return slice_impl(start, end, step, start_var.isNone(), end_var.isNone());
            }

        private:
            // Internal slice implementation with flags for None handling
            var slice_impl(long long start, long long end, long long step, bool start_is_none, bool end_is_none) const
            {
                if (step == 0)
                {
                    throw std::invalid_argument("slice step cannot be zero");
                }

                // For List
                if (auto *lst = std::get_if<List>(&value))
                {
                    long long size = static_cast<long long>(lst->size());

                    if (step > 0)
                    {
                        if (start_is_none)
                            start = 0;
                        if (end_is_none)
                            end = size;

                        // Handle negative indices
                        if (start < 0)
                            start = std::max(0LL, size + start);
                        if (end < 0)
                            end = std::max(0LL, size + end);

                        // Clamp to valid range
                        start = std::max(0LL, std::min(start, size));
                        end = std::max(0LL, std::min(end, size));
                    }
                    else
                    {
                        // Negative step
                        if (start_is_none)
                            start = size - 1;
                        if (end_is_none)
                            end = -1; // Python: means "go to before beginning"

                        // Handle negative indices
                        if (start < 0 && !start_is_none)
                            start = std::max(-1LL, size + start);
                        if (end < -1 && !end_is_none)
                            end = std::max(-1LL, size + end);

                        // Clamp start to valid range (can be size-1 to -1)
                        start = std::min(start, size - 1);
                    }

                    List result;
                    if (step > 0)
                    {
                        for (long long i = start; i < end; i += step)
                        {
                            result.push_back((*lst)[static_cast<size_t>(i)]);
                        }
                    }
                    else
                    {
                        for (long long i = start; i > end; i += step)
                        {
                            if (i >= 0 && i < size)
                            {
                                result.push_back((*lst)[static_cast<size_t>(i)]);
                            }
                        }
                    }
                    return var(result);
                }

                // For String
                if (auto *s = std::get_if<std::string>(&value))
                {
                    long long size = static_cast<long long>(s->size());

                    if (step > 0)
                    {
                        if (start_is_none)
                            start = 0;
                        if (end_is_none)
                            end = size;

                        // Handle negative indices
                        if (start < 0)
                            start = std::max(0LL, size + start);
                        if (end < 0)
                            end = std::max(0LL, size + end);

                        // Clamp to valid range
                        start = std::max(0LL, std::min(start, size));
                        end = std::max(0LL, std::min(end, size));
                    }
                    else
                    {
                        // Negative step
                        if (start_is_none)
                            start = size - 1;
                        if (end_is_none)
                            end = -1;

                        // Handle negative indices
                        if (start < 0 && !start_is_none)
                            start = std::max(-1LL, size + start);
                        if (end < -1 && !end_is_none)
                            end = std::max(-1LL, size + end);

                        // Clamp start
                        start = std::min(start, size - 1);
                    }

                    std::string result;
                    if (step > 0)
                    {
                        for (long long i = start; i < end; i += step)
                        {
                            result += (*s)[static_cast<size_t>(i)];
                        }
                    }
                    else
                    {
                        for (long long i = start; i > end; i += step)
                        {
                            if (i >= 0 && i < size)
                            {
                                result += (*s)[static_cast<size_t>(i)];
                            }
                        }
                    }
                    return var(result);
                }

                throw std::runtime_error("slice() requires a list or string");
            }

        public:
            // operator() for slice-like access: var(1, 5) or var(1, 5, 2)
            var operator()(long long start, long long end = LLONG_MAX, long long step = 1) const
            {
                return slice(start, end, step);
            }

            // operator() overload for var parameters (supports None)
            var operator()(const var &start_var, const var &end_var, const var &step_var = var(1)) const
            {
                return slice(start_var, end_var, step_var);
            }

            // ============ String Methods ============

            // upper() - convert to uppercase
            var upper() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string result = *s;
                    for (char &c : result)
                    {
                        c = std::toupper(static_cast<unsigned char>(c));
                    }
                    return var(result);
                }
                throw std::runtime_error("upper() requires a string");
            }

            // lower() - convert to lowercase
            var lower() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string result = *s;
                    for (char &c : result)
                    {
                        c = std::tolower(static_cast<unsigned char>(c));
                    }
                    return var(result);
                }
                throw std::runtime_error("lower() requires a string");
            }

            // strip() - remove leading/trailing whitespace
            var strip() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string result = *s;
                    // Left trim
                    result.erase(result.begin(), std::find_if(result.begin(), result.end(),
                                                              [](unsigned char ch)
                                                              { return !std::isspace(ch); }));
                    // Right trim
                    result.erase(std::find_if(result.rbegin(), result.rend(),
                                              [](unsigned char ch)
                                              { return !std::isspace(ch); })
                                     .base(),
                                 result.end());
                    return var(result);
                }
                throw std::runtime_error("strip() requires a string");
            }

            // lstrip() - remove leading whitespace
            var lstrip() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string result = *s;
                    result.erase(result.begin(), std::find_if(result.begin(), result.end(),
                                                              [](unsigned char ch)
                                                              { return !std::isspace(ch); }));
                    return var(result);
                }
                throw std::runtime_error("lstrip() requires a string");
            }

            // rstrip() - remove trailing whitespace
            var rstrip() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string result = *s;
                    result.erase(std::find_if(result.rbegin(), result.rend(),
                                              [](unsigned char ch)
                                              { return !std::isspace(ch); })
                                     .base(),
                                 result.end());
                    return var(result);
                }
                throw std::runtime_error("rstrip() requires a string");
            }

            // replace(old, new) - replace all occurrences
            var replace(const var &old_str, const var &new_str) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string old_s = old_str.get<std::string>();
                    std::string new_s = new_str.get<std::string>();
                    std::string result = *s;
                    size_t pos = 0;
                    while ((pos = result.find(old_s, pos)) != std::string::npos)
                    {
                        result.replace(pos, old_s.length(), new_s);
                        pos += new_s.length();
                    }
                    return var(result);
                }
                throw std::runtime_error("replace() requires a string");
            }

            // find(substring) - find position of substring (-1 if not found)
            var find(const var &substr) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string sub = substr.get<std::string>();
                    size_t pos = s->find(sub);
                    if (pos == std::string::npos)
                    {
                        return var(-1);
                    }
                    return var(static_cast<long long>(pos));
                }
                throw std::runtime_error("find() requires a string");
            }

            // startswith(prefix) - check if string starts with prefix
            var startswith(const var &prefix) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string pre = prefix.get<std::string>();
                    return var(s->substr(0, pre.length()) == pre);
                }
                throw std::runtime_error("startswith() requires a string");
            }

            // endswith(suffix) - check if string ends with suffix
            var endswith(const var &suffix) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string suf = suffix.get<std::string>();
                    if (s->length() < suf.length())
                    {
                        return var(false);
                    }
                    return var(s->substr(s->length() - suf.length()) == suf);
                }
                throw std::runtime_error("endswith() requires a string");
            }

            // isdigit() - check if all characters are digits
            var isdigit() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    if (s->empty())
                        return var(false);
                    for (char c : *s)
                    {
                        if (!std::isdigit(static_cast<unsigned char>(c)))
                        {
                            return var(false);
                        }
                    }
                    return var(true);
                }
                throw std::runtime_error("isdigit() requires a string");
            }

            // isalpha() - check if all characters are alphabetic
            var isalpha() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    if (s->empty())
                        return var(false);
                    for (char c : *s)
                    {
                        if (!std::isalpha(static_cast<unsigned char>(c)))
                        {
                            return var(false);
                        }
                    }
                    return var(true);
                }
                throw std::runtime_error("isalpha() requires a string");
            }

            // isalnum() - check if all characters are alphanumeric
            var isalnum() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    if (s->empty())
                        return var(false);
                    for (char c : *s)
                    {
                        if (!std::isalnum(static_cast<unsigned char>(c)))
                        {
                            return var(false);
                        }
                    }
                    return var(true);
                }
                throw std::runtime_error("isalnum() requires a string");
            }

            // isspace() - check if all characters are whitespace
            var isspace() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    if (s->empty())
                        return var(false);
                    for (char c : *s)
                    {
                        if (!std::isspace(static_cast<unsigned char>(c)))
                        {
                            return var(false);
                        }
                    }
                    return var(true);
                }
                throw std::runtime_error("isspace() requires a string");
            }

            // capitalize() - capitalize first character
            var capitalize() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    if (s->empty())
                        return var(*s);
                    std::string result = *s;
                    result[0] = std::toupper(static_cast<unsigned char>(result[0]));
                    for (size_t i = 1; i < result.length(); ++i)
                    {
                        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
                    }
                    return var(result);
                }
                throw std::runtime_error("capitalize() requires a string");
            }

            // title() - title case (first letter of each word capitalized)
            var title() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string result = *s;
                    bool capitalize_next = true;
                    for (char &c : result)
                    {
                        if (std::isspace(static_cast<unsigned char>(c)))
                        {
                            capitalize_next = true;
                        }
                        else if (capitalize_next)
                        {
                            c = std::toupper(static_cast<unsigned char>(c));
                            capitalize_next = false;
                        }
                        else
                        {
                            c = std::tolower(static_cast<unsigned char>(c));
                        }
                    }
                    return var(result);
                }
                throw std::runtime_error("title() requires a string");
            }

            // count(substring) - count occurrences of substring
            var count(const var &substr) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string sub = substr.get<std::string>();
                    if (sub.empty())
                        return var(0);
                    int count = 0;
                    size_t pos = 0;
                    while ((pos = s->find(sub, pos)) != std::string::npos)
                    {
                        ++count;
                        pos += sub.length();
                    }
                    return var(count);
                }
                // Also works for List - count occurrences of element
                if (auto *lst = std::get_if<List>(&value))
                {
                    int count = 0;
                    for (const auto &item : *lst)
                    {
                        if (item == substr)
                            ++count;
                    }
                    return var(count);
                }
                throw std::runtime_error("count() requires a string or list");
            }

            // reverse() - reverse string or list (returns new var)
            var reverse() const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string result(*s);
                    std::reverse(result.begin(), result.end());
                    return var(result);
                }
                if (auto *lst = std::get_if<List>(&value))
                {
                    List result(*lst);
                    std::reverse(result.begin(), result.end());
                    return var(result);
                }
                throw std::runtime_error("reverse() requires a string or list");
            }

            // split() - split string by delimiter (default: whitespace)
            var split(const var &delim = var(" ")) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    List result;
                    std::string d = delim.get<std::string>();

                    if (d == " ")
                    {
                        // Split by whitespace (like Python's default split)
                        std::istringstream iss(*s);
                        std::string token;
                        while (iss >> token)
                        {
                            result.push_back(var(token));
                        }
                    }
                    else
                    {
                        // Split by specific delimiter
                        size_t start = 0;
                        size_t end = s->find(d);
                        while (end != std::string::npos)
                        {
                            result.push_back(var(s->substr(start, end - start)));
                            start = end + d.length();
                            end = s->find(d, start);
                        }
                        result.push_back(var(s->substr(start)));
                    }
                    return var(result);
                }
                throw std::runtime_error("split() requires a string");
            }

            // join(list) - join list elements with this string as separator
            var join(const var &lst) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    if (auto *l = std::get_if<List>(&lst.value))
                    {
                        std::string result;
                        for (size_t i = 0; i < l->size(); ++i)
                        {
                            if (i > 0)
                                result += *s;
                            result += (*l)[i].get<std::string>();
                        }
                        return var(result);
                    }
                }
                throw std::runtime_error("join() requires a string separator and a list");
            }

            // center(width, fillchar) - center string in field of given width
            var center(int width, const var &fillchar = var(" ")) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    std::string fill = fillchar.get<std::string>();
                    char fc = fill.empty() ? ' ' : fill[0];
                    int len = static_cast<int>(s->length());
                    if (width <= len)
                        return var(*s);
                    int total_pad = width - len;
                    int left_pad = total_pad / 2;
                    int right_pad = total_pad - left_pad;
                    return var(std::string(left_pad, fc) + *s + std::string(right_pad, fc));
                }
                throw std::runtime_error("center() requires a string");
            }

            // zfill(width) - pad string with zeros on the left
            var zfill(int width) const
            {
                if (auto *s = std::get_if<std::string>(&value))
                {
                    int len = static_cast<int>(s->length());
                    if (width <= len)
                        return var(*s);
                    return var(std::string(width - len, '0') + *s);
                }
                throw std::runtime_error("zfill() requires a string");
            }

        }; // class var

        // ============ Factory functions for containers ============

        // list(1, 2, 3) -> List
        template <typename... Args>
        var list(Args &&...args)
        {
            List lst;
            (lst.push_back(var(std::forward<Args>(args))), ...);
            return var(lst);
        }

        // set(1, 2, 3) -> Set
        template <typename... Args>
        var set(Args &&...args)
        {
            Set st;
            (st.insert(var(std::forward<Args>(args))), ...);
            return var(st);
        }

        // dict({"key", value}, ...) -> Dict
        inline var dict(std::initializer_list<std::pair<std::string, var>> items)
        {
            Dict dct;
            for (const auto &[k, v] : items)
            {
                dct[k] = v;
            }
            return var(dct);
        }

        // Empty containers
        inline var list() { return var(List{}); }
        inline var set() { return var(Set{}); }
        inline var dict() { return var(Dict{}); }

        // None constant - Python's None equivalent
        inline const var None = var(NoneType{});

        // len() free function
        inline size_t len(const var &v)
        {
            return v.len();
        }

        // ============ Runtime variable table ============

        inline std::unordered_map<std::string, var> _vars;

        // Proxy Wrapper for variables
        struct DynamicVar
        {
            std::string name;

            DynamicVar &operator=(const var &v)
            {
                _vars[name] = v;
                return *this;
            }

            operator var() const
            {
                auto it = _vars.find(name);
                if (it != _vars.end())
                {
                    return it->second;
                }
                return var();
            }

            // Forward container operations
            var &operator[](size_t index)
            {
                return _vars[name][index];
            }

            var &operator[](const std::string &key)
            {
                return _vars[name][key];
            }

            var &operator[](const char *key)
            {
                return _vars[name][std::string(key)];
            }

            // String conversion for printing
            std::string str() const
            {
                auto it = _vars.find(name);
                if (it != _vars.end())
                {
                    return it->second.str();
                }
                return "None";
            }

            // Enable printing via operator<<
            friend std::ostream &operator<<(std::ostream &os, const DynamicVar &dv)
            {
                os << dv.str();
                return os;
            }
        };

        // ============ Input Function ============
        // Type introspection - isinstance() function
        // Usage: isinstance<int>(v), isinstance<std::string>(v), isinstance<List>(v), etc.
        template <typename T>
        inline bool isinstance(const var &v)
        {
            return v.is<T>();
        }

        // isinstance with string type name - like Python's isinstance(obj, type)
        // Usage: isinstance(v, "int"), isinstance(v, "list"), etc.
        inline bool isinstance(const var &v, const std::string &type_name)
        {
            return v.type() == type_name;
        }

        inline bool isinstance(const var &v, const char *type_name)
        {
            return v.type() == std::string(type_name);
        }

        // ============ Python Built-in Functions ============

        // bool() - Python truthiness rules
        // Empty containers, 0, empty string = False; else True
        inline var Bool(const var &v)
        {
            std::string t = v.type();
            if (t == "bool")
                return v;
            if (t == "int")
                return var(v.get<int>() != 0);
            if (t == "float")
                return var(v.get<float>() != 0.0f);
            if (t == "double")
                return var(v.get<double>() != 0.0);
            if (t == "long")
                return var(v.get<long>() != 0);
            if (t == "long long")
                return var(v.get<long long>() != 0);
            if (t == "str")
                return var(!v.get<std::string>().empty());
            if (t == "list")
                return var(!v.get<List>().empty());
            if (t == "dict")
                return var(!v.get<Dict>().empty());
            if (t == "set")
                return var(!v.get<Set>().empty());
            return var(true); // unknown types are truthy
        }

        // repr() - String representation with quotes and escapes
        inline var repr(const var &v)
        {
            std::string t = v.type();
            if (t == "str")
            {
                std::string s = v.get<std::string>();
                std::ostringstream ss;
                ss << "'";
                for (char c : s)
                {
                    switch (c)
                    {
                    case '\n':
                        ss << "\\n";
                        break;
                    case '\t':
                        ss << "\\t";
                        break;
                    case '\r':
                        ss << "\\r";
                        break;
                    case '\\':
                        ss << "\\\\";
                        break;
                    case '\'':
                        ss << "\\'";
                        break;
                    default:
                        ss << c;
                    }
                }
                ss << "'";
                return var(ss.str());
            }
            return var(v.str());
        }

        // str() - convert to string
        inline var Str(const var &v)
        {
            return var(v.str());
        }

        // int() - convert to int
        inline var Int(const var &v)
        {
            std::string t = v.type();
            if (t == "int")
                return v;
            if (t == "float")
                return var(static_cast<int>(v.get<float>()));
            if (t == "double")
                return var(static_cast<int>(v.get<double>()));
            if (t == "long")
                return var(static_cast<int>(v.get<long>()));
            if (t == "long long")
                return var(static_cast<int>(v.get<long long>()));
            if (t == "bool")
                return var(v.get<bool>() ? 1 : 0);
            if (t == "str")
            {
                try
                {
                    return var(std::stoi(v.get<std::string>()));
                }
                catch (...)
                {
                    throw std::runtime_error("invalid literal for int(): '" + v.get<std::string>() + "'");
                }
            }
            throw std::runtime_error("cannot convert " + t + " to int");
        }

        // float() - convert to float/double
        inline var Float(const var &v)
        {
            std::string t = v.type();
            if (t == "double" || t == "float")
                return v;
            if (t == "int")
                return var(static_cast<double>(v.get<int>()));
            if (t == "long")
                return var(static_cast<double>(v.get<long>()));
            if (t == "long long")
                return var(static_cast<double>(v.get<long long>()));
            if (t == "bool")
                return var(v.get<bool>() ? 1.0 : 0.0);
            if (t == "str")
            {
                try
                {
                    return var(std::stod(v.get<std::string>()));
                }
                catch (...)
                {
                    throw std::runtime_error("could not convert string to float: '" + v.get<std::string>() + "'");
                }
            }
            throw std::runtime_error("cannot convert " + t + " to float");
        }

        // abs() - absolute value
        inline var abs(const var &v)
        {
            std::string t = v.type();
            if (t == "int")
                return var(std::abs(v.get<int>()));
            if (t == "float")
                return var(std::abs(v.get<float>()));
            if (t == "double")
                return var(std::abs(v.get<double>()));
            if (t == "long")
                return var(std::abs(v.get<long>()));
            if (t == "long long")
                return var(std::abs(v.get<long long>()));
            throw std::runtime_error("abs() requires numeric type, got " + t);
        }

        // min() - minimum of list or two values
        inline var min(const var &a, const var &b)
        {
            if (a < b)
                return a;
            return b;
        }

        inline var min(const var &lst)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("min() expects a list or two arguments");
            }
            const auto &l = lst.get<List>();
            if (l.empty())
                throw std::runtime_error("min() arg is an empty sequence");
            var result = l[0];
            for (size_t i = 1; i < l.size(); ++i)
            {
                if (l[i] < result)
                    result = l[i];
            }
            return result;
        }

        // max() - maximum of list or two values
        inline var max(const var &a, const var &b)
        {
            if (a < b)
                return b;
            return a;
        }

        inline var max(const var &lst)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("max() expects a list or two arguments");
            }
            const auto &l = lst.get<List>();
            if (l.empty())
                throw std::runtime_error("max() arg is an empty sequence");
            var result = l[0];
            for (size_t i = 1; i < l.size(); ++i)
            {
                if (result < l[i])
                    result = l[i];
            }
            return result;
        }

        // sum() - sum of list elements
        inline var sum(const var &lst, const var &start = var(0))
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("sum() expects a list");
            }
            var result = start;
            const auto &l = lst.get<List>();
            for (const auto &item : l)
            {
                result = result + item;
            }
            return result;
        }

        // sorted() - return new sorted list
        inline var sorted(const var &lst, bool reverse_order = false)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("sorted() expects a list");
            }
            List result = lst.get<List>();
            if (reverse_order)
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

        // reversed_var() - return new reversed list (var version)
        // Note: Use this for var types, or pythonic::loop::reversed for generic iterables
        inline var reversed_var(const var &v)
        {
            std::string t = v.type();
            if (t == "list")
            {
                List result = v.get<List>();
                std::reverse(result.begin(), result.end());
                return var(result);
            }
            else if (t == "str")
            {
                std::string s = v.get<std::string>();
                std::reverse(s.begin(), s.end());
                return var(s);
            }
            throw std::runtime_error("reversed_var() expects list or string");
        }

        // all_var() - return True if all elements are truthy (var version)
        inline var all_var(const var &lst)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("all_var() expects a list");
            }
            const auto &l = lst.get<List>();
            for (const auto &item : l)
            {
                if (!Bool(item).get<bool>())
                    return var(false);
            }
            return var(true);
        }

        // any_var() - return True if any element is truthy (var version)
        inline var any_var(const var &lst)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("any_var() expects a list");
            }
            const auto &l = lst.get<List>();
            for (const auto &item : l)
            {
                if (Bool(item).get<bool>())
                    return var(true);
            }
            return var(false);
        }

        // map() - apply function to each element, return new list
        template <typename Func>
        inline var map(Func func, const var &lst)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("map() expects a list");
            }
            List result;
            const auto &l = lst.get<List>();
            for (const auto &item : l)
            {
                result.push_back(func(item));
            }
            return var(result);
        }

        // filter() - filter elements by predicate, return new list
        template <typename Func>
        inline var filter(Func predicate, const var &lst)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("filter() expects a list");
            }
            List result;
            const auto &l = lst.get<List>();
            for (const auto &item : l)
            {
                if (predicate(item))
                {
                    result.push_back(item);
                }
            }
            return var(result);
        }

        // reduce() - reduce list with binary function
        template <typename Func>
        inline var reduce(Func func, const var &lst, const var &initial)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("reduce() expects a list");
            }
            var result = initial;
            const auto &l = lst.get<List>();
            for (const auto &item : l)
            {
                result = func(result, item);
            }
            return result;
        }

        template <typename Func>
        inline var reduce(Func func, const var &lst)
        {
            if (lst.type() != "list")
            {
                throw std::runtime_error("reduce() expects a list");
            }
            const auto &l = lst.get<List>();
            if (l.empty())
                throw std::runtime_error("reduce() of empty sequence with no initial value");
            var result = l[0];
            for (size_t i = 1; i < l.size(); ++i)
            {
                result = func(result, l[i]);
            }
            return result;
        }

        // Python-like input() function

        inline var input(const var &prompt = var(""))
        {
            // Print prompt without newline
            if (auto *s = std::get_if<std::string>(&prompt.getValue()))
            {
                if (!s->empty())
                {
                    std::cout << *s;
                    std::cout.flush();
                }
            }

            // Read line from stdin
            std::string line;
            std::getline(std::cin, line);
            return var(line);
        }

        // Overload for const char* prompt
        inline var input(const char *prompt)
        {
            return input(var(prompt));
        }

        // ============ Tuple Access Helper ============
        // Provides Python-like tuple[0], tuple[1] access via function call syntax
        // Usage: get(pair, 0) instead of std::get<0>(pair)

        template <typename Tuple>
        auto get(const Tuple &t, size_t index)
            -> std::enable_if_t<(std::tuple_size_v<std::decay_t<Tuple>> > 0), var>
        {
            constexpr size_t size = std::tuple_size_v<std::decay_t<Tuple>>;
            return get_impl(t, index, std::make_index_sequence<size>{});
        }

        template <typename Tuple, size_t... Is>
        var get_impl(const Tuple &t, size_t index, std::index_sequence<Is...>)
        {
            var result;
            bool found = false;
            // Use fold expression to find the right index at runtime
            ((Is == index ? (result = var(std::get<Is>(t)), found = true) : false), ...);
            if (!found)
            {
                throw std::out_of_range("Tuple index out of range");
            }
            return result;
        }

        // Tuple to list conversion for easier iteration
        template <typename... Ts>
        var tuple_to_list(const std::tuple<Ts...> &t)
        {
            List result;
            std::apply([&result](auto &&...args)
                       { (result.push_back(var(args)), ...); }, t);
            return var(result);
        }

        // Unpack helper - converts tuple to list for var-based processing
        template <typename Tuple>
        var unpack(const Tuple &t)
        {
            return tuple_to_list(t);
        }

// Pythonic variables declaration
#define let(name) \
    pythonic::vars::DynamicVar { #name }

    } // namespace vars
} // namespace pythonic
