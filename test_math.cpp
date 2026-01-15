#include "pythonicMath.hpp"
#include "pythonicPrint.hpp"
#include <iostream>
#include <cmath>

using namespace pythonic::vars;
using namespace pythonic::math;
using namespace pythonic::print;

#define TEST(name) std::cout << "Testing: " << name << "... "
#define ASSERT(condition, msg)                       \
    if (!(condition))                                \
    {                                                \
        std::cout << "FAILED: " << msg << std::endl; \
        return false;                                \
    }
#define PASS() std::cout << "PASSED" << std::endl

bool test_basic_math()
{
    TEST("round()");
    ASSERT(round(var(3.7)).get<double>() == 4.0, "round(3.7) should be 4");
    ASSERT(round(var(3.2)).get<double>() == 3.0, "round(3.2) should be 3");
    PASS();

    TEST("pow()");
    ASSERT(pow(var(2), var(10)).get<double>() == 1024.0, "2^10 should be 1024");
    ASSERT(pow(var(3), var(3)).get<double>() == 27.0, "3^3 should be 27");
    PASS();

    TEST("sqrt()");
    ASSERT(sqrt(var(16)).get<double>() == 4.0, "sqrt(16) should be 4");
    ASSERT(sqrt(var(9)).get<double>() == 3.0, "sqrt(9) should be 3");
    PASS();

    TEST("nthroot()");
    auto cube_root_8 = nthroot(var(8), var(3)).get<double>();
    ASSERT(std::abs(cube_root_8 - 2.0) < 0.0001, "cube root of 8 should be 2");
    PASS();

    TEST("exp()");
    auto exp_val = exp(var(0)).get<double>();
    ASSERT(std::abs(exp_val - 1.0) < 0.0001, "exp(0) should be 1");
    PASS();

    TEST("log()");
    auto log_val = log(var(M_E)).get<double>();
    ASSERT(std::abs(log_val - 1.0) < 0.0001, "log(e) should be 1");
    PASS();

    TEST("log10()");
    ASSERT(log10(var(100)).get<double>() == 2.0, "log10(100) should be 2");
    PASS();

    TEST("log2()");
    ASSERT(log2(var(8)).get<double>() == 3.0, "log2(8) should be 3");
    PASS();

    return true;
}

bool test_trig()
{
    TEST("sin()");
    auto sin_val = sin(var(M_PI / 2)).get<double>();
    ASSERT(std::abs(sin_val - 1.0) < 0.0001, "sin(π/2) should be 1");
    PASS();

    TEST("cos()");
    auto cos_val = cos(var(0)).get<double>();
    ASSERT(std::abs(cos_val - 1.0) < 0.0001, "cos(0) should be 1");
    PASS();

    TEST("tan()");
    auto tan_val = tan(var(M_PI / 4)).get<double>();
    ASSERT(std::abs(tan_val - 1.0) < 0.0001, "tan(π/4) should be 1");
    PASS();

    TEST("cot()");
    auto cot_val = cot(var(M_PI / 4)).get<double>();
    ASSERT(std::abs(cot_val - 1.0) < 0.0001, "cot(π/4) should be 1");
    PASS();

    TEST("sec()");
    auto sec_val = sec(var(0)).get<double>();
    ASSERT(std::abs(sec_val - 1.0) < 0.0001, "sec(0) should be 1");
    PASS();

    TEST("cosec()");
    auto cosec_val = cosec(var(M_PI / 2)).get<double>();
    ASSERT(std::abs(cosec_val - 1.0) < 0.0001, "cosec(π/2) should be 1");
    PASS();

    return true;
}

bool test_inverse_trig()
{
    TEST("asin()");
    auto asin_val = asin(var(1)).get<double>();
    ASSERT(std::abs(asin_val - M_PI / 2) < 0.0001, "asin(1) should be π/2");
    PASS();

    TEST("acos()");
    auto acos_val = acos(var(1)).get<double>();
    ASSERT(std::abs(acos_val - 0.0) < 0.0001, "acos(1) should be 0");
    PASS();

    TEST("atan()");
    auto atan_val = atan(var(1)).get<double>();
    ASSERT(std::abs(atan_val - M_PI / 4) < 0.0001, "atan(1) should be π/4");
    PASS();

    TEST("atan2()");
    auto atan2_val = atan2(var(1), var(1)).get<double>();
    ASSERT(std::abs(atan2_val - M_PI / 4) < 0.0001, "atan2(1,1) should be π/4");
    PASS();

    return true;
}

bool test_hyperbolic()
{
    TEST("sinh()");
    auto sinh_val = sinh(var(0)).get<double>();
    ASSERT(std::abs(sinh_val - 0.0) < 0.0001, "sinh(0) should be 0");
    PASS();

    TEST("cosh()");
    auto cosh_val = cosh(var(0)).get<double>();
    ASSERT(std::abs(cosh_val - 1.0) < 0.0001, "cosh(0) should be 1");
    PASS();

    TEST("tanh()");
    auto tanh_val = tanh(var(0)).get<double>();
    ASSERT(std::abs(tanh_val - 0.0) < 0.0001, "tanh(0) should be 0");
    PASS();

    return true;
}

bool test_rounding()
{
    TEST("floor()");
    ASSERT(floor(var(3.7)).get<double>() == 3.0, "floor(3.7) should be 3");
    ASSERT(floor(var(-2.3)).get<double>() == -3.0, "floor(-2.3) should be -3");
    PASS();

    TEST("ceil()");
    ASSERT(ceil(var(3.2)).get<double>() == 4.0, "ceil(3.2) should be 4");
    ASSERT(ceil(var(-2.7)).get<double>() == -2.0, "ceil(-2.7) should be -2");
    PASS();

    TEST("trunc()");
    ASSERT(trunc(var(3.7)).get<double>() == 3.0, "trunc(3.7) should be 3");
    ASSERT(trunc(var(-3.7)).get<double>() == -3.0, "trunc(-3.7) should be -3");
    PASS();

    return true;
}

bool test_constants()
{
    TEST("pi()");
    ASSERT(std::abs(pi().get<double>() - M_PI) < 0.0001, "pi() should be M_PI");
    PASS();

    TEST("e()");
    ASSERT(std::abs(e().get<double>() - M_E) < 0.0001, "e() should be M_E");
    PASS();

    return true;
}

bool test_conversions()
{
    TEST("radians()");
    auto rad = radians(var(180)).get<double>();
    ASSERT(std::abs(rad - M_PI) < 0.0001, "180 degrees should be π radians");
    PASS();

    TEST("degrees()");
    auto deg = degrees(var(M_PI)).get<double>();
    ASSERT(std::abs(deg - 180.0) < 0.0001, "π radians should be 180 degrees");
    PASS();

    return true;
}

bool test_random()
{
    TEST("random_int()");
    var r1 = random_int(var(1), var(10));
    int val = r1.get<int>();
    ASSERT(val >= 1 && val <= 10, "random_int should be in range [1, 10]");
    PASS();

    TEST("random_float()");
    var r2 = random_float(var(0.0), var(1.0));
    double fval = r2.get<double>();
    ASSERT(fval >= 0.0 && fval < 1.0, "random_float should be in range [0, 1)");
    PASS();

    TEST("random_choice(list)");
    var lst = list(1, 2, 3, 4, 5);
    var choice = random_choice(lst);
    ASSERT(choice.is<int>(), "random_choice should return an int from list");
    PASS();

    TEST("random_choice_set(set)");
    var s = set(10, 20, 30);
    var choice_s = random_choice_set(s);
    ASSERT(choice_s.is<int>(), "random_choice_set should return int");
    PASS();

    TEST("fill_random() - list of random ints");
    var rand_list = fill_random(10, var(1), var(100));
    ASSERT(rand_list.type() == "list", "Should return a list");
    ASSERT(rand_list.len() == 10, "Should have 10 elements");
    // Check all are in range
    const auto &l = rand_list.get<List>();
    for (const auto &item : l)
    {
        int val = item.get<int>();
        ASSERT(val >= 1 && val <= 100, "All values should be in [1, 100]");
    }
    PASS();

    TEST("fill_randomf() - list of random floats (uniform)");
    var rand_floats = fill_randomf(10, var(0.0), var(1.0));
    ASSERT(rand_floats.type() == "list", "Should return a list");
    ASSERT(rand_floats.len() == 10, "Should have 10 elements");
    const auto &fl = rand_floats.get<List>();
    for (const auto &item : fl)
    {
        double val = item.get<double>();
        ASSERT(val >= 0.0 && val < 1.0, "All values should be in [0, 1)");
    }
    PASS();

    TEST("fill_randomn() - list of random floats (Gaussian)");
    var rand_gauss = fill_randomn(10, var(0.0), var(1.0)); // mean=0, stddev=1
    ASSERT(rand_gauss.type() == "list", "Should return a list");
    ASSERT(rand_gauss.len() == 10, "Should have 10 elements");
    // Can't test range strictly for Gaussian, just verify it's a list of doubles
    const auto &gl = rand_gauss.get<List>();
    for (const auto &item : gl)
    {
        ASSERT(item.is<double>(), "Should contain doubles");
    }
    PASS();

    TEST("fill_random_set() - set of random ints");
    var rand_set = fill_random_set(5, var(1), var(100));
    ASSERT(rand_set.type() == "set", "Should return a set");
    ASSERT(rand_set.len() == 5, "Should have 5 unique elements");
    PASS();

    TEST("fill_randomf_set() - set of random floats (uniform)");
    var rand_set_f = fill_randomf_set(5, var(0.0), var(1.0));
    ASSERT(rand_set_f.type() == "set", "Should return a set");
    ASSERT(rand_set_f.len() == 5, "Should have 5 unique elements");
    PASS();

    TEST("fill_randomn_set() - set of random floats (Gaussian)");
    var rand_set_g = fill_randomn_set(5, var(0.0), var(1.0));
    ASSERT(rand_set_g.type() == "set", "Should return a set");
    ASSERT(rand_set_g.len() == 5, "Should have 5 unique elements");
    PASS();

    return true;
}

bool test_product()
{
    TEST("product(list)");
    var lst = list(2, 3, 4);
    var prod = product(lst);
    ASSERT(prod.get<int>() == 24, "product([2,3,4]) should be 24");
    PASS();

    TEST("product(list) with start");
    var prod2 = product(lst, var(10));
    ASSERT(prod2.get<int>() == 240, "product([2,3,4], 10) should be 240");
    PASS();

    TEST("product(set)");
    var s = set(2, 3, 4);
    var prod_set = product(s);
    ASSERT(prod_set.get<int>() == 24, "product({2,3,4}) should be 24");
    PASS();

    return true;
}

bool test_advanced()
{
    TEST("gcd()");
    ASSERT(gcd(var(48), var(18)).get<long long>() == 6, "gcd(48, 18) should be 6");
    PASS();

    TEST("lcm()");
    ASSERT(lcm(var(12), var(18)).get<long long>() == 36, "lcm(12, 18) should be 36");
    PASS();

    TEST("factorial()");
    ASSERT(factorial(var(5)).get<long long>() == 120, "5! should be 120");
    ASSERT(factorial(var(0)).get<long long>() == 1, "0! should be 1");
    PASS();

    TEST("hypot()");
    auto h = hypot(var(3), var(4)).get<double>();
    ASSERT(std::abs(h - 5.0) < 0.0001, "hypot(3, 4) should be 5");
    PASS();

    return true;
}

int main()
{
    std::cout << "=== Pythonic Math Library Tests ===\n\n";

    int passed = 0, total = 0;

    auto run_test = [&](bool (*test_func)(), const char *name)
    {
        std::cout << "\n--- " << name << " ---\n";
        total++;
        if (test_func())
        {
            passed++;
        }
        else
        {
            std::cout << "Test suite FAILED!\n";
        }
    };

    run_test(test_basic_math, "Basic Math Functions");
    run_test(test_trig, "Trigonometric Functions");
    run_test(test_inverse_trig, "Inverse Trigonometric Functions");
    run_test(test_hyperbolic, "Hyperbolic Functions");
    run_test(test_rounding, "Rounding Functions");
    run_test(test_constants, "Mathematical Constants");
    run_test(test_conversions, "Angle Conversions");
    run_test(test_random, "Random Functions");
    run_test(test_product, "Product Function");
    run_test(test_advanced, "Advanced Functions");

    std::cout << "\n====================================\n";
    std::cout << "Test Suites: " << passed << "/" << total << " passed\n";
    std::cout << "====================================\n";

    return (passed == total) ? 0 : 1;
}
