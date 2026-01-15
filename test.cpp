#include <iostream>
#include <cassert>
#include <cstdio>
#include "pythonicVars.hpp"
#include "pythonicPrint.hpp"
#include "pythonicLoop.hpp"
#include "pythonicFunction.hpp"
#include "pythonicFile.hpp"

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::func;
using namespace pythonic::file;

// Test counter
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) std::cout << "\n  Testing: " << name << "... ";
#define PASS() { std::cout << "PASS"; tests_passed++; }
#define FAIL(msg) { std::cout << "FAIL: " << msg; tests_failed++; }
#define ASSERT(cond, msg) if (!(cond)) { FAIL(msg); } else { PASS(); }

void test_slicing() {
    std::cout << "\n=== Testing Slicing ===";

    TEST("List slice(1, 4)");
    var lst = list(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    var sliced = lst.slice(1, 4);
    ASSERT(sliced.len() == 3 && sliced[static_cast<size_t>(0)].get<int>() == 1, 
           "Should get [1, 2, 3]");

    TEST("List slice with step");
    var stepped = lst.slice(0, 10, 2);
    ASSERT(stepped.len() == 5 && stepped[static_cast<size_t>(2)].get<int>() == 4, 
           "Should get [0, 2, 4, 6, 8]");

    TEST("List negative indices");
    var neg = lst.slice(-3);
    ASSERT(neg.len() == 3 && neg[static_cast<size_t>(0)].get<int>() == 7, 
           "Should get last 3 elements");

    TEST("String slice");
    var str = "Hello, World!";
    var sub = str.slice(0, 5);
    ASSERT(sub.get<std::string>() == "Hello", "Should get Hello");

    TEST("operator() for slicing");
    var via_op = lst(2, 5);
    ASSERT(via_op.len() == 3, "operator() should work like slice()");
}

void test_string_methods() {
    std::cout << "\n\n=== Testing String Methods ===";

    var str = "  Hello, World!  ";
    
    TEST("upper()");
    ASSERT(str.upper().get<std::string>() == "  HELLO, WORLD!  ", "Should uppercase");

    TEST("lower()");
    ASSERT(str.lower().get<std::string>() == "  hello, world!  ", "Should lowercase");

    TEST("strip()");
    ASSERT(str.strip().get<std::string>() == "Hello, World!", "Should remove whitespace");

    TEST("replace()");
    var s2 = "hello world";
    ASSERT(s2.replace(var("world"), var("there")).get<std::string>() == "hello there", 
           "Should replace");

    TEST("find()");
    ASSERT(s2.find(var("world")).get<long long>() == 6, "Should find at index 6");

    TEST("startswith()");
    ASSERT(s2.startswith(var("hello")).get<bool>() == true, "Should start with hello");

    TEST("endswith()");
    ASSERT(s2.endswith(var("world")).get<bool>() == true, "Should end with world");

    TEST("isdigit()");
    var digits = "12345";
    ASSERT(digits.isdigit().get<bool>() == true, "Should be all digits");

    TEST("split()");
    var sentence = "hello world test";
    var words = sentence.split();
    ASSERT(words.len() == 3, "Should split into 3 words");

    TEST("join()");
    var sep = "-";
    var joined = sep.join(list("a", "b", "c"));
    ASSERT(joined.get<std::string>() == "a-b-c", "Should join with -");

    TEST("center()");
    var ctr = "hi";
    ASSERT(ctr.center(6).get<std::string>() == "  hi  ", "Should center");

    TEST("zfill()");
    var num = "42";
    ASSERT(num.zfill(5).get<std::string>() == "00042", "Should zero-fill");
}

void test_comparison_operators() {
    std::cout << "\n\n=== Testing Comparison Operators ===";

    var a = 10;
    var b = 20;
    var c = 10;

    TEST("if(a < b) - true case");
    bool result1 = false;
    if (a < b) { result1 = true; }
    ASSERT(result1 == true, "10 < 20 should be true");

    TEST("var3 = (var1 < var2)");
    var lt_result = (a < b);
    ASSERT(lt_result.get<bool>() == true, "Should assign true");

    TEST("var3 = (var1 == var2)");
    var eq_result = (a == c);
    ASSERT(eq_result.get<bool>() == true, "Should assign true");

    TEST("var3 = (var1 != var2)");
    var ne_result = (a != b);
    ASSERT(ne_result.get<bool>() == true, "Should assign true");

    TEST("String comparison");
    var s1 = "apple";
    var s2 = "banana";
    var str_cmp = (s1 < s2);
    ASSERT(str_cmp.get<bool>() == true, "apple < banana should be true");
}

void test_file_io() {
    std::cout << "\n\n=== Testing File I/O ===";

    TEST("write_file()");
    write_file("test_output.txt", var("Hello, World!\nLine 2\nLine 3\n"));
    ASSERT(file_exists("test_output.txt"), "File should exist");

    TEST("read_file()");
    var content = read_file("test_output.txt");
    ASSERT(content.get<std::string>().substr(0, 13) == "Hello, World!", 
           "Should read content");

    TEST("read_lines()");
    var lines = read_lines("test_output.txt");
    ASSERT(lines.len() == 3, "Should read 3 lines");

    TEST("File class read");
    {
        File f("test_output.txt", "r");
        var line = f.readline();
        ASSERT(line.get<std::string>() == "Hello, World!", "Should read first line");
    }

    TEST("with_open macro");
    bool macro_worked = false;
    with_open("test_output.txt", "r", file) {
        var c = file.read();
        if (!c.get<std::string>().empty()) {
            macro_worked = true;
        }
    }
    ASSERT(macro_worked, "with_open should work");

    // Cleanup
    std::remove("test_output.txt");
}

void test_type_introspection() {
    std::cout << "\n\n=== Testing Type Introspection ===";

    TEST("type() on int");
    var i = 42;
    ASSERT(i.type() == "int", "Should be 'int'");

    TEST("type() on string");
    var s = "hello";
    ASSERT(s.type() == "str", "Should be 'str'");

    TEST("type() on list");
    var lst = list(1, 2, 3);
    ASSERT(lst.type() == "list", "Should be 'list'");

    TEST("type() on dict");
    var d = dict();
    d["key"] = "value";
    ASSERT(d.type() == "dict", "Should be 'dict'");

    TEST("type() on bool");
    var b = true;
    ASSERT(b.type() == "bool", "Should be 'bool'");

    TEST("isinstance<int>");
    ASSERT(isinstance<int>(i) == true, "Should be int");

    TEST("isinstance(v, 'str')");
    ASSERT(isinstance(s, "str") == true, "Should be str");

    TEST("isinstance(v, 'list')");
    ASSERT(isinstance(lst, "list") == true, "Should be list");
}

void test_pretty_print() {
    std::cout << "\n\n=== Testing Pretty Print ===";

    TEST("Simple list print");
    var simple_lst = list(1, 2, 3);
    std::string result = simple_lst.str();
    ASSERT(result.find('[') != std::string::npos, "Should have bracket");

    TEST("Nested list pretty_str");
    var nested = list(list(1, 2), list(3, 4));
    std::string pretty = nested.pretty_str();
    ASSERT(pretty.find('\n') != std::string::npos, "Pretty str should have newlines");

    TEST("Dict pretty_str");
    var d = dict();
    d["name"] = "test";
    d["value"] = 42;
    std::string dict_pretty = d.pretty_str();
    ASSERT(dict_pretty.find("name") != std::string::npos, "Should contain key");

    TEST("pprint doesn't crash");
    bool pprint_ok = true;
    try {
        std::cout << "\n    Output: ";
        pprint(simple_lst);
    } catch (...) {
        pprint_ok = false;
    }
    ASSERT(pprint_ok, "pprint should work");
}

void test_builtin_functions() {
    std::cout << "\n\n=== Testing Built-in Functions ===";

    // Bool()
    TEST("Bool(0)");
    ASSERT(Bool(var(0)).get<bool>() == false, "0 should be false");

    TEST("Bool(1)");
    ASSERT(Bool(var(1)).get<bool>() == true, "1 should be true");

    TEST("Bool(empty string)");
    ASSERT(Bool(var("")).get<bool>() == false, "Empty string should be false");

    TEST("Bool(non-empty string)");
    ASSERT(Bool(var("hello")).get<bool>() == true, "Non-empty string should be true");

    TEST("Bool(empty list)");
    ASSERT(Bool(list()).get<bool>() == false, "Empty list should be false");

    // repr()
    TEST("repr() on string");
    var s = "hello\nworld";
    var r = repr(s);
    ASSERT(r.get<std::string>().find("\\n") != std::string::npos, 
           "Should escape newline");

    // Str()
    TEST("Str() on int");
    var i = 42;
    ASSERT(Str(i).get<std::string>() == "42", "Should convert to string");

    // Int()
    TEST("Int() on string");
    var num_str = "123";
    ASSERT(Int(num_str).get<int>() == 123, "Should parse int");

    TEST("Int() on float");
    var f = 3.7;
    ASSERT(Int(f).get<int>() == 3, "Should truncate float");

    // Float()
    TEST("Float() on int");
    var i2 = 42;
    var f2 = Float(i2);
    ASSERT(f2.type() == "double", "Should be double");

    TEST("Float() on string");
    var float_str = "3.14";
    var f3 = Float(float_str);
    ASSERT(f3.get<double>() > 3.1 && f3.get<double>() < 3.2, "Should parse float");

    // abs()
    TEST("abs() on negative int");
    ASSERT(pythonic::vars::abs(var(-5)).get<int>() == 5, "Should be 5");

    TEST("abs() on negative float");
    var neg_f = -3.5;
    ASSERT(pythonic::vars::abs(neg_f).get<double>() > 3.4, "Should be positive");

    // min/max
    TEST("min(a, b)");
    ASSERT(pythonic::vars::min(var(3), var(7)).get<int>() == 3, "Should be 3");

    TEST("max(a, b)");
    ASSERT(pythonic::vars::max(var(3), var(7)).get<int>() == 7, "Should be 7");

    TEST("min(list)");
    var lst = list(5, 2, 8, 1, 9);
    ASSERT(pythonic::vars::min(lst).get<int>() == 1, "Should be 1");

    TEST("max(list)");
    ASSERT(pythonic::vars::max(lst).get<int>() == 9, "Should be 9");

    // sum
    TEST("sum(list)");
    var nums = list(1, 2, 3, 4, 5);
    ASSERT(sum(nums).get<int>() == 15, "Should be 15");

    TEST("sum(list, start)");
    ASSERT(sum(nums, var(10)).get<int>() == 25, "Should be 25");

    // sorted
    TEST("sorted(list)");
    var unsorted = list(3, 1, 4, 1, 5, 9, 2, 6);
    var s_list = pythonic::vars::sorted(unsorted);
    ASSERT(s_list[static_cast<size_t>(0)].get<int>() == 1, "First should be 1");

    TEST("sorted(list, reverse=true)");
    var r_list = pythonic::vars::sorted(unsorted, true);
    ASSERT(r_list[static_cast<size_t>(0)].get<int>() == 9, "First should be 9");

    // reversed_var
    TEST("reversed_var(list)");
    var orig = list(1, 2, 3);
    var rev = reversed_var(orig);
    ASSERT(rev[static_cast<size_t>(0)].get<int>() == 3, "First should be 3");

    TEST("reversed_var(string)");
    var str = "hello";
    ASSERT(reversed_var(str).get<std::string>() == "olleh", "Should reverse string");

    // all_var/any_var
    TEST("all_var() with all true");
    var all_true = list(1, 2, 3, true, "non-empty");
    ASSERT(all_var(all_true).get<bool>() == true, "Should be true");

    TEST("all_var() with one false");
    var has_false = list(1, 0, 3);
    ASSERT(all_var(has_false).get<bool>() == false, "Should be false");

    TEST("any_var() with one true");
    var has_true = list(0, 0, 1);
    ASSERT(any_var(has_true).get<bool>() == true, "Should be true");

    TEST("any_var() with all false");
    var all_false = list(0, "", list());
    ASSERT(any_var(all_false).get<bool>() == false, "Should be false");
}

void test_functional() {
    std::cout << "\n\n=== Testing Functional (map/filter/reduce) ===";

    var nums = list(1, 2, 3, 4, 5);

    TEST("map() double each");
    var doubled = pythonic::vars::map([](const var& x) { 
        return var(x.get<int>() * 2); 
    }, nums);
    ASSERT(doubled[static_cast<size_t>(0)].get<int>() == 2 && 
           doubled[static_cast<size_t>(4)].get<int>() == 10, "Should double");

    TEST("filter() evens only");
    var evens = pythonic::vars::filter([](const var& x) { 
        return x.get<int>() % 2 == 0; 
    }, nums);
    ASSERT(evens.len() == 2, "Should have 2 even numbers");

    TEST("reduce() sum");
    var total = reduce([](const var& acc, const var& x) { 
        return acc + x; 
    }, nums);
    ASSERT(total.get<int>() == 15, "Should sum to 15");

    TEST("reduce() with initial");
    var total2 = reduce([](const var& acc, const var& x) { 
        return acc + x; 
    }, nums, var(100));
    ASSERT(total2.get<int>() == 115, "Should sum to 115");

    TEST("map() strings");
    var words = list("hello", "world");
    var upper_words = pythonic::vars::map([](const var& x) { 
        return x.upper(); 
    }, words);
    ASSERT(upper_words[static_cast<size_t>(0)].get<std::string>() == "HELLO", 
           "Should uppercase");
}

void test_edge_cases() {
    std::cout << "\n\n=== Testing Edge Cases ===";

    TEST("Empty list operations");
    var empty = list();
    ASSERT(Bool(empty).get<bool>() == false, "Empty list is falsy");

    TEST("sorted empty list");
    var sorted_empty = pythonic::vars::sorted(empty);
    ASSERT(sorted_empty.len() == 0, "Sorted empty should be empty");

    TEST("File not found exception");
    bool threw = false;
    try {
        File f("nonexistent_file_xyz.txt", "r");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    ASSERT(threw, "Should throw on file not found");

    TEST("Int() invalid string");
    bool int_threw = false;
    try {
        Int(var("not a number"));
    } catch (const std::runtime_error&) {
        int_threw = true;
    }
    ASSERT(int_threw, "Should throw on invalid int");
}

int main() {
    std::cout << "==========================================\n";
    std::cout << "   PYTHONIC LIBRARY COMPREHENSIVE TESTS   \n";
    std::cout << "==========================================\n";

    test_slicing();
    test_string_methods();
    test_comparison_operators();
    test_file_io();
    test_type_introspection();
    test_pretty_print();
    test_builtin_functions();
    test_functional();
    test_edge_cases();

    std::cout << "\n\n==========================================\n";
    std::cout << "          TEST RESULTS SUMMARY            \n";
    std::cout << "==========================================\n";
    std::cout << "  Passed: " << tests_passed << "\n";
    std::cout << "  Failed: " << tests_failed << "\n";
    std::cout << "==========================================\n";

    return tests_failed > 0 ? 1 : 0;
}
