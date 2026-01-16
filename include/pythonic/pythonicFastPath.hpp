/**
 * @file pythonicFastPath.hpp
 * @brief Fast Path Cache for hot-loop optimization in pythonic library
 *
 * This file implements the CachedBinOp system that was deferred from the
 * previous iteration. The key insight is that in tight loops, var's type
 * usually doesn't change between iterations. We can exploit this by caching
 * the type-dispatched function pointer and reusing it.
 *
 * Design Goals:
 * 1. Zero overhead when not used (opt-in)
 * 2. Minimal overhead when types change (graceful degradation)
 * 3. Significant speedup for homogeneous loops
 * 4. Thread-safe (no shared mutable state)
 *
 * Usage:
 *   // Hot loop with repeated additions
 *   pythonic::fast::CachedAdd adder;
 *   for (auto& x : large_list) {
 *       sum = adder(sum, x);  // Caches type-dispatch after first call
 *   }
 */

#pragma once

#include <functional>
#include <cstdint>
#include "pythonicVars.hpp"
#include "pythonicError.hpp"

namespace pythonic
{
    namespace fast
    {

        using namespace pythonic::vars;

        // ============================================================================
        // Type-Pair Key for Fast Path Cache
        // ============================================================================

        /**
         * @brief Combines two TypeTags into a single 16-bit key for fast comparison
         *
         * Layout: [8 bits: left tag][8 bits: right tag]
         * This allows single-comparison cache hit checking instead of two comparisons.
         */
        struct TypePairKey
        {
            uint16_t key;

            constexpr TypePairKey() noexcept : key(0) {}

            constexpr TypePairKey(TypeTag left, TypeTag right) noexcept
                : key(static_cast<uint16_t>((static_cast<uint8_t>(left) << 8) |
                                            static_cast<uint8_t>(right))) {}

            constexpr bool operator==(TypePairKey other) const noexcept
            {
                return key == other.key;
            }

            constexpr bool operator!=(TypePairKey other) const noexcept
            {
                return key != other.key;
            }

            static constexpr TypePairKey invalid() noexcept
            {
                return TypePairKey{static_cast<TypeTag>(255), static_cast<TypeTag>(255)};
            }
        };

        // ============================================================================
        // Fast Operation Function Types
        // ============================================================================

        // Function pointer type for binary operations (add, sub, mul, div, mod)
        using BinaryOpFn = var (*)(const var &, const var &);

        // ============================================================================
        // Type-Specific Fast Path Implementations
        // ============================================================================

        namespace detail
        {

            // ---- Addition Fast Paths ----

            inline var add_int_int(const var &a, const var &b)
            {
                return var(overflow::add(a.var_get<int>(), b.var_get<int>()));
            }

            inline var add_ll_ll(const var &a, const var &b)
            {
                return var(overflow::add(a.var_get<long long>(), b.var_get<long long>()));
            }

            inline var add_double_double(const var &a, const var &b)
            {
                return var(a.var_get<double>() + b.var_get<double>());
            }

            inline var add_float_float(const var &a, const var &b)
            {
                return var(a.var_get<float>() + b.var_get<float>());
            }

            inline var add_str_str(const var &a, const var &b)
            {
                return var(a.var_get<std::string>() + b.var_get<std::string>());
            }

            // Cross-type additions with proper promotion
            inline var add_int_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<int>()) + b.var_get<double>());
            }

            inline var add_double_int(const var &a, const var &b)
            {
                return var(a.var_get<double>() + static_cast<double>(b.var_get<int>()));
            }

            inline var add_int_ll(const var &a, const var &b)
            {
                return var(overflow::add(static_cast<long long>(a.var_get<int>()), b.var_get<long long>()));
            }

            inline var add_ll_int(const var &a, const var &b)
            {
                return var(overflow::add(a.var_get<long long>(), static_cast<long long>(b.var_get<int>())));
            }

            // Float cross-type additions
            inline var add_int_float(const var &a, const var &b)
            {
                return var(static_cast<float>(a.var_get<int>()) + b.var_get<float>());
            }

            inline var add_float_int(const var &a, const var &b)
            {
                return var(a.var_get<float>() + static_cast<float>(b.var_get<int>()));
            }

            inline var add_float_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<float>()) + b.var_get<double>());
            }

            inline var add_double_float(const var &a, const var &b)
            {
                return var(a.var_get<double>() + static_cast<double>(b.var_get<float>()));
            }

            inline var add_ll_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<long long>()) + b.var_get<double>());
            }

            inline var add_double_ll(const var &a, const var &b)
            {
                return var(a.var_get<double>() + static_cast<double>(b.var_get<long long>()));
            }

            // Promoted fallback for any other numeric types
            inline var add_promoted(const var &a, const var &b)
            {
                return a.addPromoted(b);
            }

            inline var add_generic(const var &a, const var &b)
            {
                return a + b; // Falls back to full type dispatch
            }

            // ---- Subtraction Fast Paths ----

            inline var sub_int_int(const var &a, const var &b)
            {
                return var(overflow::sub(a.var_get<int>(), b.var_get<int>()));
            }

            inline var sub_ll_ll(const var &a, const var &b)
            {
                return var(overflow::sub(a.var_get<long long>(), b.var_get<long long>()));
            }

            inline var sub_double_double(const var &a, const var &b)
            {
                return var(a.var_get<double>() - b.var_get<double>());
            }

            inline var sub_float_float(const var &a, const var &b)
            {
                return var(a.var_get<float>() - b.var_get<float>());
            }

            // Cross-type subtraction fast paths
            inline var sub_int_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<int>()) - b.var_get<double>());
            }

            inline var sub_double_int(const var &a, const var &b)
            {
                return var(a.var_get<double>() - static_cast<double>(b.var_get<int>()));
            }

            inline var sub_int_ll(const var &a, const var &b)
            {
                return var(overflow::sub(static_cast<long long>(a.var_get<int>()), b.var_get<long long>()));
            }

            inline var sub_ll_int(const var &a, const var &b)
            {
                return var(overflow::sub(a.var_get<long long>(), static_cast<long long>(b.var_get<int>())));
            }

            // Float cross-type subtractions
            inline var sub_int_float(const var &a, const var &b)
            {
                return var(static_cast<float>(a.var_get<int>()) - b.var_get<float>());
            }

            inline var sub_float_int(const var &a, const var &b)
            {
                return var(a.var_get<float>() - static_cast<float>(b.var_get<int>()));
            }

            inline var sub_float_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<float>()) - b.var_get<double>());
            }

            inline var sub_double_float(const var &a, const var &b)
            {
                return var(a.var_get<double>() - static_cast<double>(b.var_get<float>()));
            }

            inline var sub_ll_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<long long>()) - b.var_get<double>());
            }

            inline var sub_double_ll(const var &a, const var &b)
            {
                return var(a.var_get<double>() - static_cast<double>(b.var_get<long long>()));
            }

            // Promoted fallback for other numeric types
            inline var sub_promoted(const var &a, const var &b)
            {
                return a.subPromoted(b);
            }

            inline var sub_generic(const var &a, const var &b)
            {
                return a - b;
            }

            // ---- Multiplication Fast Paths ----

            inline var mul_int_int(const var &a, const var &b)
            {
                return var(overflow::mul(a.var_get<int>(), b.var_get<int>()));
            }

            inline var mul_ll_ll(const var &a, const var &b)
            {
                return var(overflow::mul(a.var_get<long long>(), b.var_get<long long>()));
            }

            inline var mul_double_double(const var &a, const var &b)
            {
                return var(a.var_get<double>() * b.var_get<double>());
            }

            inline var mul_float_float(const var &a, const var &b)
            {
                return var(a.var_get<float>() * b.var_get<float>());
            }

            // Cross-type multiplication fast paths
            inline var mul_int_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<int>()) * b.var_get<double>());
            }

            inline var mul_double_int(const var &a, const var &b)
            {
                return var(a.var_get<double>() * static_cast<double>(b.var_get<int>()));
            }

            inline var mul_int_ll(const var &a, const var &b)
            {
                return var(overflow::mul(static_cast<long long>(a.var_get<int>()), b.var_get<long long>()));
            }

            inline var mul_ll_int(const var &a, const var &b)
            {
                return var(overflow::mul(a.var_get<long long>(), static_cast<long long>(b.var_get<int>())));
            }

            // Float cross-type multiplications
            inline var mul_int_float(const var &a, const var &b)
            {
                return var(static_cast<float>(a.var_get<int>()) * b.var_get<float>());
            }

            inline var mul_float_int(const var &a, const var &b)
            {
                return var(a.var_get<float>() * static_cast<float>(b.var_get<int>()));
            }

            inline var mul_float_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<float>()) * b.var_get<double>());
            }

            inline var mul_double_float(const var &a, const var &b)
            {
                return var(a.var_get<double>() * static_cast<double>(b.var_get<float>()));
            }

            inline var mul_ll_double(const var &a, const var &b)
            {
                return var(static_cast<double>(a.var_get<long long>()) * b.var_get<double>());
            }

            inline var mul_double_ll(const var &a, const var &b)
            {
                return var(a.var_get<double>() * static_cast<double>(b.var_get<long long>()));
            }

            // Promoted fallback for other numeric types
            inline var mul_promoted(const var &a, const var &b)
            {
                return a.mulPromoted(b);
            }

            inline var mul_generic(const var &a, const var &b)
            {
                return a * b;
            }

            // ---- Division Fast Paths ----
            // Note: Division always promotes to at least double for precision

            inline var div_double_double(const var &a, const var &b)
            {
                double divisor = b.var_get<double>();
                if (divisor == 0.0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(a.var_get<double>() / divisor);
            }

            inline var div_int_int(const var &a, const var &b)
            {
                // Integer division promotes to double for safety
                int divisor = b.var_get<int>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<double>(a.var_get<int>()) / divisor);
            }

            inline var div_float_float(const var &a, const var &b)
            {
                float divisor = b.var_get<float>();
                if (divisor == 0.0f)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(a.var_get<float>() / divisor);
            }

            inline var div_ll_ll(const var &a, const var &b)
            {
                long long divisor = b.var_get<long long>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<double>(a.var_get<long long>()) / divisor);
            }

            // Cross-type division fast paths (always return double for precision)
            inline var div_int_double(const var &a, const var &b)
            {
                double divisor = b.var_get<double>();
                if (divisor == 0.0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<double>(a.var_get<int>()) / divisor);
            }

            inline var div_double_int(const var &a, const var &b)
            {
                int divisor = b.var_get<int>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(a.var_get<double>() / static_cast<double>(divisor));
            }

            inline var div_int_float(const var &a, const var &b)
            {
                float divisor = b.var_get<float>();
                if (divisor == 0.0f)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<float>(a.var_get<int>()) / divisor);
            }

            inline var div_float_int(const var &a, const var &b)
            {
                int divisor = b.var_get<int>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(a.var_get<float>() / static_cast<float>(divisor));
            }

            inline var div_float_double(const var &a, const var &b)
            {
                double divisor = b.var_get<double>();
                if (divisor == 0.0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<double>(a.var_get<float>()) / divisor);
            }

            inline var div_double_float(const var &a, const var &b)
            {
                float divisor = b.var_get<float>();
                if (divisor == 0.0f)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(a.var_get<double>() / static_cast<double>(divisor));
            }

            inline var div_ll_double(const var &a, const var &b)
            {
                double divisor = b.var_get<double>();
                if (divisor == 0.0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<double>(a.var_get<long long>()) / divisor);
            }

            inline var div_double_ll(const var &a, const var &b)
            {
                long long divisor = b.var_get<long long>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(a.var_get<double>() / static_cast<double>(divisor));
            }

            inline var div_int_ll(const var &a, const var &b)
            {
                long long divisor = b.var_get<long long>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<double>(a.var_get<int>()) / static_cast<double>(divisor));
            }

            inline var div_ll_int(const var &a, const var &b)
            {
                int divisor = b.var_get<int>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::division();
                }
                return var(static_cast<double>(a.var_get<long long>()) / static_cast<double>(divisor));
            }

            // Promoted fallback for other numeric types
            inline var div_promoted(const var &a, const var &b)
            {
                return a.divPromoted(b);
            }

            inline var div_generic(const var &a, const var &b)
            {
                return a / b;
            }

            // ---- Modulo Fast Paths ----

            inline var mod_int_int(const var &a, const var &b)
            {
                int divisor = b.var_get<int>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::modulo();
                }
                return var(a.var_get<int>() % divisor);
            }

            inline var mod_ll_ll(const var &a, const var &b)
            {
                long long divisor = b.var_get<long long>();
                if (divisor == 0)
                {
                    throw PythonicZeroDivisionError::modulo();
                }
                return var(a.var_get<long long>() % divisor);
            }

            inline var mod_generic(const var &a, const var &b)
            {
                return a % b;
            }

            // ============================================================================
            // Fast Path Lookup Tables
            // ============================================================================

            /**
             * @brief Returns the optimized function pointer for addition, or nullptr if none
             */
            inline BinaryOpFn get_add_fast_path(TypeTag left, TypeTag right) noexcept
            {
                // Same-type optimizations (most common case)
                if (left == right)
                {
                    switch (left)
                    {
                    case TypeTag::INT:
                        return add_int_int;
                    case TypeTag::LONG_LONG:
                        return add_ll_ll;
                    case TypeTag::DOUBLE:
                        return add_double_double;
                    case TypeTag::FLOAT:
                        return add_float_float;
                    case TypeTag::STRING:
                        return add_str_str;
                    default:
                        break;
                    }
                }

                // Cross-type promotions with proper type-specific functions
                // int + long long -> long long
                if (left == TypeTag::INT && right == TypeTag::LONG_LONG)
                    return add_int_ll;
                if (left == TypeTag::LONG_LONG && right == TypeTag::INT)
                    return add_ll_int;

                // int + double -> double
                if (left == TypeTag::INT && right == TypeTag::DOUBLE)
                    return add_int_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::INT)
                    return add_double_int;

                // int + float -> float
                if (left == TypeTag::INT && right == TypeTag::FLOAT)
                    return add_int_float;
                if (left == TypeTag::FLOAT && right == TypeTag::INT)
                    return add_float_int;

                // float + double -> double
                if (left == TypeTag::FLOAT && right == TypeTag::DOUBLE)
                    return add_float_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::FLOAT)
                    return add_double_float;

                // long long + double -> double
                if (left == TypeTag::LONG_LONG && right == TypeTag::DOUBLE)
                    return add_ll_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::LONG_LONG)
                    return add_double_ll;

                // For any other numeric type combination, use the promoted fallback
                // This covers: uint, long, ulong, ulong long, long double, etc.
                int rankA = var::getTypeRank(left);
                int rankB = var::getTypeRank(right);
                if (rankA >= 0 && rankB >= 0)
                {
                    return add_promoted;
                }

                return nullptr; // No fast path for non-numeric types
            }

            inline BinaryOpFn get_sub_fast_path(TypeTag left, TypeTag right) noexcept
            {
                // Same-type optimizations
                if (left == right)
                {
                    switch (left)
                    {
                    case TypeTag::INT:
                        return sub_int_int;
                    case TypeTag::LONG_LONG:
                        return sub_ll_ll;
                    case TypeTag::DOUBLE:
                        return sub_double_double;
                    case TypeTag::FLOAT:
                        return sub_float_float;
                    default:
                        break;
                    }
                }

                // Cross-type promotions
                if (left == TypeTag::INT && right == TypeTag::LONG_LONG)
                    return sub_int_ll;
                if (left == TypeTag::LONG_LONG && right == TypeTag::INT)
                    return sub_ll_int;
                if (left == TypeTag::INT && right == TypeTag::DOUBLE)
                    return sub_int_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::INT)
                    return sub_double_int;
                if (left == TypeTag::INT && right == TypeTag::FLOAT)
                    return sub_int_float;
                if (left == TypeTag::FLOAT && right == TypeTag::INT)
                    return sub_float_int;
                if (left == TypeTag::FLOAT && right == TypeTag::DOUBLE)
                    return sub_float_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::FLOAT)
                    return sub_double_float;
                if (left == TypeTag::LONG_LONG && right == TypeTag::DOUBLE)
                    return sub_ll_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::LONG_LONG)
                    return sub_double_ll;

                // For any other numeric type combination, use the promoted fallback
                int rankA = var::getTypeRank(left);
                int rankB = var::getTypeRank(right);
                if (rankA >= 0 && rankB >= 0)
                {
                    return sub_promoted;
                }

                return nullptr;
            }

            inline BinaryOpFn get_mul_fast_path(TypeTag left, TypeTag right) noexcept
            {
                // Same-type optimizations
                if (left == right)
                {
                    switch (left)
                    {
                    case TypeTag::INT:
                        return mul_int_int;
                    case TypeTag::LONG_LONG:
                        return mul_ll_ll;
                    case TypeTag::DOUBLE:
                        return mul_double_double;
                    case TypeTag::FLOAT:
                        return mul_float_float;
                    default:
                        break;
                    }
                }

                // Cross-type promotions
                if (left == TypeTag::INT && right == TypeTag::LONG_LONG)
                    return mul_int_ll;
                if (left == TypeTag::LONG_LONG && right == TypeTag::INT)
                    return mul_ll_int;
                if (left == TypeTag::INT && right == TypeTag::DOUBLE)
                    return mul_int_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::INT)
                    return mul_double_int;
                if (left == TypeTag::INT && right == TypeTag::FLOAT)
                    return mul_int_float;
                if (left == TypeTag::FLOAT && right == TypeTag::INT)
                    return mul_float_int;
                if (left == TypeTag::FLOAT && right == TypeTag::DOUBLE)
                    return mul_float_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::FLOAT)
                    return mul_double_float;
                if (left == TypeTag::LONG_LONG && right == TypeTag::DOUBLE)
                    return mul_ll_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::LONG_LONG)
                    return mul_double_ll;

                // For any other numeric type combination, use the promoted fallback
                int rankA = var::getTypeRank(left);
                int rankB = var::getTypeRank(right);
                if (rankA >= 0 && rankB >= 0)
                {
                    return mul_promoted;
                }

                return nullptr;
            }

            inline BinaryOpFn get_div_fast_path(TypeTag left, TypeTag right) noexcept
            {
                // Same-type optimizations
                if (left == right)
                {
                    switch (left)
                    {
                    case TypeTag::INT:
                        return div_int_int;
                    case TypeTag::DOUBLE:
                        return div_double_double;
                    case TypeTag::FLOAT:
                        return div_float_float;
                    case TypeTag::LONG_LONG:
                        return div_ll_ll;
                    default:
                        break;
                    }
                }

                // Cross-type promotions
                if (left == TypeTag::INT && right == TypeTag::DOUBLE)
                    return div_int_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::INT)
                    return div_double_int;
                if (left == TypeTag::INT && right == TypeTag::FLOAT)
                    return div_int_float;
                if (left == TypeTag::FLOAT && right == TypeTag::INT)
                    return div_float_int;
                if (left == TypeTag::FLOAT && right == TypeTag::DOUBLE)
                    return div_float_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::FLOAT)
                    return div_double_float;
                if (left == TypeTag::LONG_LONG && right == TypeTag::DOUBLE)
                    return div_ll_double;
                if (left == TypeTag::DOUBLE && right == TypeTag::LONG_LONG)
                    return div_double_ll;
                if (left == TypeTag::INT && right == TypeTag::LONG_LONG)
                    return div_int_ll;
                if (left == TypeTag::LONG_LONG && right == TypeTag::INT)
                    return div_ll_int;

                // For any other numeric type combination, use the promoted fallback
                int rankA = var::getTypeRank(left);
                int rankB = var::getTypeRank(right);
                if (rankA >= 0 && rankB >= 0)
                {
                    return div_promoted;
                }

                return nullptr;
            }

            inline BinaryOpFn get_mod_fast_path(TypeTag left, TypeTag right) noexcept
            {
                if (left == right)
                {
                    switch (left)
                    {
                    case TypeTag::INT:
                        return mod_int_int;
                    case TypeTag::LONG_LONG:
                        return mod_ll_ll;
                    default:
                        break;
                    }
                }
                return nullptr;
            }

        } // namespace detail

        // ============================================================================
        // CachedBinOp - The Main Fast Path Cache Class
        // ============================================================================

        /**
         * @brief A cached binary operation that remembers the last type-pair
         *
         * Template parameter Op should be one of: Add, Sub, Mul, Div, Mod
         *
         * Example usage:
         *   CachedAdd adder;
         *   for (auto& x : list) {
         *       sum = adder(sum, x);
         *   }
         */
        template <typename OpTag>
        class CachedBinOp
        {
        private:
            TypePairKey cached_key_{TypePairKey::invalid()};
            BinaryOpFn cached_fn_{nullptr};

            // Get the appropriate fast-path lookup function based on OpTag
            static BinaryOpFn lookup_fast_path(TypeTag left, TypeTag right) noexcept;

            // Fallback generic operation
            static var generic_op(const var &a, const var &b);

        public:
            CachedBinOp() = default;

            // Non-copyable but movable (to prevent accidental sharing of cache state)
            CachedBinOp(const CachedBinOp &) = delete;
            CachedBinOp &operator=(const CachedBinOp &) = delete;
            CachedBinOp(CachedBinOp &&) = default;
            CachedBinOp &operator=(CachedBinOp &&) = default;

            /**
             * @brief Perform the cached binary operation
             *
             * On first call or type change: looks up optimal function and caches it
             * On subsequent calls with same types: uses cached function directly
             */
            var operator()(const var &a, const var &b)
            {
                TypePairKey current_key{a.getTag(), b.getTag()};

                // Fast path: cache hit
                if (current_key == cached_key_ && cached_fn_ != nullptr) [[likely]]
                {
                    return cached_fn_(a, b);
                }

                // Slow path: cache miss - need to look up
                cached_key_ = current_key;
                cached_fn_ = lookup_fast_path(a.getTag(), b.getTag());

                if (cached_fn_ != nullptr)
                {
                    return cached_fn_(a, b);
                }
                else
                {
                    // No fast path available, use generic (don't cache nullptr)
                    return generic_op(a, b);
                }
            }

            /**
             * @brief Reset the cache (useful if you know types will change)
             */
            void reset() noexcept
            {
                cached_key_ = TypePairKey::invalid();
                cached_fn_ = nullptr;
            }

            /**
             * @brief Check if there's a cached fast path
             */
            bool has_fast_path() const noexcept
            {
                return cached_fn_ != nullptr;
            }
        };

        // ============================================================================
        // Operation Tags and Specializations
        // ============================================================================

        struct AddTag
        {
        };
        struct SubTag
        {
        };
        struct MulTag
        {
        };
        struct DivTag
        {
        };
        struct ModTag
        {
        };

        // --- Addition ---
        template <>
        inline BinaryOpFn CachedBinOp<AddTag>::lookup_fast_path(TypeTag l, TypeTag r) noexcept
        {
            return detail::get_add_fast_path(l, r);
        }

        template <>
        inline var CachedBinOp<AddTag>::generic_op(const var &a, const var &b)
        {
            return a + b;
        }

        // --- Subtraction ---
        template <>
        inline BinaryOpFn CachedBinOp<SubTag>::lookup_fast_path(TypeTag l, TypeTag r) noexcept
        {
            return detail::get_sub_fast_path(l, r);
        }

        template <>
        inline var CachedBinOp<SubTag>::generic_op(const var &a, const var &b)
        {
            return a - b;
        }

        // --- Multiplication ---
        template <>
        inline BinaryOpFn CachedBinOp<MulTag>::lookup_fast_path(TypeTag l, TypeTag r) noexcept
        {
            return detail::get_mul_fast_path(l, r);
        }

        template <>
        inline var CachedBinOp<MulTag>::generic_op(const var &a, const var &b)
        {
            return a * b;
        }

        // --- Division ---
        template <>
        inline BinaryOpFn CachedBinOp<DivTag>::lookup_fast_path(TypeTag l, TypeTag r) noexcept
        {
            return detail::get_div_fast_path(l, r);
        }

        template <>
        inline var CachedBinOp<DivTag>::generic_op(const var &a, const var &b)
        {
            return a / b;
        }

        // --- Modulo ---
        template <>
        inline BinaryOpFn CachedBinOp<ModTag>::lookup_fast_path(TypeTag l, TypeTag r) noexcept
        {
            return detail::get_mod_fast_path(l, r);
        }

        template <>
        inline var CachedBinOp<ModTag>::generic_op(const var &a, const var &b)
        {
            return a % b;
        }

        // ============================================================================
        // Convenient Type Aliases
        // ============================================================================

        using CachedAdd = CachedBinOp<AddTag>;
        using CachedSub = CachedBinOp<SubTag>;
        using CachedMul = CachedBinOp<MulTag>;
        using CachedDiv = CachedBinOp<DivTag>;
        using CachedMod = CachedBinOp<ModTag>;

        // ============================================================================
        // Fast Sum Implementation Using Cached Operations
        // ============================================================================

        /**
         * @brief Optimized sum using cached addition for homogeneous lists
         *
         * Significantly faster than regular sum() for large lists of same type.
         */
        template <typename Iterable>
        var fast_sum(Iterable &&iterable, var initial = var(0))
        {
            CachedAdd adder;
            var result = initial;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result = adder(result, item);
                }
                else
                {
                    result = adder(result, var(item));
                }
            }
            return result;
        }

        /**
         * @brief Optimized product using cached multiplication
         */
        template <typename Iterable>
        var fast_product(Iterable &&iterable, var initial = var(1))
        {
            CachedMul multiplier;
            var result = initial;
            for (auto &&item : iterable)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, var>)
                {
                    result = multiplier(result, item);
                }
                else
                {
                    result = multiplier(result, var(item));
                }
            }
            return result;
        }

        /**
         * @brief Optimized dot product for two lists
         */
        inline var fast_dot(const var &list1, const var &list2)
        {
            if (!list1.is_list() || !list2.is_list())
            {
                throw PythonicTypeError("fast_dot requires two lists");
            }

            const List &l1 = list1.as_list_unchecked();
            const List &l2 = list2.as_list_unchecked();

            if (l1.size() != l2.size())
            {
                throw PythonicValueError("fast_dot requires lists of equal length");
            }

            if (l1.empty())
            {
                return var(0);
            }

            // Use regular var arithmetic which handles type promotion correctly
            var sum = l1[0] * l2[0];
            for (size_t i = 1; i < l1.size(); ++i)
            {
                sum = sum + (l1[i] * l2[i]);
            }

            return sum;
        }

        // ============================================================================
        // Accumulator Pattern for Complex Reductions
        // ============================================================================

        /**
         * @brief Generic cached accumulator for custom reductions
         *
         * Usage:
         *   CachedAccumulator<CachedAdd> acc(initial_value);
         *   for (auto& x : list) {
         *       acc += x;
         *   }
         *   var result = acc.value();
         */
        template <typename CachedOpT>
        class CachedAccumulator
        {
        private:
            var value_;
            CachedOpT op_;

        public:
            explicit CachedAccumulator(var initial = var(0)) : value_(std::move(initial)) {}

            CachedAccumulator &operator+=(const var &v)
            {
                value_ = op_(value_, v);
                return *this;
            }

            const var &value() const noexcept { return value_; }
            var &value() noexcept { return value_; }

            operator var() const { return value_; }
        };

        using FastSumAccumulator = CachedAccumulator<CachedAdd>;
        using FastProductAccumulator = CachedAccumulator<CachedMul>;

    } // namespace fast
} // namespace pythonic
