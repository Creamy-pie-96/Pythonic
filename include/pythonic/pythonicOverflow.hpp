#pragma once

#include <limits>
#include <concepts>
#include <type_traits>
#include <cmath>
#include <cstdint>
#include "pythonicError.hpp"

namespace pythonic
{
    namespace overflow
    {
        // ============================================================================
        // Overflow Policy Enum
        // ============================================================================
        // Users can specify how arithmetic operations handle overflow:
        //   - Throw:   Throw PythonicOverflowError on overflow (default, safe)
        //   - Promote: Auto-promote to larger type when overflow would occur
        //   - Wrap:    Allow wrapping (undefined behavior for signed, defined for unsigned)
        // ============================================================================

        enum class Overflow : uint8_t
        {
            Throw = 0,       // Throw on overflow (default)
            Promote = 1,     // Auto-promote to larger type
            Wrap = 2,        // Allow wrapping (C++ default behavior)
            None_of_them = 3 // raw c++ op
        };

        // Concept for arithmetic types
        template <typename T>
        concept Arithmetic = std::is_arithmetic_v<T>;

        // Concept for integral types
        template <typename T>
        concept Integral = std::is_integral_v<T>;

        // Concept for signed integral types
        template <typename T>
        concept SignedIntegral = std::is_integral_v<T> && std::is_signed_v<T>;

        // Concept for unsigned integral types
        template <typename T>
        concept UnsignedIntegral = std::is_integral_v<T> && std::is_unsigned_v<T>;

        // Concept for floating point types
        template <typename T>
        concept FloatingPoint = std::is_floating_point_v<T>;

        // ============================================================================
        // Type Promotion Traits
        // ============================================================================
        // Defines promotion chain:
        //   Signed:   int -> long -> long long -> double -> long double
        //   Unsigned: unsigned int -> unsigned long -> unsigned long long -> double
        //   Float:    float -> double -> long double
        // ============================================================================

        template <typename T>
        struct NextWiderType
        {
            using type = T;
            static constexpr bool can_promote = false;
        };

        // Signed integer promotion chain: int -> long -> long long -> double -> long double
        template <>
        struct NextWiderType<int>
        {
            using type = long;
            static constexpr bool can_promote = true;
        };
        template <>
        struct NextWiderType<long>
        {
            using type = long long;
            static constexpr bool can_promote = true;
        };
        template <>
        struct NextWiderType<long long>
        {
            using type = double;
            static constexpr bool can_promote = true;
        };

        // Unsigned integer promotion chain: unsigned int -> unsigned long -> unsigned long long -> double
        template <>
        struct NextWiderType<unsigned int>
        {
            using type = unsigned long;
            static constexpr bool can_promote = true;
        };
        template <>
        struct NextWiderType<unsigned long>
        {
            using type = unsigned long long;
            static constexpr bool can_promote = true;
        };
        template <>
        struct NextWiderType<unsigned long long>
        {
            using type = double;
            static constexpr bool can_promote = true;
        };

        // Floating point promotion chain: float -> double -> long double
        template <>
        struct NextWiderType<float>
        {
            using type = double;
            static constexpr bool can_promote = true;
        };
        template <>
        struct NextWiderType<double>
        {
            using type = long double;
            static constexpr bool can_promote = true;
        };

        // ============================================================================
        // Overflow Detection Helpers
        // Check if an operation would overflow without performing it
        // NOTE: Template functions are separated by compiler to avoid MSVC parsing issues
        // ============================================================================

#ifdef _MSC_VER
        template <Integral T>
        constexpr bool would_add_overflow(T a, T b) noexcept
        {
            T res = a + b;
            if constexpr (std::is_signed_v<T>)
            {
                return ((a > 0 && b > 0 && res < 0) || (a < 0 && b < 0 && res > 0));
            }
            else
            {
                return res < a;
            }
        }
#else
        template <Integral T>
        constexpr bool would_add_overflow(T a, T b) noexcept
        {
            T res;
            return __builtin_add_overflow(a, b, &res);
        }
#endif

#ifdef _MSC_VER
        template <Integral T>
        constexpr bool would_sub_overflow(T a, T b) noexcept
        {
            T res = a - b;
            if constexpr (std::is_signed_v<T>)
            {
                return ((a > 0 && b < 0 && res < 0) || (a < 0 && b > 0 && res > 0));
            }
            else
            {
                return res > a;
            }
        }
#else
        template <Integral T>
        constexpr bool would_sub_overflow(T a, T b) noexcept
        {
            T res;
            return __builtin_sub_overflow(a, b, &res);
        }
#endif

#ifdef _MSC_VER
        template <Integral T>
        constexpr bool would_mul_overflow(T a, T b) noexcept
        {
            T res = a * b;
            if constexpr (std::is_signed_v<T>)
            {
                bool same_sign = (a >= 0) == (b >= 0);
                return (same_sign && res < 0) || (!same_sign && res > 0);
            }
            else
            {
                return (a != 0 && b > std::numeric_limits<T>::max() / a);
            }
        }
#else
        template <Integral T>
        constexpr bool would_mul_overflow(T a, T b) noexcept
        {
            T res;
            return __builtin_mul_overflow(a, b, &res);
        }
#endif

        // Floating point doesn't have traditional overflow, but we can check for infinity
        template <FloatingPoint T>
        constexpr bool would_add_overflow(T a, T b) noexcept
        {
            return std::isinf(a + b);
        }

        template <FloatingPoint T>
        constexpr bool would_sub_overflow(T a, T b) noexcept
        {
            return std::isinf(a - b);
        }

        template <FloatingPoint T>
        constexpr bool would_mul_overflow(T a, T b) noexcept
        {
            return std::isinf(a * b);
        }

        // ============================================================================
        // THROW Operations - throw on overflow
        // These are the safest and default operations
        // Return type is always T
        // ============================================================================

#ifdef _MSC_VER
        template <Integral T>
        T add_throw(T a, T b)
        {
            T res = a + b;
            bool overflow;
            if constexpr (std::is_signed_v<T>)
            {
                overflow = ((a > 0 && b > 0 && res < 0) || (a < 0 && b < 0 && res > 0));
            }
            else
            {
                overflow = res < a;
            }
            if (overflow)
            {
                throw PythonicOverflowError("integer addition overflow");
            }
            return res;
        }
#else
        template <Integral T>
        T add_throw(T a, T b)
        {
            T res;
            if (__builtin_add_overflow(a, b, &res))
            {
                throw PythonicOverflowError("integer addition overflow");
            }
            return res;
        }
#endif

        template <FloatingPoint T>
        T add_throw(T a, T b)
        {
            T res = a + b;
            if (std::isinf(res))
            {
                throw PythonicOverflowError("floating point addition overflow");
            }
            return res;
        }

#ifdef _MSC_VER
        template <Integral T>
        T sub_throw(T a, T b)
        {
            T res = a - b;
            bool overflow;
            if constexpr (std::is_signed_v<T>)
            {
                overflow = ((a > 0 && b < 0 && res < 0) || (a < 0 && b > 0 && res > 0));
            }
            else
            {
                overflow = res > a;
            }
            if (overflow)
            {
                throw PythonicOverflowError("integer subtraction overflow");
            }
            return res;
        }
#else
        template <Integral T>
        T sub_throw(T a, T b)
        {
            T res;
            if (__builtin_sub_overflow(a, b, &res))
            {
                throw PythonicOverflowError("integer subtraction overflow");
            }
            return res;
        }
#endif

        template <FloatingPoint T>
        T sub_throw(T a, T b)
        {
            T res = a - b;
            if (std::isinf(res))
            {
                throw PythonicOverflowError("floating point subtraction overflow");
            }
            return res;
        }

#ifdef _MSC_VER
        template <Integral T>
        T mul_throw(T a, T b)
        {
            T res = a * b;
            bool overflow;
            if constexpr (std::is_signed_v<T>)
            {
                bool same_sign = (a >= 0) == (b >= 0);
                overflow = (same_sign && res < 0) || (!same_sign && res > 0);
            }
            else
            {
                overflow = (a != 0 && b > std::numeric_limits<T>::max() / a);
            }
            if (overflow)
            {
                throw PythonicOverflowError("integer multiplication overflow");
            }
            return res;
        }
#else
        template <Integral T>
        T mul_throw(T a, T b)
        {
            T res;
            if (__builtin_mul_overflow(a, b, &res))
            {
                throw PythonicOverflowError("integer multiplication overflow");
            }
            return res;
        }
#endif

        template <FloatingPoint T>
        T mul_throw(T a, T b)
        {
            T res = a * b;
            if (std::isinf(res))
            {
                throw PythonicOverflowError("floating point multiplication overflow");
            }
            return res;
        }

        template <Integral T>
        double div_throw(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer division by zero");
            }
            // Check for INT_MIN / -1 overflow (only for signed types)
            if constexpr (std::is_signed_v<T>)
            {
                if (a == std::numeric_limits<T>::min() && b == T(-1))
                {
                    throw PythonicOverflowError("integer division overflow");
                }
            }
            return static_cast<double>(a) / static_cast<double>(b);
        }

        template <FloatingPoint T>
        T div_throw(T a, T b)
        {
            if (b == T(0))
            {
                throw PythonicZeroDivisionError("float division by zero");
            }
            T res = a / b;
            if (std::isinf(res))
            {
                throw PythonicOverflowError("floating point division overflow");
            }
            return res;
        }

        template <Integral T>
        T mod_throw(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer modulo by zero");
            }
            return a % b;
        }

        // ============================================================================
        // WRAP Operations - allow wrapping/UB
        // Return type is always T
        // ============================================================================

        template <Integral T>
        T add_wrap(T a, T b)
        {
            if constexpr (std::is_signed_v<T>)
            {
                // Use unsigned for well-defined wrapping, then cast back
                using U = std::make_unsigned_t<T>;
                return static_cast<T>(static_cast<U>(a) + static_cast<U>(b));
            }
            else
            {
                return a + b; // Unsigned wrapping is well-defined
            }
        }

        template <FloatingPoint T>
        T add_wrap(T a, T b)
        {
            return a + b;
        }

        template <Integral T>
        T sub_wrap(T a, T b)
        {
            if constexpr (std::is_signed_v<T>)
            {
                using U = std::make_unsigned_t<T>;
                return static_cast<T>(static_cast<U>(a) - static_cast<U>(b));
            }
            else
            {
                return a - b;
            }
        }

        template <FloatingPoint T>
        T sub_wrap(T a, T b)
        {
            return a - b;
        }

        template <Integral T>
        T mul_wrap(T a, T b)
        {
            if constexpr (std::is_signed_v<T>)
            {
                using U = std::make_unsigned_t<T>;
                return static_cast<T>(static_cast<U>(a) * static_cast<U>(b));
            }
            else
            {
                return a * b;
            }
        }

        template <FloatingPoint T>
        T mul_wrap(T a, T b)
        {
            return a * b;
        }

        template <Integral T>
        double div_wrap(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer division by zero");
            }
            // For wrap mode, INT_MIN / -1 wraps to INT_MIN
            if constexpr (std::is_signed_v<T>)
            {
                if (a == std::numeric_limits<T>::min() && b == T(-1))
                {
                    return static_cast<double>(a); // Wraps to INT_MIN
                }
            }
            return static_cast<double>(a) / static_cast<double>(b);
        }

        template <FloatingPoint T>
        T div_wrap(T a, T b)
        {
            if (b == T(0))
            {
                throw PythonicZeroDivisionError("float division by zero");
            }
            return a / b;
        }

        template <Integral T>
        T mod_wrap(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer modulo by zero");
            }
            return a % b;
        }

        // ============================================================================
        // PROMOTE Operations - promote to wider type on overflow
        // Return type is the NEXT WIDER type (for int -> long long, for long long -> double)
        // These are designed to be used with var which can hold any type
        // ============================================================================

        // For int: returns long long on success, or promotes further on overflow
        template <SignedIntegral T>
            requires(sizeof(T) <= sizeof(int))
        long long add_promote(T a, T b)
        {
            int res;
            if (__builtin_add_overflow(static_cast<int>(a), static_cast<int>(b), &res))
            {
                // Overflow in int, use long long
                return static_cast<long long>(a) + static_cast<long long>(b);
            }
            return static_cast<long long>(res);
        }

        // For long/long long: returns double on overflow (widest type)
        template <SignedIntegral T>
            requires(sizeof(T) > sizeof(int))
        double add_promote(T a, T b)
        {
            long long res;
            if (__builtin_add_overflow(static_cast<long long>(a), static_cast<long long>(b), &res))
            {
                // Overflow in long long, use double
                return static_cast<double>(a) + static_cast<double>(b);
            }
            return static_cast<double>(res);
        }

        // For unsigned int: returns unsigned long long
        template <UnsignedIntegral T>
            requires(sizeof(T) <= sizeof(unsigned int))
        unsigned long long add_promote(T a, T b)
        {
            return static_cast<unsigned long long>(a) + static_cast<unsigned long long>(b);
        }

        // For unsigned long long: returns double
        template <UnsignedIntegral T>
            requires(sizeof(T) > sizeof(unsigned int))
        double add_promote(T a, T b)
        {
            // Check for overflow
            if (a > std::numeric_limits<unsigned long long>::max() - b)
            {
                return static_cast<double>(a) + static_cast<double>(b);
            }
            return static_cast<double>(a + b);
        }

        // For float: returns double
        inline double add_promote(float a, float b)
        {
            return static_cast<double>(a) + static_cast<double>(b);
        }

        // For double: returns long double
        inline long double add_promote(double a, double b)
        {
            return static_cast<long double>(a) + static_cast<long double>(b);
        }

        // For long double: no further promotion, just compute
        inline long double add_promote(long double a, long double b)
        {
            return a + b;
        }

        // SUB_PROMOTE
        template <SignedIntegral T>
            requires(sizeof(T) <= sizeof(int))
        long long sub_promote(T a, T b)
        {
            int res;
            if (__builtin_sub_overflow(static_cast<int>(a), static_cast<int>(b), &res))
            {
                return static_cast<long long>(a) - static_cast<long long>(b);
            }
            return static_cast<long long>(res);
        }

        template <SignedIntegral T>
            requires(sizeof(T) > sizeof(int))
        double sub_promote(T a, T b)
        {
            long long res;
            if (__builtin_sub_overflow(static_cast<long long>(a), static_cast<long long>(b), &res))
            {
                return static_cast<double>(a) - static_cast<double>(b);
            }
            return static_cast<double>(res);
        }

        template <UnsignedIntegral T>
            requires(sizeof(T) <= sizeof(unsigned int))
        unsigned long long sub_promote(T a, T b)
        {
            return static_cast<unsigned long long>(a) - static_cast<unsigned long long>(b);
        }

        template <UnsignedIntegral T>
            requires(sizeof(T) > sizeof(unsigned int))
        double sub_promote(T a, T b)
        {
            if (a < b)
            {
                return static_cast<double>(a) - static_cast<double>(b);
            }
            return static_cast<double>(a - b);
        }

        inline double sub_promote(float a, float b)
        {
            return static_cast<double>(a) - static_cast<double>(b);
        }

        inline long double sub_promote(double a, double b)
        {
            return static_cast<long double>(a) - static_cast<long double>(b);
        }

        inline long double sub_promote(long double a, long double b)
        {
            return a - b;
        }

        // MUL_PROMOTE
        template <SignedIntegral T>
            requires(sizeof(T) <= sizeof(int))
        long long mul_promote(T a, T b)
        {
            int res;
            if (__builtin_mul_overflow(static_cast<int>(a), static_cast<int>(b), &res))
            {
                return static_cast<long long>(a) * static_cast<long long>(b);
            }
            return static_cast<long long>(res);
        }

        template <SignedIntegral T>
            requires(sizeof(T) > sizeof(int))
        double mul_promote(T a, T b)
        {
            long long res;
            if (__builtin_mul_overflow(static_cast<long long>(a), static_cast<long long>(b), &res))
            {
                return static_cast<double>(a) * static_cast<double>(b);
            }
            return static_cast<double>(res);
        }

        template <UnsignedIntegral T>
            requires(sizeof(T) <= sizeof(unsigned int))
        unsigned long long mul_promote(T a, T b)
        {
            return static_cast<unsigned long long>(a) * static_cast<unsigned long long>(b);
        }

        template <UnsignedIntegral T>
            requires(sizeof(T) > sizeof(unsigned int))
        double mul_promote(T a, T b)
        {
            // Approximate overflow check
            if (a != 0 && b > std::numeric_limits<unsigned long long>::max() / a)
            {
                return static_cast<double>(a) * static_cast<double>(b);
            }
            return static_cast<double>(a * b);
        }

        inline double mul_promote(float a, float b)
        {
            return static_cast<double>(a) * static_cast<double>(b);
        }

        inline long double mul_promote(double a, double b)
        {
            return static_cast<long double>(a) * static_cast<long double>(b);
        }

        inline long double mul_promote(long double a, long double b)
        {
            return a * b;
        }

        // DIV_PROMOTE
        template <SignedIntegral T>
            requires(sizeof(T) <= sizeof(int))
        long long div_promote(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer division by zero");
            }
            // Check for overflow (INT_MIN / -1)
            if (a == std::numeric_limits<int>::min() && b == -1)
            {
                return static_cast<long long>(a) / static_cast<long long>(b);
            }
            return static_cast<long long>(a / b);
        }

        template <SignedIntegral T>
            requires(sizeof(T) > sizeof(int))
        double div_promote(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer division by zero");
            }
            // Check for overflow (LLONG_MIN / -1)
            if (a == std::numeric_limits<long long>::min() && b == -1)
            {
                return static_cast<double>(a) / static_cast<double>(b);
            }
            return static_cast<double>(a / b);
        }

        template <UnsignedIntegral T>
        double div_promote(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer division by zero");
            }
            return static_cast<double>(a / b);
        }

        inline double div_promote(float a, float b)
        {
            if (b == 0.0f)
            {
                throw PythonicZeroDivisionError("float division by zero");
            }
            return static_cast<double>(a) / static_cast<double>(b);
        }

        inline long double div_promote(double a, double b)
        {
            if (b == 0.0)
            {
                throw PythonicZeroDivisionError("float division by zero");
            }
            return static_cast<long double>(a) / static_cast<long double>(b);
        }

        inline long double div_promote(long double a, long double b)
        {
            if (b == 0.0L)
            {
                throw PythonicZeroDivisionError("float division by zero");
            }
            return a / b;
        }

        // MOD_PROMOTE
        template <Integral T>
        auto mod_promote(T a, T b)
        {
            if (b == 0)
            {
                throw PythonicZeroDivisionError("integer modulo by zero");
            }
            using Wider = typename NextWiderType<T>::type;
            if constexpr (std::is_same_v<Wider, double>)
            {
                return static_cast<double>(a % b);
            }
            else if constexpr (NextWiderType<T>::can_promote)
            {
                return static_cast<Wider>(a % b);
            }
            else
            {
                return a % b;
            }
        }

    } // namespace overflow
} // namespace pythonic
