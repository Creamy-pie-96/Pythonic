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
#include <cstdint>
#include <memory>
#include <optional>

namespace pythonic
{
    namespace vars
    {

        // Forward declaration
        class var;

        // Custom hasher for var - implementation deferred until var is complete
        struct VarHasher
        {
            size_t operator()(const var &v) const noexcept;
        };

        // Custom equality for var in unordered containers
        struct VarEqual
        {
            bool operator()(const var &a, const var &b) const noexcept;
        };

        // Type aliases for containers (Python-like naming)
        // Set: Hash-based set (like Python's set) - O(1) average operations
        // OrderedSet: Tree-based set - maintains sorted order, O(log n)
        // Dict: Hash-based dict (like Python's dict) - O(1) average operations
        // OrderedDict: Tree-based dict - maintains key order, O(log n)
        using List = std::vector<var>;
        using Set = std::unordered_set<var, VarHasher, VarEqual>;
        using OrderedSet = std::set<var>;
        using Dict = std::unordered_map<std::string, var>;
        using OrderedDict = std::map<std::string, var>;

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
        struct is_container<OrderedSet> : std::true_type
        {
        };
        template <>
        struct is_container<Dict> : std::true_type
        {
        };
        template <>
        struct is_container<OrderedDict> : std::true_type
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

        // Forward declaration for Graph - actual include is at the end of this file
        // This avoids circular dependency since Graph uses var for node metadata
        class VarGraphWrapper;
        using GraphPtr = std::shared_ptr<VarGraphWrapper>;

        // The main variant type (primitives + all container types + graph)
        using varType = std::variant<
            NoneType,
            int, float, std::string, bool, double,
            long, long long, long double,
            unsigned int, unsigned long, unsigned long long,
            List, Set, Dict, OrderedSet, OrderedDict, GraphPtr>;

        // TypeTag enum for fast type dispatch (avoids repeated std::get_if/holds_alternative calls)
        // Using uint8_t as underlying type to minimize memory overhead (1 byte)
        // IMPORTANT: Order must match varType order exactly!
        enum class TypeTag : uint8_t
        {
            NONE = 0,
            INT,
            FLOAT,
            STRING,
            BOOL,
            DOUBLE,
            LONG,
            LONG_LONG,
            LONG_DOUBLE,
            UINT,
            ULONG,
            ULONG_LONG,
            LIST,
            SET,
            DICT,
            ORDEREDSET,
            ORDEREDDICT,
            GRAPH
        };

        class var
        {
        private:
            varType value;
            TypeTag tag_; // Cached type tag for fast dispatch

            // Helper to compute tag from variant
            static TypeTag compute_tag(const varType &v)
            {
                return static_cast<TypeTag>(v.index());
            }

            // Type promotion helpers
            // Returns the TypeTag that should be used for mixed-type operations
            // STRING has highest rank - any operation with string converts other to string
            // For numeric types: bool < int < uint < long < ulong < long long < ulong long < float < double < long double

            // Get promotion rank for a type tag (higher = wider type)
            static int getTypeRank(TypeTag t) noexcept
            {
                switch (t)
                {
                case TypeTag::BOOL:
                    return 0;
                case TypeTag::INT:
                    return 1;
                case TypeTag::UINT:
                    return 2;
                case TypeTag::LONG:
                    return 3;
                case TypeTag::ULONG:
                    return 4;
                case TypeTag::LONG_LONG:
                    return 5;
                case TypeTag::ULONG_LONG:
                    return 6;
                case TypeTag::FLOAT:
                    return 7;
                case TypeTag::DOUBLE:
                    return 8;
                case TypeTag::LONG_DOUBLE:
                    return 9;
                case TypeTag::STRING:
                    return 100; // Highest rank - forces string concatenation
                default:
                    return -1; // Non-promotable types (containers, None, etc.)
                }
            }

            // Type promoted operation helpers
            // Perform addition with proper type promotion
            var addPromoted(const var &other) const
            {
                TypeTag resultType = getPromotedType(tag_, other.tag_);

                switch (resultType)
                {
                case TypeTag::STRING:
                    return var(toString() + other.toString());
                case TypeTag::LONG_DOUBLE:
                    return var(toLongDouble() + other.toLongDouble());
                case TypeTag::DOUBLE:
                    return var(toDouble() + other.toDouble());
                case TypeTag::FLOAT:
                    return var(toFloat() + other.toFloat());
                case TypeTag::ULONG_LONG:
                    return var(toULongLong() + other.toULongLong());
                case TypeTag::LONG_LONG:
                    return var(toLongLong() + other.toLongLong());
                case TypeTag::ULONG:
                    return var(toULong() + other.toULong());
                case TypeTag::LONG:
                    return var(toLong() + other.toLong());
                case TypeTag::UINT:
                    return var(toUInt() + other.toUInt());
                case TypeTag::INT:
                case TypeTag::BOOL:
                default:
                    return var(toInt() + other.toInt());
                }
            }

            // Perform subtraction with proper type promotion
            var subPromoted(const var &other) const
            {
                TypeTag resultType = getPromotedType(tag_, other.tag_);

                // String subtraction doesn't make sense, throw error
                if (resultType == TypeTag::STRING)
                {
                    throw std::runtime_error("Cannot subtract strings");
                }

                switch (resultType)
                {
                case TypeTag::LONG_DOUBLE:
                    return var(toLongDouble() - other.toLongDouble());
                case TypeTag::DOUBLE:
                    return var(toDouble() - other.toDouble());
                case TypeTag::FLOAT:
                    return var(toFloat() - other.toFloat());
                case TypeTag::ULONG_LONG:
                    return var(toULongLong() - other.toULongLong());
                case TypeTag::LONG_LONG:
                    return var(toLongLong() - other.toLongLong());
                case TypeTag::ULONG:
                    return var(toULong() - other.toULong());
                case TypeTag::LONG:
                    return var(toLong() - other.toLong());
                case TypeTag::UINT:
                    return var(toUInt() - other.toUInt());
                case TypeTag::INT:
                case TypeTag::BOOL:
                default:
                    return var(toInt() - other.toInt());
                }
            }

            // Perform multiplication with proper type promotion
            var mulPromoted(const var &other) const
            {
                TypeTag resultType = getPromotedType(tag_, other.tag_);

                // String multiplication: "ab" * 3 = "ababab"
                if (tag_ == TypeTag::STRING && other.isIntegral())
                {
                    std::string result;
                    std::string s = std::get<std::string>(value);
                    int count = other.toInt();
                    result.reserve(s.size() * count);
                    for (int i = 0; i < count; ++i)
                        result += s;
                    return var(result);
                }
                if (other.tag_ == TypeTag::STRING && isIntegral())
                {
                    std::string result;
                    std::string s = std::get<std::string>(other.value);
                    int count = toInt();
                    result.reserve(s.size() * count);
                    for (int i = 0; i < count; ++i)
                        result += s;
                    return var(result);
                }
                if (resultType == TypeTag::STRING)
                {
                    throw std::runtime_error("Cannot multiply two strings");
                }

                switch (resultType)
                {
                case TypeTag::LONG_DOUBLE:
                    return var(toLongDouble() * other.toLongDouble());
                case TypeTag::DOUBLE:
                    return var(toDouble() * other.toDouble());
                case TypeTag::FLOAT:
                    return var(toFloat() * other.toFloat());
                case TypeTag::ULONG_LONG:
                    return var(toULongLong() * other.toULongLong());
                case TypeTag::LONG_LONG:
                    return var(toLongLong() * other.toLongLong());
                case TypeTag::ULONG:
                    return var(toULong() * other.toULong());
                case TypeTag::LONG:
                    return var(toLong() * other.toLong());
                case TypeTag::UINT:
                    return var(toUInt() * other.toUInt());
                case TypeTag::INT:
                case TypeTag::BOOL:
                default:
                    return var(toInt() * other.toInt());
                }
            }

            // Perform division with proper type promotion (always returns floating point for safety)
            var divPromoted(const var &other) const
            {
                TypeTag resultType = getPromotedType(tag_, other.tag_);

                if (resultType == TypeTag::STRING)
                {
                    throw std::runtime_error("Cannot divide strings");
                }

                // For division, promote to at least double for precision
                // unless one operand is long double
                if (resultType == TypeTag::LONG_DOUBLE)
                {
                    long double divisor = other.toLongDouble();
                    if (divisor == 0.0L)
                        throw std::runtime_error("Division by zero");
                    return var(toLongDouble() / divisor);
                }

                // All other cases use double for safety
                double divisor = other.toDouble();
                if (divisor == 0.0)
                    throw std::runtime_error("Division by zero");
                return var(toDouble() / divisor);
            }

            // Perform modulo with proper type promotion
            var modPromoted(const var &other) const
            {
                TypeTag resultType = getPromotedType(tag_, other.tag_);

                if (resultType == TypeTag::STRING)
                {
                    throw std::runtime_error("Cannot perform modulo on strings");
                }

                // For floating point, use fmod
                if (resultType == TypeTag::LONG_DOUBLE || resultType == TypeTag::DOUBLE || resultType == TypeTag::FLOAT)
                {
                    double divisor = other.toDouble();
                    if (divisor == 0.0)
                        throw std::runtime_error("Modulo by zero");
                    return var(std::fmod(toDouble(), divisor));
                }

                // For integers, use integer modulo
                switch (resultType)
                {
                case TypeTag::ULONG_LONG:
                {
                    auto divisor = other.toULongLong();
                    if (divisor == 0)
                        throw std::runtime_error("Modulo by zero");
                    return var(toULongLong() % divisor);
                }
                case TypeTag::LONG_LONG:
                {
                    auto divisor = other.toLongLong();
                    if (divisor == 0)
                        throw std::runtime_error("Modulo by zero");
                    return var(toLongLong() % divisor);
                }
                case TypeTag::ULONG:
                {
                    auto divisor = other.toULong();
                    if (divisor == 0)
                        throw std::runtime_error("Modulo by zero");
                    return var(toULong() % divisor);
                }
                case TypeTag::LONG:
                {
                    auto divisor = other.toLong();
                    if (divisor == 0)
                        throw std::runtime_error("Modulo by zero");
                    return var(toLong() % divisor);
                }
                case TypeTag::UINT:
                {
                    auto divisor = other.toUInt();
                    if (divisor == 0)
                        throw std::runtime_error("Modulo by zero");
                    return var(toUInt() % divisor);
                }
                default:
                {
                    auto divisor = other.toInt();
                    if (divisor == 0)
                        throw std::runtime_error("Modulo by zero");
                    return var(toInt() % divisor);
                }
                }
            }

        public:
            // Fast type tag accessor
            TypeTag tag() const { return tag_; }

            // Constructors - all set tag_ appropriately
            var() : value(0), tag_(TypeTag::INT) {}
            var(const varType &v) : value(v), tag_(compute_tag(v)) {}
            var(const char *s) : value(std::string(s)), tag_(TypeTag::STRING) {}
            var(NoneType) : value(NoneType{}), tag_(TypeTag::NONE) {}

            // Specialized constructors for common types (faster than varType constructor)
            var(int v) : value(v), tag_(TypeTag::INT) {}
            var(double v) : value(v), tag_(TypeTag::DOUBLE) {}
            var(float v) : value(v), tag_(TypeTag::FLOAT) {}
            var(bool v) : value(v), tag_(TypeTag::BOOL) {}
            var(long v) : value(v), tag_(TypeTag::LONG) {}
            var(long long v) : value(v), tag_(TypeTag::LONG_LONG) {}
            var(long double v) : value(v), tag_(TypeTag::LONG_DOUBLE) {}
            var(unsigned int v) : value(v), tag_(TypeTag::UINT) {}
            var(unsigned long v) : value(v), tag_(TypeTag::ULONG) {}
            var(unsigned long long v) : value(v), tag_(TypeTag::ULONG_LONG) {}
            var(const std::string &s) : value(s), tag_(TypeTag::STRING) {}
            var(std::string &&s) : value(std::move(s)), tag_(TypeTag::STRING) {}
            var(const List &l) : value(l), tag_(TypeTag::LIST) {}
            var(List &&l) : value(std::move(l)), tag_(TypeTag::LIST) {}
            var(const Set &s) : value(s), tag_(TypeTag::SET) {}
            var(Set &&s) : value(std::move(s)), tag_(TypeTag::SET) {}
            var(const Dict &d) : value(d), tag_(TypeTag::DICT) {}
            var(Dict &&d) : value(std::move(d)), tag_(TypeTag::DICT) {}
            var(const OrderedSet &hs) : value(hs), tag_(TypeTag::ORDEREDSET) {}
            var(OrderedSet &&hs) : value(std::move(hs)), tag_(TypeTag::ORDEREDSET) {}
            var(const OrderedDict &od) : value(od), tag_(TypeTag::ORDEREDDICT) {}
            var(OrderedDict &&od) : value(std::move(od)), tag_(TypeTag::ORDEREDDICT) {}
            var(const GraphPtr &g) : value(g), tag_(TypeTag::GRAPH) {}
            var(GraphPtr &&g) : value(std::move(g)), tag_(TypeTag::GRAPH) {}

            // Check if this var is None
            bool isNone() const { return tag_ == TypeTag::NONE; }

            // Accessors
            const varType &getValue() const { return value; }
            void setValue(const varType &v)
            {
                this->value = v;
                tag_ = compute_tag(v);
            }

            // Type checking - now O(1) using tag
            template <typename T>
            bool is() const
            {
                if constexpr (std::is_same_v<T, int>)
                    return tag_ == TypeTag::INT;
                else if constexpr (std::is_same_v<T, double>)
                    return tag_ == TypeTag::DOUBLE;
                else if constexpr (std::is_same_v<T, float>)
                    return tag_ == TypeTag::FLOAT;
                else if constexpr (std::is_same_v<T, bool>)
                    return tag_ == TypeTag::BOOL;
                else if constexpr (std::is_same_v<T, std::string>)
                    return tag_ == TypeTag::STRING;
                else if constexpr (std::is_same_v<T, long>)
                    return tag_ == TypeTag::LONG;
                else if constexpr (std::is_same_v<T, long long>)
                    return tag_ == TypeTag::LONG_LONG;
                else if constexpr (std::is_same_v<T, long double>)
                    return tag_ == TypeTag::LONG_DOUBLE;
                else if constexpr (std::is_same_v<T, unsigned int>)
                    return tag_ == TypeTag::UINT;
                else if constexpr (std::is_same_v<T, unsigned long>)
                    return tag_ == TypeTag::ULONG;
                else if constexpr (std::is_same_v<T, unsigned long long>)
                    return tag_ == TypeTag::ULONG_LONG;
                else if constexpr (std::is_same_v<T, List>)
                    return tag_ == TypeTag::LIST;
                else if constexpr (std::is_same_v<T, Set>)
                    return tag_ == TypeTag::SET;
                else if constexpr (std::is_same_v<T, Dict>)
                    return tag_ == TypeTag::DICT;
                else if constexpr (std::is_same_v<T, OrderedSet>)
                    return tag_ == TypeTag::ORDEREDSET;
                else if constexpr (std::is_same_v<T, OrderedDict>)
                    return tag_ == TypeTag::ORDEREDDICT;
                else if constexpr (std::is_same_v<T, GraphPtr>)
                    return tag_ == TypeTag::GRAPH;
                else if constexpr (std::is_same_v<T, NoneType>)
                    return tag_ == TypeTag::NONE;
                else
                    return std::holds_alternative<T>(value);
            }

            // Check if this var holds a graph
            bool isGraph() const { return tag_ == TypeTag::GRAPH; }

            // Helper: Check if this var holds a numeric type
            bool isNumeric() const
            {
                return tag_ >= TypeTag::INT && tag_ <= TypeTag::ULONG_LONG && tag_ != TypeTag::STRING && tag_ != TypeTag::BOOL;
            }

            // Helper: Check if this var holds an integer type
            bool isIntegral() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                case TypeTag::LONG:
                case TypeTag::LONG_LONG:
                case TypeTag::UINT:
                case TypeTag::ULONG:
                case TypeTag::ULONG_LONG:
                    return true;
                default:
                    return false;
                }
            }

            // ============ Fast Type Checking Methods ============
            // These provide O(1) type checks without variant overhead
            bool is_list() const noexcept { return tag_ == TypeTag::LIST; }
            bool is_dict() const noexcept { return tag_ == TypeTag::DICT; }
            bool is_set() const noexcept { return tag_ == TypeTag::SET; }
            bool is_string() const noexcept { return tag_ == TypeTag::STRING; }
            bool is_int() const noexcept { return tag_ == TypeTag::INT; }
            bool is_double() const noexcept { return tag_ == TypeTag::DOUBLE; }
            bool is_float() const noexcept { return tag_ == TypeTag::FLOAT; }
            bool is_bool() const noexcept { return tag_ == TypeTag::BOOL; }
            bool is_none() const noexcept { return tag_ == TypeTag::NONE; }
            bool is_ordered_dict() const noexcept { return tag_ == TypeTag::ORDEREDDICT; }
            bool is_ordered_set() const noexcept { return tag_ == TypeTag::ORDEREDSET; }
            bool is_long() const noexcept { return tag_ == TypeTag::LONG; }
            bool is_long_long() const noexcept { return tag_ == TypeTag::LONG_LONG; }
            bool is_long_double() const noexcept { return tag_ == TypeTag::LONG_DOUBLE; }
            bool is_uint() const noexcept { return tag_ == TypeTag::UINT; }
            bool is_ulong() const noexcept { return tag_ == TypeTag::ULONG; }
            bool is_ulong_long() const noexcept { return tag_ == TypeTag::ULONG_LONG; }
            bool is_graph() const noexcept { return tag_ == TypeTag::GRAPH; }
            // Aggregate checks for common patterns
            bool is_any_integral() const noexcept { return isIntegral(); }
            bool is_any_floating() const noexcept
            {
                return tag_ == TypeTag::FLOAT || tag_ == TypeTag::DOUBLE || tag_ == TypeTag::LONG_DOUBLE;
            }
            bool is_any_numeric() const noexcept { return isNumeric(); }

            // ============ Fast Typed Accessors (Unchecked) ============
            // These skip type checking for maximum performance in hot paths
            // IMPORTANT: Caller must verify type first using is_list(), etc.
            // Container types
            List &as_list_unchecked() noexcept { return std::get<List>(value); }
            const List &as_list_unchecked() const noexcept { return std::get<List>(value); }
            Dict &as_dict_unchecked() noexcept { return std::get<Dict>(value); }
            const Dict &as_dict_unchecked() const noexcept { return std::get<Dict>(value); }
            Set &as_set_unchecked() noexcept { return std::get<Set>(value); }
            const Set &as_set_unchecked() const noexcept { return std::get<Set>(value); }
            OrderedDict &as_ordered_dict_unchecked() noexcept { return std::get<OrderedDict>(value); }
            const OrderedDict &as_ordered_dict_unchecked() const noexcept { return std::get<OrderedDict>(value); }
            OrderedSet &as_ordered_set_unchecked() noexcept { return std::get<OrderedSet>(value); }
            const OrderedSet &as_ordered_set_unchecked() const noexcept { return std::get<OrderedSet>(value); }
            // String type
            std::string &as_string_unchecked() noexcept { return std::get<std::string>(value); }
            const std::string &as_string_unchecked() const noexcept { return std::get<std::string>(value); }
            // Basic numeric types
            int &as_int_unchecked() noexcept { return std::get<int>(value); }
            int as_int_unchecked() const noexcept { return std::get<int>(value); }
            double &as_double_unchecked() noexcept { return std::get<double>(value); }
            double as_double_unchecked() const noexcept { return std::get<double>(value); }
            float &as_float_unchecked() noexcept { return std::get<float>(value); }
            float as_float_unchecked() const noexcept { return std::get<float>(value); }
            bool &as_bool_unchecked() noexcept { return std::get<bool>(value); }
            bool as_bool_unchecked() const noexcept { return std::get<bool>(value); }
            // Extended numeric types
            long &as_long_unchecked() noexcept { return std::get<long>(value); }
            long as_long_unchecked() const noexcept { return std::get<long>(value); }
            long long &as_long_long_unchecked() noexcept { return std::get<long long>(value); }
            long long as_long_long_unchecked() const noexcept { return std::get<long long>(value); }
            long double &as_long_double_unchecked() noexcept { return std::get<long double>(value); }
            long double as_long_double_unchecked() const noexcept { return std::get<long double>(value); }
            // Unsigned types
            unsigned int &as_uint_unchecked() noexcept { return std::get<unsigned int>(value); }
            unsigned int as_uint_unchecked() const noexcept { return std::get<unsigned int>(value); }
            unsigned long &as_ulong_unchecked() noexcept { return std::get<unsigned long>(value); }
            unsigned long as_ulong_unchecked() const noexcept { return std::get<unsigned long>(value); }
            unsigned long long &as_ulong_long_unchecked() noexcept { return std::get<unsigned long long>(value); }
            unsigned long long as_ulong_long_unchecked() const noexcept { return std::get<unsigned long long>(value); }
            // Graph type
            GraphPtr &as_graph_unchecked() noexcept { return std::get<GraphPtr>(value); }
            const GraphPtr &as_graph_unchecked() const noexcept { return std::get<GraphPtr>(value); }

            // ============ Safe Typed Accessors ============
            // These check type and throw if mismatched
            // Container types
            List &as_list()
            {
                if (tag_ != TypeTag::LIST)
                    throw std::runtime_error("as_list() requires a list");
                return std::get<List>(value);
            }
            const List &as_list() const
            {
                if (tag_ != TypeTag::LIST)
                    throw std::runtime_error("as_list() requires a list");
                return std::get<List>(value);
            }
            Dict &as_dict()
            {
                if (tag_ != TypeTag::DICT)
                    throw std::runtime_error("as_dict() requires a dict");
                return std::get<Dict>(value);
            }
            const Dict &as_dict() const
            {
                if (tag_ != TypeTag::DICT)
                    throw std::runtime_error("as_dict() requires a dict");
                return std::get<Dict>(value);
            }
            Set &as_set()
            {
                if (tag_ != TypeTag::SET)
                    throw std::runtime_error("as_set() requires a set");
                return std::get<Set>(value);
            }
            const Set &as_set() const
            {
                if (tag_ != TypeTag::SET)
                    throw std::runtime_error("as_set() requires a set");
                return std::get<Set>(value);
            }
            OrderedDict &as_ordered_dict()
            {
                if (tag_ != TypeTag::ORDEREDDICT)
                    throw std::runtime_error("as_ordered_dict() requires an ordered dict");
                return std::get<OrderedDict>(value);
            }
            const OrderedDict &as_ordered_dict() const
            {
                if (tag_ != TypeTag::ORDEREDDICT)
                    throw std::runtime_error("as_ordered_dict() requires an ordered dict");
                return std::get<OrderedDict>(value);
            }
            OrderedSet &as_ordered_set()
            {
                if (tag_ != TypeTag::ORDEREDSET)
                    throw std::runtime_error("as_ordered_set() requires an ordered set");
                return std::get<OrderedSet>(value);
            }
            const OrderedSet &as_ordered_set() const
            {
                if (tag_ != TypeTag::ORDEREDSET)
                    throw std::runtime_error("as_ordered_set() requires an ordered set");
                return std::get<OrderedSet>(value);
            }
            // String type
            std::string &as_string()
            {
                if (tag_ != TypeTag::STRING)
                    throw std::runtime_error("as_string() requires a string");
                return std::get<std::string>(value);
            }
            const std::string &as_string() const
            {
                if (tag_ != TypeTag::STRING)
                    throw std::runtime_error("as_string() requires a string");
                return std::get<std::string>(value);
            }
            // Basic numeric types
            int as_int() const
            {
                if (tag_ != TypeTag::INT)
                    throw std::runtime_error("as_int() requires an int");
                return std::get<int>(value);
            }
            double as_double() const
            {
                if (tag_ != TypeTag::DOUBLE)
                    throw std::runtime_error("as_double() requires a double");
                return std::get<double>(value);
            }
            float as_float() const
            {
                if (tag_ != TypeTag::FLOAT)
                    throw std::runtime_error("as_float() requires a float");
                return std::get<float>(value);
            }
            bool as_bool() const
            {
                if (tag_ != TypeTag::BOOL)
                    throw std::runtime_error("as_bool() requires a bool");
                return std::get<bool>(value);
            }
            // Extended numeric types
            long as_long() const
            {
                if (tag_ != TypeTag::LONG)
                    throw std::runtime_error("as_long() requires a long");
                return std::get<long>(value);
            }
            long long as_long_long() const
            {
                if (tag_ != TypeTag::LONG_LONG)
                    throw std::runtime_error("as_long_long() requires a long long");
                return std::get<long long>(value);
            }
            long double as_long_double() const
            {
                if (tag_ != TypeTag::LONG_DOUBLE)
                    throw std::runtime_error("as_long_double() requires a long double");
                return std::get<long double>(value);
            }
            // Unsigned types
            unsigned int as_uint() const
            {
                if (tag_ != TypeTag::UINT)
                    throw std::runtime_error("as_uint() requires an unsigned int");
                return std::get<unsigned int>(value);
            }
            unsigned long as_ulong() const
            {
                if (tag_ != TypeTag::ULONG)
                    throw std::runtime_error("as_ulong() requires an unsigned long");
                return std::get<unsigned long>(value);
            }
            unsigned long long as_ulong_long() const
            {
                if (tag_ != TypeTag::ULONG_LONG)
                    throw std::runtime_error("as_ulong_long() requires an unsigned long long");
                return std::get<unsigned long long>(value);
            }

            // ============ Type Conversion Methods ============
            // These convert any numeric type to a specific type for mixed-type arithmetic
            // The result type is determined by type promotion rules

            // Convert to int (widest narrow conversion, may lose precision)
            int toInt() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return std::get<int>(value);
                case TypeTag::FLOAT:
                    return static_cast<int>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return static_cast<int>(std::get<double>(value));
                case TypeTag::LONG:
                    return static_cast<int>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return static_cast<int>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return static_cast<int>(std::get<long double>(value));
                case TypeTag::UINT:
                    return static_cast<int>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return static_cast<int>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return static_cast<int>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1 : 0;
                default:
                    throw std::runtime_error("Cannot convert to int");
                }
            }

            // Convert to unsigned int
            unsigned int toUInt() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<unsigned int>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return static_cast<unsigned int>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return static_cast<unsigned int>(std::get<double>(value));
                case TypeTag::LONG:
                    return static_cast<unsigned int>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return static_cast<unsigned int>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return static_cast<unsigned int>(std::get<long double>(value));
                case TypeTag::UINT:
                    return std::get<unsigned int>(value);
                case TypeTag::ULONG:
                    return static_cast<unsigned int>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return static_cast<unsigned int>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1U : 0U;
                default:
                    throw std::runtime_error("Cannot convert to unsigned int");
                }
            }

            // Convert to long
            long toLong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<long>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return static_cast<long>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return static_cast<long>(std::get<double>(value));
                case TypeTag::LONG:
                    return std::get<long>(value);
                case TypeTag::LONG_LONG:
                    return static_cast<long>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return static_cast<long>(std::get<long double>(value));
                case TypeTag::UINT:
                    return static_cast<long>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return static_cast<long>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return static_cast<long>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1L : 0L;
                default:
                    throw std::runtime_error("Cannot convert to long");
                }
            }

            // Convert to unsigned long
            unsigned long toULong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<unsigned long>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return static_cast<unsigned long>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return static_cast<unsigned long>(std::get<double>(value));
                case TypeTag::LONG:
                    return static_cast<unsigned long>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return static_cast<unsigned long>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return static_cast<unsigned long>(std::get<long double>(value));
                case TypeTag::UINT:
                    return static_cast<unsigned long>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return std::get<unsigned long>(value);
                case TypeTag::ULONG_LONG:
                    return static_cast<unsigned long>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1UL : 0UL;
                default:
                    throw std::runtime_error("Cannot convert to unsigned long");
                }
            }

            // Convert to long long (for integer arithmetic that needs range)
            long long toLongLong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<long long>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return static_cast<long long>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return static_cast<long long>(std::get<double>(value));
                case TypeTag::LONG:
                    return static_cast<long long>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return std::get<long long>(value);
                case TypeTag::LONG_DOUBLE:
                    return static_cast<long long>(std::get<long double>(value));
                case TypeTag::UINT:
                    return static_cast<long long>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return static_cast<long long>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return static_cast<long long>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1LL : 0LL;
                default:
                    throw std::runtime_error("Cannot convert to long long");
                }
            }

            // Convert to unsigned long long
            unsigned long long toULongLong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<unsigned long long>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return static_cast<unsigned long long>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return static_cast<unsigned long long>(std::get<double>(value));
                case TypeTag::LONG:
                    return static_cast<unsigned long long>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return static_cast<unsigned long long>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return static_cast<unsigned long long>(std::get<long double>(value));
                case TypeTag::UINT:
                    return static_cast<unsigned long long>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return static_cast<unsigned long long>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return std::get<unsigned long long>(value);
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1ULL : 0ULL;
                default:
                    throw std::runtime_error("Cannot convert to unsigned long long");
                }
            }

            // Convert to float
            float toFloat() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<float>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return std::get<float>(value);
                case TypeTag::DOUBLE:
                    return static_cast<float>(std::get<double>(value));
                case TypeTag::LONG:
                    return static_cast<float>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return static_cast<float>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return static_cast<float>(std::get<long double>(value));
                case TypeTag::UINT:
                    return static_cast<float>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return static_cast<float>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return static_cast<float>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1.0f : 0.0f;
                default:
                    throw std::runtime_error("Cannot convert to float");
                }
            }

            // Convert to double (most common floating point conversion)
            double toDouble() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<double>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return static_cast<double>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return std::get<double>(value);
                case TypeTag::LONG:
                    return static_cast<double>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return static_cast<double>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return static_cast<double>(std::get<long double>(value));
                case TypeTag::UINT:
                    return static_cast<double>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return static_cast<double>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return static_cast<double>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1.0 : 0.0;
                default:
                    throw std::runtime_error("Cannot convert to double");
                }
            }

            // Convert to long double (highest precision floating point)
            long double toLongDouble() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<long double>(std::get<int>(value));
                case TypeTag::FLOAT:
                    return static_cast<long double>(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return static_cast<long double>(std::get<double>(value));
                case TypeTag::LONG:
                    return static_cast<long double>(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return static_cast<long double>(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return std::get<long double>(value);
                case TypeTag::UINT:
                    return static_cast<long double>(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return static_cast<long double>(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return static_cast<long double>(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? 1.0L : 0.0L;
                default:
                    throw std::runtime_error("Cannot convert to long double");
                }
            }

            // Convert to string (for string concatenation)
            std::string toString() const
            {
                switch (tag_)
                {
                case TypeTag::NONE:
                    return "None";
                case TypeTag::INT:
                    return std::to_string(std::get<int>(value));
                case TypeTag::FLOAT:
                    return std::to_string(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return std::to_string(std::get<double>(value));
                case TypeTag::LONG:
                    return std::to_string(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return std::to_string(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return std::to_string(std::get<long double>(value));
                case TypeTag::UINT:
                    return std::to_string(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return std::to_string(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return std::to_string(std::get<unsigned long long>(value));
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? "True" : "False";
                case TypeTag::STRING:
                    return std::get<std::string>(value);
                default:
                    return "[" + type() + "]";
                }
            }

            // ============ Type Promotion System ============

            // Get promoted type for binary operations
            static TypeTag getPromotedType(TypeTag a, TypeTag b) noexcept
            {
                int rankA = getTypeRank(a);
                int rankB = getTypeRank(b);

                // If either is STRING, result is STRING
                if (a == TypeTag::STRING || b == TypeTag::STRING)
                    return TypeTag::STRING;

                // If either is non-promotable, return the one that is promotable
                if (rankA < 0)
                    return (rankB >= 0) ? b : TypeTag::NONE;
                if (rankB < 0)
                    return a;

                // Return the higher rank type
                return (rankA >= rankB) ? a : b;
            }

            // Type name - returns string like "int", "str", "list", "dict", "NoneType" etc.
            // OPTIMIZED: Use tag for common cases
            std::string type() const
            {
                switch (tag_)
                {
                case TypeTag::NONE:
                    return "NoneType";
                case TypeTag::INT:
                    return "int";
                case TypeTag::FLOAT:
                    return "float";
                case TypeTag::STRING:
                    return "str";
                case TypeTag::BOOL:
                    return "bool";
                case TypeTag::DOUBLE:
                    return "double";
                case TypeTag::LONG:
                    return "long";
                case TypeTag::LONG_LONG:
                    return "long long";
                case TypeTag::LONG_DOUBLE:
                    return "long double";
                case TypeTag::UINT:
                    return "unsigned int";
                case TypeTag::ULONG:
                    return "unsigned long";
                case TypeTag::ULONG_LONG:
                    return "unsigned long long";
                case TypeTag::LIST:
                    return "list";
                case TypeTag::SET:
                    return "set";
                case TypeTag::DICT:
                    return "dict";
                case TypeTag::ORDEREDSET:
                    return "ordered_set";
                case TypeTag::ORDEREDDICT:
                    return "ordereddict";
                case TypeTag::GRAPH:
                    return "graph";
                default:
                    return "unknown";
                }
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
            // OPTIMIZED: Uses TypeTag for fast dispatch instead of std::visit
            std::string str() const
            {
                switch (tag_)
                {
                case TypeTag::NONE:
                    return "None";
                case TypeTag::STRING:
                    return std::get<std::string>(value);
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? "True" : "False";
                case TypeTag::INT:
                    return std::to_string(std::get<int>(value));
                case TypeTag::LONG:
                    return std::to_string(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return std::to_string(std::get<long long>(value));
                case TypeTag::UINT:
                    return std::to_string(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return std::to_string(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return std::to_string(std::get<unsigned long long>(value));
                case TypeTag::FLOAT:
                {
                    std::ostringstream ss;
                    ss << std::get<float>(value);
                    return ss.str();
                }
                case TypeTag::DOUBLE:
                {
                    std::ostringstream ss;
                    ss << std::get<double>(value);
                    return ss.str();
                }
                case TypeTag::LONG_DOUBLE:
                {
                    std::ostringstream ss;
                    ss << std::get<long double>(value);
                    return ss.str();
                }
                case TypeTag::LIST:
                {
                    const auto &lst = std::get<List>(value);
                    std::string result = "[";
                    for (size_t i = 0; i < lst.size(); ++i)
                    {
                        if (i > 0)
                            result += ", ";
                        result += lst[i].str();
                    }
                    result += "]";
                    return result;
                }
                case TypeTag::SET:
                {
                    const auto &st = std::get<Set>(value);
                    std::string result = "{";
                    bool first = true;
                    for (const auto &item : st)
                    {
                        if (!first)
                            result += ", ";
                        result += item.str();
                        first = false;
                    }
                    result += "}";
                    return result;
                }
                case TypeTag::DICT:
                {
                    const auto &dct = std::get<Dict>(value);
                    std::string result = "{";
                    bool first = true;
                    for (const auto &[k, v] : dct)
                    {
                        if (!first)
                            result += ", ";
                        result += "\"" + k + "\": " + v.str();
                        first = false;
                    }
                    result += "}";
                    return result;
                }
                case TypeTag::ORDEREDSET:
                {
                    const auto &hs = std::get<OrderedSet>(value);
                    std::string result = "OrderedSet{";
                    bool first = true;
                    for (const auto &item : hs)
                    {
                        if (!first)
                            result += ", ";
                        result += item.str();
                        first = false;
                    }
                    result += "}";
                    return result;
                }
                case TypeTag::ORDEREDDICT:
                {
                    const auto &od = std::get<OrderedDict>(value);
                    std::string result = "OrderedDict{";
                    bool first = true;
                    for (const auto &[k, v] : od)
                    {
                        if (!first)
                            result += ", ";
                        result += "\"" + k + "\": " + v.str();
                        first = false;
                    }
                    result += "}";
                    return result;
                }
                case TypeTag::GRAPH:
                {
                    // Note: We can't call g->str() here because VarGraphWrapper is forward-declared
                    // The graph_str() helper method is defined after VarGraphWrapper
                    return graph_str_impl();
                }
                default:
                    return "[unknown]";
                }
            }

            // Private helper for graph string - defined after VarGraphWrapper
            std::string graph_str_impl() const;

            // Pretty string with indentation (for pprint)
            // OPTIMIZED: Uses TypeTag for fast dispatch instead of std::visit
            std::string pretty_str(int indent = 0, int indent_step = 2) const
            {
                std::string ind(indent, ' ');
                std::string inner_ind(indent + indent_step, ' ');

                switch (tag_)
                {
                case TypeTag::NONE:
                    return "None";
                case TypeTag::STRING:
                    return "\"" + std::get<std::string>(value) + "\"";
                case TypeTag::BOOL:
                    return std::get<bool>(value) ? "True" : "False";
                case TypeTag::INT:
                    return std::to_string(std::get<int>(value));
                case TypeTag::LONG:
                    return std::to_string(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return std::to_string(std::get<long long>(value));
                case TypeTag::UINT:
                    return std::to_string(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return std::to_string(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return std::to_string(std::get<unsigned long long>(value));
                case TypeTag::FLOAT:
                {
                    std::ostringstream ss;
                    ss << std::get<float>(value);
                    return ss.str();
                }
                case TypeTag::DOUBLE:
                {
                    std::ostringstream ss;
                    ss << std::get<double>(value);
                    return ss.str();
                }
                case TypeTag::LONG_DOUBLE:
                {
                    std::ostringstream ss;
                    ss << std::get<long double>(value);
                    return ss.str();
                }
                case TypeTag::LIST:
                {
                    const auto &lst = std::get<List>(value);
                    if (lst.empty())
                        return "[]";
                    std::string result = "[\n";
                    for (size_t i = 0; i < lst.size(); ++i)
                    {
                        result += inner_ind + lst[i].pretty_str(indent + indent_step, indent_step);
                        if (i < lst.size() - 1)
                            result += ",";
                        result += "\n";
                    }
                    result += ind + "]";
                    return result;
                }
                case TypeTag::SET:
                {
                    const auto &st = std::get<Set>(value);
                    if (st.empty())
                        return "{}";
                    std::string result = "{\n";
                    size_t i = 0;
                    for (const auto &item : st)
                    {
                        result += inner_ind + item.pretty_str(indent + indent_step, indent_step);
                        if (i < st.size() - 1)
                            result += ",";
                        result += "\n";
                        ++i;
                    }
                    result += ind + "}";
                    return result;
                }
                case TypeTag::DICT:
                {
                    const auto &dct = std::get<Dict>(value);
                    if (dct.empty())
                        return "{}";
                    std::string result = "{\n";
                    size_t i = 0;
                    for (const auto &[k, v] : dct)
                    {
                        result += inner_ind + "\"" + k + "\": " + v.pretty_str(indent + indent_step, indent_step);
                        if (i < dct.size() - 1)
                            result += ",";
                        result += "\n";
                        ++i;
                    }
                    result += ind + "}";
                    return result;
                }
                case TypeTag::ORDEREDSET:
                {
                    const auto &hs = std::get<OrderedSet>(value);
                    if (hs.empty())
                        return "OrderedSet{}";
                    std::string result = "OrderedSet{\n";
                    size_t i = 0;
                    for (const auto &item : hs)
                    {
                        result += inner_ind + item.pretty_str(indent + indent_step, indent_step);
                        if (i < hs.size() - 1)
                            result += ",";
                        result += "\n";
                        ++i;
                    }
                    result += ind + "}";
                    return result;
                }
                case TypeTag::ORDEREDDICT:
                {
                    const auto &od = std::get<OrderedDict>(value);
                    if (od.empty())
                        return "OrderedDict{}";
                    std::string result = "OrderedDict{\n";
                    size_t i = 0;
                    for (const auto &[k, v] : od)
                    {
                        result += inner_ind + "\"" + k + "\": " + v.pretty_str(indent + indent_step, indent_step);
                        if (i < od.size() - 1)
                            result += ",";
                        result += "\n";
                        ++i;
                    }
                    result += ind + "}";
                    return result;
                }
                case TypeTag::GRAPH:
                {
                    return graph_str_impl(); // Graphs don't have nested pretty printing
                }
                default:
                    return "[unknown]";
                }
            }

            friend std::ostream &operator<<(std::ostream &os, const var &v)
            {
                os << v.str();
                return os;
            }

            // Comparison operator for use in containers (std::set)
            // OPTIMIZED: Uses TypeTag for fast same-type dispatch
            bool operator<(const var &other) const
            {
                // Fast-path: same type comparison using tag
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return std::get<int>(value) < std::get<int>(other.value);
                    case TypeTag::DOUBLE:
                        return std::get<double>(value) < std::get<double>(other.value);
                    case TypeTag::STRING:
                        return std::get<std::string>(value) < std::get<std::string>(other.value);
                    case TypeTag::LONG_LONG:
                        return std::get<long long>(value) < std::get<long long>(other.value);
                    case TypeTag::FLOAT:
                        return std::get<float>(value) < std::get<float>(other.value);
                    case TypeTag::LONG:
                        return std::get<long>(value) < std::get<long>(other.value);
                    case TypeTag::BOOL:
                        return std::get<bool>(value) < std::get<bool>(other.value);
                    default:
                        break;
                    }
                }
                // Mixed numeric types - promote to double
                if (isNumeric() && other.isNumeric())
                {
                    return toDouble() < other.toDouble();
                }
                // Different types: compare by type index
                return static_cast<int>(tag_) < static_cast<int>(other.tag_);
            }

            // Arithmetic operators
            // OPTIMIZED: Uses TypeTag for fast same-type dispatch, falls back to type promotion
            var operator+(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) + std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) + std::get<double>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) + std::get<long long>(other.value));
                    case TypeTag::STRING:
                        return var(std::get<std::string>(value) + std::get<std::string>(other.value));
                    case TypeTag::FLOAT:
                        return var(std::get<float>(value) + std::get<float>(other.value));
                    case TypeTag::LONG:
                        return var(std::get<long>(value) + std::get<long>(other.value));
                    case TypeTag::UINT:
                        return var(std::get<unsigned int>(value) + std::get<unsigned int>(other.value));
                    case TypeTag::ULONG:
                        return var(std::get<unsigned long>(value) + std::get<unsigned long>(other.value));
                    case TypeTag::ULONG_LONG:
                        return var(std::get<unsigned long long>(value) + std::get<unsigned long long>(other.value));
                    case TypeTag::LONG_DOUBLE:
                        return var(std::get<long double>(value) + std::get<long double>(other.value));
                    case TypeTag::LIST:
                    {
                        const auto &a = std::get<List>(value);
                        const auto &b = std::get<List>(other.value);
                        List result;
                        result.reserve(a.size() + b.size());
                        result.insert(result.end(), a.begin(), a.end());
                        result.insert(result.end(), b.begin(), b.end());
                        return var(std::move(result));
                    }
                    default:
                        break;
                    }
                }
                // Mixed types - use type promotion (handles numeric + numeric, string + anything, etc.)
                return addPromoted(other);
            }

            var operator-(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) - std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) - std::get<double>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) - std::get<long long>(other.value));
                    case TypeTag::FLOAT:
                        return var(std::get<float>(value) - std::get<float>(other.value));
                    case TypeTag::LONG:
                        return var(std::get<long>(value) - std::get<long>(other.value));
                    case TypeTag::UINT:
                        return var(std::get<unsigned int>(value) - std::get<unsigned int>(other.value));
                    case TypeTag::ULONG:
                        return var(std::get<unsigned long>(value) - std::get<unsigned long>(other.value));
                    case TypeTag::ULONG_LONG:
                        return var(std::get<unsigned long long>(value) - std::get<unsigned long long>(other.value));
                    case TypeTag::LONG_DOUBLE:
                        return var(std::get<long double>(value) - std::get<long double>(other.value));
                    case TypeTag::SET:
                    {
                        const auto &a = std::get<Set>(value);
                        const auto &b = std::get<Set>(other.value);
                        Set result;
                        for (const auto &item : a)
                        {
                            if (b.find(item) == b.end())
                            {
                                result.insert(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::LIST:
                    {
                        const auto &a = std::get<List>(value);
                        const auto &b = std::get<List>(other.value);
                        List result;
                        Set b_set(b.begin(), b.end());
                        for (const auto &item : a)
                        {
                            if (b_set.find(item) == b_set.end())
                            {
                                result.push_back(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::DICT:
                    {
                        const auto &a = std::get<Dict>(value);
                        const auto &b = std::get<Dict>(other.value);
                        Dict result;
                        for (const auto &[key, val] : a)
                        {
                            if (b.find(key) == b.end())
                            {
                                result[key] = val;
                            }
                        }
                        return var(std::move(result));
                    }
                    default:
                        break;
                    }
                }
                // Mixed numeric types - use proper type promotion
                if (isNumeric() && other.isNumeric())
                {
                    return subPromoted(other);
                }
                throw std::runtime_error("operator- requires arithmetic types or containers (difference)");
            }

            var operator*(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) * std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) * std::get<double>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) * std::get<long long>(other.value));
                    case TypeTag::FLOAT:
                        return var(std::get<float>(value) * std::get<float>(other.value));
                    case TypeTag::LONG:
                        return var(std::get<long>(value) * std::get<long>(other.value));
                    case TypeTag::UINT:
                        return var(std::get<unsigned int>(value) * std::get<unsigned int>(other.value));
                    case TypeTag::ULONG:
                        return var(std::get<unsigned long>(value) * std::get<unsigned long>(other.value));
                    case TypeTag::ULONG_LONG:
                        return var(std::get<unsigned long long>(value) * std::get<unsigned long long>(other.value));
                    case TypeTag::LONG_DOUBLE:
                        return var(std::get<long double>(value) * std::get<long double>(other.value));
                    default:
                        break;
                    }
                }
                // Mixed numeric types - use proper type promotion
                if (isNumeric() && other.isNumeric())
                {
                    return mulPromoted(other);
                }
                // String * int = repetition
                if (tag_ == TypeTag::STRING && other.isIntegral())
                {
                    const auto &s = std::get<std::string>(value);
                    long long n = other.toLongLong();
                    if (n <= 0)
                        return var(std::string(""));
                    std::string result;
                    result.reserve(s.size() * n);
                    for (long long i = 0; i < n; ++i)
                        result += s;
                    return var(std::move(result));
                }
                // int * String = repetition (commutative)
                if (isIntegral() && other.tag_ == TypeTag::STRING)
                {
                    const auto &s = std::get<std::string>(other.value);
                    long long n = toLongLong();
                    if (n <= 0)
                        return var(std::string(""));
                    std::string result;
                    result.reserve(s.size() * n);
                    for (long long i = 0; i < n; ++i)
                        result += s;
                    return var(std::move(result));
                }
                // List * int = repetition
                if (tag_ == TypeTag::LIST && other.isIntegral())
                {
                    const auto &lst = std::get<List>(value);
                    long long n = other.toLongLong();
                    if (n <= 0)
                        return var(List{});
                    List result;
                    result.reserve(lst.size() * n);
                    for (long long i = 0; i < n; ++i)
                    {
                        result.insert(result.end(), lst.begin(), lst.end());
                    }
                    return var(std::move(result));
                }
                throw std::runtime_error("Unsupported types for multiplication");
            }

            var operator/(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                    {
                        int b = std::get<int>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Division by zero");
                        return var(std::get<int>(value) / b);
                    }
                    case TypeTag::DOUBLE:
                    {
                        double b = std::get<double>(other.value);
                        if (b == 0.0)
                            throw std::runtime_error("Division by zero");
                        return var(std::get<double>(value) / b);
                    }
                    case TypeTag::LONG_LONG:
                    {
                        long long b = std::get<long long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Division by zero");
                        return var(std::get<long long>(value) / b);
                    }
                    case TypeTag::FLOAT:
                    {
                        float b = std::get<float>(other.value);
                        if (b == 0.0f)
                            throw std::runtime_error("Division by zero");
                        return var(std::get<float>(value) / b);
                    }
                    case TypeTag::LONG:
                    {
                        long b = std::get<long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Division by zero");
                        return var(std::get<long>(value) / b);
                    }
                    case TypeTag::LONG_DOUBLE:
                    {
                        long double b = std::get<long double>(other.value);
                        if (b == 0.0L)
                            throw std::runtime_error("Division by zero");
                        return var(std::get<long double>(value) / b);
                    }
                    default:
                        break;
                    }
                }
                // Mixed numeric types - use proper type promotion
                if (isNumeric() && other.isNumeric())
                {
                    return divPromoted(other);
                }
                throw std::runtime_error("Unsupported types for division");
            }

            var operator%(const var &other) const
            {
                // Fast-path: same integer type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                    {
                        int b = std::get<int>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        return var(std::get<int>(value) % b);
                    }
                    case TypeTag::LONG_LONG:
                    {
                        long long b = std::get<long long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        return var(std::get<long long>(value) % b);
                    }
                    case TypeTag::LONG:
                    {
                        long b = std::get<long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        return var(std::get<long>(value) % b);
                    }
                    case TypeTag::UINT:
                    {
                        unsigned int b = std::get<unsigned int>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        return var(std::get<unsigned int>(value) % b);
                    }
                    case TypeTag::ULONG:
                    {
                        unsigned long b = std::get<unsigned long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        return var(std::get<unsigned long>(value) % b);
                    }
                    case TypeTag::ULONG_LONG:
                    {
                        unsigned long long b = std::get<unsigned long long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        return var(std::get<unsigned long long>(value) % b);
                    }
                    default:
                        break;
                    }
                }
                // Mixed integer types - use proper type promotion
                if (isIntegral() && other.isIntegral())
                {
                    return modPromoted(other);
                }
                // Mixed with floating point
                if (isNumeric() && other.isNumeric())
                {
                    return modPromoted(other);
                }
                throw std::runtime_error("Unsupported types for modulo");
            }

            // ============ In-Place Arithmetic Operators ============
            // These modify the value in-place for better performance
            // Avoids creating new var objects in hot loops

            var &operator+=(const var &other)
            {
                // Fast-path: same type, modify in-place
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        std::get<int>(value) += std::get<int>(other.value);
                        return *this;
                    case TypeTag::DOUBLE:
                        std::get<double>(value) += std::get<double>(other.value);
                        return *this;
                    case TypeTag::LONG_LONG:
                        std::get<long long>(value) += std::get<long long>(other.value);
                        return *this;
                    case TypeTag::FLOAT:
                        std::get<float>(value) += std::get<float>(other.value);
                        return *this;
                    case TypeTag::LONG:
                        std::get<long>(value) += std::get<long>(other.value);
                        return *this;
                    case TypeTag::STRING:
                        std::get<std::string>(value) += std::get<std::string>(other.value);
                        return *this;
                    default:
                        break;
                    }
                }
                // Fallback to operator+ (creates new var then assigns)
                *this = *this + other;
                return *this;
            }

            var &operator-=(const var &other)
            {
                // Fast-path: same type, modify in-place
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        std::get<int>(value) -= std::get<int>(other.value);
                        return *this;
                    case TypeTag::DOUBLE:
                        std::get<double>(value) -= std::get<double>(other.value);
                        return *this;
                    case TypeTag::LONG_LONG:
                        std::get<long long>(value) -= std::get<long long>(other.value);
                        return *this;
                    case TypeTag::FLOAT:
                        std::get<float>(value) -= std::get<float>(other.value);
                        return *this;
                    case TypeTag::LONG:
                        std::get<long>(value) -= std::get<long>(other.value);
                        return *this;
                    default:
                        break;
                    }
                }
                // Fallback
                *this = *this - other;
                return *this;
            }

            var &operator*=(const var &other)
            {
                // Fast-path: same type, modify in-place
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        std::get<int>(value) *= std::get<int>(other.value);
                        return *this;
                    case TypeTag::DOUBLE:
                        std::get<double>(value) *= std::get<double>(other.value);
                        return *this;
                    case TypeTag::LONG_LONG:
                        std::get<long long>(value) *= std::get<long long>(other.value);
                        return *this;
                    case TypeTag::FLOAT:
                        std::get<float>(value) *= std::get<float>(other.value);
                        return *this;
                    case TypeTag::LONG:
                        std::get<long>(value) *= std::get<long>(other.value);
                        return *this;
                    default:
                        break;
                    }
                }
                // Fallback
                *this = *this * other;
                return *this;
            }

            var &operator/=(const var &other)
            {
                // Fast-path: same type, modify in-place
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                    {
                        int b = std::get<int>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Division by zero");
                        std::get<int>(value) /= b;
                        return *this;
                    }
                    case TypeTag::DOUBLE:
                    {
                        double b = std::get<double>(other.value);
                        if (b == 0.0)
                            throw std::runtime_error("Division by zero");
                        std::get<double>(value) /= b;
                        return *this;
                    }
                    case TypeTag::LONG_LONG:
                    {
                        long long b = std::get<long long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Division by zero");
                        std::get<long long>(value) /= b;
                        return *this;
                    }
                    case TypeTag::FLOAT:
                    {
                        float b = std::get<float>(other.value);
                        if (b == 0.0f)
                            throw std::runtime_error("Division by zero");
                        std::get<float>(value) /= b;
                        return *this;
                    }
                    default:
                        break;
                    }
                }
                // Fallback
                *this = *this / other;
                return *this;
            }

            var &operator%=(const var &other)
            {
                // Fast-path: same type, modify in-place
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                    {
                        int b = std::get<int>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        std::get<int>(value) %= b;
                        return *this;
                    }
                    case TypeTag::LONG_LONG:
                    {
                        long long b = std::get<long long>(other.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        std::get<long long>(value) %= b;
                        return *this;
                    }
                    default:
                        break;
                    }
                }
                // Fallback
                *this = *this % other;
                return *this;
            }

            // In-place operators with native types
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var &operator+=(T other)
            {
                if (tag_ == TypeTag::INT && std::is_same_v<T, int>)
                {
                    std::get<int>(value) += other;
                }
                else if (tag_ == TypeTag::DOUBLE && std::is_floating_point_v<T>)
                {
                    std::get<double>(value) += static_cast<double>(other);
                }
                else if (tag_ == TypeTag::LONG_LONG && std::is_integral_v<T>)
                {
                    std::get<long long>(value) += static_cast<long long>(other);
                }
                else
                {
                    *this = *this + var(other);
                }
                return *this;
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var &operator-=(T other)
            {
                if (tag_ == TypeTag::INT && std::is_same_v<T, int>)
                {
                    std::get<int>(value) -= other;
                }
                else if (tag_ == TypeTag::DOUBLE && std::is_floating_point_v<T>)
                {
                    std::get<double>(value) -= static_cast<double>(other);
                }
                else if (tag_ == TypeTag::LONG_LONG && std::is_integral_v<T>)
                {
                    std::get<long long>(value) -= static_cast<long long>(other);
                }
                else
                {
                    *this = *this - var(other);
                }
                return *this;
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var &operator*=(T other)
            {
                if (tag_ == TypeTag::INT && std::is_same_v<T, int>)
                {
                    std::get<int>(value) *= other;
                }
                else if (tag_ == TypeTag::DOUBLE && std::is_floating_point_v<T>)
                {
                    std::get<double>(value) *= static_cast<double>(other);
                }
                else if (tag_ == TypeTag::LONG_LONG && std::is_integral_v<T>)
                {
                    std::get<long long>(value) *= static_cast<long long>(other);
                }
                else
                {
                    *this = *this * var(other);
                }
                return *this;
            }

            // Comparison operators (return var(bool) for Pythonic style)
            // OPTIMIZED: Uses TypeTag for fast same-type dispatch
            var operator==(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) == std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) == std::get<double>(other.value));
                    case TypeTag::STRING:
                        return var(std::get<std::string>(value) == std::get<std::string>(other.value));
                    case TypeTag::BOOL:
                        return var(std::get<bool>(value) == std::get<bool>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) == std::get<long long>(other.value));
                    case TypeTag::FLOAT:
                        return var(std::get<float>(value) == std::get<float>(other.value));
                    default:
                        break;
                    }
                }
                // Mixed numeric types - promote to double
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() == other.toDouble());
                }
                return var(false);
            }

            var operator!=(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) != std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) != std::get<double>(other.value));
                    case TypeTag::STRING:
                        return var(std::get<std::string>(value) != std::get<std::string>(other.value));
                    case TypeTag::BOOL:
                        return var(std::get<bool>(value) != std::get<bool>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) != std::get<long long>(other.value));
                    default:
                        break;
                    }
                }
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() != other.toDouble());
                }
                return var(true);
            }

            var operator>(const var &other) const
            {
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) > std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) > std::get<double>(other.value));
                    case TypeTag::STRING:
                        return var(std::get<std::string>(value) > std::get<std::string>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) > std::get<long long>(other.value));
                    default:
                        break;
                    }
                }
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() > other.toDouble());
                }
                throw std::runtime_error("Unsupported types for comparison");
            }

            var operator>=(const var &other) const
            {
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) >= std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) >= std::get<double>(other.value));
                    case TypeTag::STRING:
                        return var(std::get<std::string>(value) >= std::get<std::string>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) >= std::get<long long>(other.value));
                    default:
                        break;
                    }
                }
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() >= other.toDouble());
                }
                throw std::runtime_error("Unsupported types for comparison");
            }

            var operator<=(const var &other) const
            {
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) <= std::get<int>(other.value));
                    case TypeTag::DOUBLE:
                        return var(std::get<double>(value) <= std::get<double>(other.value));
                    case TypeTag::STRING:
                        return var(std::get<std::string>(value) <= std::get<std::string>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) <= std::get<long long>(other.value));
                    default:
                        break;
                    }
                }
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() <= other.toDouble());
                }
                throw std::runtime_error("Unsupported types for comparison");
            }

            // ============ OPTIMIZED Implicit Conversion Operators for Arithmetic Types ============
            // Fast-path: directly operate on primitives using TypeTag

            // Addition with primitives
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator+(T other) const
            {
                // Fast-path using tag
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) + other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(std::get<long long>(value) + other);
                    if (tag_ == TypeTag::INT)
                        return var(static_cast<long long>(std::get<int>(value)) + other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(std::get<double>(value) + static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() + static_cast<double>(other));
                }
                return *this + var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator+(T lhs, const var &rhs)
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (rhs.tag_ == TypeTag::INT)
                        return var(lhs + std::get<int>(rhs.value));
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (rhs.tag_ == TypeTag::LONG_LONG)
                        return var(lhs + std::get<long long>(rhs.value));
                    if (rhs.tag_ == TypeTag::INT)
                        return var(lhs + static_cast<long long>(std::get<int>(rhs.value)));
                }
                return var(lhs) + rhs;
            }

            // OPTIMIZED: Direct const char* handling avoids var construction
            var operator+(const char *other) const
            {
                if (tag_ == TypeTag::STRING)
                {
                    return var(std::get<std::string>(value) + other);
                }
                return var(str() + other);
            }
            friend var operator+(const char *lhs, const var &rhs)
            {
                if (rhs.tag_ == TypeTag::STRING)
                {
                    return var(std::string(lhs) + std::get<std::string>(rhs.value));
                }
                return var(std::string(lhs) + rhs.str());
            }

            // Subtraction with primitives
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator-(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) - other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(std::get<long long>(value) - other);
                    if (tag_ == TypeTag::INT)
                        return var(static_cast<long long>(std::get<int>(value)) - other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(std::get<double>(value) - static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() - static_cast<double>(other));
                }
                return *this - var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator-(T lhs, const var &rhs)
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (rhs.tag_ == TypeTag::INT)
                        return var(lhs - std::get<int>(rhs.value));
                }
                return var(lhs) - rhs;
            }

            // Multiplication with primitives
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator*(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) * other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(std::get<long long>(value) * other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(std::get<double>(value) * static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() * static_cast<double>(other));
                }
                return *this * var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator*(T lhs, const var &rhs)
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (rhs.tag_ == TypeTag::INT)
                        return var(lhs * std::get<int>(rhs.value));
                }
                return var(lhs) * rhs;
            }

            // Division with primitives
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator/(T other) const
            {
                if (other == 0)
                    throw std::runtime_error("Division by zero");
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) / other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(std::get<double>(value) / static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() / static_cast<double>(other));
                }
                return *this / var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator/(T lhs, const var &rhs)
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (rhs.tag_ == TypeTag::INT)
                    {
                        int b = std::get<int>(rhs.value);
                        if (b == 0)
                            throw std::runtime_error("Division by zero");
                        return var(lhs / b);
                    }
                }
                return var(lhs) / rhs;
            }

            // Modulo with primitives
            template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
            var operator%(T other) const
            {
                if (other == 0)
                    throw std::runtime_error("Modulo by zero");
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) % other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(std::get<long long>(value) % other);
                    if (tag_ == TypeTag::INT)
                        return var(static_cast<long long>(std::get<int>(value)) % other);
                }
                return *this % var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
            friend var operator%(T lhs, const var &rhs)
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (rhs.tag_ == TypeTag::INT)
                    {
                        int b = std::get<int>(rhs.value);
                        if (b == 0)
                            throw std::runtime_error("Modulo by zero");
                        return var(lhs % b);
                    }
                }
                return var(lhs) % rhs;
            }

            // Comparison with primitives
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator==(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) == other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(std::get<double>(value) == static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() == static_cast<double>(other));
                }
                return *this == var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator==(T lhs, const var &rhs) { return rhs == lhs; }

            var operator==(const char *other) const { return *this == var(other); }
            friend var operator==(const char *lhs, const var &rhs) { return var(lhs) == rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator!=(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) != other);
                }
                return *this != var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator!=(T lhs, const var &rhs) { return rhs != lhs; }

            var operator!=(const char *other) const { return *this != var(other); }
            friend var operator!=(const char *lhs, const var &rhs) { return var(lhs) != rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator>(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) > other);
                }
                return *this > var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator>(T lhs, const var &rhs) { return var(lhs) > rhs; }

            var operator>(const char *other) const { return *this > var(other); }
            friend var operator>(const char *lhs, const var &rhs) { return var(lhs) > rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator>=(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) >= other);
                }
                return *this >= var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator>=(T lhs, const var &rhs) { return var(lhs) >= rhs; }

            var operator>=(const char *other) const { return *this >= var(other); }
            friend var operator>=(const char *lhs, const var &rhs) { return var(lhs) >= rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator<(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) < other);
                }
                return var(static_cast<int>(tag_)) < var(static_cast<int>(TypeTag::INT)); // type ordering fallback
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator<(T lhs, const var &rhs) { return var(lhs) > rhs; }

            var operator<(const char *other) const { return var(false); }
            friend var operator<(const char *lhs, const var &rhs) { return var(false); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator<=(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(std::get<int>(value) <= other);
                }
                return *this <= var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator<=(T lhs, const var &rhs) { return var(lhs) <= rhs; }

            var operator<=(const char *other) const { return *this <= var(other); }
            friend var operator<=(const char *lhs, const var &rhs) { return var(lhs) <= rhs; }

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
            // OPTIMIZED: Uses TypeTag for fast dispatch
            var operator~() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return var(~std::get<int>(value));
                case TypeTag::LONG:
                    return var(~std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return var(~std::get<long long>(value));
                case TypeTag::UINT:
                    return var(~std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return var(~std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return var(~std::get<unsigned long long>(value));
                default:
                    throw std::runtime_error("Bitwise NOT requires integral type");
                }
            }

            // OPTIMIZED: Uses TypeTag for fast dispatch
            var operator&(const var &other) const
            {
                // Fast-path: same type using tag
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) & std::get<int>(other.value));
                    case TypeTag::LONG:
                        return var(std::get<long>(value) & std::get<long>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) & std::get<long long>(other.value));
                    case TypeTag::UINT:
                        return var(std::get<unsigned int>(value) & std::get<unsigned int>(other.value));
                    case TypeTag::ULONG:
                        return var(std::get<unsigned long>(value) & std::get<unsigned long>(other.value));
                    case TypeTag::ULONG_LONG:
                        return var(std::get<unsigned long long>(value) & std::get<unsigned long long>(other.value));
                    case TypeTag::SET:
                    {
                        const auto &a = std::get<Set>(value);
                        const auto &b = std::get<Set>(other.value);
                        Set result;
                        for (const auto &item : a)
                        {
                            if (b.find(item) != b.end())
                            {
                                result.insert(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::ORDEREDSET:
                    {
                        const auto &a = std::get<OrderedSet>(value);
                        const auto &b = std::get<OrderedSet>(other.value);
                        OrderedSet result;
                        for (const auto &item : a)
                        {
                            if (b.find(item) != b.end())
                            {
                                result.insert(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::LIST:
                    {
                        const auto &a = std::get<List>(value);
                        const auto &b = std::get<List>(other.value);
                        List result;
                        Set b_set(b.begin(), b.end());
                        for (const auto &item : a)
                        {
                            if (b_set.find(item) != b_set.end())
                            {
                                result.push_back(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::DICT:
                    {
                        const auto &a = std::get<Dict>(value);
                        const auto &b = std::get<Dict>(other.value);
                        Dict result;
                        for (const auto &[key, val] : a)
                        {
                            if (b.find(key) != b.end())
                            {
                                result[key] = val;
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::ORDEREDDICT:
                    {
                        const auto &a = std::get<OrderedDict>(value);
                        const auto &b = std::get<OrderedDict>(other.value);
                        OrderedDict result;
                        for (const auto &[key, val] : a)
                        {
                            if (b.find(key) != b.end())
                            {
                                result[key] = val;
                            }
                        }
                        return var(std::move(result));
                    }
                    default:
                        break;
                    }
                }
                // Mixed integer types - promote to long long
                if (isIntegral() && other.isIntegral())
                {
                    return var(toLongLong() & other.toLongLong());
                }
                throw std::runtime_error("operator& requires integral types (bitwise) or containers (intersection)");
            }

            var operator|(const var &other) const
            {
                // Fast-path: same type using tag
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) | std::get<int>(other.value));
                    case TypeTag::LONG:
                        return var(std::get<long>(value) | std::get<long>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) | std::get<long long>(other.value));
                    case TypeTag::UINT:
                        return var(std::get<unsigned int>(value) | std::get<unsigned int>(other.value));
                    case TypeTag::ULONG:
                        return var(std::get<unsigned long>(value) | std::get<unsigned long>(other.value));
                    case TypeTag::ULONG_LONG:
                        return var(std::get<unsigned long long>(value) | std::get<unsigned long long>(other.value));
                    case TypeTag::SET:
                    {
                        const auto &a = std::get<Set>(value);
                        const auto &b = std::get<Set>(other.value);
                        Set result = a;
                        result.insert(b.begin(), b.end());
                        return var(std::move(result));
                    }
                    case TypeTag::ORDEREDSET:
                    {
                        const auto &a = std::get<OrderedSet>(value);
                        const auto &b = std::get<OrderedSet>(other.value);
                        OrderedSet result = a;
                        result.insert(b.begin(), b.end());
                        return var(std::move(result));
                    }
                    case TypeTag::LIST:
                    {
                        const auto &a = std::get<List>(value);
                        const auto &b = std::get<List>(other.value);
                        List result;
                        result.reserve(a.size() + b.size());
                        result.insert(result.end(), a.begin(), a.end());
                        result.insert(result.end(), b.begin(), b.end());
                        return var(std::move(result));
                    }
                    case TypeTag::DICT:
                    {
                        const auto &a = std::get<Dict>(value);
                        const auto &b = std::get<Dict>(other.value);
                        Dict result = a;
                        for (const auto &[key, val] : b)
                        {
                            result[key] = val;
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::ORDEREDDICT:
                    {
                        const auto &a = std::get<OrderedDict>(value);
                        const auto &b = std::get<OrderedDict>(other.value);
                        OrderedDict result = a;
                        for (const auto &[key, val] : b)
                        {
                            result[key] = val;
                        }
                        return var(std::move(result));
                    }
                    default:
                        break;
                    }
                }
                // Mixed integer types - promote to long long
                if (isIntegral() && other.isIntegral())
                {
                    return var(toLongLong() | other.toLongLong());
                }
                throw std::runtime_error("operator| requires integral types (bitwise) or containers (union/merge)");
            }

            // OPTIMIZED: Uses TypeTag for fast dispatch
            var operator^(const var &other) const
            {
                // Fast-path: same type using tag
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(std::get<int>(value) ^ std::get<int>(other.value));
                    case TypeTag::LONG:
                        return var(std::get<long>(value) ^ std::get<long>(other.value));
                    case TypeTag::LONG_LONG:
                        return var(std::get<long long>(value) ^ std::get<long long>(other.value));
                    case TypeTag::UINT:
                        return var(std::get<unsigned int>(value) ^ std::get<unsigned int>(other.value));
                    case TypeTag::ULONG:
                        return var(std::get<unsigned long>(value) ^ std::get<unsigned long>(other.value));
                    case TypeTag::ULONG_LONG:
                        return var(std::get<unsigned long long>(value) ^ std::get<unsigned long long>(other.value));
                    case TypeTag::SET:
                    {
                        const auto &a = std::get<Set>(value);
                        const auto &b = std::get<Set>(other.value);
                        Set result;
                        for (const auto &item : a)
                        {
                            if (b.find(item) == b.end())
                            {
                                result.insert(item);
                            }
                        }
                        for (const auto &item : b)
                        {
                            if (a.find(item) == a.end())
                            {
                                result.insert(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::ORDEREDSET:
                    {
                        const auto &a = std::get<OrderedSet>(value);
                        const auto &b = std::get<OrderedSet>(other.value);
                        OrderedSet result;
                        for (const auto &item : a)
                        {
                            if (b.find(item) == b.end())
                            {
                                result.insert(item);
                            }
                        }
                        for (const auto &item : b)
                        {
                            if (a.find(item) == a.end())
                            {
                                result.insert(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::LIST:
                    {
                        const auto &a = std::get<List>(value);
                        const auto &b = std::get<List>(other.value);
                        List result;
                        Set a_set(a.begin(), a.end());
                        Set b_set(b.begin(), b.end());
                        for (const auto &item : a)
                        {
                            if (b_set.find(item) == b_set.end())
                            {
                                result.push_back(item);
                            }
                        }
                        for (const auto &item : b)
                        {
                            if (a_set.find(item) == a_set.end())
                            {
                                result.push_back(item);
                            }
                        }
                        return var(std::move(result));
                    }
                    default:
                        break;
                    }
                }
                // Mixed integer types - promote to long long
                if (isIntegral() && other.isIntegral())
                {
                    return var(toLongLong() ^ other.toLongLong());
                }
                throw std::runtime_error("operator^ requires integral types (bitwise) or sets/lists (symmetric difference)");
            }

            // Bool conversion
            // OPTIMIZED: Uses TypeTag for fast dispatch instead of std::visit
            operator bool() const
            {
                switch (tag_)
                {
                case TypeTag::NONE:
                    return false;
                case TypeTag::BOOL:
                    return std::get<bool>(value);
                case TypeTag::INT:
                    return std::get<int>(value) != 0;
                case TypeTag::LONG:
                    return std::get<long>(value) != 0;
                case TypeTag::LONG_LONG:
                    return std::get<long long>(value) != 0;
                case TypeTag::UINT:
                    return std::get<unsigned int>(value) != 0;
                case TypeTag::ULONG:
                    return std::get<unsigned long>(value) != 0;
                case TypeTag::ULONG_LONG:
                    return std::get<unsigned long long>(value) != 0;
                case TypeTag::FLOAT:
                    return std::get<float>(value) != 0.0f;
                case TypeTag::DOUBLE:
                    return std::get<double>(value) != 0.0;
                case TypeTag::LONG_DOUBLE:
                    return std::get<long double>(value) != 0.0L;
                case TypeTag::STRING:
                    return !std::get<std::string>(value).empty();
                case TypeTag::LIST:
                    return !std::get<List>(value).empty();
                case TypeTag::SET:
                    return !std::get<Set>(value).empty();
                case TypeTag::DICT:
                    return !std::get<Dict>(value).empty();
                case TypeTag::ORDEREDSET:
                    return !std::get<OrderedSet>(value).empty();
                case TypeTag::ORDEREDDICT:
                    return !std::get<OrderedDict>(value).empty();
                case TypeTag::GRAPH:
                    return graph_bool_impl(); // Defined after VarGraphWrapper
                default:
                    return true;
                }
            }

            // Private helper for graph bool - defined after VarGraphWrapper
            bool graph_bool_impl() const;

            // Container access - operator[] for list and dict
            // OPTIMIZED: Uses TypeTag for fast dispatch
            var &operator[](size_t index)
            {
                if (tag_ == TypeTag::LIST)
                {
                    auto &lst = std::get<List>(value);
                    if (index >= lst.size())
                    {
                        throw std::out_of_range("List index out of range");
                    }
                    return lst[index];
                }
                throw std::runtime_error("operator[size_t] requires a list");
            }

            const var &operator[](size_t index) const
            {
                if (tag_ == TypeTag::LIST)
                {
                    const auto &lst = std::get<List>(value);
                    if (index >= lst.size())
                    {
                        throw std::out_of_range("List index out of range");
                    }
                    return lst[index];
                }
                throw std::runtime_error("operator[size_t] requires a list");
            }

            var &operator[](const std::string &key)
            {
                if (tag_ == TypeTag::DICT)
                {
                    return std::get<Dict>(value)[key];
                }
                if (tag_ == TypeTag::ORDEREDDICT)
                {
                    return std::get<OrderedDict>(value)[key];
                }
                throw std::runtime_error("operator[string] requires a dict or ordered_dict");
            }

            var &operator[](const char *key)
            {
                return (*this)[std::string(key)];
            }

            // len() support
            // OPTIMIZED: Uses TypeTag for fast dispatch instead of std::visit
            size_t len() const
            {
                switch (tag_)
                {
                case TypeTag::STRING:
                    return std::get<std::string>(value).size();
                case TypeTag::LIST:
                    return std::get<List>(value).size();
                case TypeTag::SET:
                    return std::get<Set>(value).size();
                case TypeTag::DICT:
                    return std::get<Dict>(value).size();
                case TypeTag::ORDEREDSET:
                    return std::get<OrderedSet>(value).size();
                case TypeTag::ORDEREDDICT:
                    return std::get<OrderedDict>(value).size();
                default:
                    throw std::runtime_error("len() not supported for this type");
                }
            }

            // append for list
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void append(const var &v)
            {
                if (tag_ == TypeTag::LIST)
                {
                    std::get<List>(value).push_back(v);
                }
                else
                {
                    throw std::runtime_error("append() requires a list");
                }
            }

            // OPTIMIZED: Template overload for primitives - avoids var construction
            template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, var>>>
            void append(T &&v)
            {
                if (tag_ == TypeTag::LIST)
                {
                    std::get<List>(value).emplace_back(std::forward<T>(v));
                }
                else
                {
                    throw std::runtime_error("append() requires a list");
                }
            }

            // add for set or ordered_set
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void add(const var &v)
            {
                if (tag_ == TypeTag::SET)
                {
                    std::get<Set>(value).insert(v);
                }
                else if (tag_ == TypeTag::ORDEREDSET)
                {
                    std::get<OrderedSet>(value).insert(v);
                }
                else
                {
                    throw std::runtime_error("add() requires a set or ordered_set");
                }
            }

            // OPTIMIZED: Template overload for primitives - avoids var construction
            template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, var>>>
            void add(T &&v)
            {
                if (tag_ == TypeTag::SET)
                {
                    std::get<Set>(value).emplace(std::forward<T>(v));
                }
                else if (tag_ == TypeTag::ORDEREDSET)
                {
                    std::get<OrderedSet>(value).emplace(std::forward<T>(v));
                }
                else
                {
                    throw std::runtime_error("add() requires a set or ordered_set");
                }
            }

            // extend for list - adds all elements from another iterable
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void extend(const var &other)
            {
                if (tag_ != TypeTag::LIST)
                {
                    throw std::runtime_error("extend() requires a list");
                }
                auto &lst = std::get<List>(value);
                switch (other.tag_)
                {
                case TypeTag::LIST:
                {
                    const auto &other_lst = std::get<List>(other.value);
                    lst.insert(lst.end(), other_lst.begin(), other_lst.end());
                    break;
                }
                case TypeTag::SET:
                {
                    const auto &other_set = std::get<Set>(other.value);
                    for (const auto &item : other_set)
                    {
                        lst.push_back(item);
                    }
                    break;
                }
                case TypeTag::STRING:
                {
                    const auto &other_str = std::get<std::string>(other.value);
                    for (char c : other_str)
                    {
                        lst.push_back(var(std::string(1, c)));
                    }
                    break;
                }
                default:
                    throw std::runtime_error("extend() requires an iterable (list, set, or string)");
                }
            }

            // update for set - adds all elements from another iterable (like Python's set.update())
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void update(const var &other)
            {
                if (tag_ != TypeTag::SET)
                {
                    throw std::runtime_error("update() requires a set");
                }
                auto &st = std::get<Set>(value);
                switch (other.tag_)
                {
                case TypeTag::SET:
                {
                    const auto &other_set = std::get<Set>(other.value);
                    st.insert(other_set.begin(), other_set.end());
                    break;
                }
                case TypeTag::LIST:
                {
                    const auto &other_lst = std::get<List>(other.value);
                    for (const auto &item : other_lst)
                    {
                        st.insert(item);
                    }
                    break;
                }
                default:
                    throw std::runtime_error("update() requires an iterable (set or list)");
                }
            }

            // contains check (in operator simulation)
            // OPTIMIZED: Uses TypeTag for fast dispatch
            bool contains(const var &v) const
            {
                switch (tag_)
                {
                case TypeTag::LIST:
                {
                    const auto &lst = std::get<List>(value);
                    for (const auto &item : lst)
                    {
                        if (static_cast<bool>(item == v))
                            return true;
                    }
                    return false;
                }
                case TypeTag::SET:
                    return std::get<Set>(value).find(v) != std::get<Set>(value).end();
                case TypeTag::ORDEREDSET:
                    return std::get<OrderedSet>(value).find(v) != std::get<OrderedSet>(value).end();
                case TypeTag::DICT:
                    if (v.tag_ == TypeTag::STRING)
                    {
                        return std::get<Dict>(value).find(std::get<std::string>(v.value)) != std::get<Dict>(value).end();
                    }
                    return false;
                case TypeTag::ORDEREDDICT:
                    if (v.tag_ == TypeTag::STRING)
                    {
                        return std::get<OrderedDict>(value).find(std::get<std::string>(v.value)) != std::get<OrderedDict>(value).end();
                    }
                    return false;
                case TypeTag::STRING:
                    if (v.tag_ == TypeTag::STRING)
                    {
                        return std::get<std::string>(value).find(std::get<std::string>(v.value)) != std::string::npos;
                    }
                    return false;
                default:
                    return false;
                }
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
                    std::string::iterator,
                    OrderedSet::iterator,
                    OrderedDict::iterator>
                    it;
                enum class IterType
                {
                    LIST,
                    SET,
                    DICT,
                    STRING,
                    ORDEREDSET,
                    ORDEREDDICT
                } type;

            public:
                iterator() : type(IterType::LIST) {}
                iterator(List::iterator i) : it(i), type(IterType::LIST) {}
                iterator(Set::iterator i) : it(i), type(IterType::SET) {}
                iterator(Dict::iterator i) : it(i), type(IterType::DICT) {}
                iterator(std::string::iterator i) : it(i), type(IterType::STRING) {}
                iterator(OrderedSet::iterator i) : it(i), type(IterType::ORDEREDSET) {}
                iterator(OrderedDict::iterator i) : it(i), type(IterType::ORDEREDDICT) {}

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
                    case IterType::ORDEREDSET:
                        return *std::get<OrderedSet::iterator>(it);
                    case IterType::ORDEREDDICT:
                    {
                        auto &p = *std::get<OrderedDict::iterator>(it);
                        return var(p.first); // Return key for ordereddict iteration
                    }
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
                    case IterType::ORDEREDSET:
                        ++std::get<OrderedSet::iterator>(it);
                        break;
                    case IterType::ORDEREDDICT:
                        ++std::get<OrderedDict::iterator>(it);
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
                    case IterType::ORDEREDSET:
                        return std::get<OrderedSet::iterator>(it) == std::get<OrderedSet::iterator>(other.it);
                    case IterType::ORDEREDDICT:
                        return std::get<OrderedDict::iterator>(it) == std::get<OrderedDict::iterator>(other.it);
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
                    std::string::const_iterator,
                    OrderedSet::const_iterator,
                    OrderedDict::const_iterator>
                    it;
                enum class IterType
                {
                    LIST,
                    SET,
                    DICT,
                    STRING,
                    ORDEREDSET,
                    ORDEREDDICT
                } type;

            public:
                const_iterator() : type(IterType::LIST) {}
                const_iterator(List::const_iterator i) : it(i), type(IterType::LIST) {}
                const_iterator(Set::const_iterator i) : it(i), type(IterType::SET) {}
                const_iterator(Dict::const_iterator i) : it(i), type(IterType::DICT) {}
                const_iterator(std::string::const_iterator i) : it(i), type(IterType::STRING) {}
                const_iterator(OrderedSet::const_iterator i) : it(i), type(IterType::ORDEREDSET) {}
                const_iterator(OrderedDict::const_iterator i) : it(i), type(IterType::ORDEREDDICT) {}

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
                    case IterType::ORDEREDSET:
                        return *std::get<OrderedSet::const_iterator>(it);
                    case IterType::ORDEREDDICT:
                    {
                        auto &p = *std::get<OrderedDict::const_iterator>(it);
                        return var(p.first);
                    }
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
                    case IterType::ORDEREDSET:
                        ++std::get<OrderedSet::const_iterator>(it);
                        break;
                    case IterType::ORDEREDDICT:
                        ++std::get<OrderedDict::const_iterator>(it);
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
                    case IterType::ORDEREDSET:
                        return std::get<OrderedSet::const_iterator>(it) == std::get<OrderedSet::const_iterator>(other.it);
                    case IterType::ORDEREDDICT:
                        return std::get<OrderedDict::const_iterator>(it) == std::get<OrderedDict::const_iterator>(other.it);
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
                if (auto *hs = std::get_if<OrderedSet>(&value))
                    return iterator(hs->begin());
                if (auto *od = std::get_if<OrderedDict>(&value))
                    return iterator(od->begin());
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
                if (auto *hs = std::get_if<OrderedSet>(&value))
                    return iterator(hs->end());
                if (auto *od = std::get_if<OrderedDict>(&value))
                    return iterator(od->end());
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
                if (auto *hs = std::get_if<OrderedSet>(&value))
                    return const_iterator(hs->begin());
                if (auto *od = std::get_if<OrderedDict>(&value))
                    return const_iterator(od->begin());
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
                if (auto *hs = std::get_if<OrderedSet>(&value))
                    return const_iterator(hs->end());
                if (auto *od = std::get_if<OrderedDict>(&value))
                    return const_iterator(od->end());
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

            // Hash function for var - enables use in unordered containers
            size_t hash() const
            {
                // Combine type tag with value hash for unique hashes
                size_t h = static_cast<size_t>(tag_);
                switch (tag_)
                {
                case TypeTag::NONE:
                    return h;
                case TypeTag::INT:
                    return h ^ std::hash<int>{}(std::get<int>(value));
                case TypeTag::FLOAT:
                    return h ^ std::hash<float>{}(std::get<float>(value));
                case TypeTag::DOUBLE:
                    return h ^ std::hash<double>{}(std::get<double>(value));
                case TypeTag::STRING:
                    return h ^ std::hash<std::string>{}(std::get<std::string>(value));
                case TypeTag::BOOL:
                    return h ^ std::hash<bool>{}(std::get<bool>(value));
                case TypeTag::LONG:
                    return h ^ std::hash<long>{}(std::get<long>(value));
                case TypeTag::LONG_LONG:
                    return h ^ std::hash<long long>{}(std::get<long long>(value));
                case TypeTag::LONG_DOUBLE:
                    return h ^ std::hash<long double>{}(std::get<long double>(value));
                case TypeTag::UINT:
                    return h ^ std::hash<unsigned int>{}(std::get<unsigned int>(value));
                case TypeTag::ULONG:
                    return h ^ std::hash<unsigned long>{}(std::get<unsigned long>(value));
                case TypeTag::ULONG_LONG:
                    return h ^ std::hash<unsigned long long>{}(std::get<unsigned long long>(value));
                case TypeTag::LIST:
                {
                    // Hash list by combining element hashes
                    const auto &lst = std::get<List>(value);
                    size_t seed = lst.size();
                    for (const auto &item : lst)
                    {
                        seed ^= item.hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                    }
                    return h ^ seed;
                }
                case TypeTag::SET:
                {
                    // Hash set by XORing element hashes (order-independent)
                    const auto &s = std::get<Set>(value);
                    size_t seed = s.size();
                    for (const auto &item : s)
                    {
                        seed ^= item.hash();
                    }
                    return h ^ seed;
                }
                case TypeTag::DICT:
                {
                    // Hash dict by combining key-value pairs
                    const auto &d = std::get<Dict>(value);
                    size_t seed = d.size();
                    for (const auto &[k, v] : d)
                    {
                        seed ^= std::hash<std::string>{}(k) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                        seed ^= v.hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                    }
                    return h ^ seed;
                }
                case TypeTag::ORDEREDSET:
                {
                    // Hash ordered_set by XORing element hashes (order-independent)
                    const auto &hs = std::get<OrderedSet>(value);
                    size_t seed = hs.size();
                    for (const auto &item : hs)
                    {
                        seed ^= item.hash();
                    }
                    return h ^ seed;
                }
                case TypeTag::ORDEREDDICT:
                {
                    // Hash ordereddict by combining key-value pairs
                    const auto &od = std::get<OrderedDict>(value);
                    size_t seed = od.size();
                    for (const auto &[k, v] : od)
                    {
                        seed ^= std::hash<std::string>{}(k) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                        seed ^= v.hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                    }
                    return h ^ seed;
                }
                case TypeTag::GRAPH:
                {
                    // Hash graph by its pointer address (identity-based)
                    const auto &g = std::get<GraphPtr>(value);
                    return h ^ std::hash<GraphPtr>{}(g);
                }
                default:
                    return h;
                }
            }

            // ============ GRAPH METHODS ============
            // These methods are only valid when the var holds a graph.
            // They delegate to the underlying VarGraphWrapper.
            // IMPORTANT: These are declared here but DEFINED after VarGraphWrapper
            // to avoid incomplete type errors.

        private:
            // Helper to get graph with error checking - defined after VarGraphWrapper
            VarGraphWrapper &graph_ref();
            const VarGraphWrapper &graph_ref() const;

        public:
            // ===== Graph Properties =====
            size_t node_count() const;
            size_t edge_count() const;
            bool is_connected() const;
            bool has_cycle() const;
            bool has_edge(size_t from, size_t to) const;
            std::optional<double> get_edge_weight(size_t from, size_t to) const;
            size_t out_degree(size_t node) const;
            size_t in_degree(size_t node) const;

            // ===== Graph Modification =====
            void add_edge(size_t u, size_t v, double w1 = 0.0, double w2 = 0.0, bool directed = false);
            bool remove_edge(size_t from, size_t to, bool remove_reverse = true);
            void set_edge_weight(size_t from, size_t to, double weight);

            // ===== Capacity Reservation (Optimization) =====
            void reserve_edges_per_node(size_t per_node);
            void reserve_edges_by_counts(const var &counts);

            // ===== Node Data =====
            void set_node_data(size_t node, const var &data);
            var &get_node_data(size_t node);

            // ===== Traversals - Return var (list) for Python-like interface =====
            var dfs(size_t start = 0, bool recursive = true);
            var bfs(size_t start = 0);

            // ===== Shortest Paths =====
            var get_shortest_path(size_t src, size_t dest);
            var bellman_ford(size_t src) const;
            var floyd_warshall() const;

            // ===== Graph Algorithms =====
            var topological_sort() const;
            var connected_components() const;
            var strongly_connected_components() const;
            var prim_mst() const;

            // ===== Graph Serialization =====
            void save_graph(const std::string &filename) const;
            void to_dot(const std::string &filename, bool show_weights = true) const;

            // ===== Get edges for a node =====
            var get_edges(size_t node);

        }; // class var

        // ============ Deferred implementations for VarHasher and VarEqual ============
        // These must be defined after var is complete

        inline size_t VarHasher::operator()(const var &v) const noexcept
        {
            return v.hash();
        }

        inline bool VarEqual::operator()(const var &a, const var &b) const noexcept
        {
            return static_cast<bool>(a == b);
        }

        // ============ Factory functions for containers ============

        // list(1, 2, 3) -> List - OPTIMIZED: reserve + emplace_back
        template <typename... Args>
        var list(Args &&...args)
        {
            List lst;
            lst.reserve(sizeof...(Args));
            (lst.emplace_back(std::forward<Args>(args)), ...);
            return var(std::move(lst));
        }

        // set(1, 2, 3) -> Set
        template <typename... Args>
        var set(Args &&...args)
        {
            Set st;
            (st.emplace(std::forward<Args>(args)), ...);
            return var(std::move(st));
        }

        // ordered_set(1, 2, 3) -> OrderedSet (O(1) average operations)
        template <typename... Args>
        var ordered_set(Args &&...args)
        {
            OrderedSet hs;
            (hs.emplace(std::forward<Args>(args)), ...);
            return var(std::move(hs));
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

        // ordered_dict({"key", value}, ...) -> OrderedDict (maintains key order)
        inline var ordered_dict(std::initializer_list<std::pair<std::string, var>> items)
        {
            OrderedDict od;
            for (const auto &[k, v] : items)
            {
                od[k] = v;
            }
            return var(od);
        }

        // Empty containers
        inline var list() { return var(List{}); }
        inline var set() { return var(Set{}); }
        inline var dict() { return var(Dict{}); }
        inline var ordered_set() { return var(OrderedSet{}); }
        inline var ordered_dict() { return var(OrderedDict{}); }

    } // namespace vars
} // namespace pythonic

// Include Graph after var is fully defined (Graph uses var for node metadata)
#include "Graph.hpp"

namespace pythonic
{
    namespace vars
    {
        // ============ GRAPH INTEGRATION ============

        // VarGraph is a Graph that stores var as node metadata
        // This allows Python-like flexibility: nodes can hold any data type
        using VarGraph = pythonic::graph::Graph<var>;

        /**
         * @brief VarGraphWrapper wraps a VarGraph for use inside var's variant.
         *
         * This class provides the interface between var and the underlying Graph.
         * It's stored via shared_ptr to enable lightweight copying of var containing graphs.
         */
        class VarGraphWrapper
        {
        public:
            VarGraph impl; // The actual graph implementation

            explicit VarGraphWrapper(size_t n) : impl(n) {}
            VarGraphWrapper(const VarGraph &g) : impl(g) {}
            VarGraphWrapper(VarGraph &&g) : impl(std::move(g)) {}

            // ===== Graph Properties =====
            size_t node_count() const { return impl.node_count(); }
            size_t edge_count() const { return impl.edge_count(); }
            bool is_connected() const { return impl.is_connected(); }
            bool has_cycle() const { return impl.has_cycle(); }
            bool has_edge(size_t from, size_t to) const { return impl.has_edge(from, to); }
            std::optional<double> get_edge_weight(size_t from, size_t to) const { return impl.get_edge_weight(from, to); }
            size_t out_degree(size_t node) const { return impl.out_degree(node); }
            size_t in_degree(size_t node) const { return impl.in_degree(node); }

            // ===== Graph Modification =====
            void add_edge(size_t u, size_t v, double w1 = 0.0, double w2 = 0.0, bool directed = false)
            {
                impl.add_edge(u, v, w1, w2, directed);
            }

            bool remove_edge(size_t from, size_t to, bool remove_reverse = true)
            {
                return impl.remove_edge(from, to, remove_reverse);
            }

            void set_edge_weight(size_t from, size_t to, double weight)
            {
                impl.set_edge_weight(from, to, weight);
            }

            // ===== Capacity Reservation (Optimization) =====
            void reserve_edges_per_node(size_t per_node)
            {
                impl.reserve_edges_per_node(per_node);
            }

            void reserve_edges_by_counts(const std::vector<size_t> &counts)
            {
                impl.reserve_edges_by_counts(counts);
            }

            // ===== Node Data =====
            void set_node_data(size_t node, const var &data)
            {
                impl.set_node_data(node, data);
            }

            var &get_node_data(size_t node)
            {
                return impl.get_node_data(node);
            }

            const var &get_node_data(size_t node) const
            {
                return impl.get_node_data(node);
            }

            // ===== Traversals =====
            std::vector<size_t> dfs(size_t start = 0, bool recursive = true)
            {
                return impl.dfs(start, recursive);
            }

            std::vector<size_t> bfs(size_t start = 0)
            {
                return impl.bfs(start);
            }

            // ===== Shortest Paths =====
            std::pair<std::vector<size_t>, double> get_shortest_path(size_t src, size_t dest)
            {
                return impl.get_shortest_path(src, dest);
            }

            std::pair<std::vector<double>, std::vector<size_t>> bellman_ford(size_t src) const
            {
                return impl.bellman_ford(src);
            }

            std::vector<std::vector<double>> floyd_warshall() const
            {
                return impl.floyd_warshall();
            }

            // ===== Graph Algorithms =====
            std::vector<size_t> topological_sort() const
            {
                return impl.topological_sort();
            }

            std::vector<std::vector<size_t>> connected_components() const
            {
                return impl.connected_components();
            }

            std::vector<std::vector<size_t>> strongly_connected_components() const
            {
                return impl.strongly_connected_components();
            }

            std::pair<double, std::vector<std::tuple<size_t, size_t, double>>> prim_mst() const
            {
                return impl.prim_mst();
            }

            // ===== Serialization =====
            void save(const std::string &filename) const
            {
                impl.save(filename);
            }

            void to_dot(const std::string &filename, bool show_weights = true) const
            {
                impl.to_dot(filename, show_weights);
            }

            // ===== Edges =====
            std::vector<pythonic::graph::Edge> get_edges(size_t node)
            {
                return impl.get_edges(node);
            }

            // String representation
            std::string str() const
            {
                std::ostringstream oss;
                oss << "Graph(nodes=" << impl.node_count() << ", edges=" << impl.edge_count() << ")";
                return oss.str();
            }
        };

        // ============ var Graph Method Definitions ============
        // These are defined after VarGraphWrapper is complete to avoid incomplete type errors.

        inline VarGraphWrapper &var::graph_ref()
        {
            if (tag_ != TypeTag::GRAPH)
                throw std::runtime_error("Operation requires a graph");
            auto &ptr = std::get<GraphPtr>(value);
            if (!ptr)
                throw std::runtime_error("Graph is null");
            return *ptr;
        }

        inline const VarGraphWrapper &var::graph_ref() const
        {
            if (tag_ != TypeTag::GRAPH)
                throw std::runtime_error("Operation requires a graph");
            const auto &ptr = std::get<GraphPtr>(value);
            if (!ptr)
                throw std::runtime_error("Graph is null");
            return *ptr;
        }

        // Helper implementations for str() and operator bool()
        inline std::string var::graph_str_impl() const
        {
            return graph_ref().str();
        }

        inline bool var::graph_bool_impl() const
        {
            return graph_ref().node_count() > 0;
        }

        // ===== Graph Properties =====
        inline size_t var::node_count() const { return graph_ref().node_count(); }
        inline size_t var::edge_count() const { return graph_ref().edge_count(); }
        inline bool var::is_connected() const { return graph_ref().is_connected(); }
        inline bool var::has_cycle() const { return graph_ref().has_cycle(); }
        inline bool var::has_edge(size_t from, size_t to) const { return graph_ref().has_edge(from, to); }
        inline std::optional<double> var::get_edge_weight(size_t from, size_t to) const { return graph_ref().get_edge_weight(from, to); }
        inline size_t var::out_degree(size_t node) const { return graph_ref().out_degree(node); }
        inline size_t var::in_degree(size_t node) const { return graph_ref().in_degree(node); }

        // ===== Graph Modification =====
        inline void var::add_edge(size_t u, size_t v, double w1, double w2, bool directed)
        {
            graph_ref().add_edge(u, v, w1, w2, directed);
        }

        inline bool var::remove_edge(size_t from, size_t to, bool remove_reverse)
        {
            return graph_ref().remove_edge(from, to, remove_reverse);
        }

        inline void var::set_edge_weight(size_t from, size_t to, double weight)
        {
            graph_ref().set_edge_weight(from, to, weight);
        }

        // ===== Capacity Reservation (Optimization) =====
        inline void var::reserve_edges_per_node(size_t per_node)
        {
            graph_ref().reserve_edges_per_node(per_node);
        }

        inline void var::reserve_edges_by_counts(const var &counts)
        {
            if (!counts.is<List>())
                throw std::runtime_error("reserve_edges_by_counts requires a list of counts");
            const auto &lst = counts.get<List>();
            std::vector<size_t> vec;
            vec.reserve(lst.size());
            for (const auto &item : lst)
                vec.push_back(static_cast<size_t>(static_cast<long long>(item)));
            graph_ref().reserve_edges_by_counts(vec);
        }

        // ===== Node Data =====
        inline void var::set_node_data(size_t node, const var &data)
        {
            graph_ref().set_node_data(node, data);
        }

        inline var &var::get_node_data(size_t node)
        {
            return graph_ref().get_node_data(node);
        }

        // ===== Traversals - Return var (list) for Python-like interface =====
        inline var var::dfs(size_t start, bool recursive)
        {
            auto result = graph_ref().dfs(start, recursive);
            List lst;
            lst.reserve(result.size());
            for (size_t n : result)
                lst.emplace_back(static_cast<long long>(n));
            return var(std::move(lst));
        }

        inline var var::bfs(size_t start)
        {
            auto result = graph_ref().bfs(start);
            List lst;
            lst.reserve(result.size());
            for (size_t n : result)
                lst.emplace_back(static_cast<long long>(n));
            return var(std::move(lst));
        }

        // ===== Shortest Paths =====
        inline var var::get_shortest_path(size_t src, size_t dest)
        {
            auto [path, dist] = graph_ref().get_shortest_path(src, dest);
            Dict result;
            List path_list;
            path_list.reserve(path.size());
            for (size_t n : path)
                path_list.emplace_back(static_cast<long long>(n));
            result["path"] = var(std::move(path_list));
            result["distance"] = var(dist);
            return var(std::move(result));
        }

        inline var var::bellman_ford(size_t src) const
        {
            auto [dist, prev] = graph_ref().bellman_ford(src);
            Dict result;
            List dist_list, prev_list;
            dist_list.reserve(dist.size());
            prev_list.reserve(prev.size());
            for (double d : dist)
                dist_list.emplace_back(d);
            for (size_t p : prev)
                prev_list.emplace_back(static_cast<long long>(p));
            result["distances"] = var(std::move(dist_list));
            result["predecessors"] = var(std::move(prev_list));
            return var(std::move(result));
        }

        inline var var::floyd_warshall() const
        {
            auto dist = graph_ref().floyd_warshall();
            List result;
            result.reserve(dist.size());
            for (const auto &row : dist)
            {
                List row_list;
                row_list.reserve(row.size());
                for (double d : row)
                    row_list.emplace_back(d);
                result.emplace_back(std::move(row_list));
            }
            return var(std::move(result));
        }

        // ===== Graph Algorithms =====
        inline var var::topological_sort() const
        {
            auto result = graph_ref().topological_sort();
            List lst;
            lst.reserve(result.size());
            for (size_t n : result)
                lst.emplace_back(static_cast<long long>(n));
            return var(std::move(lst));
        }

        inline var var::connected_components() const
        {
            auto comps = graph_ref().connected_components();
            List result;
            result.reserve(comps.size());
            for (const auto &comp : comps)
            {
                List comp_list;
                comp_list.reserve(comp.size());
                for (size_t n : comp)
                    comp_list.emplace_back(static_cast<long long>(n));
                result.emplace_back(std::move(comp_list));
            }
            return var(std::move(result));
        }

        inline var var::strongly_connected_components() const
        {
            auto sccs = graph_ref().strongly_connected_components();
            List result;
            result.reserve(sccs.size());
            for (const auto &scc : sccs)
            {
                List scc_list;
                scc_list.reserve(scc.size());
                for (size_t n : scc)
                    scc_list.emplace_back(static_cast<long long>(n));
                result.emplace_back(std::move(scc_list));
            }
            return var(std::move(result));
        }

        inline var var::prim_mst() const
        {
            auto [weight, edges] = graph_ref().prim_mst();
            Dict result;
            result["weight"] = var(weight);
            List edge_list;
            edge_list.reserve(edges.size());
            for (const auto &[from, to, w] : edges)
            {
                List edge;
                edge.emplace_back(static_cast<long long>(from));
                edge.emplace_back(static_cast<long long>(to));
                edge.emplace_back(w);
                edge_list.emplace_back(std::move(edge));
            }
            result["edges"] = var(std::move(edge_list));
            return var(std::move(result));
        }

        // ===== Graph Serialization =====
        inline void var::save_graph(const std::string &filename) const
        {
            graph_ref().save(filename);
        }

        inline void var::to_dot(const std::string &filename, bool show_weights) const
        {
            graph_ref().to_dot(filename, show_weights);
        }

        // ===== Get edges for a node =====
        inline var var::get_edges(size_t node)
        {
            auto edges = graph_ref().get_edges(node);
            List result;
            result.reserve(edges.size());
            for (const auto &e : edges)
            {
                Dict edge;
                edge["to"] = var(static_cast<long long>(e.id));
                edge["weight"] = var(e.weight);
                edge["directed"] = var(e.directed);
                result.emplace_back(std::move(edge));
            }
            return var(std::move(result));
        }

        /**
         * @brief Create a new graph with n nodes.
         *
         * Returns a var containing a graph. All graph operations can be performed
         * via the var's graph methods.
         *
         * @param n Number of nodes in the graph (indexed 0 to n-1).
         * @return var containing the graph.
         *
         * Example:
         * @code
         * var g = graph(5);
         * g.add_edge(0, 1);
         * g.set_node_data(0, "Start");
         * var path = g.dfs(0);
         * @endcode
         */
        inline var graph(size_t n)
        {
            return var(std::make_shared<VarGraphWrapper>(n));
        }

        /**
         * @brief Load a graph from a file.
         *
         * @param filename Path to the graph file.
         * @return var containing the loaded graph.
         */
        inline var load_graph(const std::string &filename)
        {
            auto g = VarGraph::load(filename);
            return var(std::make_shared<VarGraphWrapper>(std::move(g)));
        }

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
// std::hash specialization for var - enables use in std::unordered_set and std::unordered_map
namespace std
{
    template <>
    struct hash<pythonic::vars::var>
    {
        size_t operator()(const pythonic::vars::var &v) const noexcept
        {
            return v.hash();
        }
    };
} // namespace std
