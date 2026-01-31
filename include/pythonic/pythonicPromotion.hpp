#pragma once

#include <cmath>
#include <limits>
#include <algorithm>
#include "pythonicError.hpp"
#include "pythonic/pythonicVars.hpp"

namespace pythonic
{
    namespace promotion
    {
        using pythonic::vars::var;

        enum Type : uint8_t
        {
            Has_float,
            Both_unsigned,
            Others
        };
        // Type rank constants
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
            if (min_rank <= RANK_UINT && result <= std::numeric_limits<unsigned int>::max())
            {
                return var(static_cast<unsigned int>(result));
            }
            // Try ulong (rank=3)
            if (min_rank <= RANK_ULONG && result <= std::numeric_limits<unsigned long>::max())
            {
                return var(static_cast<unsigned long>(result));
            }
            // Try ulong_long (rank=5)
            if (min_rank <= RANK_ULONG_LONG && result <= static_cast<long double>(std::numeric_limits<unsigned long long>::max()))
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
            if (min_rank <= RANK_INT && result >= std::numeric_limits<int>::min() && result <= std::numeric_limits<int>::max())
            {
                return var(static_cast<int>(result));
            }
            // Try long (rank=4)
            if (min_rank <= RANK_LONG && result >= std::numeric_limits<long>::min() && result <= std::numeric_limits<long>::max())
            {
                return var(static_cast<long>(result));
            }
            // Try long_long (rank=6)
            if (min_rank <= RANK_LONG_LONG && result >= static_cast<long double>(std::numeric_limits<long long>::min()) && result <= static_cast<long double>(std::numeric_limits<long long>::max()))
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
        inline var fit_integer_result(long double result, Type type, int min_rank = 0, bool force_signed = false)
        {
            // Implement three distinct search strategies depending on `type`:
            // - Has_float: only search floating containers (float,double,long double)
            // - Both_unsigned: search unsigned integer containers (uint, ulong, ull), then fall back to floating
            // - Others: search signed integer containers (int, long, long long), then fall back to floating

            if (type == Has_float)
            {
                // Do not consider integer containers when inputs included a float.
                int float_min_rank = std::max(min_rank, RANK_FLOAT);
                return fit_floating_result(result, float_min_rank);
            }

            // BOTH_UNSIGNED: only consider unsigned integer containers (if non-negative and integer), otherwise floating
            if (type == Both_unsigned)
            {
                if (result >= 0 && result == std::floor(result))
                {
                    return fit_unsigned_result(result, min_rank);
                }
                // Non-integer or negative result: fall back to floating
                return fit_floating_result(result, min_rank);
            }

            // OTHERS: prefer signed integer containers; if non-integer or doesn't fit, fall to floating
            if (force_signed || result < 0)
            {
                return fit_signed_result(result, min_rank);
            }

            // Otherwise try signed integer first (non-negative integer)
            if (result == std::floor(result))
            {
                return fit_signed_result(result, min_rank);
            }

            // Non-integer: fall back to floating
            return fit_floating_result(result, min_rank);
        }

        // Find smallest floating container that fits
        // min_rank: minimum type rank to return (0 = any, RANK_DOUBLE = skip float, etc.)
        inline var fit_floating_result(long double result, int min_rank)
        {
            // First check if long double itself overflowed
            if (std::isinf(result) || std::isnan(result))
            {
                throw PythonicOverflowError("Result exceeds long double range (promote policy)");
            }

            // Check float (rank=7), only if min_rank allows it and round trip check for precision check
            if (min_rank <= RANK_FLOAT && result >= -std::numeric_limits<float>::max() && result <= std::numeric_limits<float>::max() && !std::isinf(static_cast<float>(result)) && static_cast<long double>(static_cast<float>(result)) == result )
            {
                return var(static_cast<float>(result));
            }
            // Check double (rank=8), only if min_rank allows it and round trip check for precision check
            if (min_rank <= RANK_DOUBLE && result >= -std::numeric_limits<double>::max() && result <= std::numeric_limits<double>::max() && !std::isinf(static_cast<double>(result)) && static_cast<long double>(static_cast<double>(result)) == result)
            {
                return var(static_cast<double>(result));
            }
            // Use long double (rank=9)
            return var(result);
        }

        // Smart promotion: compute in long double, fit to smallest container
        // type: one of `Type` telling whether inputs had float, both unsigned, or others
        // smallest_fit: if true, find absolute smallest container (default behavior)
        //               if false, don't downgrade below the highest input rank
        // min_rank: minimum type rank (only used when smallest_fit=false)
        // force_signed: if true, always use signed containers (for subtraction)
        inline var smart_promote(long double result, Type type,
                                 bool smallest_fit = true, int min_rank = 0, bool force_signed = false)
        {
            int effective_min_rank = smallest_fit ? 1 : min_rank; //never promote anything to bool

            if (type == Has_float)
            {
                // Ensure when inputs include a float we do not return anything
                // smaller than `float`.
                int needed_min_rank = std::max(effective_min_rank, RANK_FLOAT);
                return fit_floating_result(result, needed_min_rank);
            }
            else if (type == Both_unsigned)
            {
                return fit_integer_result(result, Both_unsigned, effective_min_rank, force_signed);
            }
            else // Others
            {
                return fit_integer_result(result, Others, effective_min_rank, force_signed);
            }
        }

    } // namespace promotion
} // namespace pythonic
