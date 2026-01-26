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
#include <span>
#include <functional>
#include <algorithm>
#include <climits>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <optional>

#include "pythonicError.hpp"
#include "pythonicOverflow.hpp"

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

        // Forward declaration for Graph actual include is at the end of this file
        // This avoids circular dependency since Graph uses var for node metadata
        class VarGraphWrapper;
        using GraphPtr = std::shared_ptr<VarGraphWrapper>;
        // TODO: Later I will add std::forward_list as llist() and std::list as dllist() in this just like I have vector container like list I will add linked lists too.

        // TypeTag enum for fast type dispatch (avoids repeated var_get_if/holds_alternative calls)
        // Using uint8_t as underlying type to minimize memory overhead (1 byte)
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

        // ============ Union-based Storage for var ============
        // This replaces std::variant for better performance:
        // - Direct field access (no var_get overhead)
        // - Smaller memory footprint (~24 bytes vs ~80 bytes for variant)
        // - Better cache locality
        //
        // Non-trivial types (string, containers) are heap-allocated via void* ptr.
        // and require manual lifetime management in var's constructors/destructor.
        // And as they are heap-allocated memory and heap-allocation is performence costly, need to optimize or minimize frequent allocation and deallocation

        union VarData
        {
            bool b;
            int i;
            unsigned int ui;
            long l;
            unsigned long ul;
            long long ll;
            unsigned long long ull;
            float f;
            double d;
            long double ld;
            void *ptr; // For: string, List, Set, Dict, OrderedSet, OrderedDict, GraphPtr,
            // TODO: will add linked list in the void pointer. also maybe will change it to use smart pointer to make sure it's safer . I will use unique pointer if it does not introduce complexities.

            // Default constructor (for union, does nothing)
            VarData() : ll(0) {}

            // Explicit constructors for each type
            VarData(bool v) : b(v) {}
            VarData(int v) : i(v) {}
            VarData(unsigned int v) : ui(v) {}
            VarData(long v) : l(v) {}
            VarData(unsigned long v) : ul(v) {}
            VarData(long long v) : ll(v) {}
            VarData(unsigned long long v) : ull(v) {}
            VarData(float v) : f(v) {}
            VarData(double v) : d(v) {}
            VarData(long double v) : ld(v) {}
            VarData(void *v) : ptr(v) {}
        };

        // Helper to check if a TypeTag represents a heap-allocated type
        inline bool is_heap_type(TypeTag tag) noexcept
        {
            switch (tag)
            {
            case TypeTag::STRING:
            case TypeTag::LIST:
            case TypeTag::SET:
            case TypeTag::DICT:
            case TypeTag::ORDEREDSET:
            case TypeTag::ORDEREDDICT:
            case TypeTag::GRAPH:
                return true;
            default:
                return false;
            }
        }

        class var
        {
        private:
            VarData data_; // Union-based storage (replaces std::variant)
            TypeTag tag_;  // Type discriminator

        public:
            // ============ Internal Accessors for Union ============
            // These provide type-safe access to the union data
            // Template specializations handle each type

            template <typename T>
            T &var_get()
            {
                if constexpr (std::is_same_v<T, bool>)
                    return data_.b;
                else if constexpr (std::is_same_v<T, int>)
                    return data_.i;
                else if constexpr (std::is_same_v<T, unsigned int>)
                    return data_.ui;
                else if constexpr (std::is_same_v<T, long>)
                    return data_.l;
                else if constexpr (std::is_same_v<T, unsigned long>)
                    return data_.ul;
                else if constexpr (std::is_same_v<T, long long>)
                    return data_.ll;
                else if constexpr (std::is_same_v<T, unsigned long long>)
                    return data_.ull;
                else if constexpr (std::is_same_v<T, float>)
                    return data_.f;
                else if constexpr (std::is_same_v<T, double>)
                    return data_.d;
                else if constexpr (std::is_same_v<T, long double>)
                    return data_.ld;
                else if constexpr (std::is_same_v<T, std::string>)
                    return *static_cast<std::string *>(data_.ptr);
                else if constexpr (std::is_same_v<T, List>)
                    return *static_cast<List *>(data_.ptr);
                else if constexpr (std::is_same_v<T, Set>)
                    return *static_cast<Set *>(data_.ptr);
                else if constexpr (std::is_same_v<T, Dict>)
                    return *static_cast<Dict *>(data_.ptr);
                else if constexpr (std::is_same_v<T, OrderedSet>)
                    return *static_cast<OrderedSet *>(data_.ptr);
                else if constexpr (std::is_same_v<T, OrderedDict>)
                    return *static_cast<OrderedDict *>(data_.ptr);
                else if constexpr (std::is_same_v<T, GraphPtr>)
                    return *static_cast<GraphPtr *>(data_.ptr);
                else
                    static_assert(sizeof(T) == 0, "Unsupported type for var_get");
            }

            template <typename T>
            const T &var_get() const
            {
                // constexpr = tells compiler: "hey, figure this out at compile-time, cut the useless branches, no runtime checks, zero cost magic"

                if constexpr (std::is_same_v<T, bool>)
                    return data_.b;
                else if constexpr (std::is_same_v<T, int>)
                    return data_.i;
                else if constexpr (std::is_same_v<T, unsigned int>)
                    return data_.ui;
                else if constexpr (std::is_same_v<T, long>)
                    return data_.l;
                else if constexpr (std::is_same_v<T, unsigned long>)
                    return data_.ul;
                else if constexpr (std::is_same_v<T, long long>)
                    return data_.ll;
                else if constexpr (std::is_same_v<T, unsigned long long>)
                    return data_.ull;
                else if constexpr (std::is_same_v<T, float>)
                    return data_.f;
                else if constexpr (std::is_same_v<T, double>)
                    return data_.d;
                else if constexpr (std::is_same_v<T, long double>)
                    return data_.ld;
                else if constexpr (std::is_same_v<T, std::string>)
                    return *static_cast<const std::string *>(data_.ptr);
                else if constexpr (std::is_same_v<T, List>)
                    return *static_cast<const List *>(data_.ptr);
                else if constexpr (std::is_same_v<T, Set>)
                    return *static_cast<const Set *>(data_.ptr);
                else if constexpr (std::is_same_v<T, Dict>)
                    return *static_cast<const Dict *>(data_.ptr);
                else if constexpr (std::is_same_v<T, OrderedSet>)
                    return *static_cast<const OrderedSet *>(data_.ptr);
                else if constexpr (std::is_same_v<T, OrderedDict>)
                    return *static_cast<const OrderedDict *>(data_.ptr);
                else if constexpr (std::is_same_v<T, GraphPtr>)
                    return *static_cast<const GraphPtr *>(data_.ptr);
                else
                    static_assert(sizeof(T) == 0, "Unsupported type for var_get");
            }

            // var_get_if - returns pointer to value if type matches, nullptr otherwise
            // This is for backward compatibility with code that used std::get_if
            template <typename T>
            T *var_get_if() noexcept
            {
                if constexpr (std::is_same_v<T, bool>)
                {
                    if (tag_ == TypeTag::BOOL)
                        return &data_.b;
                }
                else if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return &data_.i;
                }
                else if constexpr (std::is_same_v<T, unsigned int>)
                {
                    if (tag_ == TypeTag::UINT)
                        return &data_.ui;
                }
                else if constexpr (std::is_same_v<T, long>)
                {
                    if (tag_ == TypeTag::LONG)
                        return &data_.l;
                }
                else if constexpr (std::is_same_v<T, unsigned long>)
                {
                    if (tag_ == TypeTag::ULONG)
                        return &data_.ul;
                }
                else if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return &data_.ll;
                }
                else if constexpr (std::is_same_v<T, unsigned long long>)
                {
                    if (tag_ == TypeTag::ULONG_LONG)
                        return &data_.ull;
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    if (tag_ == TypeTag::FLOAT)
                        return &data_.f;
                }
                else if constexpr (std::is_same_v<T, double>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return &data_.d;
                }
                else if constexpr (std::is_same_v<T, long double>)
                {
                    if (tag_ == TypeTag::LONG_DOUBLE)
                        return &data_.ld;
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    if (tag_ == TypeTag::STRING)
                        return static_cast<std::string *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, List>)
                {
                    if (tag_ == TypeTag::LIST)
                        return static_cast<List *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, Set>)
                {
                    if (tag_ == TypeTag::SET)
                        return static_cast<Set *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, Dict>)
                {
                    if (tag_ == TypeTag::DICT)
                        return static_cast<Dict *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, OrderedSet>)
                {
                    if (tag_ == TypeTag::ORDEREDSET)
                        return static_cast<OrderedSet *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, OrderedDict>)
                {
                    if (tag_ == TypeTag::ORDEREDDICT)
                        return static_cast<OrderedDict *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, GraphPtr>)
                {
                    if (tag_ == TypeTag::GRAPH)
                        return static_cast<GraphPtr *>(data_.ptr);
                }
                return nullptr;
            }

            template <typename T>
            const T *var_get_if() const noexcept
            {
                if constexpr (std::is_same_v<T, bool>)
                {
                    if (tag_ == TypeTag::BOOL)
                        return &data_.b;
                }
                else if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return &data_.i;
                }
                else if constexpr (std::is_same_v<T, unsigned int>)
                {
                    if (tag_ == TypeTag::UINT)
                        return &data_.ui;
                }
                else if constexpr (std::is_same_v<T, long>)
                {
                    if (tag_ == TypeTag::LONG)
                        return &data_.l;
                }
                else if constexpr (std::is_same_v<T, unsigned long>)
                {
                    if (tag_ == TypeTag::ULONG)
                        return &data_.ul;
                }
                else if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return &data_.ll;
                }
                else if constexpr (std::is_same_v<T, unsigned long long>)
                {
                    if (tag_ == TypeTag::ULONG_LONG)
                        return &data_.ull;
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    if (tag_ == TypeTag::FLOAT)
                        return &data_.f;
                }
                else if constexpr (std::is_same_v<T, double>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return &data_.d;
                }
                else if constexpr (std::is_same_v<T, long double>)
                {
                    if (tag_ == TypeTag::LONG_DOUBLE)
                        return &data_.ld;
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    if (tag_ == TypeTag::STRING)
                        return static_cast<const std::string *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, List>)
                {
                    if (tag_ == TypeTag::LIST)
                        return static_cast<const List *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, Set>)
                {
                    if (tag_ == TypeTag::SET)
                        return static_cast<const Set *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, Dict>)
                {
                    if (tag_ == TypeTag::DICT)
                        return static_cast<const Dict *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, OrderedSet>)
                {
                    if (tag_ == TypeTag::ORDEREDSET)
                        return static_cast<const OrderedSet *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, OrderedDict>)
                {
                    if (tag_ == TypeTag::ORDEREDDICT)
                        return static_cast<const OrderedDict *>(data_.ptr);
                }
                else if constexpr (std::is_same_v<T, GraphPtr>)
                {
                    if (tag_ == TypeTag::GRAPH)
                        return static_cast<const GraphPtr *>(data_.ptr);
                }
                return nullptr;
            }

            // Helper to destroy heap-allocated data
            void destroy_heap_data() noexcept
            {
                switch (tag_)
                {
                case TypeTag::STRING:
                    delete static_cast<std::string *>(data_.ptr);
                    break;
                case TypeTag::LIST:
                    delete static_cast<List *>(data_.ptr);
                    break;
                case TypeTag::SET:
                    delete static_cast<Set *>(data_.ptr);
                    break;
                case TypeTag::DICT:
                    delete static_cast<Dict *>(data_.ptr);
                    break;
                case TypeTag::ORDEREDSET:
                    delete static_cast<OrderedSet *>(data_.ptr);
                    break;
                case TypeTag::ORDEREDDICT:
                    delete static_cast<OrderedDict *>(data_.ptr);
                    break;
                case TypeTag::GRAPH:
                    delete static_cast<GraphPtr *>(data_.ptr);
                    break;
                default:
                    break;
                }
            }

            // Helper to copy heap-allocated data from another var
            void copy_heap_data(const var &other)
            {
                switch (other.tag_)
                {
                case TypeTag::STRING:
                    data_.ptr = new std::string(*static_cast<const std::string *>(other.data_.ptr));
                    break;
                case TypeTag::LIST:
                    data_.ptr = new List(*static_cast<const List *>(other.data_.ptr));
                    break;
                case TypeTag::SET:
                    data_.ptr = new Set(*static_cast<const Set *>(other.data_.ptr));
                    break;
                case TypeTag::DICT:
                    data_.ptr = new Dict(*static_cast<const Dict *>(other.data_.ptr));
                    break;
                case TypeTag::ORDEREDSET:
                    data_.ptr = new OrderedSet(*static_cast<const OrderedSet *>(other.data_.ptr));
                    break;
                case TypeTag::ORDEREDDICT:
                    data_.ptr = new OrderedDict(*static_cast<const OrderedDict *>(other.data_.ptr));
                    break;
                case TypeTag::GRAPH:
                    data_.ptr = new GraphPtr(*static_cast<const GraphPtr *>(other.data_.ptr));
                    break;
                default:
                    break;
                }
            }

            // Helper to move heap-allocated data from another var
            void move_heap_data(var &&other) noexcept
            {
                data_.ptr = other.data_.ptr;
                other.data_.ptr = nullptr;
                other.tag_ = TypeTag::NONE;
            }

            // Type promotion helpers
            // Returns the TypeTag that should be used for mixed-type operations
            // STRING has highest rank - any operation with string converts the other to string
            // For numeric types the ordering (narrow -> wide) is:
            // bool < uint < int < ulong < long < ulong_long < long_long < float < double < long double
            // Unsigned types have LOWER rank than their signed counterparts.
            // Promotion rules:
            // - If BOTH inputs are unsigned, the result may be unsigned.
            // - If ANY input is signed, the result will be signed.

            // Get promotion rank for a type tag (higher = wider type)
            // Ranking: bool=0 < uint=1 < int=2 < ulong=3 < long=4 < ulong_long=5 < long_long=6 < float=7 < double=8 < long_double=9
            static int getTypeRank(TypeTag t) noexcept
            {
                switch (t)
                {
                case TypeTag::BOOL:
                    return 0;
                case TypeTag::UINT:
                    return 1;
                case TypeTag::INT:
                    return 2;
                case TypeTag::ULONG:
                    return 3;
                case TypeTag::LONG:
                    return 4;
                case TypeTag::ULONG_LONG:
                    return 5;
                case TypeTag::LONG_LONG:
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

            // Helper: Check if a TypeTag is unsigned integer
            static bool isUnsignedTag(TypeTag t) noexcept
            {
                return t == TypeTag::UINT || t == TypeTag::ULONG || t == TypeTag::ULONG_LONG;
            }

            // Helper: Check if a TypeTag is signed integer
            static bool isSignedIntegerTag(TypeTag t) noexcept
            {
                return t == TypeTag::INT || t == TypeTag::LONG || t == TypeTag::LONG_LONG || t == TypeTag::BOOL;
            }

            // Helper: Check if a TypeTag is floating point
            static bool isFloatingTag(TypeTag t) noexcept
            {
                return t == TypeTag::FLOAT || t == TypeTag::DOUBLE || t == TypeTag::LONG_DOUBLE;
            }

            // Type promoted operation helpers
            // NEW DESIGN: Use smart promotion strategy
            // - If BOTH inputs are unsigned → result can be unsigned
            // - If ANY input is signed → result will be signed
            // - If EITHER input is floating → use floating containers
            // - Subtraction always uses signed (can produce negative)

            // Perform addition with proper type promotion
            var addPromoted(const var &other) const
            {
                // Handle string concatenation
                if (tag_ == TypeTag::STRING || other.tag_ == TypeTag::STRING)
                {
                    return var(toString() + other.toString());
                }

                // Check if any input is floating point
                bool has_floating = isFloatingTag(tag_) || isFloatingTag(other.tag_);

                // Check if BOTH inputs are unsigned (only then can result be unsigned)
                // If ANY input is signed, result will be signed
                bool both_unsigned = isUnsignedTag(tag_) && isUnsignedTag(other.tag_);

                // Compute in long double
                long double result = toLongDouble() + other.toLongDouble();

                // Use smart fit to find smallest container
                if (has_floating)
                {
                    // Floating point result
                    if (std::isinf(result))
                        throw PythonicOverflowError("Addition overflow");
                    if (result >= -std::numeric_limits<float>::max() &&
                        result <= std::numeric_limits<float>::max())
                        return var(static_cast<float>(result));
                    if (result >= -std::numeric_limits<double>::max() &&
                        result <= std::numeric_limits<double>::max())
                        return var(static_cast<double>(result));
                    return var(result);
                }
                else if (both_unsigned && result >= 0)
                {
                    // Only if BOTH are unsigned, use unsigned containers
                    if (result <= std::numeric_limits<unsigned int>::max())
                        return var(static_cast<unsigned int>(result));
                    if (result <= std::numeric_limits<unsigned long>::max())
                        return var(static_cast<unsigned long>(result));
                    if (result <= static_cast<long double>(std::numeric_limits<unsigned long long>::max()))
                        return var(static_cast<unsigned long long>(result));
                    // Overflow to float
                    return var(static_cast<float>(result));
                }
                else
                {
                    // If ANY is signed, use signed containers
                    if (result >= std::numeric_limits<int>::min() &&
                        result <= std::numeric_limits<int>::max())
                        return var(static_cast<int>(result));
                    if (result >= std::numeric_limits<long>::min() &&
                        result <= std::numeric_limits<long>::max())
                        return var(static_cast<long>(result));
                    if (result >= static_cast<long double>(std::numeric_limits<long long>::min()) &&
                        result <= static_cast<long double>(std::numeric_limits<long long>::max()))
                        return var(static_cast<long long>(result));
                    // Overflow to float
                    return var(static_cast<float>(result));
                }
            }

            // Perform subtraction with proper type promotion
            // ALWAYS uses signed containers (subtraction can produce negative)
            var subPromoted(const var &other) const
            {
                if (tag_ == TypeTag::STRING || other.tag_ == TypeTag::STRING)
                {
                    throw PythonicTypeError("Cannot subtract strings");
                }

                // Check if any input is floating point
                bool has_floating = isFloatingTag(tag_) || isFloatingTag(other.tag_);

                // Compute in long double
                long double result = toLongDouble() - other.toLongDouble();

                // Subtraction always uses signed containers
                if (has_floating)
                {
                    if (std::isinf(result))
                        throw PythonicOverflowError("Subtraction overflow");
                    if (result >= -std::numeric_limits<float>::max() &&
                        result <= std::numeric_limits<float>::max())
                        return var(static_cast<float>(result));
                    if (result >= -std::numeric_limits<double>::max() &&
                        result <= std::numeric_limits<double>::max())
                        return var(static_cast<double>(result));
                    return var(result);
                }
                else
                {
                    // Always signed for subtraction
                    if (result >= std::numeric_limits<int>::min() &&
                        result <= std::numeric_limits<int>::max())
                        return var(static_cast<int>(result));
                    if (result >= std::numeric_limits<long>::min() &&
                        result <= std::numeric_limits<long>::max())
                        return var(static_cast<long>(result));
                    if (result >= static_cast<long double>(std::numeric_limits<long long>::min()) &&
                        result <= static_cast<long double>(std::numeric_limits<long long>::max()))
                        return var(static_cast<long long>(result));
                    // Overflow to float
                    return var(static_cast<float>(result));
                }
            }

            // Perform multiplication with proper type promotion
            var mulPromoted(const var &other) const
            {
                // String multiplication: "ab" * 3 = "ababab"
                if (tag_ == TypeTag::STRING && other.isIntegral())
                {
                    std::string result;
                    const std::string &s = var_get<std::string>();
                    int count = other.toInt();
                    result.reserve(s.size() * count);
                    for (int i = 0; i < count; ++i)
                        result += s;
                    return var(result);
                }
                if (other.tag_ == TypeTag::STRING && isIntegral())
                {
                    std::string result;
                    const std::string &s = other.var_get<std::string>();
                    int count = toInt();
                    result.reserve(s.size() * count);
                    for (int i = 0; i < count; ++i)
                        result += s;
                    return var(result);
                }
                if (tag_ == TypeTag::STRING || other.tag_ == TypeTag::STRING)
                {
                    throw pythonic::PythonicTypeError("cannot multiply two strings");
                }

                // Check if any input is floating point
                bool has_floating = isFloatingTag(tag_) || isFloatingTag(other.tag_);

                // Check if BOTH inputs are unsigned (only then can result be unsigned)
                // If ANY input is signed, result will be signed
                bool both_unsigned = isUnsignedTag(tag_) && isUnsignedTag(other.tag_);

                // Compute in long double
                long double result = toLongDouble() * other.toLongDouble();

                // Use smart fit
                if (has_floating)
                {
                    if (std::isinf(result))
                        throw PythonicOverflowError("Multiplication overflow");
                    if (result >= -std::numeric_limits<float>::max() &&
                        result <= std::numeric_limits<float>::max())
                        return var(static_cast<float>(result));
                    if (result >= -std::numeric_limits<double>::max() &&
                        result <= std::numeric_limits<double>::max())
                        return var(static_cast<double>(result));
                    return var(result);
                }
                else if (both_unsigned && result >= 0)
                {
                    // Only if BOTH are unsigned, use unsigned containers
                    if (result <= std::numeric_limits<unsigned int>::max())
                        return var(static_cast<unsigned int>(result));
                    if (result <= std::numeric_limits<unsigned long>::max())
                        return var(static_cast<unsigned long>(result));
                    if (result <= static_cast<long double>(std::numeric_limits<unsigned long long>::max()))
                        return var(static_cast<unsigned long long>(result));
                    return var(static_cast<float>(result));
                }
                else
                {
                    // If ANY is signed, use signed containers
                    if (result >= std::numeric_limits<int>::min() &&
                        result <= std::numeric_limits<int>::max())
                        return var(static_cast<int>(result));
                    if (result >= std::numeric_limits<long>::min() &&
                        result <= std::numeric_limits<long>::max())
                        return var(static_cast<long>(result));
                    if (result >= static_cast<long double>(std::numeric_limits<long long>::min()) &&
                        result <= static_cast<long double>(std::numeric_limits<long long>::max()))
                        return var(static_cast<long long>(result));
                    return var(static_cast<float>(result));
                }
            }

            // Perform division with proper type promotion (always returns floating point)
            var divPromoted(const var &other) const
            {
                if (tag_ == TypeTag::STRING || other.tag_ == TypeTag::STRING)
                {
                    throw pythonic::PythonicTypeError("cannot divide strings");
                }

                long double divisor = other.toLongDouble();
                if (divisor == 0.0L)
                    throw PythonicZeroDivisionError("Division by zero");

                long double result = toLongDouble() / divisor;

                // Division always returns floating point
                if (std::isinf(result))
                    throw PythonicOverflowError("Division overflow");
                if (result >= -std::numeric_limits<float>::max() &&
                    result <= std::numeric_limits<float>::max())
                    return var(static_cast<float>(result));
                if (result >= -std::numeric_limits<double>::max() &&
                    result <= std::numeric_limits<double>::max())
                    return var(static_cast<double>(result));
                return var(result);
            }

            // Perform modulo with proper type promotion
            var modPromoted(const var &other) const
            {
                if (tag_ == TypeTag::STRING || other.tag_ == TypeTag::STRING)
                {
                    throw pythonic::PythonicTypeError("cannot perform modulo on strings");
                }

                long double divisor = other.toLongDouble();
                if (divisor == 0.0L)
                    throw PythonicZeroDivisionError("Modulo by zero");

                // Check if any input is floating point
                bool has_floating = isFloatingTag(tag_) || isFloatingTag(other.tag_);

                // Check if BOTH inputs are unsigned (only then can result be unsigned)
                // If ANY input is signed, result will be signed
                bool both_unsigned = isUnsignedTag(tag_) && isUnsignedTag(other.tag_);

                // Compute modulo
                long double result = std::fmod(toLongDouble(), divisor);

                // Use smart fit
                if (has_floating)
                {
                    if (result >= -std::numeric_limits<float>::max() &&
                        result <= std::numeric_limits<float>::max())
                        return var(static_cast<float>(result));
                    if (result >= -std::numeric_limits<double>::max() &&
                        result <= std::numeric_limits<double>::max())
                        return var(static_cast<double>(result));
                    return var(result);
                }
                else if (both_unsigned && result >= 0)
                {
                    // Only if BOTH are unsigned, use unsigned containers
                    if (result <= std::numeric_limits<unsigned int>::max())
                        return var(static_cast<unsigned int>(result));
                    if (result <= std::numeric_limits<unsigned long>::max())
                        return var(static_cast<unsigned long>(result));
                    if (result <= static_cast<long double>(std::numeric_limits<unsigned long long>::max()))
                        return var(static_cast<unsigned long long>(result));
                    return var(static_cast<float>(result));
                }
                else
                {
                    // If ANY is signed, use signed containers
                    if (result >= std::numeric_limits<int>::min() &&
                        result <= std::numeric_limits<int>::max())
                        return var(static_cast<int>(result));
                    if (result >= std::numeric_limits<long>::min() &&
                        result <= std::numeric_limits<long>::max())
                        return var(static_cast<long>(result));
                    if (result >= static_cast<long double>(std::numeric_limits<long long>::min()) &&
                        result <= static_cast<long double>(std::numeric_limits<long long>::max()))
                        return var(static_cast<long long>(result));
                    return var(static_cast<float>(result));
                }
            }

        public:
            // ============ Constructors ============
            // Default constructor - initializes to int(0)
            var() : data_(0), tag_(TypeTag::INT) {}

            // None constructor
            var(NoneType) : data_(), tag_(TypeTag::NONE) {}

            // Primitive type constructors - direct union assignment
            var(bool v) : data_(v), tag_(TypeTag::BOOL) {}
            var(int v) : data_(v), tag_(TypeTag::INT) {}
            var(unsigned int v) : data_(v), tag_(TypeTag::UINT) {}
            var(long v) : data_(v), tag_(TypeTag::LONG) {}
            var(unsigned long v) : data_(v), tag_(TypeTag::ULONG) {}
            var(long long v) : data_(v), tag_(TypeTag::LONG_LONG) {}
            var(unsigned long long v) : data_(v), tag_(TypeTag::ULONG_LONG) {}
            var(float v) : data_(v), tag_(TypeTag::FLOAT) {}
            var(double v) : data_(v), tag_(TypeTag::DOUBLE) {}
            var(long double v) : data_(v), tag_(TypeTag::LONG_DOUBLE) {}

            // String constructors - heap allocated
            var(const char *s) : data_(static_cast<void *>(new std::string(s))), tag_(TypeTag::STRING) {}
            var(const std::string &s) : data_(static_cast<void *>(new std::string(s))), tag_(TypeTag::STRING) {}
            var(std::string &&s) : data_(static_cast<void *>(new std::string(std::move(s)))), tag_(TypeTag::STRING) {}

            // Container constructors - heap allocated
            var(const List &l) : data_(static_cast<void *>(new List(l))), tag_(TypeTag::LIST) {}
            var(List &&l) : data_(static_cast<void *>(new List(std::move(l)))), tag_(TypeTag::LIST) {}
            var(const Set &s) : data_(static_cast<void *>(new Set(s))), tag_(TypeTag::SET) {}
            var(Set &&s) : data_(static_cast<void *>(new Set(std::move(s)))), tag_(TypeTag::SET) {}
            var(const Dict &d) : data_(static_cast<void *>(new Dict(d))), tag_(TypeTag::DICT) {}
            var(Dict &&d) : data_(static_cast<void *>(new Dict(std::move(d)))), tag_(TypeTag::DICT) {}
            var(const OrderedSet &hs) : data_(static_cast<void *>(new OrderedSet(hs))), tag_(TypeTag::ORDEREDSET) {}
            var(OrderedSet &&hs) : data_(static_cast<void *>(new OrderedSet(std::move(hs)))), tag_(TypeTag::ORDEREDSET) {}
            var(const OrderedDict &od) : data_(static_cast<void *>(new OrderedDict(od))), tag_(TypeTag::ORDEREDDICT) {}
            var(OrderedDict &&od) : data_(static_cast<void *>(new OrderedDict(std::move(od)))), tag_(TypeTag::ORDEREDDICT) {}
            var(const GraphPtr &g) : data_(static_cast<void *>(new GraphPtr(g))), tag_(TypeTag::GRAPH) {}
            var(GraphPtr &&g) : data_(static_cast<void *>(new GraphPtr(std::move(g)))), tag_(TypeTag::GRAPH) {}

            // ============ Destructor ============
            ~var()
            {
                if (is_heap_type(tag_))
                {
                    destroy_heap_data();
                }
            }

            // ============ Copy Constructor ============
            var(const var &other) : tag_(other.tag_)
            {
                if (is_heap_type(tag_))
                {
                    copy_heap_data(other);
                }
                else
                {
                    data_ = other.data_;
                }
            }

            // ============ Move Constructor ============
            var(var &&other) noexcept : tag_(other.tag_)
            {
                if (is_heap_type(tag_))
                {
                    move_heap_data(std::move(other));
                }
                else
                {
                    data_ = other.data_;
                }
            }

            // ============ Copy Assignment ============
            var &operator=(const var &other)
            {
                if (this != &other)
                {
                    // Destroy current heap data if any
                    if (is_heap_type(tag_))
                    {
                        destroy_heap_data();
                    }
                    tag_ = other.tag_;
                    if (is_heap_type(tag_))
                    {
                        copy_heap_data(other);
                    }
                    else
                    {
                        data_ = other.data_;
                    }
                }
                return *this;
            }

            // ============ Move Assignment ============
            var &operator=(var &&other) noexcept
            {
                if (this != &other)
                {
                    // Destroy current heap data if any
                    if (is_heap_type(tag_))
                    {
                        destroy_heap_data();
                    }
                    tag_ = other.tag_;
                    if (is_heap_type(tag_))
                    {
                        move_heap_data(std::move(other));
                    }
                    else
                    {
                        data_ = other.data_;
                    }
                }
                return *this;
            }

            // Check if this var is None
            bool isNone() const { return tag_ == TypeTag::NONE; }

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
                    return false; // Unknown type
            }

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
            // TODO: these are not properly documented yet. need to reference them in the proper readme
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
            List &as_list_unchecked() noexcept { return var_get<List>(); }
            const List &as_list_unchecked() const noexcept { return var_get<List>(); }
            Dict &as_dict_unchecked() noexcept { return var_get<Dict>(); }
            const Dict &as_dict_unchecked() const noexcept { return var_get<Dict>(); }
            Set &as_set_unchecked() noexcept { return var_get<Set>(); }
            const Set &as_set_unchecked() const noexcept { return var_get<Set>(); }
            OrderedDict &as_ordered_dict_unchecked() noexcept { return var_get<OrderedDict>(); }
            const OrderedDict &as_ordered_dict_unchecked() const noexcept { return var_get<OrderedDict>(); }
            OrderedSet &as_ordered_set_unchecked() noexcept { return var_get<OrderedSet>(); }
            const OrderedSet &as_ordered_set_unchecked() const noexcept { return var_get<OrderedSet>(); }
            // String type
            std::string &as_string_unchecked() noexcept { return var_get<std::string>(); }
            const std::string &as_string_unchecked() const noexcept { return var_get<std::string>(); }
            // Basic numeric types
            int &as_int_unchecked() noexcept { return var_get<int>(); }
            int as_int_unchecked() const noexcept { return var_get<int>(); }
            double &as_double_unchecked() noexcept { return var_get<double>(); }
            double as_double_unchecked() const noexcept { return var_get<double>(); }
            float &as_float_unchecked() noexcept { return var_get<float>(); }
            float as_float_unchecked() const noexcept { return var_get<float>(); }
            bool &as_bool_unchecked() noexcept { return var_get<bool>(); }
            bool as_bool_unchecked() const noexcept { return var_get<bool>(); }
            // Extended numeric types
            long &as_long_unchecked() noexcept { return var_get<long>(); }
            long as_long_unchecked() const noexcept { return var_get<long>(); }
            long long &as_long_long_unchecked() noexcept { return var_get<long long>(); }
            long long as_long_long_unchecked() const noexcept { return var_get<long long>(); }
            long double &as_long_double_unchecked() noexcept { return var_get<long double>(); }
            long double as_long_double_unchecked() const noexcept { return var_get<long double>(); }
            // Unsigned types
            unsigned int &as_uint_unchecked() noexcept { return var_get<unsigned int>(); }
            unsigned int as_uint_unchecked() const noexcept { return var_get<unsigned int>(); }
            unsigned long &as_ulong_unchecked() noexcept { return var_get<unsigned long>(); }
            unsigned long as_ulong_unchecked() const noexcept { return var_get<unsigned long>(); }
            unsigned long long &as_ulong_long_unchecked() noexcept { return var_get<unsigned long long>(); }
            unsigned long long as_ulong_long_unchecked() const noexcept { return var_get<unsigned long long>(); }
            // Graph type
            GraphPtr &as_graph_unchecked() noexcept { return var_get<GraphPtr>(); }
            const GraphPtr &as_graph_unchecked() const noexcept { return var_get<GraphPtr>(); }

            // ============ Safe Typed Accessors ============
            // These check type and throw if mismatched
            // Container types
            List &as_list()
            {
                if (tag_ != TypeTag::LIST)
                    throw pythonic::PythonicTypeError("as_list() requires a list");
                return var_get<List>();
            }
            const List &as_list() const
            {
                if (tag_ != TypeTag::LIST)
                    throw pythonic::PythonicTypeError("as_list() requires a list");
                return var_get<List>();
            }
            // C++20 span view support
            std::span<var> as_span()
            {
                if (tag_ != TypeTag::LIST)
                    throw pythonic::PythonicTypeError("as_span() requires a list");
                return std::span<var>(var_get<List>());
            }
            std::span<const var> as_span() const
            {
                if (tag_ != TypeTag::LIST)
                    throw pythonic::PythonicTypeError("as_span() requires a list");
                return std::span<const var>(var_get<List>());
            }

            Dict &as_dict()
            {
                if (tag_ != TypeTag::DICT)
                    throw pythonic::PythonicTypeError("as_dict() requires a dict");
                return var_get<Dict>();
            }
            const Dict &as_dict() const
            {
                if (tag_ != TypeTag::DICT)
                    throw pythonic::PythonicTypeError("as_dict() requires a dict");
                return var_get<Dict>();
            }
            Set &as_set()
            {
                if (tag_ != TypeTag::SET)
                    throw pythonic::PythonicTypeError("as_set() requires a set");
                return var_get<Set>();
            }
            const Set &as_set() const
            {
                if (tag_ != TypeTag::SET)
                    throw pythonic::PythonicTypeError("as_set() requires a set");
                return var_get<Set>();
            }
            OrderedDict &as_ordered_dict()
            {
                if (tag_ != TypeTag::ORDEREDDICT)
                    throw pythonic::PythonicTypeError("as_ordered_dict() requires an ordered dict");
                return var_get<OrderedDict>();
            }
            const OrderedDict &as_ordered_dict() const
            {
                if (tag_ != TypeTag::ORDEREDDICT)
                    throw pythonic::PythonicTypeError("as_ordered_dict() requires an ordered dict");
                return var_get<OrderedDict>();
            }
            OrderedSet &as_ordered_set()
            {
                if (tag_ != TypeTag::ORDEREDSET)
                    throw pythonic::PythonicTypeError("as_ordered_set() requires an ordered set");
                return var_get<OrderedSet>();
            }
            const OrderedSet &as_ordered_set() const
            {
                if (tag_ != TypeTag::ORDEREDSET)
                    throw pythonic::PythonicTypeError("as_ordered_set() requires an ordered set");
                return var_get<OrderedSet>();
            }
            // String type
            std::string &as_string()
            {
                if (tag_ != TypeTag::STRING)
                    throw pythonic::PythonicTypeError("as_string() requires a string");
                return var_get<std::string>();
            }
            const std::string &as_string() const
            {
                if (tag_ != TypeTag::STRING)
                    throw pythonic::PythonicTypeError("as_string() requires a string");
                return var_get<std::string>();
            }
            // Basic numeric types
            int as_int() const
            {
                if (tag_ != TypeTag::INT)
                    throw pythonic::PythonicTypeError("as_int() requires an int");
                return var_get<int>();
            }
            double as_double() const
            {
                if (tag_ != TypeTag::DOUBLE)
                    throw pythonic::PythonicTypeError("as_double() requires a double");
                return var_get<double>();
            }
            float as_float() const
            {
                if (tag_ != TypeTag::FLOAT)
                    throw pythonic::PythonicTypeError("as_float() requires a float");
                return var_get<float>();
            }
            bool as_bool() const
            {
                if (tag_ != TypeTag::BOOL)
                    throw pythonic::PythonicTypeError("as_bool() requires a bool");
                return var_get<bool>();
            }
            // Extended numeric types
            long as_long() const
            {
                if (tag_ != TypeTag::LONG)
                    throw pythonic::PythonicTypeError("as_long() requires a long");
                return var_get<long>();
            }
            long long as_long_long() const
            {
                if (tag_ != TypeTag::LONG_LONG)
                    throw pythonic::PythonicTypeError("as_long_long() requires a long long");
                return var_get<long long>();
            }
            long double as_long_double() const
            {
                if (tag_ != TypeTag::LONG_DOUBLE)
                    throw pythonic::PythonicTypeError("as_long_double() requires a long double");
                return var_get<long double>();
            }
            // Unsigned types
            unsigned int as_uint() const
            {
                if (tag_ != TypeTag::UINT)
                    throw pythonic::PythonicTypeError("as_uint() requires an unsigned int");
                return var_get<unsigned int>();
            }
            unsigned long as_ulong() const
            {
                if (tag_ != TypeTag::ULONG)
                    throw pythonic::PythonicTypeError("as_ulong() requires an unsigned long");
                return var_get<unsigned long>();
            }
            unsigned long long as_ulong_long() const
            {
                if (tag_ != TypeTag::ULONG_LONG)
                    throw pythonic::PythonicTypeError("as_ulong_long() requires an unsigned long long");
                return var_get<unsigned long long>();
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
                    return var_get<int>();
                case TypeTag::FLOAT:
                    return static_cast<int>(var_get<float>());
                case TypeTag::DOUBLE:
                    return static_cast<int>(var_get<double>());
                case TypeTag::LONG:
                    return static_cast<int>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return static_cast<int>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return static_cast<int>(var_get<long double>());
                case TypeTag::UINT:
                    return static_cast<int>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return static_cast<int>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return static_cast<int>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1 : 0;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to int");
                }
            }

            // Convert to unsigned int
            unsigned int toUInt() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<unsigned int>(var_get<int>());
                case TypeTag::FLOAT:
                    return static_cast<unsigned int>(var_get<float>());
                case TypeTag::DOUBLE:
                    return static_cast<unsigned int>(var_get<double>());
                case TypeTag::LONG:
                    return static_cast<unsigned int>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return static_cast<unsigned int>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return static_cast<unsigned int>(var_get<long double>());
                case TypeTag::UINT:
                    return var_get<unsigned int>();
                case TypeTag::ULONG:
                    return static_cast<unsigned int>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return static_cast<unsigned int>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1U : 0U;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to unsigned int");
                }
            }

            // Convert to long
            long toLong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<long>(var_get<int>());
                case TypeTag::FLOAT:
                    return static_cast<long>(var_get<float>());
                case TypeTag::DOUBLE:
                    return static_cast<long>(var_get<double>());
                case TypeTag::LONG:
                    return var_get<long>();
                case TypeTag::LONG_LONG:
                    return static_cast<long>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return static_cast<long>(var_get<long double>());
                case TypeTag::UINT:
                    return static_cast<long>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return static_cast<long>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return static_cast<long>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1L : 0L;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to long");
                }
            }

            // Convert to unsigned long
            unsigned long toULong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<unsigned long>(var_get<int>());
                case TypeTag::FLOAT:
                    return static_cast<unsigned long>(var_get<float>());
                case TypeTag::DOUBLE:
                    return static_cast<unsigned long>(var_get<double>());
                case TypeTag::LONG:
                    return static_cast<unsigned long>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return static_cast<unsigned long>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return static_cast<unsigned long>(var_get<long double>());
                case TypeTag::UINT:
                    return static_cast<unsigned long>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return var_get<unsigned long>();
                case TypeTag::ULONG_LONG:
                    return static_cast<unsigned long>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1UL : 0UL;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to unsigned long");
                }
            }

            // Convert to long long (for integer arithmetic that needs range)
            long long toLongLong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<long long>(var_get<int>());
                case TypeTag::FLOAT:
                    return static_cast<long long>(var_get<float>());
                case TypeTag::DOUBLE:
                    return static_cast<long long>(var_get<double>());
                case TypeTag::LONG:
                    return static_cast<long long>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return var_get<long long>();
                case TypeTag::LONG_DOUBLE:
                    return static_cast<long long>(var_get<long double>());
                case TypeTag::UINT:
                    return static_cast<long long>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return static_cast<long long>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return static_cast<long long>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1LL : 0LL;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to long long");
                }
            }

            // Convert to unsigned long long
            unsigned long long toULongLong() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<unsigned long long>(var_get<int>());
                case TypeTag::FLOAT:
                    return static_cast<unsigned long long>(var_get<float>());
                case TypeTag::DOUBLE:
                    return static_cast<unsigned long long>(var_get<double>());
                case TypeTag::LONG:
                    return static_cast<unsigned long long>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return static_cast<unsigned long long>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return static_cast<unsigned long long>(var_get<long double>());
                case TypeTag::UINT:
                    return static_cast<unsigned long long>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return static_cast<unsigned long long>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return var_get<unsigned long long>();
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1ULL : 0ULL;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to unsigned long long");
                }
            }

            // Convert to float
            float toFloat() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<float>(var_get<int>());
                case TypeTag::FLOAT:
                    return var_get<float>();
                case TypeTag::DOUBLE:
                    return static_cast<float>(var_get<double>());
                case TypeTag::LONG:
                    return static_cast<float>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return static_cast<float>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return static_cast<float>(var_get<long double>());
                case TypeTag::UINT:
                    return static_cast<float>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return static_cast<float>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return static_cast<float>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1.0f : 0.0f;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to float");
                }
            }

            // Convert to double (most common floating point conversion)
            double toDouble() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<double>(var_get<int>());
                case TypeTag::FLOAT:
                    return static_cast<double>(var_get<float>());
                case TypeTag::DOUBLE:
                    return var_get<double>();
                case TypeTag::LONG:
                    return static_cast<double>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return static_cast<double>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return static_cast<double>(var_get<long double>());
                case TypeTag::UINT:
                    return static_cast<double>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return static_cast<double>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return static_cast<double>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1.0 : 0.0;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to double");
                }
            }

            // Convert to long double (highest precision floating point)
            long double toLongDouble() const
            {
                switch (tag_)
                {
                case TypeTag::INT:
                    return static_cast<long double>(var_get<int>());
                case TypeTag::FLOAT:
                    return static_cast<long double>(var_get<float>());
                case TypeTag::DOUBLE:
                    return static_cast<long double>(var_get<double>());
                case TypeTag::LONG:
                    return static_cast<long double>(var_get<long>());
                case TypeTag::LONG_LONG:
                    return static_cast<long double>(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return var_get<long double>();
                case TypeTag::UINT:
                    return static_cast<long double>(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return static_cast<long double>(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return static_cast<long double>(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? 1.0L : 0.0L;
                default:
                    throw pythonic::PythonicTypeError("cannot convert to long double");
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
                    return std::to_string(var_get<int>());
                case TypeTag::FLOAT:
                    return std::to_string(var_get<float>());
                case TypeTag::DOUBLE:
                    return std::to_string(var_get<double>());
                case TypeTag::LONG:
                    return std::to_string(var_get<long>());
                case TypeTag::LONG_LONG:
                    return std::to_string(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return std::to_string(var_get<long double>());
                case TypeTag::UINT:
                    return std::to_string(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return std::to_string(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return std::to_string(var_get<unsigned long long>());
                case TypeTag::BOOL:
                    return var_get<bool>() ? "True" : "False";
                case TypeTag::STRING:
                    return var_get<std::string>();
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
            // OPTIMIZED: Use tag for common cases, returns const char* to avoid allocation
            const char *type_cstr() const noexcept
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

            // type() returns std::string for compatibility, but prefer type_cstr() for performance
            std::string type() const
            {
                return std::string(type_cstr());
            }

            // Fast type tag accessor for performance-critical code
            TypeTag type_tag() const noexcept { return tag_; }

            // Get value as specific type
            template <typename T>
            T &get()
            {
                return var_get<T>();
            }

            template <typename T>
            const T &get() const
            {
                return var_get<T>();
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
                    return var_get<std::string>();
                case TypeTag::BOOL:
                    return var_get<bool>() ? "True" : "False";
                case TypeTag::INT:
                    return std::to_string(var_get<int>());
                case TypeTag::LONG:
                    return std::to_string(var_get<long>());
                case TypeTag::LONG_LONG:
                    return std::to_string(var_get<long long>());
                case TypeTag::UINT:
                    return std::to_string(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return std::to_string(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return std::to_string(var_get<unsigned long long>());
                case TypeTag::FLOAT:
                {
                    std::ostringstream ss;
                    ss << var_get<float>();
                    return ss.str();
                }
                case TypeTag::DOUBLE:
                {
                    std::ostringstream ss;
                    ss << var_get<double>();
                    return ss.str();
                }
                case TypeTag::LONG_DOUBLE:
                {
                    std::ostringstream ss;
                    ss << var_get<long double>();
                    return ss.str();
                }
                case TypeTag::LIST:
                {
                    const auto &lst = var_get<List>();
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
                    const auto &st = var_get<Set>();
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
                    const auto &dct = var_get<Dict>();
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
                    const auto &hs = var_get<OrderedSet>();
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
                    const auto &od = var_get<OrderedDict>();
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

            std::string pretty_str(size_t indent = 0, size_t indent_step = 2) const
            {
                std::string ind(indent, ' ');
                std::string inner_ind(indent + indent_step, ' ');

                switch (tag_)
                {
                case TypeTag::NONE:
                    return "None";
                case TypeTag::STRING:
                    return "\"" + var_get<std::string>() + "\"";
                case TypeTag::BOOL:
                    return var_get<bool>() ? "True" : "False";
                case TypeTag::INT:
                    return std::to_string(var_get<int>());
                case TypeTag::LONG:
                    return std::to_string(var_get<long>());
                case TypeTag::LONG_LONG:
                    return std::to_string(var_get<long long>());
                case TypeTag::UINT:
                    return std::to_string(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return std::to_string(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return std::to_string(var_get<unsigned long long>());
                case TypeTag::FLOAT:
                {
                    std::ostringstream ss;
                    ss << var_get<float>();
                    return ss.str();
                }
                case TypeTag::DOUBLE:
                {
                    std::ostringstream ss;
                    ss << var_get<double>();
                    return ss.str();
                }
                case TypeTag::LONG_DOUBLE:
                {
                    std::ostringstream ss;
                    ss << var_get<long double>();
                    return ss.str();
                }
                case TypeTag::LIST:
                {
                    const auto &lst = var_get<List>();
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
                    const auto &st = var_get<Set>();
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
                    const auto &dct = var_get<Dict>();
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
                    const auto &hs = var_get<OrderedSet>();
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
                    const auto &od = var_get<OrderedDict>();
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
                    return graph_str_impl(); // Graphs don't have nested pretty printing . Maybe this comment is old and Now we have pretty print for graph or atleast print for graph
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
                        return var_get<int>() < other.var_get<int>();
                    case TypeTag::DOUBLE:
                        return var_get<double>() < other.var_get<double>();
                    case TypeTag::STRING:
                        return var_get<std::string>() < other.var_get<std::string>();
                    case TypeTag::LONG_LONG:
                        return var_get<long long>() < other.var_get<long long>();
                    case TypeTag::FLOAT:
                        return var_get<float>() < other.var_get<float>();
                    case TypeTag::LONG:
                        return var_get<long>() < other.var_get<long>();
                    case TypeTag::BOOL:
                        return var_get<bool>() < other.var_get<bool>();
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
                        [[likely]] return var(pythonic::overflow::add_throw(var_get<int>(), other.var_get<int>()));
                    case TypeTag::DOUBLE:
                        [[likely]] return var(pythonic::overflow::add_throw(var_get<double>(), other.var_get<double>()));
                    case TypeTag::LONG_LONG:
                        return var(pythonic::overflow::add_throw(var_get<long long>(), other.var_get<long long>()));
                    case TypeTag::STRING:
                        [[likely]] return var(var_get<std::string>() + other.var_get<std::string>());
                    case TypeTag::FLOAT:
                        return var(pythonic::overflow::add_throw(var_get<float>(), other.var_get<float>()));
                    case TypeTag::LONG:
                        return var(pythonic::overflow::add_throw(var_get<long>(), other.var_get<long>()));
                    case TypeTag::UINT:
                        return var(pythonic::overflow::add_throw(var_get<unsigned int>(), other.var_get<unsigned int>()));
                    case TypeTag::ULONG:
                        return var(pythonic::overflow::add_throw(var_get<unsigned long>(), other.var_get<unsigned long>()));
                    case TypeTag::ULONG_LONG:
                        return var(pythonic::overflow::add_throw(var_get<unsigned long long>(), other.var_get<unsigned long long>()));
                    case TypeTag::LONG_DOUBLE:
                        return var(pythonic::overflow::add_throw(var_get<long double>(), other.var_get<long double>()));
                    case TypeTag::LIST:
                    {
                        const auto &a = var_get<List>();
                        const auto &b = other.var_get<List>();
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
                        [[likely]] return var(pythonic::overflow::sub_throw(var_get<int>(), other.var_get<int>()));
                    case TypeTag::DOUBLE:
                        [[likely]] return var(pythonic::overflow::sub_throw(var_get<double>(), other.var_get<double>()));
                    case TypeTag::LONG_LONG:
                        return var(pythonic::overflow::sub_throw(var_get<long long>(), other.var_get<long long>()));
                    case TypeTag::FLOAT:
                        return var(pythonic::overflow::sub_throw(var_get<float>(), other.var_get<float>()));
                    case TypeTag::LONG:
                        return var(pythonic::overflow::sub_throw(var_get<long>(), other.var_get<long>()));
                    case TypeTag::UINT:
                        return var(pythonic::overflow::sub_throw(var_get<unsigned int>(), other.var_get<unsigned int>()));
                    case TypeTag::ULONG:
                        return var(pythonic::overflow::sub_throw(var_get<unsigned long>(), other.var_get<unsigned long>()));
                    case TypeTag::ULONG_LONG:
                        return var(pythonic::overflow::sub_throw(var_get<unsigned long long>(), other.var_get<unsigned long long>()));
                    case TypeTag::LONG_DOUBLE:
                        return var(pythonic::overflow::sub_throw(var_get<long double>(), other.var_get<long double>()));
                    case TypeTag::SET:
                    {
                        const auto &a = var_get<Set>();
                        const auto &b = other.var_get<Set>();
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
                        const auto &a = var_get<List>();
                        const auto &b = other.var_get<List>();
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
                        const auto &a = var_get<Dict>();
                        const auto &b = other.var_get<Dict>();
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
                throw pythonic::PythonicTypeError("operator- requires arithmetic types or containers");
            }

            var operator*(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        [[likely]] return var(pythonic::overflow::mul_throw(var_get<int>(), other.var_get<int>()));
                    case TypeTag::DOUBLE:
                        [[likely]] return var(pythonic::overflow::mul_throw(var_get<double>(), other.var_get<double>()));
                    case TypeTag::LONG_LONG:
                        return var(pythonic::overflow::mul_throw(var_get<long long>(), other.var_get<long long>()));
                    case TypeTag::FLOAT:
                        return var(pythonic::overflow::mul_throw(var_get<float>(), other.var_get<float>()));
                    case TypeTag::LONG:
                        return var(pythonic::overflow::mul_throw(var_get<long>(), other.var_get<long>()));
                    case TypeTag::UINT:
                        return var(pythonic::overflow::mul_throw(var_get<unsigned int>(), other.var_get<unsigned int>()));
                    case TypeTag::ULONG:
                        return var(pythonic::overflow::mul_throw(var_get<unsigned long>(), other.var_get<unsigned long>()));
                    case TypeTag::ULONG_LONG:
                        return var(pythonic::overflow::mul_throw(var_get<unsigned long long>(), other.var_get<unsigned long long>()));
                    case TypeTag::LONG_DOUBLE:
                        return var(pythonic::overflow::mul_throw(var_get<long double>(), other.var_get<long double>()));
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
                    const auto &s = var_get<std::string>();
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
                    const auto &s = other.var_get<std::string>();
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
                    const auto &lst = var_get<List>();
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
                throw pythonic::PythonicTypeError("unsupported types for multiplication");
            }

            var operator/(const var &other) const
            {
                // Fast-path: same type
                // Note: Python semantics - / always returns float, even for int/int
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        [[likely]]
                        {
                            int a = var_get<int>();
                            int b = other.var_get<int>();
                            if (b == 0)
                                throw pythonic::PythonicZeroDivisionError::division();
                            // Check for INT_MIN / -1 overflow
                            if (a == std::numeric_limits<int>::min() && b == -1)
                                throw pythonic::PythonicOverflowError("integer division overflow");
                            // Python: int / int -> float (always)
                            return var(static_cast<double>(a) / static_cast<double>(b));
                        }
                    case TypeTag::DOUBLE:
                        [[likely]]
                        {
                            double b = other.var_get<double>();
                            if (b == 0.0)
                                throw pythonic::PythonicZeroDivisionError::division();
                            return var(var_get<double>() / b);
                        }
                    case TypeTag::LONG_LONG:
                    {
                        long long a = var_get<long long>();
                        long long b = other.var_get<long long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        // Check for LLONG_MIN / -1 overflow
                        if (a == std::numeric_limits<long long>::min() && b == -1LL)
                            throw pythonic::PythonicOverflowError("integer division overflow");
                        // Python: int / int -> float (always)
                        return var(static_cast<double>(a) / static_cast<double>(b));
                    }
                    case TypeTag::FLOAT:
                    {
                        float b = other.var_get<float>();
                        if (b == 0.0f)
                            throw pythonic::PythonicZeroDivisionError::division();
                        return var(var_get<float>() / b);
                    }
                    case TypeTag::LONG:
                    {
                        long a = var_get<long>();
                        long b = other.var_get<long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        // Check for LONG_MIN / -1 overflow
                        if (a == std::numeric_limits<long>::min() && b == -1L)
                            throw pythonic::PythonicOverflowError("integer division overflow");
                        // Python: int / int -> float (always)
                        return var(static_cast<double>(a) / static_cast<double>(b));
                    }
                    case TypeTag::UINT:
                    {
                        unsigned int b = other.var_get<unsigned int>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        // Python: int / int -> float (always)
                        return var(static_cast<double>(var_get<unsigned int>()) / static_cast<double>(b));
                    }
                    case TypeTag::ULONG:
                    {
                        unsigned long b = other.var_get<unsigned long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        // Python: int / int -> float (always)
                        return var(static_cast<double>(var_get<unsigned long>()) / static_cast<double>(b));
                    }
                    case TypeTag::ULONG_LONG:
                    {
                        unsigned long long b = other.var_get<unsigned long long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        // Python: int / int -> float (always)
                        return var(static_cast<double>(var_get<unsigned long long>()) / static_cast<double>(b));
                    }
                    case TypeTag::LONG_DOUBLE:
                    {
                        long double b = other.var_get<long double>();
                        if (b == 0.0L)
                            throw pythonic::PythonicZeroDivisionError::division();
                        return var(var_get<long double>() / b);
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
                throw pythonic::PythonicTypeError("unsupported types for division");
            }

            var operator%(const var &other) const
            {
                // Fast-path: same integer type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        [[likely]]
                        {
                            int b = other.var_get<int>();
                            if (b == 0)
                                throw pythonic::PythonicZeroDivisionError::modulo();
                            return var(pythonic::overflow::mod_throw(var_get<int>(), b));
                        }
                    case TypeTag::LONG_LONG:
                    {
                        long long b = other.var_get<long long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
                        return var(pythonic::overflow::mod_throw(var_get<long long>(), b));
                    }
                    case TypeTag::LONG:
                    {
                        long b = other.var_get<long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
                        return var(pythonic::overflow::mod_throw(var_get<long>(), b));
                    }
                    case TypeTag::UINT:
                    {
                        unsigned int b = other.var_get<unsigned int>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
                        return var(pythonic::overflow::mod_throw(var_get<unsigned int>(), b));
                    }
                    case TypeTag::ULONG:
                    {
                        unsigned long b = other.var_get<unsigned long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
                        return var(pythonic::overflow::mod_throw(var_get<unsigned long>(), b));
                    }
                    case TypeTag::ULONG_LONG:
                    {
                        unsigned long long b = other.var_get<unsigned long long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
                        return var(pythonic::overflow::mod_throw(var_get<unsigned long long>(), b));
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
                throw pythonic::PythonicTypeError("unsupported types for modulo");
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
                        var_get<int>() += other.var_get<int>();
                        return *this;
                    case TypeTag::DOUBLE:
                        var_get<double>() += other.var_get<double>();
                        return *this;
                    case TypeTag::LONG_LONG:
                        var_get<long long>() += other.var_get<long long>();
                        return *this;
                    case TypeTag::FLOAT:
                        var_get<float>() += other.var_get<float>();
                        return *this;
                    case TypeTag::LONG:
                        var_get<long>() += other.var_get<long>();
                        return *this;
                    case TypeTag::STRING:
                        var_get<std::string>() += other.var_get<std::string>();
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
                        var_get<int>() -= other.var_get<int>();
                        return *this;
                    case TypeTag::DOUBLE:
                        var_get<double>() -= other.var_get<double>();
                        return *this;
                    case TypeTag::LONG_LONG:
                        var_get<long long>() -= other.var_get<long long>();
                        return *this;
                    case TypeTag::FLOAT:
                        var_get<float>() -= other.var_get<float>();
                        return *this;
                    case TypeTag::LONG:
                        var_get<long>() -= other.var_get<long>();
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
                        var_get<int>() *= other.var_get<int>();
                        return *this;
                    case TypeTag::DOUBLE:
                        var_get<double>() *= other.var_get<double>();
                        return *this;
                    case TypeTag::LONG_LONG:
                        var_get<long long>() *= other.var_get<long long>();
                        return *this;
                    case TypeTag::FLOAT:
                        var_get<float>() *= other.var_get<float>();
                        return *this;
                    case TypeTag::LONG:
                        var_get<long>() *= other.var_get<long>();
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
                        int b = other.var_get<int>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        var_get<int>() /= b;
                        return *this;
                    }
                    case TypeTag::DOUBLE:
                    {
                        double b = other.var_get<double>();
                        if (b == 0.0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        var_get<double>() /= b;
                        return *this;
                    }
                    case TypeTag::LONG_LONG:
                    {
                        long long b = other.var_get<long long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        var_get<long long>() /= b;
                        return *this;
                    }
                    case TypeTag::FLOAT:
                    {
                        float b = other.var_get<float>();
                        if (b == 0.0f)
                            throw pythonic::PythonicZeroDivisionError::division();
                        var_get<float>() /= b;
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
                        int b = other.var_get<int>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
                        var_get<int>() %= b;
                        return *this;
                    }
                    case TypeTag::LONG_LONG:
                    {
                        long long b = other.var_get<long long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
                        var_get<long long>() %= b;
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
                    var_get<int>() += other;
                }
                else if (tag_ == TypeTag::DOUBLE && std::is_floating_point_v<T>)
                {
                    var_get<double>() += static_cast<double>(other);
                }
                else if (tag_ == TypeTag::LONG_LONG && std::is_integral_v<T>)
                {
                    var_get<long long>() += static_cast<long long>(other);
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
                    var_get<int>() -= other;
                }
                else if (tag_ == TypeTag::DOUBLE && std::is_floating_point_v<T>)
                {
                    var_get<double>() -= static_cast<double>(other);
                }
                else if (tag_ == TypeTag::LONG_LONG && std::is_integral_v<T>)
                {
                    var_get<long long>() -= static_cast<long long>(other);
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
                    var_get<int>() *= other;
                }
                else if (tag_ == TypeTag::DOUBLE && std::is_floating_point_v<T>)
                {
                    var_get<double>() *= static_cast<double>(other);
                }
                else if (tag_ == TypeTag::LONG_LONG && std::is_integral_v<T>)
                {
                    var_get<long long>() *= static_cast<long long>(other);
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
                    case TypeTag::NONE:
                        return var(true); // None == None is always true
                    case TypeTag::INT:
                        return var(var_get<int>() == other.var_get<int>());
                    case TypeTag::DOUBLE:
                        return var(var_get<double>() == other.var_get<double>());
                    case TypeTag::STRING:
                        return var(var_get<std::string>() == other.var_get<std::string>());
                    case TypeTag::BOOL:
                        return var(var_get<bool>() == other.var_get<bool>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() == other.var_get<long long>());
                    case TypeTag::FLOAT:
                        return var(var_get<float>() == other.var_get<float>());
                    case TypeTag::LIST:
                    {
                        const auto &lst1 = var_get<List>();
                        const auto &lst2 = other.var_get<List>();
                        if (lst1.size() != lst2.size())
                            return var(false);
                        for (size_t i = 0; i < lst1.size(); ++i)
                        {
                            // Recursively compare elements
                            if (!static_cast<bool>(lst1[i] == lst2[i]))
                                return var(false);
                        }
                        return var(true);
                    }
                    case TypeTag::SET:
                    {
                        const auto &set1 = var_get<Set>();
                        const auto &set2 = other.var_get<Set>();
                        if (set1.size() != set2.size())
                            return var(false);
                        // Check if all elements in set1 are in set2
                        for (const auto &elem : set1)
                        {
                            if (set2.find(elem) == set2.end())
                                return var(false);
                        }
                        return var(true);
                    }
                    case TypeTag::ORDEREDSET:
                    {
                        const auto &set1 = var_get<OrderedSet>();
                        const auto &set2 = other.var_get<OrderedSet>();
                        if (set1.size() != set2.size())
                            return var(false);
                        auto it1 = set1.begin();
                        auto it2 = set2.begin();
                        while (it1 != set1.end())
                        {
                            if (!static_cast<bool>(*it1 == *it2))
                                return var(false);
                            ++it1;
                            ++it2;
                        }
                        return var(true);
                    }
                    case TypeTag::DICT:
                    {
                        const auto &dict1 = var_get<Dict>();
                        const auto &dict2 = other.var_get<Dict>();
                        if (dict1.size() != dict2.size())
                            return var(false);
                        for (const auto &[key, val] : dict1)
                        {
                            auto it = dict2.find(key);
                            if (it == dict2.end() || !static_cast<bool>(val == it->second))
                                return var(false);
                        }
                        return var(true);
                    }
                    case TypeTag::ORDEREDDICT:
                    {
                        const auto &dict1 = var_get<OrderedDict>();
                        const auto &dict2 = other.var_get<OrderedDict>();
                        if (dict1.size() != dict2.size())
                            return var(false);
                        auto it1 = dict1.begin();
                        auto it2 = dict2.begin();
                        while (it1 != dict1.end())
                        {
                            if (it1->first != it2->first || !static_cast<bool>(it1->second == it2->second))
                                return var(false);
                            ++it1;
                            ++it2;
                        }
                        return var(true);
                    }
                    default:
                        break;
                    }
                }
                // Mixed numeric types - promote to double
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() == other.toDouble());
                }
                // Different types (except numeric) are not equal
                return var(false);
            }

            var operator!=(const var &other) const
            {
                // Fast-path: same type
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::NONE:
                        return var(false); // None != None is always false
                    case TypeTag::INT:
                        return var(var_get<int>() != other.var_get<int>());
                    case TypeTag::DOUBLE:
                        return var(var_get<double>() != other.var_get<double>());
                    case TypeTag::STRING:
                        return var(var_get<std::string>() != other.var_get<std::string>());
                    case TypeTag::BOOL:
                        return var(var_get<bool>() != other.var_get<bool>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() != other.var_get<long long>());
                    case TypeTag::LIST:
                    {
                        const auto &lst1 = var_get<List>();
                        const auto &lst2 = other.var_get<List>();
                        if (lst1.size() != lst2.size())
                            return var(true);
                        for (size_t i = 0; i < lst1.size(); ++i)
                        {
                            if (static_cast<bool>(lst1[i] != lst2[i]))
                                return var(true);
                        }
                        return var(false);
                    }
                    case TypeTag::SET:
                    {
                        const auto &set1 = var_get<Set>();
                        const auto &set2 = other.var_get<Set>();
                        if (set1.size() != set2.size())
                            return var(true);
                        for (const auto &elem : set1)
                        {
                            if (set2.find(elem) == set2.end())
                                return var(true);
                        }
                        return var(false);
                    }
                    case TypeTag::ORDEREDSET:
                    {
                        const auto &set1 = var_get<OrderedSet>();
                        const auto &set2 = other.var_get<OrderedSet>();
                        if (set1.size() != set2.size())
                            return var(true);
                        auto it1 = set1.begin();
                        auto it2 = set2.begin();
                        while (it1 != set1.end())
                        {
                            if (static_cast<bool>(*it1 != *it2))
                                return var(true);
                            ++it1;
                            ++it2;
                        }
                        return var(false);
                    }
                    case TypeTag::DICT:
                    {
                        const auto &dict1 = var_get<Dict>();
                        const auto &dict2 = other.var_get<Dict>();
                        if (dict1.size() != dict2.size())
                            return var(true);
                        for (const auto &[key, val] : dict1)
                        {
                            auto it = dict2.find(key);
                            if (it == dict2.end() || static_cast<bool>(val != it->second))
                                return var(true);
                        }
                        return var(false);
                    }
                    case TypeTag::ORDEREDDICT:
                    {
                        const auto &dict1 = var_get<OrderedDict>();
                        const auto &dict2 = other.var_get<OrderedDict>();
                        if (dict1.size() != dict2.size())
                            return var(true);
                        auto it1 = dict1.begin();
                        auto it2 = dict2.begin();
                        while (it1 != dict1.end())
                        {
                            if (it1->first != it2->first || static_cast<bool>(it1->second != it2->second))
                                return var(true);
                            ++it1;
                            ++it2;
                        }
                        return var(false);
                    }
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
                        return var(var_get<int>() > other.var_get<int>());
                    case TypeTag::DOUBLE:
                        return var(var_get<double>() > other.var_get<double>());
                    case TypeTag::STRING:
                        return var(var_get<std::string>() > other.var_get<std::string>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() > other.var_get<long long>());
                    default:
                        break;
                    }
                }
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() > other.toDouble());
                }
                throw pythonic::PythonicTypeError("unsupported types for comparison");
            }

            var operator>=(const var &other) const
            {
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(var_get<int>() >= other.var_get<int>());
                    case TypeTag::DOUBLE:
                        return var(var_get<double>() >= other.var_get<double>());
                    case TypeTag::STRING:
                        return var(var_get<std::string>() >= other.var_get<std::string>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() >= other.var_get<long long>());
                    default:
                        break;
                    }
                }
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() >= other.toDouble());
                }
                throw pythonic::PythonicTypeError("unsupported types for comparison");
            }

            var operator<=(const var &other) const
            {
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(var_get<int>() <= other.var_get<int>());
                    case TypeTag::DOUBLE:
                        return var(var_get<double>() <= other.var_get<double>());
                    case TypeTag::STRING:
                        return var(var_get<std::string>() <= other.var_get<std::string>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() <= other.var_get<long long>());
                    default:
                        break;
                    }
                }
                if (isNumeric() && other.isNumeric())
                {
                    return var(toDouble() <= other.toDouble());
                }
                throw pythonic::PythonicTypeError("unsupported types for comparison");
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
                        return var(var_get<int>() + other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(var_get<long long>() + other);
                    if (tag_ == TypeTag::INT)
                        return var(static_cast<long long>(var_get<int>()) + other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(var_get<double>() + static_cast<double>(other));
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
                        return var(lhs + rhs.var_get<int>());
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (rhs.tag_ == TypeTag::LONG_LONG)
                        return var(lhs + rhs.var_get<long long>());
                    if (rhs.tag_ == TypeTag::INT)
                        return var(lhs + static_cast<long long>(rhs.var_get<int>()));
                }
                return var(lhs) + rhs;
            }

            // OPTIMIZED: Direct const char* handling avoids var construction
            var operator+(const char *other) const
            {
                if (tag_ == TypeTag::STRING)
                {
                    return var(var_get<std::string>() + other);
                }
                return var(str() + other);
            }
            friend var operator+(const char *lhs, const var &rhs)
            {
                if (rhs.tag_ == TypeTag::STRING)
                {
                    return var(std::string(lhs) + rhs.var_get<std::string>());
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
                        return var(var_get<int>() - other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(var_get<long long>() - other);
                    if (tag_ == TypeTag::INT)
                        return var(static_cast<long long>(var_get<int>()) - other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(var_get<double>() - static_cast<double>(other));
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
                        return var(lhs - rhs.var_get<int>());
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
                        return var(var_get<int>() * other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(var_get<long long>() * other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(var_get<double>() * static_cast<double>(other));
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
                        return var(lhs * rhs.var_get<int>());
                }
                return var(lhs) * rhs;
            }

            // Division with primitives
            // Note: Python semantics - / always returns float, even for int/int
            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator/(T other) const
            {
                if (other == 0)
                    throw pythonic::PythonicZeroDivisionError::division();
                if constexpr (std::is_integral_v<T>)
                {
                    // Python: int / int -> float (always)
                    if (tag_ == TypeTag::INT)
                        return var(static_cast<double>(var_get<int>()) / static_cast<double>(other));
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(static_cast<double>(var_get<long long>()) / static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() / static_cast<double>(other));
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(var_get<double>() / static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() / static_cast<double>(other));
                }
                return *this / var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend var operator/(T lhs, const var &rhs)
            {
                if constexpr (std::is_integral_v<T>)
                {
                    if (rhs.tag_ == TypeTag::INT)
                    {
                        int b = rhs.var_get<int>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        // Python: int / int -> float (always)
                        return var(static_cast<double>(lhs) / static_cast<double>(b));
                    }
                    if (rhs.tag_ == TypeTag::LONG_LONG)
                    {
                        long long b = rhs.var_get<long long>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::division();
                        // Python: int / int -> float (always)
                        return var(static_cast<double>(lhs) / static_cast<double>(b));
                    }
                }
                return var(lhs) / rhs;
            }

            // Modulo with primitives
            template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
            var operator%(T other) const
            {
                if (other == 0)
                    throw pythonic::PythonicZeroDivisionError::modulo();
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(var_get<int>() % other);
                }
                if constexpr (std::is_same_v<T, long long>)
                {
                    if (tag_ == TypeTag::LONG_LONG)
                        return var(var_get<long long>() % other);
                    if (tag_ == TypeTag::INT)
                        return var(static_cast<long long>(var_get<int>()) % other);
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
                        int b = rhs.var_get<int>();
                        if (b == 0)
                            throw pythonic::PythonicZeroDivisionError::modulo();
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
                        return var(var_get<int>() == other);
                }
                if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag_ == TypeTag::DOUBLE)
                        return var(var_get<double>() == static_cast<double>(other));
                    if (isNumeric())
                        return var(toDouble() == static_cast<double>(other));
                }
                return *this == var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend bool operator==(T lhs, const var &rhs) { return static_cast<bool>(rhs == var(lhs)); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator!=(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(var_get<int>() != other);
                }
                return *this != var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend bool operator!=(T lhs, const var &rhs) { return static_cast<bool>(rhs != var(lhs)); }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator>(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(var_get<int>() > other);
                }
                return *this > var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend bool operator>(T lhs, const var &rhs) { return var(lhs) > rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator>=(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(var_get<int>() >= other);
                }
                return *this >= var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend bool operator>=(T lhs, const var &rhs) { return var(lhs) >= rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator<(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(var_get<int>() < other);
                }
                // Convert both to common numeric type for comparison
                return *this < var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend bool operator<(T lhs, const var &rhs) { return var(lhs) < rhs; }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            var operator<=(T other) const
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    if (tag_ == TypeTag::INT)
                        return var(var_get<int>() <= other);
                }
                return *this <= var(other);
            }

            template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
            friend bool operator<=(T lhs, const var &rhs) { return var(lhs) <= rhs; }

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
                    return var(~var_get<int>());
                case TypeTag::LONG:
                    return var(~var_get<long>());
                case TypeTag::LONG_LONG:
                    return var(~var_get<long long>());
                case TypeTag::UINT:
                    return var(~var_get<unsigned int>());
                case TypeTag::ULONG:
                    return var(~var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return var(~var_get<unsigned long long>());
                default:
                    throw pythonic::PythonicTypeError("bitwise NOT requires integral type");
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
                        return var(var_get<int>() & other.var_get<int>());
                    case TypeTag::LONG:
                        return var(var_get<long>() & other.var_get<long>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() & other.var_get<long long>());
                    case TypeTag::UINT:
                        return var(var_get<unsigned int>() & other.var_get<unsigned int>());
                    case TypeTag::ULONG:
                        return var(var_get<unsigned long>() & other.var_get<unsigned long>());
                    case TypeTag::ULONG_LONG:
                        return var(var_get<unsigned long long>() & other.var_get<unsigned long long>());
                    case TypeTag::SET:
                    {
                        const auto &a = var_get<Set>();
                        const auto &b = other.var_get<Set>();
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
                        const auto &a = var_get<OrderedSet>();
                        const auto &b = other.var_get<OrderedSet>();
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
                        const auto &a = var_get<List>();
                        const auto &b = other.var_get<List>();
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
                        const auto &a = var_get<Dict>();
                        const auto &b = other.var_get<Dict>();
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
                        const auto &a = var_get<OrderedDict>();
                        const auto &b = other.var_get<OrderedDict>();
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
                throw pythonic::PythonicTypeError("operator& requires integral types or containers");
            }

            var operator|(const var &other) const
            {
                // Fast-path: same type using tag
                if (tag_ == other.tag_)
                {
                    switch (tag_)
                    {
                    case TypeTag::INT:
                        return var(var_get<int>() | other.var_get<int>());
                    case TypeTag::LONG:
                        return var(var_get<long>() | other.var_get<long>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() | other.var_get<long long>());
                    case TypeTag::UINT:
                        return var(var_get<unsigned int>() | other.var_get<unsigned int>());
                    case TypeTag::ULONG:
                        return var(var_get<unsigned long>() | other.var_get<unsigned long>());
                    case TypeTag::ULONG_LONG:
                        return var(var_get<unsigned long long>() | other.var_get<unsigned long long>());
                    case TypeTag::SET:
                    {
                        const auto &a = var_get<Set>();
                        const auto &b = other.var_get<Set>();
                        Set result = a;
                        result.insert(b.begin(), b.end());
                        return var(std::move(result));
                    }
                    case TypeTag::ORDEREDSET:
                    {
                        const auto &a = var_get<OrderedSet>();
                        const auto &b = other.var_get<OrderedSet>();
                        OrderedSet result = a;
                        result.insert(b.begin(), b.end());
                        return var(std::move(result));
                    }
                    case TypeTag::LIST:
                    {
                        const auto &a = var_get<List>();
                        const auto &b = other.var_get<List>();
                        List result;
                        result.reserve(a.size() + b.size());
                        result.insert(result.end(), a.begin(), a.end());
                        result.insert(result.end(), b.begin(), b.end());
                        return var(std::move(result));
                    }
                    case TypeTag::DICT:
                    {
                        const auto &a = var_get<Dict>();
                        const auto &b = other.var_get<Dict>();
                        Dict result = a;
                        for (const auto &[key, val] : b)
                        {
                            result[key] = val;
                        }
                        return var(std::move(result));
                    }
                    case TypeTag::ORDEREDDICT:
                    {
                        const auto &a = var_get<OrderedDict>();
                        const auto &b = other.var_get<OrderedDict>();
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
                throw pythonic::PythonicTypeError("operator| requires integral types or containers");
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
                        return var(var_get<int>() ^ other.var_get<int>());
                    case TypeTag::LONG:
                        return var(var_get<long>() ^ other.var_get<long>());
                    case TypeTag::LONG_LONG:
                        return var(var_get<long long>() ^ other.var_get<long long>());
                    case TypeTag::UINT:
                        return var(var_get<unsigned int>() ^ other.var_get<unsigned int>());
                    case TypeTag::ULONG:
                        return var(var_get<unsigned long>() ^ other.var_get<unsigned long>());
                    case TypeTag::ULONG_LONG:
                        return var(var_get<unsigned long long>() ^ other.var_get<unsigned long long>());
                    case TypeTag::SET:
                    {
                        const auto &a = var_get<Set>();
                        const auto &b = other.var_get<Set>();
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
                        const auto &a = var_get<OrderedSet>();
                        const auto &b = other.var_get<OrderedSet>();
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
                        const auto &a = var_get<List>();
                        const auto &b = other.var_get<List>();
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
                throw pythonic::PythonicTypeError("operator^ requires integral types or sets/lists");
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
                    return var_get<bool>();
                case TypeTag::INT:
                    return var_get<int>() != 0;
                case TypeTag::LONG:
                    return var_get<long>() != 0;
                case TypeTag::LONG_LONG:
                    return var_get<long long>() != 0;
                case TypeTag::UINT:
                    return var_get<unsigned int>() != 0;
                case TypeTag::ULONG:
                    return var_get<unsigned long>() != 0;
                case TypeTag::ULONG_LONG:
                    return var_get<unsigned long long>() != 0;
                case TypeTag::FLOAT:
                    return var_get<float>() != 0.0f;
                case TypeTag::DOUBLE:
                    return var_get<double>() != 0.0;
                case TypeTag::LONG_DOUBLE:
                    return var_get<long double>() != 0.0L;
                case TypeTag::STRING:
                    return !var_get<std::string>().empty();
                case TypeTag::LIST:
                    return !var_get<List>().empty();
                case TypeTag::SET:
                    return !var_get<Set>().empty();
                case TypeTag::DICT:
                    return !var_get<Dict>().empty();
                case TypeTag::ORDEREDSET:
                    return !var_get<OrderedSet>().empty();
                case TypeTag::ORDEREDDICT:
                    return !var_get<OrderedDict>().empty();
                case TypeTag::GRAPH:
                    return graph_bool_impl(); // Defined after VarGraphWrapper
                default:
                    return true;
                }
            }

            // Explicit integer conversion operator
            // Allows: int i = static_cast<int>(some_var);
            // Note: Use explicit to prevent implicit conversion in mixed expressions
            explicit operator int() const
            {
                return toInt();
            }

            // Explicit long long conversion operator
            explicit operator long long() const
            {
                return toLongLong();
            }

            // Explicit double conversion operator
            // Allows: double d = static_cast<double>(some_var);
            explicit operator double() const
            {
                return toDouble();
            }

            // Explicit float conversion operator
            explicit operator float() const
            {
                return static_cast<float>(toDouble());
            }

            // Explicit size_t conversion operator (useful for indexing)
            // Allows: size_t idx = static_cast<size_t>(some_var);
            explicit operator size_t() const
            {
                long long v = toLongLong();
                if (v < 0)
                {
                    throw pythonic::PythonicTypeError("Cannot convert negative value to size_t");
                }
                return static_cast<size_t>(v);
            }

            // Explicit string conversion operator
            explicit operator std::string() const
            {
                return toString();
            }

            // Private helper for graph bool - defined after VarGraphWrapper
            bool graph_bool_impl() const;

            // Container access - operator[] for list and dict
            // OPTIMIZED: Uses TypeTag for fast dispatch
            var &operator[](size_t index)
            {
                if (tag_ == TypeTag::LIST)
                {
                    auto &lst = var_get<List>();
                    if (index >= lst.size())
                    {
                        throw PythonicIndexError("list", static_cast<long long>(index), lst.size());
                    }
                    return lst[index];
                }
                throw pythonic::PythonicTypeError("operator[] requires a list");
            }

            const var &operator[](size_t index) const
            {
                if (tag_ == TypeTag::LIST)
                {
                    const auto &lst = var_get<List>();
                    if (index >= lst.size())
                    {
                        throw PythonicIndexError("list", static_cast<long long>(index), lst.size());
                    }
                    return lst[index];
                }
                throw pythonic::PythonicTypeError("operator[] requires a list");
            }

            var &operator[](const std::string &key)
            {
                if (tag_ == TypeTag::DICT)
                {
                    return var_get<Dict>()[key];
                }
                if (tag_ == TypeTag::ORDEREDDICT)
                {
                    return var_get<OrderedDict>()[key];
                }
                throw pythonic::PythonicTypeError("operator[] requires a dict or ordered_dict");
            }

            var &operator[](const char *key)
            {
                return (*this)[std::string(key)];
            }

            // len() support
            // OPTIMIZED: Uses TypeTag for fast dispatch instead of std::visit
            var len() const
            {
                switch (tag_)
                {
                case TypeTag::STRING:
                    return var(static_cast<int64_t>(var_get<std::string>().size()));
                case TypeTag::LIST:
                    return var(static_cast<int64_t>(var_get<List>().size()));
                case TypeTag::SET:
                    return var(static_cast<int64_t>(var_get<Set>().size()));
                case TypeTag::DICT:
                    return var(static_cast<int64_t>(var_get<Dict>().size()));
                case TypeTag::ORDEREDSET:
                    return var(static_cast<int64_t>(var_get<OrderedSet>().size()));
                case TypeTag::ORDEREDDICT:
                    return var(static_cast<int64_t>(var_get<OrderedDict>().size()));
                default:
                    throw pythonic::PythonicTypeError("object has no len()");
                }
            }

            // append for list
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void append(const var &v)
            {
                if (tag_ == TypeTag::LIST)
                {
                    var_get<List>().push_back(v);
                }
                else
                {
                    throw pythonic::PythonicAttributeError("append() requires a list");
                }
            }

            // OPTIMIZED: Template overload for primitives - avoids var construction
            template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, var>>>
            void append(T &&v)
            {
                if (tag_ == TypeTag::LIST)
                {
                    var_get<List>().emplace_back(std::forward<T>(v));
                }
                else
                {
                    throw pythonic::PythonicAttributeError("append() requires a list");
                }
            }

            // add for set or ordered_set
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void add(const var &v)
            {
                if (tag_ == TypeTag::SET)
                {
                    var_get<Set>().insert(v);
                }
                else if (tag_ == TypeTag::ORDEREDSET)
                {
                    var_get<OrderedSet>().insert(v);
                }
                else
                {
                    throw pythonic::PythonicAttributeError("add() requires a set");
                }
            }

            // OPTIMIZED: Template overload for primitives - avoids var construction
            template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, var>>>
            void add(T &&v)
            {
                if (tag_ == TypeTag::SET)
                {
                    var_get<Set>().emplace(std::forward<T>(v));
                }
                else if (tag_ == TypeTag::ORDEREDSET)
                {
                    var_get<OrderedSet>().emplace(std::forward<T>(v));
                }
                else
                {
                    throw pythonic::PythonicAttributeError("add() requires a set");
                }
            }

            // extend for list - adds all elements from another iterable
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void extend(const var &other)
            {
                if (tag_ != TypeTag::LIST)
                {
                    throw pythonic::PythonicAttributeError("extend() requires a list");
                }
                auto &lst = var_get<List>();
                switch (other.tag_)
                {
                case TypeTag::LIST:
                {
                    const auto &other_lst = other.var_get<List>();
                    lst.insert(lst.end(), other_lst.begin(), other_lst.end());
                    break;
                }
                case TypeTag::SET:
                {
                    const auto &other_set = other.var_get<Set>();
                    for (const auto &item : other_set)
                    {
                        lst.push_back(item);
                    }
                    break;
                }
                case TypeTag::STRING:
                {
                    const auto &other_str = other.var_get<std::string>();
                    for (char c : other_str)
                    {
                        lst.push_back(var(std::string(1, c)));
                    }
                    break;
                }
                default:
                    throw pythonic::PythonicTypeError("extend() requires an iterable");
                }
            }

            // update for set - adds all elements from another iterable (like Python's set.update())
            // OPTIMIZED: Uses TypeTag for fast dispatch
            void update(const var &other)
            {
                if (tag_ != TypeTag::SET)
                {
                    throw pythonic::PythonicAttributeError("update() requires a set");
                }
                auto &st = var_get<Set>();
                switch (other.tag_)
                {
                case TypeTag::SET:
                {
                    const auto &other_set = other.var_get<Set>();
                    st.insert(other_set.begin(), other_set.end());
                    break;
                }
                case TypeTag::LIST:
                {
                    const auto &other_lst = other.var_get<List>();
                    for (const auto &item : other_lst)
                    {
                        st.insert(item);
                    }
                    break;
                }
                default:
                    throw pythonic::PythonicTypeError("update() requires an iterable");
                }
            }

            // contains check (in operator simulation)
            // OPTIMIZED: Uses TypeTag for fast dispatch
            var contains(const var &v) const
            {
                switch (tag_)
                {
                case TypeTag::LIST:
                {
                    const auto &lst = var_get<List>();
                    for (const auto &item : lst)
                    {
                        if (static_cast<bool>(item == v))
                            return var(true);
                    }
                    return var(false);
                }
                case TypeTag::SET:
                    return var(var_get<Set>().find(v) != var_get<Set>().end());
                case TypeTag::ORDEREDSET:
                    return var(var_get<OrderedSet>().find(v) != var_get<OrderedSet>().end());
                case TypeTag::DICT:
                    if (v.tag_ == TypeTag::STRING)
                    {
                        return var(var_get<Dict>().find(v.var_get<std::string>()) != var_get<Dict>().end());
                    }
                    return var(false);
                case TypeTag::ORDEREDDICT:
                    if (v.tag_ == TypeTag::STRING)
                    {
                        return var(var_get<OrderedDict>().find(v.var_get<std::string>()) != var_get<OrderedDict>().end());
                    }
                    return var(false);
                case TypeTag::STRING:
                    if (v.tag_ == TypeTag::STRING)
                    {
                        return var(var_get<std::string>().find(v.var_get<std::string>()) != std::string::npos);
                    }
                    return var(false);
                default:
                    return var(false);
                }
            }

            // has() - alias for contains(), commonly used with dicts/sets
            var has(const var &v) const
            {
                return contains(v);
            }

            // remove() - remove element from set or list
            void remove(const var &v)
            {
                switch (tag_)
                {
                case TypeTag::SET:
                {
                    auto &s = var_get<Set>();
                    s.erase(v);
                    break;
                }
                case TypeTag::ORDEREDSET:
                {
                    auto &s = var_get<OrderedSet>();
                    s.erase(v);
                    break;
                }
                case TypeTag::LIST:
                {
                    auto &lst = var_get<List>();
                    auto it = std::find_if(lst.begin(), lst.end(),
                                           [&v](const var &item)
                                           { return static_cast<bool>(item == v); });
                    if (it != lst.end())
                    {
                        lst.erase(it);
                    }
                    break;
                }
                default:
                    throw pythonic::PythonicTypeError("remove() not supported for this type");
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
                switch (tag_)
                {
                case TypeTag::LIST:
                    return iterator(var_get<List>().begin());
                case TypeTag::SET:
                    return iterator(var_get<Set>().begin());
                case TypeTag::DICT:
                    return iterator(var_get<Dict>().begin());
                case TypeTag::STRING:
                    return iterator(var_get<std::string>().begin());
                case TypeTag::ORDEREDSET:
                    return iterator(var_get<OrderedSet>().begin());
                case TypeTag::ORDEREDDICT:
                    return iterator(var_get<OrderedDict>().begin());
                default:
                    throw pythonic::PythonicIterationError("type is not iterable");
                }
            }

            iterator end()
            {
                switch (tag_)
                {
                case TypeTag::LIST:
                    return iterator(var_get<List>().end());
                case TypeTag::SET:
                    return iterator(var_get<Set>().end());
                case TypeTag::DICT:
                    return iterator(var_get<Dict>().end());
                case TypeTag::STRING:
                    return iterator(var_get<std::string>().end());
                case TypeTag::ORDEREDSET:
                    return iterator(var_get<OrderedSet>().end());
                case TypeTag::ORDEREDDICT:
                    return iterator(var_get<OrderedDict>().end());
                default:
                    throw pythonic::PythonicIterationError("type is not iterable");
                }
            }

            const_iterator begin() const
            {
                switch (tag_)
                {
                case TypeTag::LIST:
                    return const_iterator(var_get<List>().begin());
                case TypeTag::SET:
                    return const_iterator(var_get<Set>().begin());
                case TypeTag::DICT:
                    return const_iterator(var_get<Dict>().begin());
                case TypeTag::STRING:
                    return const_iterator(var_get<std::string>().begin());
                case TypeTag::ORDEREDSET:
                    return const_iterator(var_get<OrderedSet>().begin());
                case TypeTag::ORDEREDDICT:
                    return const_iterator(var_get<OrderedDict>().begin());
                default:
                    throw pythonic::PythonicIterationError("type is not iterable");
                }
            }

            const_iterator end() const
            {
                switch (tag_)
                {
                case TypeTag::LIST:
                    return const_iterator(var_get<List>().end());
                case TypeTag::SET:
                    return const_iterator(var_get<Set>().end());
                case TypeTag::DICT:
                    return const_iterator(var_get<Dict>().end());
                case TypeTag::STRING:
                    return const_iterator(var_get<std::string>().end());
                case TypeTag::ORDEREDSET:
                    return const_iterator(var_get<OrderedSet>().end());
                case TypeTag::ORDEREDDICT:
                    return const_iterator(var_get<OrderedDict>().end());
                default:
                    throw pythonic::PythonicIterationError("type is not iterable");
                }
            }

            const_iterator cbegin() const { return begin(); }
            const_iterator cend() const { return end(); }

            // items() for dict - returns list of [key, value] pairs
            // Note: For better performance in loops, consider using items_fast()
            // which returns an iterable view instead of materializing a list
            var items() const
            {
                if (tag_ == TypeTag::DICT)
                {
                    const Dict &dct = var_get<Dict>();
                    List result;
                    result.reserve(dct.size()); // Pre-allocate
                    for (const auto &[k, v] : dct)
                    {
                        // Create a [key, value] list pair
                        List pair;
                        pair.reserve(2); // Pre-allocate for 2 elements
                        pair.emplace_back(k);
                        pair.emplace_back(v);
                        result.emplace_back(std::move(pair));
                    }
                    return var(std::move(result));
                }
                throw pythonic::PythonicAttributeError("items() requires a dict");
            }

            // keys() for dict
            var keys() const
            {
                if (tag_ == TypeTag::DICT)
                {
                    const Dict &dct = var_get<Dict>();
                    List result;
                    for (const auto &[k, v] : dct)
                    {
                        result.push_back(var(k));
                    }
                    return var(result);
                }
                throw pythonic::PythonicAttributeError("keys() requires a dict");
            }

            // values() for dict
            var values() const
            {
                if (tag_ == TypeTag::DICT)
                {
                    const Dict &dct = var_get<Dict>();
                    List result;
                    for (const auto &[k, v] : dct)
                    {
                        result.push_back(v);
                    }
                    return var(result);
                }
                throw pythonic::PythonicAttributeError("values() requires a dict");
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
                if (auto *lst = var_get_if<List>())
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
                if (auto *s = var_get_if<std::string>())
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

                throw pythonic::PythonicTypeError("slice() requires a list or string");
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
                if (auto *lst = var_get_if<List>())
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
                if (auto *s = var_get_if<std::string>())
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

                throw pythonic::PythonicTypeError("slice() requires a list or string");
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
                if (auto *s = var_get_if<std::string>())
                {
                    std::string result = *s;
                    for (char &c : result)
                    {
                        c = std::toupper(static_cast<unsigned char>(c));
                    }
                    return var(result);
                }
                throw pythonic::PythonicAttributeError("upper() requires a string");
            }

            // lower() - convert to lowercase
            var lower() const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    std::string result = *s;
                    for (char &c : result)
                    {
                        c = std::tolower(static_cast<unsigned char>(c));
                    }
                    return var(result);
                }
                throw pythonic::PythonicAttributeError("lower() requires a string");
            }

            // strip() - remove leading/trailing whitespace
            var strip() const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("strip() requires a string");
            }

            // lstrip() - remove leading whitespace
            var lstrip() const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    std::string result = *s;
                    result.erase(result.begin(), std::find_if(result.begin(), result.end(),
                                                              [](unsigned char ch)
                                                              { return !std::isspace(ch); }));
                    return var(result);
                }
                throw pythonic::PythonicAttributeError("lstrip() requires a string");
            }

            // rstrip() - remove trailing whitespace
            var rstrip() const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    std::string result = *s;
                    result.erase(std::find_if(result.rbegin(), result.rend(),
                                              [](unsigned char ch)
                                              { return !std::isspace(ch); })
                                     .base(),
                                 result.end());
                    return var(result);
                }
                throw pythonic::PythonicAttributeError("rstrip() requires a string");
            }

            // replace(old, new) - replace all occurrences
            var replace(const var &old_str, const var &new_str) const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("replace() requires a string");
            }

            // find(substring) - find position of substring (-1 if not found)
            var find(const var &substr) const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    std::string sub = substr.get<std::string>();
                    size_t pos = s->find(sub);
                    if (pos == std::string::npos)
                    {
                        return var(-1);
                    }
                    return var(static_cast<long long>(pos));
                }
                throw pythonic::PythonicAttributeError("find() requires a string");
            }

            // startswith(prefix) - check if string starts with prefix
            var startswith(const var &prefix) const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    std::string pre = prefix.get<std::string>();
                    return var(s->substr(0, pre.length()) == pre);
                }
                throw pythonic::PythonicAttributeError("startswith() requires a string");
            }

            // endswith(suffix) - check if string ends with suffix
            var endswith(const var &suffix) const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    std::string suf = suffix.get<std::string>();
                    if (s->length() < suf.length())
                    {
                        return var(false);
                    }
                    return var(s->substr(s->length() - suf.length()) == suf);
                }
                throw pythonic::PythonicAttributeError("endswith() requires a string");
            }

            // isdigit() - check if all characters are digits
            var isdigit() const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("isdigit() requires a string");
            }

            // isalpha() - check if all characters are alphabetic
            var isalpha() const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("isalpha() requires a string");
            }

            // isalnum() - check if all characters are alphanumeric
            var isalnum() const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("isalnum() requires a string");
            }

            // isspace() - check if all characters are whitespace
            var isspace() const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("isspace() requires a string");
            }

            // capitalize() - capitalize first character
            var capitalize() const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("capitalize() requires a string");
            }

            // title() - title case (first letter of each word capitalized)
            var title() const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("title() requires a string");
            }

            // count(substring) - count occurrences of substring
            var count(const var &substr) const
            {
                if (auto *s = var_get_if<std::string>())
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
                if (auto *lst = var_get_if<List>())
                {
                    int count = 0;
                    for (const auto &item : *lst)
                    {
                        if (item == substr)
                            ++count;
                    }
                    return var(count);
                }
                throw pythonic::PythonicTypeError("count() requires a string or list");
            }

            // reverse() - reverse string or list (returns new var)
            var reverse() const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    std::string result(*s);
                    std::reverse(result.begin(), result.end());
                    return var(result);
                }
                if (auto *lst = var_get_if<List>())
                {
                    List result(*lst);
                    std::reverse(result.begin(), result.end());
                    return var(result);
                }
                throw pythonic::PythonicTypeError("reverse() requires a string or list");
            }

            // split() - split string by delimiter (default: whitespace)
            var split(const var &delim = var(" ")) const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("split() requires a string");
            }

            // join(list) - join list elements with this string as separator
            var join(const var &lst) const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    if (auto *l = lst.var_get_if<List>())
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
                throw pythonic::PythonicTypeError("join() requires a string separator and a list");
            }

            // center(width, fillchar) - center string in field of given width
            var center(int width, const var &fillchar = var(" ")) const
            {
                if (auto *s = var_get_if<std::string>())
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
                throw pythonic::PythonicAttributeError("center() requires a string");
            }

            // zfill(width) - pad string with zeros on the left
            var zfill(int width) const
            {
                if (auto *s = var_get_if<std::string>())
                {
                    int len = static_cast<int>(s->length());
                    if (width <= len)
                        return var(*s);
                    return var(std::string(width - len, '0') + *s);
                }
                throw pythonic::PythonicAttributeError("zfill() requires a string");
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
                    return h ^ std::hash<int>{}(var_get<int>());
                case TypeTag::FLOAT:
                    return h ^ std::hash<float>{}(var_get<float>());
                case TypeTag::DOUBLE:
                    return h ^ std::hash<double>{}(var_get<double>());
                case TypeTag::STRING:
                    return h ^ std::hash<std::string>{}(var_get<std::string>());
                case TypeTag::BOOL:
                    return h ^ std::hash<bool>{}(var_get<bool>());
                case TypeTag::LONG:
                    return h ^ std::hash<long>{}(var_get<long>());
                case TypeTag::LONG_LONG:
                    return h ^ std::hash<long long>{}(var_get<long long>());
                case TypeTag::LONG_DOUBLE:
                    return h ^ std::hash<long double>{}(var_get<long double>());
                case TypeTag::UINT:
                    return h ^ std::hash<unsigned int>{}(var_get<unsigned int>());
                case TypeTag::ULONG:
                    return h ^ std::hash<unsigned long>{}(var_get<unsigned long>());
                case TypeTag::ULONG_LONG:
                    return h ^ std::hash<unsigned long long>{}(var_get<unsigned long long>());
                case TypeTag::LIST:
                {
                    // Hash list by combining element hashes
                    const auto &lst = var_get<List>();
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
                    const auto &s = var_get<Set>();
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
                    const auto &d = var_get<Dict>();
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
                    const auto &hs = var_get<OrderedSet>();
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
                    const auto &od = var_get<OrderedDict>();
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
                    const auto &g = var_get<GraphPtr>();
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

            // ===== Node Manipulation =====
            size_t add_node();
            size_t add_node(const var &data);
            void remove_node(size_t node);
            var neighbors(size_t node) const;

            // ===== Graph Modification =====
            void add_edge(size_t u, size_t v, double w1 = 0.0, double w2 = std::numeric_limits<double>::quiet_NaN(), bool directed = false);
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

            // ===== Interactive Graph Viewer =====
#ifdef PYTHONIC_ENABLE_GRAPH_VIEWER
            /**
             * @brief Open interactive graph viewer window
             * @param blocking If true, blocks until viewer window is closed
             *
             * View Mode: Click nodes to trigger signal flow, drag for fun (snaps back)
             * Edit Mode: Double-click to add nodes, drag node-to-node to add edges
             *
             * Usage:
             *   var g = graph(5);
             *   g.add_edge(0, 1);
             *   g.show();  // Opens interactive viewer
             */
            void show(bool blocking = true);
#endif

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
            size_t size() const { return impl.size(); } // Pythonic alias for node_count
            bool is_connected() const { return impl.is_connected(); }
            bool has_cycle() const { return impl.has_cycle(); }
            bool has_edge(size_t from, size_t to) const { return impl.has_edge(from, to); }
            std::optional<double> get_edge_weight(size_t from, size_t to) const { return impl.get_edge_weight(from, to); }
            size_t out_degree(size_t node) const { return impl.out_degree(node); }
            size_t in_degree(size_t node) const { return impl.in_degree(node); }

            // ===== Node Manipulation =====
            size_t add_node() { return impl.add_node(); }
            size_t add_node(const var &data) { return impl.add_node(data); }
            std::vector<size_t> neighbors(size_t node) const { return impl.neighbors(node); }

            // Remove a node from the graph (renumbers subsequent nodes)
            void remove_node(size_t node)
            {
                impl.remove_node(node);
            }

            // ===== Graph Modification =====
            void add_edge(size_t u, size_t v, double w1 = 0.0, double w2 = std::numeric_limits<double>::quiet_NaN(), bool directed = false)
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

            // Pretty ASCII tree representation for terminal output
            std::string pretty_str(size_t start = 0,
                                   size_t indent_step = 2,
                                   size_t max_depth = 64,
                                   bool show_weights = true,
                                   bool show_node_data = false,
                                   bool collapse_visited = true,
                                   bool directed = false) const
            {
                std::ostringstream out;

                // Header metadata
                out << "Graph: nodes=" << impl.node_count()
                    << ", edges=" << impl.edge_count()
                    << ", directed=" << (directed ? "yes" : "no")
                    << ", connected=" << (impl.is_connected() ? "yes" : "no")
                    << ", cycle=" << (impl.has_cycle() ? "yes" : "no")
                    << "\n";

                if (impl.node_count() == 0)
                    return out.str();

                std::unordered_set<size_t> visited;
                std::vector<char> in_path(impl.node_count(), 0);

                std::function<void(size_t, std::vector<bool> &, size_t, size_t, size_t)> print_node;

                print_node = [&](size_t node, std::vector<bool> &last_stack, size_t depth, size_t parent, size_t idx_in_parent)
                {
                    if (depth > max_depth)
                    {
                        out << std::string(last_stack.size() * indent_step, ' ') << "...\n";
                        return;
                    }

                    // Build prefix using box-drawing style: '│   ', '├── ', '└── '
                    std::string prefix;
                    for (size_t i = 0; i + 1 < last_stack.size(); ++i)
                    {
                        prefix += last_stack[i] ? std::string("    ") : std::string("│   ");
                    }

                    if (!last_stack.empty())
                    {
                        prefix += last_stack.back() ? std::string("└── ") : std::string("├── ");
                    }

                    // Node label
                    out << prefix << node;

                    if (show_node_data)
                    {
                        try
                        {
                            auto &nd = impl.get_node_data(node);
                            std::string s = nd.str();
                            if (s.size() > 80)
                                s = s.substr(0, 77) + "...";
                            out << ": " << s;
                        }
                        catch (...)
                        {
                        }
                    }

                    out << "\n";

                    if (visited.count(node) && collapse_visited)
                        return;

                    visited.insert(node);
                    in_path[node] = 1;

                    auto nbrs = impl.neighbors(node);
                    for (size_t i = 0; i < nbrs.size(); ++i)
                    {
                        size_t nbr = nbrs[i];

                        // In undirected mode, skip parent to avoid trivial back-edge
                        if (!directed && parent != static_cast<size_t>(-1) && nbr == parent)
                            continue;

                        bool is_last = (i + 1 == nbrs.size());
                        last_stack.push_back(is_last);

                        // If neighbor is in current DFS path -> cycle
                        if (in_path.size() > nbr && in_path[nbr])
                        {
                            std::string child_prefix;
                            for (size_t j = 0; j + 1 < last_stack.size(); ++j)
                            {
                                child_prefix += last_stack[j] ? std::string("    ") : std::string("│   ");
                            }
                            child_prefix += last_stack.back() ? std::string("└── ") : std::string("├── ");
                            out << child_prefix << "[Ref] " << nbr << "  <-- (Cycle Detected)\n";
                            last_stack.pop_back();
                            continue;
                        }

                        // If neighbor already visited and collapsing, print a reference line
                        if (visited.count(nbr) && collapse_visited)
                        {
                            std::string child_prefix;
                            for (size_t j = 0; j + 1 < last_stack.size(); ++j)
                            {
                                child_prefix += last_stack[j] ? std::string("    ") : std::string("│   ");
                            }
                            child_prefix += last_stack.back() ? std::string("└── ") : std::string("├── ");
                            out << child_prefix << "[Ref] " << nbr << "\n";
                            last_stack.pop_back();
                            continue;
                        }

                        print_node(nbr, last_stack, depth + 1, node, i);
                        last_stack.pop_back();
                    }
                    in_path[node] = 0;
                };

                std::vector<bool> empty_stack;
                print_node(start, empty_stack, 0, static_cast<size_t>(-1), 0);

                // If multiple components exist, and start != 0, list other components
                if (impl.node_count() > 0 && start != 0)
                {
                    for (size_t i = 0; i < impl.node_count(); ++i)
                    {
                        if (!visited.count(i))
                        {
                            out << "Component starting at " << i << ":\n";
                            std::vector<bool> st;
                            print_node(i, st, 0, static_cast<size_t>(-1), 0);
                        }
                    }
                }

                return out.str();
            }
        };

        // ============ var Graph Method Definitions ============
        // These are defined after VarGraphWrapper is complete to avoid incomplete type errors.

        inline VarGraphWrapper &var::graph_ref()
        {
            if (tag_ != TypeTag::GRAPH)
                throw pythonic::PythonicGraphError("operation requires a graph");
            auto &ptr = var_get<GraphPtr>();
            if (!ptr)
                throw pythonic::PythonicGraphError("graph is null");
            return *ptr;
        }

        inline const VarGraphWrapper &var::graph_ref() const
        {
            if (tag_ != TypeTag::GRAPH)
                throw pythonic::PythonicGraphError("operation requires a graph");
            const auto &ptr = var_get<GraphPtr>();
            if (!ptr)
                throw pythonic::PythonicGraphError("graph is null");
            return *ptr;
        }

        // Helper implementations for str() and operator bool()
        inline std::string var::graph_str_impl() const
        {
            return graph_ref().pretty_str();
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

        // ===== Node Manipulation =====
        inline size_t var::add_node() { return graph_ref().add_node(); }
        inline size_t var::add_node(const var &data) { return graph_ref().add_node(data); }
        inline void var::remove_node(size_t node) { graph_ref().remove_node(node); }
        inline var var::neighbors(size_t node) const
        {
            auto nbrs = graph_ref().neighbors(node);
            List result;
            result.reserve(nbrs.size());
            for (size_t n : nbrs)
            {
                result.emplace_back(var(static_cast<long long>(n)));
            }
            return var(std::move(result));
        }

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
                throw pythonic::PythonicGraphError("reserve_edges_by_counts requires a list of counts");
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

        // ===== Interactive Graph Viewer =====

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

        // Helper: Convert type name string to TypeTag for fast comparison
        inline TypeTag type_name_to_tag(const char *type_name) noexcept
        {
            // Fast path for common single-char differences
            if (!type_name || !type_name[0])
                return TypeTag::NONE;

            switch (type_name[0])
            {
            case 'i':
                return TypeTag::INT; // "int"
            case 'f':
                return TypeTag::FLOAT; // "float"
            case 's':
                if (type_name[1] == 't')
                    return TypeTag::STRING; // "str"
                return TypeTag::SET;        // "set"
            case 'b':
                return TypeTag::BOOL; // "bool"
            case 'd':
                if (type_name[1] == 'i')
                    return TypeTag::DICT; // "dict"
                return TypeTag::DOUBLE;   // "double"
            case 'l':
                if (type_name[1] == 'i')
                    return TypeTag::LIST; // "list"
                if (type_name[1] == 'o' && type_name[2] == 'n' && type_name[3] == 'g')
                {
                    if (type_name[4] == ' ')
                    {
                        if (type_name[5] == 'l')
                            return TypeTag::LONG_LONG; // "long long"
                        if (type_name[5] == 'd')
                            return TypeTag::LONG_DOUBLE; // "long double"
                    }
                    return TypeTag::LONG; // "long"
                }
                break;
            case 'N':
                return TypeTag::NONE; // "NoneType"
            case 'u':
                if (type_name[9] == 'i')
                    return TypeTag::UINT; // "unsigned int"
                if (type_name[9] == 'l')
                {
                    if (type_name[13] == ' ')
                        return TypeTag::ULONG_LONG; // "unsigned long long"
                    return TypeTag::ULONG;          // "unsigned long"
                }
                break;
            case 'o':
                return TypeTag::ORDEREDSET; // "ordered_set" or "ordereddict"
            case 'g':
                return TypeTag::GRAPH; // "graph"
            }
            return TypeTag::NONE;
        }

        // isinstance with string type name - like Python's isinstance(obj, type)
        // Usage: isinstance(v, "int"), isinstance(v, "list"), etc.
        // OPTIMIZED: Uses TypeTag comparison instead of string comparison
        inline bool isinstance(const var &v, const std::string &type_name)
        {
            return v.type_tag() == type_name_to_tag(type_name.c_str());
        }

        inline bool isinstance(const var &v, const char *type_name)
        {
            return v.type_tag() == type_name_to_tag(type_name);
        }

        // ============ Python Built-in Functions ============

        // bool() - Python truthiness rules
        // Empty containers, 0, empty string = False; else True
        // OPTIMIZED: Uses TypeTag for fast dispatch instead of string comparison
        inline var Bool(const var &v)
        {
            switch (v.type_tag())
            {
            case TypeTag::BOOL:
                return v;
            case TypeTag::INT:
                return var(v.get<int>() != 0);
            case TypeTag::FLOAT:
                return var(v.get<float>() != 0.0f);
            case TypeTag::DOUBLE:
                return var(v.get<double>() != 0.0);
            case TypeTag::LONG:
                return var(v.get<long>() != 0);
            case TypeTag::LONG_LONG:
                return var(v.get<long long>() != 0);
            case TypeTag::UINT:
                return var(v.get<unsigned int>() != 0);
            case TypeTag::ULONG:
                return var(v.get<unsigned long>() != 0);
            case TypeTag::ULONG_LONG:
                return var(v.get<unsigned long long>() != 0);
            case TypeTag::LONG_DOUBLE:
                return var(v.get<long double>() != 0.0L);
            case TypeTag::STRING:
                return var(!v.get<std::string>().empty());
            case TypeTag::LIST:
                return var(!v.get<List>().empty());
            case TypeTag::DICT:
                return var(!v.get<Dict>().empty());
            case TypeTag::SET:
                return var(!v.get<Set>().empty());
            case TypeTag::ORDEREDSET:
                return var(!v.get<OrderedSet>().empty());
            case TypeTag::ORDEREDDICT:
                return var(!v.get<OrderedDict>().empty());
            case TypeTag::NONE:
                return var(false);
            default:
                return var(true); // unknown types are truthy
            }
        }

        // repr() - String representation with quotes and escapes
        // OPTIMIZED: Uses TypeTag for fast dispatch
        inline var repr(const var &v)
        {
            if (v.type_tag() == TypeTag::STRING)
            {
                const std::string &s = v.get<std::string>();
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
        // OPTIMIZED: Uses TypeTag for fast dispatch
        inline var Int(const var &v)
        {
            switch (v.type_tag())
            {
            case TypeTag::INT:
                return v;
            case TypeTag::FLOAT:
                return var(static_cast<int>(v.get<float>()));
            case TypeTag::DOUBLE:
                return var(static_cast<int>(v.get<double>()));
            case TypeTag::LONG:
                return var(static_cast<int>(v.get<long>()));
            case TypeTag::LONG_LONG:
                return var(static_cast<int>(v.get<long long>()));
            case TypeTag::UINT:
                return var(static_cast<int>(v.get<unsigned int>()));
            case TypeTag::ULONG:
                return var(static_cast<int>(v.get<unsigned long>()));
            case TypeTag::ULONG_LONG:
                return var(static_cast<int>(v.get<unsigned long long>()));
            case TypeTag::LONG_DOUBLE:
                return var(static_cast<int>(v.get<long double>()));
            case TypeTag::BOOL:
                return var(v.get<bool>() ? 1 : 0);
            case TypeTag::STRING:
                try
                {
                    return var(std::stoi(v.get<std::string>()));
                }
                catch (...)
                {
                    throw pythonic::PythonicValueError("invalid literal for int(): '" + v.get<std::string>() + "'");
                }
            default:
                throw pythonic::PythonicValueError("cannot convert " + v.type() + " to int");
            }
        }

        // float() - convert to float/double
        // OPTIMIZED: Uses TypeTag for fast dispatch
        inline var Float(const var &v)
        {
            switch (v.type_tag())
            {
            case TypeTag::DOUBLE:
            case TypeTag::FLOAT:
                return v;
            case TypeTag::INT:
                return var(static_cast<double>(v.get<int>()));
            case TypeTag::LONG:
                return var(static_cast<double>(v.get<long>()));
            case TypeTag::LONG_LONG:
                return var(static_cast<double>(v.get<long long>()));
            case TypeTag::UINT:
                return var(static_cast<double>(v.get<unsigned int>()));
            case TypeTag::ULONG:
                return var(static_cast<double>(v.get<unsigned long>()));
            case TypeTag::ULONG_LONG:
                return var(static_cast<double>(v.get<unsigned long long>()));
            case TypeTag::LONG_DOUBLE:
                return var(static_cast<double>(v.get<long double>()));
            case TypeTag::BOOL:
                return var(v.get<bool>() ? 1.0 : 0.0);
            case TypeTag::STRING:
                try
                {
                    return var(std::stod(v.get<std::string>()));
                }
                catch (...)
                {
                    throw pythonic::PythonicValueError("could not convert string to float: '" + v.get<std::string>() + "'");
                }
            default:
                throw pythonic::PythonicValueError("cannot convert " + v.type() + " to float");
            }
        }

        // abs() - absolute value
        // OPTIMIZED: Uses TypeTag for fast dispatch
        inline var abs(const var &v)
        {
            switch (v.type_tag())
            {
            case TypeTag::INT:
                return var(std::abs(v.get<int>()));
            case TypeTag::FLOAT:
                return var(std::abs(v.get<float>()));
            case TypeTag::DOUBLE:
                return var(std::abs(v.get<double>()));
            case TypeTag::LONG:
                return var(std::abs(v.get<long>()));
            case TypeTag::LONG_LONG:
                return var(std::abs(v.get<long long>()));
            case TypeTag::LONG_DOUBLE:
                return var(std::abs(v.get<long double>()));
            default:
                throw pythonic::PythonicTypeError("abs() requires numeric type, got " + v.type());
            }
        }

        // min() - minimum of list or two values
        inline var min(const var &a, const var &b)
        {
            if (a < b)
                return a;
            return b;
        }

        // OPTIMIZED: Uses TypeTag for fast dispatch
        inline var min(const var &lst)
        {
            if (lst.type_tag() != TypeTag::LIST)
            {
                throw pythonic::PythonicTypeError("min() expects a list or two arguments");
            }
            const auto &l = lst.get<List>();
            if (l.empty())
                throw pythonic::PythonicValueError("min() arg is an empty sequence");
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

        // OPTIMIZED: Uses TypeTag for fast dispatch
        inline var max(const var &lst)
        {
            if (lst.type_tag() != TypeTag::LIST)
            {
                throw pythonic::PythonicTypeError("max() expects a list or two arguments");
            }
            const auto &l = lst.get<List>();
            if (l.empty())
                throw pythonic::PythonicValueError("max() arg is an empty sequence");
            var result = l[0];
            for (size_t i = 1; i < l.size(); ++i)
            {
                if (result < l[i])
                    result = l[i];
            }
            return result;
        }

        // sum() - sum of list elements
        // OPTIMIZED: Uses TypeTag for fast dispatch
        inline var sum(const var &lst, const var &start = var(0))
        {
            if (lst.type_tag() != TypeTag::LIST)
            {
                throw pythonic::PythonicTypeError("sum() expects a list");
            }
            var result = start;
            const auto &l = lst.get<List>();
            for (const auto &item : l)
            {
                result = result + item;
            }
            return result;
        }

        // NOTE: sorted() moved to pythonicFunction.hpp to avoid ambiguity
        // Use pythonic::func::sorted() instead

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
            throw pythonic::PythonicTypeError("reversed_var() expects list or string");
        }

        // all_var() - return True if all elements are truthy (var version)
        inline var all_var(const var &lst)
        {
            if (lst.type() != "list")
            {
                throw pythonic::PythonicTypeError("all_var() expects a list");
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
                throw pythonic::PythonicTypeError("any_var() expects a list");
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
                throw pythonic::PythonicTypeError("map() expects a list");
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
                throw pythonic::PythonicTypeError("filter() expects a list");
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
                throw pythonic::PythonicTypeError("reduce() expects a list");
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
                throw pythonic::PythonicTypeError("reduce() expects a list");
            }
            const auto &l = lst.get<List>();
            if (l.empty())
                throw pythonic::PythonicValueError("reduce() of empty sequence with no initial value");
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
            if (prompt.is_string())
            {
                const std::string &s = prompt.as_string_unchecked();
                if (!s.empty())
                {
                    std::cout << s;
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
        // Usage: get(pair, 0) instead of var_get<0>(pair)

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
                throw PythonicIndexError("tuple", static_cast<long long>(index), sizeof...(Is));
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

// ============ Interactive Graph Viewer Implementation ============
#ifdef PYTHONIC_ENABLE_GRAPH_VIEWER
    } // namespace vars
    namespace viewer
    {
        void show_graph(pythonic::vars::var &, bool);
    }
    namespace vars
    {
        inline void var::show(bool blocking)
        {
            pythonic::viewer::show_graph(*this, blocking);
        }
#endif

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
