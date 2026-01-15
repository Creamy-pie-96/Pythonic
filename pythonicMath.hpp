#pragma once

#include <cmath>
#include <random>
#include <algorithm>
#include "pythonicVars.hpp"

namespace pythonic
{
    namespace math
    {
        using namespace pythonic::vars;

        // Helper to extract numeric value from var
        inline double to_numeric(const var &v)
        {
            if (v.is<int>())
                return static_cast<double>(v.get<int>());
            else if (v.is<float>())
                return static_cast<double>(v.get<float>());
            else if (v.is<double>())
                return v.get<double>();
            else if (v.is<long>())
                return static_cast<double>(v.get<long>());
            else if (v.is<long long>())
                return static_cast<double>(v.get<long long>());
            else if (v.is<long double>())
                return static_cast<double>(v.get<long double>());
            else if (v.is<unsigned int>())
                return static_cast<double>(v.get<unsigned int>());
            else if (v.is<unsigned long>())
                return static_cast<double>(v.get<unsigned long>());
            else if (v.is<unsigned long long>())
                return static_cast<double>(v.get<unsigned long long>());
            else
                throw std::runtime_error("Math function requires numeric type");
        }

        // ============ Basic Math Functions ============

        inline var round(const var &v)
        {
            return var(std::round(to_numeric(v)));
        }

        inline var pow(const var &base, const var &exponent)
        {
            return var(std::pow(to_numeric(base), to_numeric(exponent)));
        }

        inline var sqrt(const var &v)
        {
            return var(std::sqrt(to_numeric(v)));
        }

        inline var nthroot(const var &value, const var &n)
        {
            return var(std::pow(to_numeric(value), 1.0 / to_numeric(n)));
        }

        inline var exp(const var &v)
        {
            return var(std::exp(to_numeric(v)));
        }

        inline var log(const var &v)
        {
            return var(std::log(to_numeric(v)));
        }

        inline var log10(const var &v)
        {
            return var(std::log10(to_numeric(v)));
        }

        inline var log2(const var &v)
        {
            return var(std::log2(to_numeric(v)));
        }

        // ============ Trigonometric Functions ============

        inline var sin(const var &v)
        {
            return var(std::sin(to_numeric(v)));
        }

        inline var cos(const var &v)
        {
            return var(std::cos(to_numeric(v)));
        }

        inline var tan(const var &v)
        {
            return var(std::tan(to_numeric(v)));
        }

        inline var cot(const var &v)
        {
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

        inline var floor(const var &v)
        {
            return var(std::floor(to_numeric(v)));
        }

        inline var ceil(const var &v)
        {
            return var(std::ceil(to_numeric(v)));
        }

        inline var trunc(const var &v)
        {
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
                throw std::runtime_error("random_choice() requires a list");

            const auto &l = lst.get<List>();
            if (l.empty())
                throw std::runtime_error("random_choice() from empty list");

            std::uniform_int_distribution<size_t> dist(0, l.size() - 1);
            return l[dist(get_random_engine())];
        }

        // Random element from set
        inline var random_choice_set(const var &s)
        {
            if (s.type() != "set")
                throw std::runtime_error("random_choice_set() requires a set");

            const auto &set_val = s.get<Set>();
            if (set_val.empty())
                throw std::runtime_error("random_choice_set() from empty set");

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
                throw std::runtime_error("fill_random_set(): range too small for unique count");
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
                throw std::runtime_error("product() requires a list or set");
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
                throw std::runtime_error("factorial() not defined for negative values");

            long long result = 1;
            for (int i = 2; i <= num; ++i)
            {
                result *= i;
            }
            return var(result);
        }

    } // namespace math
} // namespace pythonic
