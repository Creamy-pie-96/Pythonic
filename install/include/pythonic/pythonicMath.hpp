#pragma once

#include <cmath>
#include <random>
#include "pythonicError.hpp"
#include <algorithm>
#include "pythonicVars.hpp"

namespace pythonic
{
    namespace math
    {
        using namespace pythonic::vars;

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

        inline var pow(const var &base, const var &exponent)
        {
            // Fast path: int ^ int (common case) - returns int for memory efficiency
            if (base.is_int() && exponent.is_int())
            {
                int b = base.as_int_unchecked();
                int e = exponent.as_int_unchecked();
                if (e >= 0 && e < 31)
                { // Avoid overflow for small exponents
                    int result = 1;
                    for (int i = 0; i < e; ++i)
                        result *= b;
                    return var(result);
                }
            }
            // Fast path: long long ^ int - returns long long
            if (base.is_long_long() && exponent.is_int())
            {
                long long b = base.as_long_long_unchecked();
                int e = exponent.as_int_unchecked();
                if (e >= 0 && e < 63)
                {
                    long long result = 1;
                    for (int i = 0; i < e; ++i)
                        result *= b;
                    return var(result);
                }
            }
            // Fast path: double ^ int or double ^ double - returns double
            if (base.is_double())
            {
                if (exponent.is_int())
                    return var(std::pow(base.as_double_unchecked(), exponent.as_int_unchecked()));
                if (exponent.is_double())
                    return var(std::pow(base.as_double_unchecked(), exponent.as_double_unchecked()));
            }
            // Fast path: float ^ int or float ^ float - returns double for precision
            if (base.is_float())
            {
                if (exponent.is_int())
                    return var(std::pow(static_cast<double>(base.as_float_unchecked()), exponent.as_int_unchecked()));
                if (exponent.is_float())
                    return var(std::pow(static_cast<double>(base.as_float_unchecked()), static_cast<double>(exponent.as_float_unchecked())));
            }
            // Fallback: convert to double - returns double
            return var(std::pow(to_numeric(base), to_numeric(exponent)));
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

        inline var product(const var &iterable, const var &start = var(1))
        {
            var result = start;

            if (iterable.type() == "list")
            {
                const auto &l = iterable.get<List>();
                for (const auto &item : l)
                {
                    result = result * item;
                }
            }
            else if (iterable.type() == "set")
            {
                const auto &s = iterable.get<Set>();
                for (const auto &item : s)
                {
                    result = result * item;
                }
            }
            else
            {
                throw pythonic::PythonicTypeError("product() requires a list or set");
            }

            return result;
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

        inline var lcm(const var &a, const var &b)
        {
            return var(std::lcm(static_cast<long long>(to_numeric(a)),
                                static_cast<long long>(to_numeric(b))));
        }

        inline var factorial(const var &n)
        {
            int num = static_cast<int>(to_numeric(n));
            if (num < 0)
                throw pythonic::PythonicValueError("factorial() not defined for negative values");

            long long result = 1;
            for (int i = 2; i <= num; ++i)
            {
                result *= i;
            }
            return var(result);
        }

        // ============ Checked Arithmetic Operations ============
        // Perform arithmetic with overflow detection

        inline var checked_add(const var &a, const var &b)
        {
            // For simplicity, use int64_t for checked operations
            // In production, would use compiler intrinsics or boost::safe_numerics
            if (a.is_int() && b.is_int())
            {
                int64_t a_val = a.as_int_unchecked();
                int64_t b_val = b.as_int_unchecked();
                
                // Check for overflow
                if ((b_val > 0 && a_val > std::numeric_limits<int64_t>::max() - b_val) ||
                    (b_val < 0 && a_val < std::numeric_limits<int64_t>::min() - b_val))
                {
                    throw pythonic::PythonicValueError("Integer overflow in checked_add");
                }
                return var(a_val + b_val);
            }
            // Fall back to regular addition for doubles
            return var(to_numeric(a) + to_numeric(b));
        }

        inline var checked_sub(const var &a, const var &b)
        {
            if (a.is_int() && b.is_int())
            {
                int64_t a_val = a.as_int_unchecked();
                int64_t b_val = b.as_int_unchecked();
                
                if ((b_val < 0 && a_val > std::numeric_limits<int64_t>::max() + b_val) ||
                    (b_val > 0 && a_val < std::numeric_limits<int64_t>::min() + b_val))
                {
                    throw pythonic::PythonicValueError("Integer overflow in checked_sub");
                }
                return var(a_val - b_val);
            }
            return var(to_numeric(a) - to_numeric(b));
        }

        inline var checked_mul(const var &a, const var &b)
        {
            if (a.is_int() && b.is_int())
            {
                int64_t a_val = a.as_int_unchecked();
                int64_t b_val = b.as_int_unchecked();
                
                // Check for overflow in multiplication
                if (a_val != 0 && b_val != 0)
                {
                    if (std::abs(a_val) > std::numeric_limits<int64_t>::max() / std::abs(b_val))
                    {
                        throw pythonic::PythonicValueError("Integer overflow in checked_mul");
                    }
                }
                return var(a_val * b_val);
            }
            return var(to_numeric(a) * to_numeric(b));
        }

        inline var checked_div(const var &a, const var &b)
        {
            if (b.is_int() && b.as_int_unchecked() == 0)
            {
                throw pythonic::PythonicValueError("Division by zero in checked_div");
            }
            if (a.is_int() && b.is_int())
            {
                int64_t a_val = a.as_int_unchecked();
                int64_t b_val = b.as_int_unchecked();
                
                // Check for special case: INT_MIN / -1 causes overflow
                if (a_val == std::numeric_limits<int64_t>::min() && b_val == -1)
                {
                    throw pythonic::PythonicValueError("Integer overflow in checked_div");
                }
                return var(a_val / b_val);
            }
            
            double b_num = to_numeric(b);
            if (b_num == 0.0)
            {
                throw pythonic::PythonicValueError("Division by zero in checked_div");
            }
            return var(to_numeric(a) / b_num);
        }

    } // namespace math
} // namespace pythonic
