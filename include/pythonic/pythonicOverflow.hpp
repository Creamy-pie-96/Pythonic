#pragma once

#include <limits>
#include <concepts>
#include <type_traits>
#include "pythonicError.hpp"

namespace pythonic
{
    namespace overflow
    {

        // Use GCC/Clang builtins for overflow detection
        // __builtin_add_overflow(a, b, res) returns true on overflow

        template <typename T>
        concept Arithmetic = std::is_arithmetic_v<T>;

        /**
         * @brief Checked addition. Throws PythonicOverflowError on overflow.
         */
        template <Arithmetic T>
        constexpr T add(T a, T b)
        {
            if constexpr (std::is_integral_v<T>)
            {
                T res;
                if (__builtin_add_overflow(a, b, &res))
                {
                    throw PythonicOverflowError("integer addition overflow");
                }
                return res;
            }
            else
            {
                return a + b; // Floating point handles overflow via INF usually
            }
        }

        /**
         * @brief Checked subtraction. Throws PythonicOverflowError on overflow.
         */
        template <Arithmetic T>
        constexpr T sub(T a, T b)
        {
            if constexpr (std::is_integral_v<T>)
            {
                T res;
                if (__builtin_sub_overflow(a, b, &res))
                {
                    throw PythonicOverflowError("integer subtraction overflow");
                }
                return res;
            }
            else
            {
                return a - b;
            }
        }

        /**
         * @brief Checked multiplication. Throws PythonicOverflowError on overflow.
         */
        template <Arithmetic T>
        constexpr T mul(T a, T b)
        {
            if constexpr (std::is_integral_v<T>)
            {
                T res;
                if (__builtin_mul_overflow(a, b, &res))
                {
                    throw PythonicOverflowError("integer multiplication overflow");
                }
                return res;
            }
            else
            {
                return a * b;
            }
        }

        /**
         * @brief Checked division. Throws PythonicZeroDivisionError on zero divisor.
         * Note: INT_MIN / -1 is also an overflow on 2's complement.
         */
        template <Arithmetic T>
        constexpr T div(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("division by zero");
            }
            if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
            {
                // Check for INT_MIN / -1 overflow (only for signed types)
                if (a == std::numeric_limits<T>::min() && b == static_cast<T>(-1))
                {
                    throw PythonicOverflowError("integer division overflow");
                }
            }
            return a / b;
        }

        /**
         * @brief Checked modulo. Throws PythonicZeroDivisionError on zero divisor.
         */
        template <Arithmetic T>
        constexpr T mod(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer modulo by zero");
            }
            if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
            {
                // Check for INT_MIN % -1 (undefined behavior in C++, though usually 0)
                if (a == std::numeric_limits<T>::min() && b == static_cast<T>(-1))
                {
                    return 0; // Not strictly an overflow, just a special case
                }
            }
            return a % b;
        }

    } // namespace overflow
} // namespace pythonic
