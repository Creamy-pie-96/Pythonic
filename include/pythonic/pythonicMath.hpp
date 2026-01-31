#pragma once

#include <cmath>
#include <random>
#include "pythonicError.hpp"
#include <algorithm>
#include "pythonicVars.hpp"
#include "pythonicPromotion.hpp"
#include "pythonicDispatch.hpp"

namespace pythonic
{
    namespace math
    {
        using namespace pythonic::vars;
        using pythonic::overflow::Overflow;

        // Forward declarations for overflow-aware arithmetic operations
        inline var add(const var &a, const var &b, Overflow policy = Overflow::Throw, bool smallest_fit = false);
        inline var sub(const var &a, const var &b, Overflow policy = Overflow::Throw, bool smallest_fit = false);
        inline var mul(const var &a, const var &b, Overflow policy = Overflow::Throw, bool smallest_fit = false);
        inline var div(const var &a, const var &b, Overflow policy = Overflow::Throw, bool smallest_fit = false);
        inline var mod(const var &a, const var &b, Overflow policy = Overflow::Throw, bool smallest_fit = false);

        // Helper to extract numeric value from var - OPTIMIZED with fast type checks
        // Handles ALL numeric types with fast paths before falling back to toDouble()
        inline double to_numeric(const var &v)
        {
            // Fast paths for all numeric types using is_* methods and unchecked accessors
            // Ordered by expected frequency of use
            if (v.is_int())
                return static_cast<double>(v.as_int_unchecked());
            if (v.is_double())
                return v.as_double_unchecked();
            if (v.is_float())
                return static_cast<double>(v.as_float_unchecked());
            if (v.is_long())
                return static_cast<double>(v.as_long_unchecked());
            if (v.is_long_long())
                return static_cast<double>(v.as_long_long_unchecked());
            if (v.is_long_double())
                return static_cast<double>(v.as_long_double_unchecked());
            if (v.is_uint())
                return static_cast<double>(v.as_uint_unchecked());
            if (v.is_ulong())
                return static_cast<double>(v.as_ulong_unchecked());
            if (v.is_ulong_long())
                return static_cast<double>(v.as_ulong_long_unchecked());
            if (v.is_bool())
                return v.as_bool_unchecked() ? 1.0 : 0.0;
            // This should not be reached for numeric types, but fallback to toDouble()
            return v.toDouble();
        }

        // ============ Basic Math Functions ============
        // OPTIMIZED: Fast paths for common int/double/float cases

        inline var round(const var &v)
        {
            // Integral types are already rounded
            if (v.is_int())
                return v;
            if (v.is_long())
                return v;
            if (v.is_long_long())
                return v;
            if (v.is_uint())
                return v;
            if (v.is_ulong())
                return v;
            if (v.is_ulong_long())
                return v;
            // Floating point types need rounding
            if (v.is_double())
                return var(std::round(v.as_double_unchecked()));
            if (v.is_float())
                return var(std::round(v.as_float_unchecked()));
            if (v.is_long_double())
                return var(std::round(static_cast<double>(v.as_long_double_unchecked())));
            return var(std::round(to_numeric(v)));
        }

        // ============================================================================
        // SMART PROMOTION HELPERS
        // ============================================================================
        // Strategy: Compute in long double, then find smallest container that fits.
        // - If inputs were all integers: try smallest integer container → float → double → long double
        // - If any input was floating point or it's division: try float → double → long double
        // - Signed/Unsigned handling:
        //   * Both unsigned + result >= 0: try uint → ulong → ulong_long → float → ...
        //   * Any signed or result < 0: try int → long → long_long → float → ...
        // This is O(1), no chains, simple and fast!
        // ============================================================================

        // Helper to get long double value from var
        inline long double to_long_double(const var &v)
        {
            if (v.is_long_double())
                return v.as_long_double_unchecked();
            return static_cast<long double>(to_numeric(v));
        }

        // Check if var is an unsigned integer type
        inline bool is_unsigned_type(const var &v)
        {
            return v.is_uint() || v.is_ulong() || v.is_ulong_long();
        }

        // Check if var is a signed integer type
        inline bool is_signed_integer_type(const var &v)
        {
            return v.is_int() || v.is_long() || v.is_long_long();
        }

        // Check if var is any integer type (signed or unsigned)
        inline bool is_integer_type(const var &v)
        {
            return is_signed_integer_type(v) || is_unsigned_type(v);
        }

        inline bool is_floating_type(const var &v)
        {
            return v.is_float() || v.is_double() || v.is_long_double();
        }

        // ============================================================================
        // TYPE RANKING SYSTEM FOR SMALLEST_FIT FEATURE
        // ============================================================================
        // Ranking: bool=0 < uint=1 < int=2 < ulong=3 < long=4 < ulong_long=5 < long_long=6 < float=7 < double=8 < long_double=9
        // Unsigned types have LOWER rank than their signed counterparts.
        // This means: if EITHER input is unsigned, the result prefers unsigned containers.
        // When smallest_fit=false, we won't downgrade below the highest input rank.
        // ============================================================================

        // Get type rank from var (uses var::getTypeRank internally)
        inline int getTypeRankFromVar(const var &v)
        {
            return var::getTypeRank(v.type_tag());
        }

        // Get max rank from two vars
        inline int getMaxRank(const var &a, const var &b)
        {
            return std::max(getTypeRankFromVar(a), getTypeRankFromVar(b));
        }

        // Type rank constants for comparison
        // New ranking: bool=0 < uint=1 < int=2 < ulong=3 < long=4 < ulong_long=5 < long_long=6 < float=7 < double=8 < long_double=9
        constexpr int RANK_BOOL = 0;
        constexpr int RANK_UINT = 1;
        constexpr int RANK_INT = 2;
        constexpr int RANK_ULONG = 3;
        constexpr int RANK_LONG = 4;
        constexpr int RANK_ULONG_LONG = 5;
        constexpr int RANK_LONG_LONG = 6;
        constexpr int RANK_FLOAT = 7;
        constexpr int RANK_DOUBLE = 8;
        constexpr int RANK_LONG_DOUBLE = 9;

        // Forward declaration for fit_floating_result (used by fit_integer_result)
        inline var fit_floating_result(long double result, int min_rank = 0);

        // Find smallest UNSIGNED integer container that fits the result
        // Returns: var with smallest fitting unsigned type, or calls fit_floating_result if overflow
        inline var fit_unsigned_result(long double result, int min_rank = 0)
        {
            // Can only use unsigned for non-negative values
            if (result < 0 || result != std::floor(result))
            {
                // Negative or non-integer - fall to floating point
                return fit_floating_result(result, min_rank);
            }

            // Try uint (rank=1)
            if (min_rank <= RANK_UINT &&
                result <= std::numeric_limits<unsigned int>::max())
            {
                return var(static_cast<unsigned int>(result));
            }
            // Try ulong (rank=3)
            if (min_rank <= RANK_ULONG &&
                result <= std::numeric_limits<unsigned long>::max())
            {
                return var(static_cast<unsigned long>(result));
            }
            // Try ulong_long (rank=5)
            if (min_rank <= RANK_ULONG_LONG &&
                result <= static_cast<long double>(std::numeric_limits<unsigned long long>::max()))
            {
                return var(static_cast<unsigned long long>(result));
            }
            // Fall through to floating point
            return fit_floating_result(result, min_rank);
        }

        // Find smallest SIGNED integer container that fits the result
        // Returns: var with smallest fitting signed type, or calls fit_floating_result if overflow
        inline var fit_signed_result(long double result, int min_rank = 0)
        {
            // Check if it's an integer value
            if (result != std::floor(result))
            {
                // Non-integer - fall to floating point
                return fit_floating_result(result, min_rank);
            }

            // Try int (rank=2)
            if (min_rank <= RANK_INT &&
                result >= std::numeric_limits<int>::min() &&
                result <= std::numeric_limits<int>::max())
            {
                return var(static_cast<int>(result));
            }
            // Try long (rank=4)
            if (min_rank <= RANK_LONG &&
                result >= std::numeric_limits<long>::min() &&
                result <= std::numeric_limits<long>::max())
            {
                return var(static_cast<long>(result));
            }
            // Try long_long (rank=6)
            if (min_rank <= RANK_LONG_LONG &&
                result >= static_cast<long double>(std::numeric_limits<long long>::min()) &&
                result <= static_cast<long double>(std::numeric_limits<long long>::max()))
            {
                return var(static_cast<long long>(result));
            }
            // Fall through to floating point
            return fit_floating_result(result, min_rank);
        }

        // Find smallest container for an integer result
        // both_unsigned: if true, BOTH inputs were unsigned types (result can be unsigned)
        //                if false, at least one input is signed (result must be signed)
        // min_rank: minimum type rank to return (0 = any, RANK_LONG_LONG = skip int, etc.)
        // force_signed: if true, always use signed path (e.g., for subtraction where result could be negative)
        inline var fit_integer_result(long double result, bool both_unsigned, int min_rank = 0, bool force_signed = false)
        {
            // If force_signed or result is negative, use signed containers
            if (force_signed || result < 0)
            {
                return fit_signed_result(result, min_rank);
            }

            // Only if BOTH inputs were unsigned and result is non-negative, use unsigned containers
            if (both_unsigned && result >= 0)
            {
                return fit_unsigned_result(result, min_rank);
            }

            // Default: if ANY input is signed, use signed containers
            return fit_signed_result(result, min_rank);
        }

        // Find smallest floating container that fits
        // min_rank: minimum type rank to return (0 = any, RANK_DOUBLE = skip float, etc.)
        // TODO: When adding support for larger dtypes (e.g., __float128, arbitrary precision),
        // update this function to check for those types before throwing on overflow.
        inline var fit_floating_result(long double result, int min_rank)
        {
            // First check if long double itself overflowed
            if (std::isinf(result) || std::isnan(result))
            {
                throw PythonicOverflowError("Result exceeds long double range (promote policy)");
            }

            // Check float (rank=7), only if min_rank allows it
            if (min_rank <= RANK_FLOAT &&
                result >= -std::numeric_limits<float>::max() &&
                result <= std::numeric_limits<float>::max() &&
                !std::isinf(static_cast<float>(result)))
            {
                return var(static_cast<float>(result));
            }
            // Check double (rank=8), only if min_rank allows it
            if (min_rank <= RANK_DOUBLE &&
                result >= -std::numeric_limits<double>::max() &&
                result <= std::numeric_limits<double>::max() &&
                !std::isinf(static_cast<double>(result)))
            {
                return var(static_cast<double>(result));
            }
            // Use long double (rank=9)
            return var(result);
        }

        // has_floating_input: if any input was floating point
        // both_unsigned: if BOTH inputs were unsigned integer types
        //                (if ANY input is signed, result will be signed)
        // smallest_fit: if true, find absolute smallest container (default behavior)
        //               if false, don't downgrade below the highest input rank
        // min_rank: minimum type rank (only used when smallest_fit=false)
        // force_signed: if true, always use signed containers (for subtraction)
        // TODO: When adding support for larger dtypes, update this function
        // to handle the new type hierarchy.

        // Helper to determine result type for integer power operations
        // The result type is the larger of the two types
        template <typename T, typename U>
        struct PowResultType
        {
            using type = std::conditional_t<(sizeof(T) > sizeof(U)), T, U>;
        };

        // pow: Power operation with configurable overflow policy
        // smallest_fit: only applies to Overflow::Promote policy
        //               if true (default), find absolute smallest container
        //               if false, don't downgrade below the highest input type rank
        inline var pow(const var &base, const var &exponent, Overflow policy = Overflow::Throw,
                       bool smallest_fit = true)
        {
            // ================================================================
            // WRAP POLICY: Fast path - native operations, no overflow checking
            // Returns the same type as the larger operand
            // ================================================================
            if (policy == Overflow::Wrap)
            {
                // int ^ int -> int
                if (base.is_int() && exponent.is_int())
                {
                    int e = exponent.as_int_unchecked();
                    if (e >= 0)
                        return var(static_cast<int>(std::pow(base.as_int_unchecked(), e)));
                    return var(std::pow(static_cast<double>(base.as_int_unchecked()), e));
                }
                // long ^ int or int ^ long -> long
                if ((base.is_long() && exponent.is_int()) || (base.is_int() && exponent.is_long()))
                {
                    long b = base.is_long() ? base.as_long_unchecked() : static_cast<long>(base.as_int_unchecked());
                    long e = exponent.is_long() ? exponent.as_long_unchecked() : static_cast<long>(exponent.as_int_unchecked());
                    if (e >= 0)
                        return var(static_cast<long>(std::pow(static_cast<long double>(b), e)));
                    return var(std::pow(static_cast<double>(b), e));
                }
                // long ^ long -> long
                if (base.is_long() && exponent.is_long())
                {
                    long e = exponent.as_long_unchecked();
                    if (e >= 0)
                        return var(static_cast<long>(std::pow(static_cast<long double>(base.as_long_unchecked()), e)));
                    return var(std::pow(static_cast<double>(base.as_long_unchecked()), e));
                }
                // long long ^ X or X ^ long long -> long long (for integers)
                if ((base.is_long_long() || exponent.is_long_long()) && base.isNumeric() && exponent.isNumeric())
                {
                    if (!base.is_double() && !base.is_float() && !base.is_long_double() &&
                        !exponent.is_double() && !exponent.is_float() && !exponent.is_long_double())
                    {
                        long long b = base.toLongLong();
                        long long e = exponent.toLongLong();
                        if (e >= 0)
                            return var(static_cast<long long>(std::pow(static_cast<long double>(b), e)));
                        return var(std::pow(static_cast<double>(b), e));
                    }
                }
                // float ^ X (where X is not double/long double) -> float
                if (base.is_float() && !exponent.is_double() && !exponent.is_long_double())
                {
                    return var(static_cast<float>(std::pow(base.as_float_unchecked(), to_numeric(exponent))));
                }
                // X ^ float (where X is not double/long double) -> float
                if (exponent.is_float() && !base.is_double() && !base.is_long_double())
                {
                    return var(static_cast<float>(std::pow(to_numeric(base), exponent.as_float_unchecked())));
                }
                // double ^ X or X ^ double -> double
                if (base.is_double() || exponent.is_double())
                {
                    return var(std::pow(to_numeric(base), to_numeric(exponent)));
                }
                // long double ^ X or X ^ long double -> long double
                if (base.is_long_double() || exponent.is_long_double())
                {
                    long double b = base.is_long_double() ? base.as_long_double_unchecked() : static_cast<long double>(to_numeric(base));
                    long double e = exponent.is_long_double() ? exponent.as_long_double_unchecked() : static_cast<long double>(to_numeric(exponent));
                    return var(std::pow(b, e));
                }
                // Fallback
                return var(std::pow(to_numeric(base), to_numeric(exponent)));
            }

            // ================================================================
            // THROW POLICY: Check for overflow, throw if it occurs
            // ================================================================
            if (policy == Overflow::Throw)
            {
                // int ^ int -> int (throw on overflow)
                if (base.is_int() && exponent.is_int())
                {
                    int e = exponent.as_int_unchecked();
                    if (e >= 0)
                    {
                        long double result = std::pow(static_cast<long double>(base.as_int_unchecked()), e);
                        if (result > std::numeric_limits<int>::max() || result < std::numeric_limits<int>::min())
                            throw PythonicOverflowError("integer pow overflow");
                        return var(static_cast<int>(result));
                    }
                    return var(std::pow(static_cast<double>(base.as_int_unchecked()), e));
                }
                // long ^ int or int ^ long -> long (throw on overflow)
                if ((base.is_long() && exponent.is_int()) || (base.is_int() && exponent.is_long()) ||
                    (base.is_long() && exponent.is_long()))
                {
                    long b = base.is_int() ? static_cast<long>(base.as_int_unchecked()) : base.as_long_unchecked();
                    long e = exponent.is_int() ? static_cast<long>(exponent.as_int_unchecked()) : exponent.as_long_unchecked();
                    if (e >= 0)
                    {
                        long double result = std::pow(static_cast<long double>(b), e);
                        if (result > std::numeric_limits<long>::max() || result < std::numeric_limits<long>::min())
                            throw PythonicOverflowError("long pow overflow");
                        return var(static_cast<long>(result));
                    }
                    return var(std::pow(static_cast<double>(b), e));
                }
                // long long ^ X or X ^ long long -> long long (throw on overflow)
                if ((base.is_long_long() || exponent.is_long_long()) && base.isNumeric() && exponent.isNumeric())
                {
                    if (!base.is_double() && !base.is_float() && !base.is_long_double() &&
                        !exponent.is_double() && !exponent.is_float() && !exponent.is_long_double())
                    {
                        long long b = base.toLongLong();
                        long long e = exponent.toLongLong();
                        if (e >= 0)
                        {
                            long double result = std::pow(static_cast<long double>(b), e);
                            if (result > std::numeric_limits<long long>::max() || result < std::numeric_limits<long long>::min())
                                throw PythonicOverflowError("long long pow overflow");
                            return var(static_cast<long long>(result));
                        }
                        return var(std::pow(static_cast<double>(b), e));
                    }
                }
                // float ^ X (not double/long double) -> float (throw on inf)
                if (base.is_float() && !exponent.is_double() && !exponent.is_long_double())
                {
                    float result = std::pow(base.as_float_unchecked(), static_cast<float>(to_numeric(exponent)));
                    if (std::isinf(result))
                        throw PythonicOverflowError("float pow overflow");
                    return var(result);
                }
                // X ^ float (not double/long double) -> float
                if (exponent.is_float() && !base.is_double() && !base.is_long_double())
                {
                    float result = std::pow(static_cast<float>(to_numeric(base)), exponent.as_float_unchecked());
                    if (std::isinf(result))
                        throw PythonicOverflowError("float pow overflow");
                    return var(result);
                }
                // double ^ X or X ^ double -> double (throw on inf)
                if (base.is_double() || exponent.is_double())
                {
                    double result = std::pow(to_numeric(base), to_numeric(exponent));
                    if (std::isinf(result))
                        throw PythonicOverflowError("double pow overflow");
                    return var(result);
                }
                // long double -> long double
                if (base.is_long_double() || exponent.is_long_double())
                {
                    long double b = base.is_long_double() ? base.as_long_double_unchecked() : static_cast<long double>(to_numeric(base));
                    long double e = exponent.is_long_double() ? exponent.as_long_double_unchecked() : static_cast<long double>(to_numeric(exponent));
                    long double result = std::pow(b, e);
                    if (std::isinf(result))
                        throw PythonicOverflowError("long double pow overflow");
                    return var(result);
                }
                // Fallback
                double result = std::pow(to_numeric(base), to_numeric(exponent));
                if (std::isinf(result))
                    throw PythonicOverflowError("pow overflow");
                return var(result);
            }

            // ================================================================
            // PROMOTE POLICY: Compute in long double, fit to smallest container
            // Uses the same smart_promote strategy as add/sub/mul/mod
            // smallest_fit controls whether we can downgrade below input ranks
            // ================================================================
            if (!base.isNumeric() || !exponent.isNumeric())
                throw pythonic::PythonicTypeError("pow() requires numeric types");

            // Check if any input is floating point
            pythonic::promotion::Type t;

            if(is_floating_type(base) || is_floating_type(exponent)) 
                t=pythonic::promotion::Has_float;

            // Check if BOTH inputs are unsigned (only then can result be unsigned)
            // If ANY input is signed, result will be signed
            else if(is_unsigned_type(base) && is_unsigned_type(exponent))
                t = pythonic::promotion::Both_unsigned;
            else 
                t = pythonic::promotion::Signed;

            // Get max rank of inputs (for smallest_fit=false)
            int min_rank = getMaxRank(base, exponent);

            // Compute in long double
            long double result = std::pow(to_long_double(base), to_long_double(exponent));

            // For negative exponent, result is always floating
            if (to_long_double(exponent) < 0)
                return fit_floating_result(result, smallest_fit ? 0 : min_rank);

            // Find smallest container that fits
            return smart_promote(result, t, smallest_fit, min_rank, false);
        }

        // Convenience versions
        inline var pow_throw(const var &base, const var &exponent)
        {
            return pow(base, exponent, Overflow::Throw);
        }

        inline var pow_promote(const var &base, const var &exponent, bool smallest_fit = true)
        {
            return pow(base, exponent, Overflow::Promote, smallest_fit);
        }

        inline var pow_wrap(const var &base, const var &exponent)
        {
            return pow(base, exponent, Overflow::Wrap);
        }

        // Primitive overloads for pow
        template <typename T1, typename T2,
                  typename = std::enable_if_t<std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>>>
        inline var pow(T1 base, T2 exponent, Overflow policy = Overflow::Throw)
        {
            return pow(var(base), var(exponent), policy);
        }

        inline var sqrt(const var &v)
        {
            if (v.is_double())
                return var(std::sqrt(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::sqrt(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::sqrt(static_cast<double>(v.as_int_unchecked())));
            if (v.is_long())
                return var(std::sqrt(static_cast<double>(v.as_long_unchecked())));
            if (v.is_long_long())
                return var(std::sqrt(static_cast<double>(v.as_long_long_unchecked())));
            return var(std::sqrt(to_numeric(v)));
        }

        inline var nthroot(const var &value, const var &n)
        {
            return var(std::pow(to_numeric(value), 1.0 / to_numeric(n)));
        }

        inline var exp(const var &v)
        {
            if (v.is_double())
                return var(std::exp(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::exp(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::exp(static_cast<double>(v.as_int_unchecked())));
            return var(std::exp(to_numeric(v)));
        }

        inline var log(const var &v)
        {
            if (v.is_double())
                return var(std::log(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::log(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::log(static_cast<double>(v.as_int_unchecked())));
            return var(std::log(to_numeric(v)));
        }

        inline var log10(const var &v)
        {
            if (v.is_double())
                return var(std::log10(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::log10(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::log10(static_cast<double>(v.as_int_unchecked())));
            return var(std::log10(to_numeric(v)));
        }

        inline var log2(const var &v)
        {
            if (v.is_double())
                return var(std::log2(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::log2(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::log2(static_cast<double>(v.as_int_unchecked())));
            return var(std::log2(to_numeric(v)));
        }

        // ============ Trigonometric Functions ============

        inline var sin(const var &v)
        {
            if (v.is_double())
                return var(std::sin(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::sin(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::sin(static_cast<double>(v.as_int_unchecked())));
            return var(std::sin(to_numeric(v)));
        }

        inline var cos(const var &v)
        {
            if (v.is_double())
                return var(std::cos(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::cos(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::cos(static_cast<double>(v.as_int_unchecked())));
            return var(std::cos(to_numeric(v)));
        }

        inline var tan(const var &v)
        {
            if (v.is_double())
                return var(std::tan(v.as_double_unchecked()));
            if (v.is_float())
                return var(static_cast<double>(std::tan(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::tan(static_cast<double>(v.as_int_unchecked())));
            return var(std::tan(to_numeric(v)));
        }

        inline var cot(const var &v)
        {
            if (v.is_double())
                return var(1.0 / std::tan(v.as_double_unchecked()));
            if (v.is_float())
                return var(1.0 / std::tan(static_cast<double>(v.as_float_unchecked())));
            if (v.is_int())
                return var(1.0 / std::tan(static_cast<double>(v.as_int_unchecked())));
            return var(1.0 / std::tan(to_numeric(v)));
        }

        inline var sec(const var &v)
        {
            return var(1.0 / std::cos(to_numeric(v)));
        }

        inline var cosec(const var &v)
        {
            return var(1.0 / std::sin(to_numeric(v)));
        }

        // Aliases for cosec
        inline var csc(const var &v) { return cosec(v); }

        // ============ Inverse Trigonometric Functions ============

        inline var asin(const var &v)
        {
            return var(std::asin(to_numeric(v)));
        }

        inline var acos(const var &v)
        {
            return var(std::acos(to_numeric(v)));
        }

        inline var atan(const var &v)
        {
            return var(std::atan(to_numeric(v)));
        }

        inline var atan2(const var &y, const var &x)
        {
            return var(std::atan2(to_numeric(y), to_numeric(x)));
        }

        inline var acot(const var &v)
        {
            return var(std::atan(1.0 / to_numeric(v)));
        }

        inline var asec(const var &v)
        {
            return var(std::acos(1.0 / to_numeric(v)));
        }

        inline var acosec(const var &v)
        {
            return var(std::asin(1.0 / to_numeric(v)));
        }

        // Alias
        inline var acsc(const var &v) { return acosec(v); }

        // ============ Hyperbolic Functions ============

        inline var sinh(const var &v)
        {
            return var(std::sinh(to_numeric(v)));
        }

        inline var cosh(const var &v)
        {
            return var(std::cosh(to_numeric(v)));
        }

        inline var tanh(const var &v)
        {
            return var(std::tanh(to_numeric(v)));
        }

        inline var asinh(const var &v)
        {
            return var(std::asinh(to_numeric(v)));
        }

        inline var acosh(const var &v)
        {
            return var(std::acosh(to_numeric(v)));
        }

        inline var atanh(const var &v)
        {
            return var(std::atanh(to_numeric(v)));
        }

        // ============ Additional Math Functions ============
        // OPTIMIZED: Fast paths for all numeric types

        inline var floor(const var &v)
        {
            // All integral types are already floored
            if (v.is_int())
                return v;
            if (v.is_long())
                return v;
            if (v.is_long_long())
                return v;
            if (v.is_uint())
                return v;
            if (v.is_ulong())
                return v;
            if (v.is_ulong_long())
                return v;
            // Floating point types need flooring
            if (v.is_double())
                return var(std::floor(v.as_double_unchecked()));
            if (v.is_float())
                return var(std::floor(static_cast<double>(v.as_float_unchecked())));
            if (v.is_long_double())
                return var(std::floor(static_cast<double>(v.as_long_double_unchecked())));
            return var(std::floor(to_numeric(v)));
        }

        inline var ceil(const var &v)
        {
            // All integral types are already ceiled
            if (v.is_int())
                return v;
            if (v.is_long())
                return v;
            if (v.is_long_long())
                return v;
            if (v.is_uint())
                return v;
            if (v.is_ulong())
                return v;
            if (v.is_ulong_long())
                return v;
            // Floating point types need ceiling
            if (v.is_double())
                return var(std::ceil(v.as_double_unchecked()));
            if (v.is_float())
                return var(std::ceil(static_cast<double>(v.as_float_unchecked())));
            if (v.is_long_double())
                return var(std::ceil(static_cast<double>(v.as_long_double_unchecked())));
            return var(std::ceil(to_numeric(v)));
        }

        inline var trunc(const var &v)
        {
            // All integral types are already truncated
            if (v.is_int())
                return v;
            if (v.is_long())
                return v;
            if (v.is_long_long())
                return v;
            if (v.is_uint())
                return v;
            if (v.is_ulong())
                return v;
            if (v.is_ulong_long())
                return v;
            // Floating point types need truncation
            if (v.is_double())
                return var(std::trunc(v.as_double_unchecked()));
            if (v.is_float())
                return var(std::trunc(static_cast<double>(v.as_float_unchecked())));
            if (v.is_long_double())
                return var(std::trunc(static_cast<double>(v.as_long_double_unchecked())));
            return var(std::trunc(to_numeric(v)));
        }

        inline var fmod(const var &x, const var &y)
        {
            return var(std::fmod(to_numeric(x), to_numeric(y)));
        }

        inline var copysign(const var &x, const var &y)
        {
            return var(std::copysign(to_numeric(x), to_numeric(y)));
        }

        inline var fabs(const var &v)
        {
            // Fast paths for all numeric types
            if (v.is_double())
                return var(std::fabs(v.as_double_unchecked()));
            if (v.is_float())
                return var(std::fabs(static_cast<double>(v.as_float_unchecked())));
            if (v.is_int())
                return var(std::abs(v.as_int_unchecked()));
            if (v.is_long())
                return var(std::abs(v.as_long_unchecked()));
            if (v.is_long_long())
                return var(std::abs(v.as_long_long_unchecked()));
            if (v.is_long_double())
                return var(std::fabs(static_cast<double>(v.as_long_double_unchecked())));
            // Unsigned types are always positive
            if (v.is_uint())
                return v;
            if (v.is_ulong())
                return v;
            if (v.is_ulong_long())
                return v;
            return var(std::fabs(to_numeric(v)));
        }

        inline var hypot(const var &x, const var &y)
        {
            return var(std::hypot(to_numeric(x), to_numeric(y)));
        }

        // ============ Constants ============

        inline var pi()
        {
            return var(M_PI);
        }

        inline var e()
        {
            return var(M_E);
        }

        // ============ Random Functions ============

        // Random engine (thread_local for thread safety)
        inline std::mt19937 &get_random_engine()
        {
            thread_local std::mt19937 engine(std::random_device{}());
            return engine;
        }

        // Random integer in range [min, max]
        inline var random_int(const var &min_val, const var &max_val)
        {
            int min_i = static_cast<int>(to_numeric(min_val));
            int max_i = static_cast<int>(to_numeric(max_val));
            std::uniform_int_distribution<int> dist(min_i, max_i);
            return var(dist(get_random_engine()));
        }

        // Random float in range [min, max)
        inline var random_float(const var &min_val, const var &max_val)
        {
            double min_d = to_numeric(min_val);
            double max_d = to_numeric(max_val);
            std::uniform_real_distribution<double> dist(min_d, max_d);
            return var(dist(get_random_engine()));
        }

        // Random element from list
        inline var random_choice(const var &lst)
        {
            if (lst.type() != "list")
                throw pythonic::PythonicTypeError("random_choice() requires a list");

            const auto &l = lst.get<List>();
            if (l.empty())
                throw pythonic::PythonicValueError("random_choice() from empty list");

            std::uniform_int_distribution<size_t> dist(0, l.size() - 1);
            return l[dist(get_random_engine())];
        }

        // Random element from set
        inline var random_choice_set(const var &s)
        {
            if (s.type() != "set")
                throw pythonic::PythonicTypeError("random_choice_set() requires a set");

            const auto &set_val = s.get<Set>();
            if (set_val.empty())
                throw pythonic::PythonicValueError("random_choice_set() from empty set");

            std::uniform_int_distribution<size_t> dist(0, set_val.size() - 1);
            size_t idx = dist(get_random_engine());

            auto it = set_val.begin();
            std::advance(it, idx);
            return *it;
        }

        // Fill list with random integers
        inline var fill_random(size_t count, const var &min_val, const var &max_val)
        {
            List result;
            result.reserve(count);
            int min_i = static_cast<int>(to_numeric(min_val));
            int max_i = static_cast<int>(to_numeric(max_val));
            std::uniform_int_distribution<int> dist(min_i, max_i);

            for (size_t i = 0; i < count; ++i)
            {
                result.push_back(var(dist(get_random_engine())));
            }
            return var(result);
        }

        // Fill list with random floats/doubles (uniform distribution)
        inline var fill_randomf(size_t count, const var &min_val, const var &max_val)
        {
            List result;
            result.reserve(count);
            double min_d = to_numeric(min_val);
            double max_d = to_numeric(max_val);
            std::uniform_real_distribution<double> dist(min_d, max_d);

            for (size_t i = 0; i < count; ++i)
            {
                result.push_back(var(dist(get_random_engine())));
            }
            return var(result);
        }

        // Fill list with random floats from normal/Gaussian distribution
        inline var fill_randomn(size_t count, const var &mean, const var &stddev)
        {
            List result;
            result.reserve(count);
            double mean_d = to_numeric(mean);
            double stddev_d = to_numeric(stddev);
            std::normal_distribution<double> dist(mean_d, stddev_d);

            for (size_t i = 0; i < count; ++i)
            {
                result.push_back(var(dist(get_random_engine())));
            }
            return var(result);
        }

        // Fill set with random integers (unique values)
        inline var fill_random_set(size_t count, const var &min_val, const var &max_val)
        {
            Set result;
            int min_i = static_cast<int>(to_numeric(min_val));
            int max_i = static_cast<int>(to_numeric(max_val));

            if (max_i - min_i + 1 < static_cast<int>(count))
            {
                throw pythonic::PythonicValueError("fill_random_set(): range too small for unique count");
            }

            std::uniform_int_distribution<int> dist(min_i, max_i);

            while (result.size() < count)
            {
                result.insert(var(dist(get_random_engine())));
            }
            return var(result);
        }

        // Fill set with random floats (uniform distribution, practically unique)
        inline var fill_randomf_set(size_t count, const var &min_val, const var &max_val)
        {
            Set result;
            double min_d = to_numeric(min_val);
            double max_d = to_numeric(max_val);
            std::uniform_real_distribution<double> dist(min_d, max_d);

            while (result.size() < count)
            {
                result.insert(var(dist(get_random_engine())));
            }
            return var(result);
        }

        // Fill set with random floats from normal/Gaussian distribution
        inline var fill_randomn_set(size_t count, const var &mean, const var &stddev)
        {
            Set result;
            double mean_d = to_numeric(mean);
            double stddev_d = to_numeric(stddev);
            std::normal_distribution<double> dist(mean_d, stddev_d);

            while (result.size() < count)
            {
                result.insert(var(dist(get_random_engine())));
            }
            return var(result);
        }

        // ============ Product Function (for lists and sets) ============

        inline var product(const var &iterable, const var &start = var(1), Overflow policy = Overflow::Throw)
        {
            var result = start;

            if (iterable.type() == "list")
            {
                const auto &l = iterable.get<List>();
                for (const auto &item : l)
                {
                    result = mul(result, item, policy);
                }
            }
            else if (iterable.type() == "set")
            {
                const auto &s = iterable.get<Set>();
                for (const auto &item : s)
                {
                    result = mul(result, item, policy);
                }
            }
            else
            {
                throw pythonic::PythonicTypeError("product() requires a list or set");
            }

            return result;
        }

        // Convenience versions
        inline var product_throw(const var &iterable, const var &start = var(1))
        {
            return product(iterable, start, Overflow::Throw);
        }

        inline var product_promote(const var &iterable, const var &start = var(1))
        {
            return product(iterable, start, Overflow::Promote);
        }

        inline var product_wrap(const var &iterable, const var &start = var(1))
        {
            return product(iterable, start, Overflow::Wrap);
        }

        // ============ Degree/Radian Conversion ============

        inline var radians(const var &degrees)
        {
            return var(to_numeric(degrees) * M_PI / 180.0);
        }

        inline var degrees(const var &radians_val)
        {
            return var(to_numeric(radians_val) * 180.0 / M_PI);
        }

        // ============ Advanced Functions ============

        inline var gcd(const var &a, const var &b)
        {
            return var(std::gcd(static_cast<long long>(to_numeric(a)),
                                static_cast<long long>(to_numeric(b))));
        }

        inline var lcm(const var &a, const var &b, Overflow policy = Overflow::Throw)
        {
            // Extract values directly to preserve precision (to_numeric loses precision for large long long)
            long long av, bv;
            if (a.is_int())
                av = a.as_int_unchecked();
            else if (a.is_long())
                av = a.as_long_unchecked();
            else if (a.is_long_long())
                av = a.as_long_long_unchecked();
            else
                av = static_cast<long long>(to_numeric(a));

            if (b.is_int())
                bv = b.as_int_unchecked();
            else if (b.is_long())
                bv = b.as_long_unchecked();
            else if (b.is_long_long())
                bv = b.as_long_long_unchecked();
            else
                bv = static_cast<long long>(to_numeric(b));

            long long g = std::gcd(av, bv);
            if (g == 0)
                return var(0LL);

            // lcm = |a * b| / gcd(a, b)
            // To avoid overflow, compute (a / gcd) * b
            long long a_div_g = av / g;
            switch (policy)
            {
            case Overflow::Throw:
                return mul(var(a_div_g), var(bv), Overflow::Throw);
            case Overflow::Promote:
                return mul(var(a_div_g), var(bv), Overflow::Promote); // Uses smart_promote
            case Overflow::Wrap:
                return mul(var(a_div_g), var(bv), Overflow::Wrap);
            default:
                return mul(var(a_div_g), var(bv), Overflow::Throw);
            }
        }

        // Convenience versions
        inline var lcm_throw(const var &a, const var &b)
        {
            return lcm(a, b, Overflow::Throw);
        }

        inline var lcm_promote(const var &a, const var &b)
        {
            return lcm(a, b, Overflow::Promote);
        }

        inline var lcm_wrap(const var &a, const var &b)
        {
            return lcm(a, b, Overflow::Wrap);
        }

        inline var factorial(const var &n, Overflow policy = Overflow::Throw)
        {
            int num = static_cast<int>(to_numeric(n));
            if (num < 0)
                throw pythonic::PythonicValueError("factorial() not defined for negative values");

            switch (policy)
            {
            case Overflow::Throw:
            {
                long long result = 1;
                for (int i = 2; i <= num; ++i)
                {
                    result = pythonic::overflow::mul_throw(result, static_cast<long long>(i));
                }
                return var(result);
            }
            case Overflow::Promote:
            {
                // Compute in long double (widest type)
                long double result = 1.0L;
                for (int i = 2; i <= num; ++i)
                {
                    result = result * static_cast<long double>(i);
                }
                // All inputs are integers (factorial is always positive), use signed containers
                return smart_promote(result, pythonic::promotion::Signed , true, 0, false); // no floating, not both unsigned, smallest_fit=true
            }
            case Overflow::Wrap:
            {
                long long result = 1;
                for (int i = 2; i <= num; ++i)
                {
                    result = pythonic::overflow::mul_wrap(result, static_cast<long long>(i));
                }
                return var(result);
            }
            default:
            {
                long long result = 1;
                for (int i = 2; i <= num; ++i)
                {
                    result = pythonic::overflow::mul_throw(result, static_cast<long long>(i));
                }
                return var(result);
            }
            }
        }

        // ============ Checked Arithmetic Operations ============
        // Perform arithmetic with overflow detection and configurable overflow policy
        // Uses the new pythonic::overflow::Overflow enum:
        //   - Overflow::Throw   (default): Throw PythonicOverflowError on overflow
        //   - Overflow::Promote : Auto-promote to larger type on overflow
        //   - Overflow::Wrap    : Allow wrapping (C++ default behavior)

        using pythonic::overflow::Overflow;

        // ============================================================================
        // Helper: Determine result type for mixed type operations
        // ============================================================================
        // Rules:
        // - Same types: use that type
        // - Mixed integers: use the larger type
        // - Integer + float: use double (for precision)
        // - Any + double: use double
        // - Any + long double: use long double
        // ============================================================================

        // ============================================================================
        // ADDITION Operations
        // ============================================================================

        // Main add function with policy selection
        inline var add(const var &a, const var &b, Overflow policy, bool smallest_fit)
        {
            return pythonic::dispatch::get_add_func(a.type_tag(), b.type_tag())(a, b, policy, smallest_fit);
        }

        // Overloads for var + primitive and primitive + var
        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var add(const var &a, T b, Overflow policy, bool smallest_fit)
        {
            return add(a, var(b), policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var add(T a, const var &b, Overflow policy, bool smallest_fit)
        {
            return add(var(a), b, policy, smallest_fit);
        }

        // ============================================================================
        // SUBTRACTION Operations
        // ============================================================================

        inline var sub(const var &a, const var &b, Overflow policy, bool smallest_fit)
        {
            return pythonic::dispatch::get_sub_func(a.type_tag(), b.type_tag())(a, b, policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var sub(const var &a, T b, Overflow policy, bool smallest_fit)
        {
            return sub(a, var(b), policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var sub(T a, const var &b, Overflow policy, bool smallest_fit)
        {
            return sub(var(a), b, policy, smallest_fit);
        }

        // ============================================================================
        // MULTIPLICATION Operations
        // ============================================================================

        inline var mul(const var &a, const var &b, Overflow policy, bool smallest_fit)
        {
            return pythonic::dispatch::get_mul_func(a.type_tag(), b.type_tag())(a, b, policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var mul(const var &a, T b, Overflow policy, bool smallest_fit)
        {
            return mul(a, var(b), policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var mul(T a, const var &b, Overflow policy, bool smallest_fit)
        {
            return mul(var(a), b, policy, smallest_fit);
        }

        // ============================================================================
        // DIVISION Operations
        // ============================================================================

        // Main div function with policy selection
        // smallest_fit: only applies to Overflow::Promote policy
        inline var div(const var &a, const var &b, Overflow policy, bool smallest_fit)
        {
            return pythonic::dispatch::get_div_func(a.type_tag(), b.type_tag())(a, b, policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var div(const var &a, T b, Overflow policy, bool smallest_fit)
        {
            return div(a, var(b), policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var div(T a, const var &b, Overflow policy, bool smallest_fit)
        {
            return div(var(a), b, policy, smallest_fit);
        }

        // ============================================================================
        // MODULO Operations
        // ============================================================================

        inline var mod(const var &a, const var &b, Overflow policy, bool smallest_fit)
        {
            return pythonic::dispatch::get_mod_func(a.type_tag(), b.type_tag())(a, b, policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var mod(const var &a, T b, Overflow policy, bool smallest_fit)
        {
            return mod(a, var(b), policy, smallest_fit);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline var mod(T a, const var &b, Overflow policy, bool smallest_fit)
        {
            return mod(var(a), b, policy, smallest_fit);
        }

    } // namespace math
} // namespace pythonic
