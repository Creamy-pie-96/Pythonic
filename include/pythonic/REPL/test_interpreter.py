"""
ScriptIt Interpreter — Comprehensive Test Suite
================================================
Tests every language feature, edge case, and error path.
Binary: ./scriptit --script  (reads code from stdin)

Run:   python3 -m unittest test_interpreter -v
"""

import subprocess
import unittest
import tempfile
import os


class TestInterpreter(unittest.TestCase):
    """Base test harness for the ScriptIt interpreter."""

    BINARY = 'scriptit'
    CWD = '/home/DATA/CODE/code/test'

    def run_code(self, code):
        """Run ScriptIt code via --script mode, return (stdout, stderr)."""
        formatted = code.strip()
        proc = subprocess.Popen(
            [self.BINARY, '--script'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=self.CWD,
            text=True,
        )
        stdout, stderr = proc.communicate(input=formatted, timeout=10)
        return stdout, stderr

    # ── Assertion helpers ──────────────────────────────

    def lines(self, stdout):
        """Clean output lines (strip, skip blanks/debug)."""
        return [
            l.strip() for l in stdout.splitlines()
            if l.strip() and not l.strip().startswith("Debug:")
            and not l.strip().startswith("---")
        ]

    def assertOutputSequence(self, stdout, expected):
        """Assert expected strings appear *in order* (substring match)."""
        got = self.lines(stdout)
        si = 0
        for li in range(len(got)):
            if si < len(expected) and expected[si] in got[li]:
                si += 1
        self.assertEqual(si, len(expected),
                         f"Expected sequence {expected!r} but got lines: {got!r}")

    def assertOutputExact(self, stdout, expected_lines):
        """Assert output lines match exactly."""
        got = self.lines(stdout)
        self.assertEqual(got, expected_lines,
                         f"Expected {expected_lines!r} but got {got!r}")

    def assertOutputContains(self, stdout, substring):
        """Assert substring appears anywhere in stdout."""
        self.assertIn(substring, stdout, f"Expected '{substring}' in output: {stdout!r}")

    def assertError(self, stderr, substring=""):
        """Assert stderr contains an error, optionally matching substring."""
        self.assertTrue(len(stderr.strip()) > 0 or "Error" in stderr,
                        f"Expected an error but got no stderr")
        if substring:
            self.assertIn(substring, stderr,
                          f"Expected '{substring}' in error: {stderr!r}")

    def assertOutputHasError(self, stdout, substring=""):
        """Assert stdout contains 'Error:' (since errors go to stdout in --script mode)."""
        self.assertIn("Error:", stdout, f"Expected error in output: {stdout!r}")
        if substring:
            self.assertIn(substring, stdout,
                          f"Expected '{substring}' in output: {stdout!r}")

    def assertFirstLine(self, stdout, expected):
        """Assert the first output line matches."""
        got = self.lines(stdout)
        self.assertTrue(len(got) > 0, "No output")
        self.assertEqual(got[0], expected, f"First line: expected {expected!r}, got {got[0]!r}")


# ═════════════════════════════════════════════════════════════
#  ARITHMETIC & NUMBERS
# ═════════════════════════════════════════════════════════════

class TestArithmetic(TestInterpreter):
    """Basic arithmetic, number types, overflow promotion."""

    def test_integer_addition(self):
        out, _ = self.run_code('print(1 + 2).')
        self.assertFirstLine(out, '3')

    def test_integer_subtraction(self):
        out, _ = self.run_code('print(10 - 3).')
        self.assertFirstLine(out, '7')

    def test_integer_multiplication(self):
        out, _ = self.run_code('print(6 * 7).')
        self.assertFirstLine(out, '42')

    def test_integer_division(self):
        out, _ = self.run_code('print(10 / 3).')
        got = self.lines(out)
        self.assertTrue(got[0].startswith('3.333'))

    def test_integer_modulo(self):
        out, _ = self.run_code('print(17 % 5).')
        self.assertFirstLine(out, '2')

    def test_exponentiation(self):
        out, _ = self.run_code('print(2 ^ 10).')
        self.assertFirstLine(out, '1024')

    def test_negative_numbers(self):
        out, _ = self.run_code('print(-5 + 3).')
        self.assertFirstLine(out, '-2')

    def test_decimal_numbers(self):
        out, _ = self.run_code('print(3.14 + 0.01).')
        self.assertFirstLine(out, '3.15')

    def test_leading_dot_decimal(self):
        out, _ = self.run_code('print(.5 + .5).')
        self.assertFirstLine(out, '1')

    def test_division_by_zero(self):
        out, _ = self.run_code('print(1 / 0).')
        self.assertOutputHasError(out, 'Division by zero')

    def test_modulo_by_zero(self):
        out, _ = self.run_code('print(5 % 0).')
        self.assertOutputHasError(out, 'Modulo by zero')

    def test_operator_precedence(self):
        out, _ = self.run_code('print(2 + 3 * 4).')
        self.assertFirstLine(out, '14')

    def test_parentheses_override_precedence(self):
        out, _ = self.run_code('print((2 + 3) * 4).')
        self.assertFirstLine(out, '20')

    def test_nested_parentheses(self):
        out, _ = self.run_code('print(((2 + 3) * (4 - 1))).')
        self.assertFirstLine(out, '15')

    def test_unary_negation(self):
        out, _ = self.run_code('print(-(-5)).')
        self.assertFirstLine(out, '5')

    def test_complex_expression(self):
        out, _ = self.run_code('print(2 ^ 3 + 4 * 2 - 1).')
        self.assertFirstLine(out, '15')

    def test_large_integer(self):
        out, _ = self.run_code('print(1000000 * 1000000).')
        got = self.lines(out)
        self.assertIn('1000000000000', got[0])

    def test_auto_print_expression(self):
        """Expression statements auto-print non-None results."""
        out, _ = self.run_code('42.')
        self.assertFirstLine(out, '42')


# ═════════════════════════════════════════════════════════════
#  VARIABLES
# ═════════════════════════════════════════════════════════════

class TestVariables(TestInterpreter):
    """Variable declaration, assignment, multi-var, compound ops."""

    def test_var_declaration(self):
        out, _ = self.run_code('var x = 10.\nprint(x).')
        self.assertFirstLine(out, '10')

    def test_var_default_none(self):
        out, _ = self.run_code('var x.\nprint(x).')
        self.assertFirstLine(out, 'None')

    def test_var_string(self):
        out, _ = self.run_code('var s = "hello".\nprint(s).')
        self.assertFirstLine(out, 'hello')

    def test_var_reassignment(self):
        out, _ = self.run_code('var x = 10.\nx = 20.\nprint(x).')
        self.assertFirstLine(out, '20')

    def test_multi_var(self):
        out, _ = self.run_code('var a = 1 b = 2 c = 3.\nprint(a + b + c).')
        self.assertFirstLine(out, '6')

    def test_let_be(self):
        out, _ = self.run_code('let x be 42.\nprint(x).')
        self.assertFirstLine(out, '42')

    def test_compound_plus_equals(self):
        out, _ = self.run_code('var x = 10.\nx += 5.\nprint(x).')
        self.assertFirstLine(out, '15')

    def test_compound_minus_equals(self):
        out, _ = self.run_code('var x = 10.\nx -= 3.\nprint(x).')
        self.assertFirstLine(out, '7')

    def test_compound_star_equals(self):
        out, _ = self.run_code('var x = 4.\nx *= 3.\nprint(x).')
        self.assertFirstLine(out, '12')

    def test_compound_slash_equals(self):
        out, _ = self.run_code('var x = 20.\nx /= 4.\nprint(x).')
        self.assertFirstLine(out, '5')

    def test_compound_percent_equals(self):
        out, _ = self.run_code('var x = 17.\nx %= 5.\nprint(x).')
        self.assertFirstLine(out, '2')

    def test_post_increment(self):
        out, _ = self.run_code('var x = 5.\nx++.\nprint(x).')
        self.assertFirstLine(out, '6')

    def test_post_decrement(self):
        out, _ = self.run_code('var x = 5.\nx--.\nprint(x).')
        self.assertFirstLine(out, '4')

    def test_pre_increment(self):
        out, _ = self.run_code('var x = 5.\n++x.\nprint(x).')
        self.assertFirstLine(out, '6')

    def test_pre_decrement(self):
        out, _ = self.run_code('var x = 5.\n--x.\nprint(x).')
        self.assertFirstLine(out, '4')

    def test_pi_constant(self):
        out, _ = self.run_code('print(PI).')
        self.assertOutputContains(out, '3.14159')

    def test_e_constant(self):
        out, _ = self.run_code('print(e).')
        self.assertOutputContains(out, '2.71828')


# ═════════════════════════════════════════════════════════════
#  BOOLEANS & COMPARISON
# ═════════════════════════════════════════════════════════════

class TestBooleans(TestInterpreter):
    """Boolean values, comparison operators, logical operators."""

    def test_true_false(self):
        out, _ = self.run_code('print(True).\nprint(False).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_equality(self):
        out, _ = self.run_code('print(1 == 1).\nprint(1 == 2).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_inequality(self):
        out, _ = self.run_code('print(1 != 2).\nprint(1 != 1).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_less_than(self):
        out, _ = self.run_code('print(1 < 2).\nprint(2 < 1).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_greater_than(self):
        out, _ = self.run_code('print(2 > 1).\nprint(1 > 2).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_less_equal(self):
        out, _ = self.run_code('print(1 <= 1).\nprint(1 <= 2).\nprint(2 <= 1).')
        self.assertOutputExact(out, ['True', 'True', 'False'])

    def test_greater_equal(self):
        out, _ = self.run_code('print(2 >= 2).\nprint(2 >= 1).\nprint(1 >= 2).')
        self.assertOutputExact(out, ['True', 'True', 'False'])

    def test_logical_and(self):
        out, _ = self.run_code('print(True and True).\nprint(True and False).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_logical_or(self):
        out, _ = self.run_code('print(False or True).\nprint(False or False).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_logical_not(self):
        out, _ = self.run_code('print(not True).\nprint(not False).')
        self.assertOutputExact(out, ['False', 'True'])

    def test_and_or_symbols(self):
        out, _ = self.run_code('print(True && False).\nprint(False || True).')
        self.assertOutputExact(out, ['False', 'True'])

    def test_is_operator(self):
        out, _ = self.run_code('print(10 is 10).\nprint(10 is 20).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_is_not_operator(self):
        out, _ = self.run_code('print(10 is not 20).\nprint(10 is not 10).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_points_operator(self):
        out, _ = self.run_code('print(10 points 10).\nprint(10 points 20).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_not_points_operator(self):
        out, _ = self.run_code('print(10 not points 20).\nprint(10 not points 10).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_points_type_strict(self):
        """'points' requires same type — int vs double should differ."""
        out, _ = self.run_code('print(10 points 10.0).')
        # int and double are different types, so 'points' should return False
        self.assertFirstLine(out, 'False')

    def test_string_equality(self):
        out, _ = self.run_code('print("hello" == "hello").\nprint("hello" == "world").')
        self.assertOutputExact(out, ['True', 'False'])

    def test_none_comparison(self):
        out, _ = self.run_code('var x.\nprint(x == None).\nprint(x is None).')
        self.assertOutputExact(out, ['True', 'True'])

    def test_bool_type(self):
        out, _ = self.run_code('print(type(True)).\nprint(type(False)).')
        self.assertOutputExact(out, ['bool', 'bool'])

    def test_and_both_true(self):
        """Both true → True."""
        out, _ = self.run_code('print(True and True).')
        self.assertFirstLine(out, 'True')

    def test_or_both_false(self):
        """Both false → False."""
        out, _ = self.run_code('print(False or False).')
        self.assertFirstLine(out, 'False')


# ═════════════════════════════════════════════════════════════
#  STRINGS
# ═════════════════════════════════════════════════════════════

class TestStrings(TestInterpreter):
    """String literals, escape sequences, concatenation, methods."""

    def test_double_quoted(self):
        out, _ = self.run_code('print("hello").')
        self.assertFirstLine(out, 'hello')

    def test_single_quoted(self):
        out, _ = self.run_code("print('world').")
        self.assertFirstLine(out, 'world')

    def test_escape_newline(self):
        out, _ = self.run_code('print("a\\nb").')
        self.assertOutputExact(out, ['a', 'b'])

    def test_escape_tab(self):
        out, _ = self.run_code('print("a\\tb").')
        self.assertOutputContains(out, 'a\tb')

    def test_escape_backslash(self):
        out, _ = self.run_code('print("a\\\\b").')
        self.assertOutputContains(out, 'a\\b')

    def test_string_concatenation(self):
        out, _ = self.run_code('print("hello" + " " + "world").')
        self.assertFirstLine(out, 'hello world')

    def test_string_repetition(self):
        out, _ = self.run_code('print("ha" * 3).')
        self.assertFirstLine(out, 'hahaha')

    def test_string_upper(self):
        out, _ = self.run_code('print("hello".upper()).')
        self.assertFirstLine(out, 'HELLO')

    def test_string_lower(self):
        out, _ = self.run_code('print("HELLO".lower()).')
        self.assertFirstLine(out, 'hello')

    def test_string_strip(self):
        out, _ = self.run_code('print("  hi  ".strip()).')
        self.assertFirstLine(out, 'hi')

    def test_string_split(self):
        out, _ = self.run_code('print("a,b,c".split(",")).')
        self.assertOutputContains(out, 'a')
        self.assertOutputContains(out, 'b')
        self.assertOutputContains(out, 'c')

    def test_string_find(self):
        out, _ = self.run_code('print("hello world".find("world")).')
        self.assertFirstLine(out, '6')

    def test_string_replace(self):
        out, _ = self.run_code('print("hello world".replace("world", "earth")).')
        self.assertFirstLine(out, 'hello earth')

    def test_string_contains(self):
        out, _ = self.run_code('print("hello".contains("ell")).\nprint("hello".contains("xyz")).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_string_startswith(self):
        out, _ = self.run_code('print("hello".startswith("hel")).\nprint("hello".startswith("xyz")).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_string_endswith(self):
        out, _ = self.run_code('print("hello".endswith("llo")).\nprint("hello".endswith("xyz")).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_string_count(self):
        out, _ = self.run_code('print("banana".count("a")).')
        self.assertFirstLine(out, '3')

    def test_string_reverse(self):
        out, _ = self.run_code('print("hello".reverse()).')
        self.assertFirstLine(out, 'olleh')

    def test_string_len(self):
        out, _ = self.run_code('print(len("hello")).')
        self.assertFirstLine(out, '5')

    def test_string_title(self):
        out, _ = self.run_code('print("hello world".title()).')
        self.assertFirstLine(out, 'Hello World')

    def test_string_capitalize(self):
        out, _ = self.run_code('print("hello".capitalize()).')
        self.assertFirstLine(out, 'Hello')

    def test_string_isdigit(self):
        out, _ = self.run_code('print("123".isdigit()).\nprint("12a".isdigit()).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_string_isalpha(self):
        out, _ = self.run_code('print("abc".isalpha()).\nprint("ab1".isalpha()).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_string_type(self):
        out, _ = self.run_code('print(type("hello")).')
        self.assertFirstLine(out, 'str')

    def test_string_number_concat(self):
        out, _ = self.run_code('print("value: " + str(42)).')
        self.assertFirstLine(out, 'value: 42')

    def test_empty_string(self):
        out, _ = self.run_code('print(len("")).')
        self.assertFirstLine(out, '0')

    def test_string_slice(self):
        out, _ = self.run_code('print("hello world".slice(0, 5)).')
        self.assertFirstLine(out, 'hello')


# ═════════════════════════════════════════════════════════════
#  LISTS
# ═════════════════════════════════════════════════════════════

class TestLists(TestInterpreter):
    """List literals, methods, operations."""

    def test_list_literal(self):
        out, _ = self.run_code('var l = [1, 2, 3].\nprint(l).')
        self.assertOutputContains(out, '1')
        self.assertOutputContains(out, '2')
        self.assertOutputContains(out, '3')

    def test_empty_list(self):
        out, _ = self.run_code('var l = [].\nprint(len(l)).')
        self.assertFirstLine(out, '0')

    def test_list_append(self):
        out, _ = self.run_code('var l = [1, 2].\nl.append(3).\nprint(l).')
        self.assertOutputContains(out, '3')

    def test_list_pop(self):
        out, _ = self.run_code('var l = [1, 2, 3].\nprint(l.pop()).')
        self.assertFirstLine(out, '3')

    def test_list_len(self):
        out, _ = self.run_code('print(len([10, 20, 30])).')
        self.assertFirstLine(out, '3')

    def test_list_concat(self):
        out, _ = self.run_code('print([1, 2] + [3, 4]).')
        self.assertOutputContains(out, '1')
        self.assertOutputContains(out, '4')

    def test_list_repetition(self):
        out, _ = self.run_code('print([0] * 3).')
        self.assertOutputContains(out, '0')

    def test_list_contains(self):
        out, _ = self.run_code('print([1, 2, 3].contains(2)).\nprint([1, 2, 3].contains(5)).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_list_index(self):
        out, _ = self.run_code('print([10, 20, 30].index(20)).')
        self.assertFirstLine(out, '1')

    def test_list_count(self):
        out, _ = self.run_code('print([1, 2, 2, 3, 2].count(2)).')
        self.assertFirstLine(out, '3')

    def test_list_reverse(self):
        out, _ = self.run_code('print([1, 2, 3].reverse()).')
        got = self.lines(out)
        self.assertIn('3', got[0])

    def test_list_sort(self):
        out, _ = self.run_code('print([3, 1, 2].sort()).')
        got = self.lines(out)
        self.assertIn('1', got[0])

    def test_list_mixed_types(self):
        out, _ = self.run_code('var l = [1, "hello", True, 3.14].\nprint(len(l)).')
        self.assertFirstLine(out, '4')

    def test_nested_list(self):
        out, _ = self.run_code('var l = [[1, 2], [3, 4]].\nprint(len(l)).')
        self.assertFirstLine(out, '2')

    def test_list_index_access(self):
        out, _ = self.run_code('var a = [10, 20, 30].\nprint(a.index(20)).')
        self.assertFirstLine(out, '1')

    def test_sorted_builtin(self):
        out, _ = self.run_code('print(sorted([3, 1, 2])).')
        got = self.lines(out)
        self.assertIn('1', got[0])

    def test_reversed_builtin(self):
        out, _ = self.run_code('print(reversed([1, 2, 3])).')
        got = self.lines(out)
        self.assertIn('3', got[0])

    def test_sum_builtin(self):
        out, _ = self.run_code('print(sum([1, 2, 3, 4])).')
        self.assertFirstLine(out, '10')


# ═════════════════════════════════════════════════════════════
#  SETS
# ═════════════════════════════════════════════════════════════

class TestSets(TestInterpreter):
    """Set literals and methods."""

    def test_set_literal(self):
        out, _ = self.run_code('var s = {1, 2, 3}.\nprint(len(s)).')
        self.assertFirstLine(out, '3')

    def test_set_deduplication(self):
        out, _ = self.run_code('var s = {1, 1, 2, 2, 3}.\nprint(len(s)).')
        self.assertFirstLine(out, '3')

    def test_set_add(self):
        out, _ = self.run_code('var s = {1, 2}.\nvar s2 = s.add(3).\nprint(len(s2)).')
        self.assertFirstLine(out, '3')

    def test_set_contains(self):
        out, _ = self.run_code('print({1, 2, 3}.contains(2)).\nprint({1, 2, 3}.contains(5)).')
        self.assertOutputExact(out, ['True', 'False'])

    def test_set_remove(self):
        out, _ = self.run_code('var s = {1, 2, 3}.\nvar s2 = s.remove(2).\nprint(len(s2)).')
        self.assertFirstLine(out, '2')


# ═════════════════════════════════════════════════════════════
#  FUNCTIONS
# ═════════════════════════════════════════════════════════════

class TestFunctions(TestInterpreter):
    """Function definition, calling, give, overloading, pass-by-ref."""

    def test_basic_function(self):
        out, _ = self.run_code('fn greet(): print("hi") ;\ngreet().')
        self.assertFirstLine(out, 'hi')

    def test_function_with_params(self):
        out, _ = self.run_code('fn add(a, b): give a + b ;\nprint(add(3, 4)).')
        self.assertFirstLine(out, '7')

    def test_give_no_parens(self):
        out, _ = self.run_code('fn dbl(x): give x * 2 ;\nprint(dbl(5)).')
        self.assertFirstLine(out, '10')

    def test_give_with_parens(self):
        out, _ = self.run_code('fn dbl(x): give(x * 2) ;\nprint(dbl(5)).')
        self.assertFirstLine(out, '10')

    def test_give_complex_expression(self):
        out, _ = self.run_code('fn calc(a, b): give a * 2 + b * 3 ;\nprint(calc(5, 10)).')
        self.assertFirstLine(out, '40')

    def test_function_returns_none(self):
        out, _ = self.run_code('fn noop(): pass ;\nprint(noop()).')
        self.assertFirstLine(out, 'None')

    def test_function_no_give_returns_none(self):
        out, _ = self.run_code('fn side_effect(): var x = 42 ;\nprint(side_effect()).')
        self.assertFirstLine(out, 'None')

    def test_overloading_by_arity(self):
        out, _ = self.run_code("""
fn add(a, b): give a + b ;
fn add(a, b, c): give a + b + c ;
print(add(1, 2)).
print(add(1, 2, 3)).
""")
        self.assertOutputExact(out, ['3', '6'])

    def test_forward_declaration(self):
        out, _ = self.run_code("""
fn myFunc(a, b).
fn myFunc(a, b): give a + b ;
print(myFunc(10, 20)).
""")
        self.assertFirstLine(out, '30')

    def test_define_before_use(self):
        """Functions must be defined before use in --script mode."""
        out, _ = self.run_code("""
fn myFunc(x): give x * 10 ;
print(myFunc(5)).
""")
        self.assertFirstLine(out, '50')

    def test_function_redefinition(self):
        out, _ = self.run_code("""
fn f(a, b): give a ;
fn f(a, b): give b ;
print(f(1, 2)).
""")
        self.assertFirstLine(out, '2')

    def test_pass_by_reference(self):
        out, _ = self.run_code("""
fn increment(@x):
    x = x + 1.
;
var val = 10.
increment(val).
print(val).
""")
        self.assertFirstLine(out, '11')

    def test_pass_by_reference_swap(self):
        out, _ = self.run_code("""
fn swap(@a, @b):
    var temp = a.
    a = b.
    b = temp.
;
var x = 10.
var y = 20.
swap(x, y).
print(x).
print(y).
""")
        self.assertOutputExact(out, ['20', '10'])

    def test_pass_by_value_default(self):
        """Without @, params are by value — caller's variable unchanged."""
        out, _ = self.run_code("""
fn tryChange(x):
    x = 999.
;
var val = 10.
tryChange(val).
print(val).
""")
        self.assertFirstLine(out, '10')

    def test_recursive_function(self):
        out, _ = self.run_code("""
fn factorial(n):
    if n <= 1:
        give 1
    ;
    give n * factorial(n - 1)
;
print(factorial(5)).
""")
        self.assertFirstLine(out, '120')

    def test_recursive_fibonacci(self):
        out, _ = self.run_code("""
fn fib(n):
    if n <= 0: give 0 ;
    if n == 1: give 1 ;
    give fib(n - 1) + fib(n - 2)
;
print(fib(10)).
""")
        self.assertFirstLine(out, '55')

    def test_duplicate_param_error(self):
        out, _ = self.run_code('fn f(a, a): give a ;')
        self.assertOutputHasError(out, 'Duplicate parameter')

    def test_function_scope_isolation(self):
        """Variables inside functions don't leak to outer scope."""
        out, _ = self.run_code("""
fn f():
    var inner_var = 42.
;
f().
print(inner_var).
""")
        # inner_var should be undefined → None (auto-create as None)
        self.assertFirstLine(out, 'None')

    def test_function_accesses_outer_scope(self):
        """Functions can read variables from outer scope."""
        out, _ = self.run_code("""
var outer = 100.
fn getOuter(): give outer ;
print(getOuter()).
""")
        self.assertFirstLine(out, '100')

    def test_unknown_function_error(self):
        out, _ = self.run_code('nonexistent().')
        self.assertOutputHasError(out, 'Unknown function')


# ═════════════════════════════════════════════════════════════
#  CONTROL FLOW — IF / ELIF / ELSE
# ═════════════════════════════════════════════════════════════

class TestConditionals(TestInterpreter):
    """If/elif/else statements."""

    def test_simple_if(self):
        out, _ = self.run_code("""
if True:
    print("yes")
;
""")
        self.assertFirstLine(out, 'yes')

    def test_if_false(self):
        out, _ = self.run_code("""
if False:
    print("no")
;
print("after").
""")
        self.assertFirstLine(out, 'after')

    def test_if_else(self):
        out, _ = self.run_code("""
if False:
    print("if")
else:
    print("else")
;
""")
        self.assertFirstLine(out, 'else')

    def test_if_elif_else(self):
        out, _ = self.run_code("""
var x = 2.
if x == 1:
    print("one")
elif x == 2:
    print("two")
else:
    print("other")
;
""")
        self.assertFirstLine(out, 'two')

    def test_multiple_elif(self):
        out, _ = self.run_code("""
var x = 3.
if x == 1:
    print("one")
elif x == 2:
    print("two")
elif x == 3:
    print("three")
else:
    print("other")
;
""")
        self.assertFirstLine(out, 'three')

    def test_nested_if(self):
        out, _ = self.run_code("""
var x = 5.
if x > 0:
    if x > 3:
        print("big")
    else:
        print("small")
    ;
;
""")
        self.assertFirstLine(out, 'big')

    def test_comparison_in_if(self):
        out, _ = self.run_code("""
var score = 85.
if score >= 90:
    print("A")
elif score >= 80:
    print("B")
elif score >= 70:
    print("C")
else:
    print("F")
;
""")
        self.assertFirstLine(out, 'B')


# ═════════════════════════════════════════════════════════════
#  CONTROL FLOW — FOR LOOPS
# ═════════════════════════════════════════════════════════════

class TestForLoops(TestInterpreter):
    """For loop: range(N), range(from..to), range(from..to step), for-in."""

    def test_range_simple(self):
        """range(N) → 0 to N (inclusive)."""
        out, _ = self.run_code("""
var s = 0.
for i in range(3):
    s += i.
;
print(s).
""")
        # 0 + 1 + 2 + 3 = 6
        self.assertFirstLine(out, '6')

    def test_range_from_to(self):
        out, _ = self.run_code("""
var s = 0.
for i in range(from 1 to 5):
    s += i.
;
print(s).
""")
        # 1 + 2 + 3 + 4 + 5 = 15
        self.assertFirstLine(out, '15')

    def test_range_step(self):
        out, _ = self.run_code("""
var items = [].
for i in range(from 0 to 10 step 2):
    items.append(i).
;
print(items).
""")
        self.assertOutputContains(out, '0')
        self.assertOutputContains(out, '2')
        self.assertOutputContains(out, '10')

    def test_range_reverse(self):
        out, _ = self.run_code("""
var items = [].
for i in range(from 5 to 1 step -1):
    items.append(i).
;
print(items).
""")
        self.assertOutputContains(out, '5')
        self.assertOutputContains(out, '1')

    def test_for_in_list(self):
        out, _ = self.run_code("""
for x in [10, 20, 30]:
    print(x).
;
""")
        self.assertOutputExact(out, ['10', '20', '30'])

    def test_for_in_string(self):
        out, _ = self.run_code("""
var s = "".
for ch in "abc":
    s = s + ch + "-".
;
print(s).
""")
        self.assertOutputContains(out, 'a-b-c-')

    def test_nested_loops(self):
        out, _ = self.run_code("""
var count = 0.
for i in range(from 1 to 3):
    for j in range(from 1 to 3):
        count++.
    ;
;
print(count).
""")
        # 3 * 3 = 9
        self.assertFirstLine(out, '9')

    def test_loop_variable_accumulation(self):
        out, _ = self.run_code("""
var result = "".
for i in range(from 1 to 5):
    result = result + str(i).
;
print(result).
""")
        self.assertFirstLine(out, '12345')


# ═════════════════════════════════════════════════════════════
#  CONTROL FLOW — WHILE LOOPS
# ═════════════════════════════════════════════════════════════

class TestWhileLoops(TestInterpreter):
    """While loops."""

    def test_basic_while(self):
        out, _ = self.run_code("""
var i = 0.
var s = 0.
while i < 5:
    s += i.
    i++.
;
print(s).
""")
        # 0 + 1 + 2 + 3 + 4 = 10
        self.assertFirstLine(out, '10')

    def test_while_false(self):
        """While with initially false condition doesn't execute."""
        out, _ = self.run_code("""
while False:
    print("never").
;
print("done").
""")
        self.assertFirstLine(out, 'done')

    def test_while_countdown(self):
        out, _ = self.run_code("""
var i = 5.
while i > 0:
    print(i).
    i--.
;
""")
        self.assertOutputExact(out, ['5', '4', '3', '2', '1'])

    def test_while_with_compound_condition(self):
        out, _ = self.run_code("""
var i = 0.
var found = False.
while i < 100 and not found:
    if i == 42:
        found = True.
    ;
    i++.
;
print(i).
""")
        self.assertFirstLine(out, '43')


# ═════════════════════════════════════════════════════════════
#  IMPLICIT MULTIPLICATION
# ═════════════════════════════════════════════════════════════

class TestImplicitMultiplication(TestInterpreter):
    """Implicit multiplication: 3x, 2(expr), etc."""

    def test_number_times_variable(self):
        out, _ = self.run_code('var x = 5.\nprint(2x).')
        self.assertFirstLine(out, '10')

    def test_number_times_paren(self):
        out, _ = self.run_code('print(2(3 + 4)).')
        self.assertFirstLine(out, '14')

    def test_paren_times_paren(self):
        out, _ = self.run_code('print((2 + 1)(3 + 1)).')
        self.assertFirstLine(out, '12')


# ═════════════════════════════════════════════════════════════
#  OF KEYWORD
# ═════════════════════════════════════════════════════════════

class TestOfKeyword(TestInterpreter):
    """The 'of' keyword for reversed method call syntax."""

    def test_of_with_method(self):
        out, _ = self.run_code('var name = "hello".\nprint(upper() of name).')
        self.assertFirstLine(out, 'HELLO')

    def test_of_with_method_args(self):
        out, _ = self.run_code('var s = "hello world".\nprint(replace("world", "earth") of s).')
        self.assertFirstLine(out, 'hello earth')

    def test_of_with_string_literal(self):
        out, _ = self.run_code('print(upper() of "hi").')
        self.assertFirstLine(out, 'HI')


# ═════════════════════════════════════════════════════════════
#  COMMENTS & SYNTAX
# ═════════════════════════════════════════════════════════════

class TestSyntax(TestInterpreter):
    """Comments, line continuation, newlines, dot terminator."""

    def test_comments(self):
        out, _ = self.run_code('--> This is a comment <--\nprint("visible").')
        self.assertFirstLine(out, 'visible')

    def test_hash_comment(self):
        """Single-line # comment should be ignored."""
        out, _ = self.run_code('# This is a comment\nprint("hello").')
        self.assertFirstLine(out, 'hello')

    def test_hash_comment_after_code(self):
        """# comment at end of line (after code) should be ignored."""
        out, _ = self.run_code('var x = 42  # set x\nprint(x).')
        self.assertFirstLine(out, '42')

    def test_hash_comment_multiple_lines(self):
        """Multiple consecutive # comments."""
        out, _ = self.run_code('# line 1\n# line 2\n# line 3\nprint("ok").')
        self.assertFirstLine(out, 'ok')

    def test_multiline_comment(self):
        out, _ = self.run_code("""--> This is a
multiline
comment <--
print("after comment").
""")
        self.assertFirstLine(out, 'after comment')

    def test_line_continuation(self):
        out, _ = self.run_code('print(1 + `\n2 + `\n3).')
        self.assertFirstLine(out, '6')

    def test_newline_as_terminator(self):
        """Newlines can act as statement terminators."""
        out, _ = self.run_code('var x = 10\nprint(x)\n')
        self.assertFirstLine(out, '10')

    def test_dot_terminator(self):
        out, _ = self.run_code('var x = 10. print(x).')
        self.assertFirstLine(out, '10')

    def test_pass_statement(self):
        out, _ = self.run_code('pass.\nprint("after pass").')
        self.assertFirstLine(out, 'after pass')

    def test_empty_function_body_error(self):
        """Empty function bodies should require 'pass'."""
        out, _ = self.run_code('fn f(): ;')
        self.assertOutputHasError(out, 'Empty function body')


# ═════════════════════════════════════════════════════════════
#  TYPE CONVERSIONS
# ═════════════════════════════════════════════════════════════

class TestTypeConversions(TestInterpreter):
    """Builtin type conversion functions."""

    def test_int_from_float(self):
        out, _ = self.run_code('print(int(3.7)).')
        self.assertFirstLine(out, '3')

    def test_int_from_double(self):
        out, _ = self.run_code('print(int(42.9)).')
        self.assertFirstLine(out, '42')

    def test_float_from_int(self):
        out, _ = self.run_code('print(float(42)).')
        self.assertOutputContains(out, '42')

    def test_str_from_int(self):
        out, _ = self.run_code('print(str(42)).')
        self.assertFirstLine(out, '42')

    def test_bool_from_int(self):
        out, _ = self.run_code('print(bool(0)).\nprint(bool(1)).')
        self.assertOutputExact(out, ['False', 'True'])

    def test_type_function(self):
        out, _ = self.run_code('print(type(42)).\nprint(type("hello")).\nprint(type([1, 2])).')
        self.assertOutputExact(out, ['int', 'str', 'list'])


# ═════════════════════════════════════════════════════════════
#  MATH FUNCTIONS
# ═════════════════════════════════════════════════════════════

class TestMathFunctions(TestInterpreter):
    """Builtin math functions."""

    def test_abs(self):
        out, _ = self.run_code('print(abs(-5)).')
        self.assertFirstLine(out, '5')

    def test_sqrt(self):
        out, _ = self.run_code('print(sqrt(16)).')
        self.assertFirstLine(out, '4')

    def test_min_max(self):
        out, _ = self.run_code('print(min(3, 7)).\nprint(max(3, 7)).')
        self.assertOutputExact(out, ['3', '7'])

    def test_ceil_floor(self):
        out, _ = self.run_code('print(ceil(3.2)).\nprint(floor(3.8)).')
        self.assertOutputExact(out, ['4', '3'])

    def test_round(self):
        out, _ = self.run_code('print(round(3.7)).')
        self.assertFirstLine(out, '4')

    def test_sin_cos(self):
        out, _ = self.run_code('print(sin(0)).\nprint(cos(0)).')
        got = self.lines(out)
        self.assertIn('0', got[0])
        self.assertIn('1', got[1])

    def test_log(self):
        out, _ = self.run_code('print(log(1)).')
        self.assertFirstLine(out, '0')


# ═════════════════════════════════════════════════════════════
#  BUILTIN FUNCTIONS
# ═════════════════════════════════════════════════════════════

class TestBuiltins(TestInterpreter):
    """Various builtin functions."""

    def test_print(self):
        out, _ = self.run_code('print("hello world").')
        self.assertFirstLine(out, 'hello world')

    def test_len_list(self):
        out, _ = self.run_code('print(len([1, 2, 3])).')
        self.assertFirstLine(out, '3')

    def test_len_string(self):
        out, _ = self.run_code('print(len("hello")).')
        self.assertFirstLine(out, '5')

    def test_isinstance(self):
        out, _ = self.run_code('print(isinstance(42, "int")).\nprint(isinstance("hi", "str")).')
        self.assertOutputExact(out, ['True', 'True'])

    def test_range_list(self):
        out, _ = self.run_code('print(range_list(0, 5)).')
        self.assertOutputContains(out, '0')
        self.assertOutputContains(out, '5')

    def test_all_any(self):
        out, _ = self.run_code('print(all([True, True, True])).\nprint(any([False, False, True])).')
        self.assertOutputExact(out, ['True', 'True'])

    def test_all_false(self):
        out, _ = self.run_code('print(all([True, False, True])).\nprint(any([False, False, False])).')
        self.assertOutputExact(out, ['False', 'False'])

    def test_sum(self):
        out, _ = self.run_code('print(sum([1, 2, 3, 4, 5])).')
        self.assertFirstLine(out, '15')

    def test_sorted(self):
        out, _ = self.run_code('print(sorted([5, 3, 1, 4, 2])).')
        got = self.lines(out)
        self.assertTrue('1' in got[0] and '5' in got[0])

    def test_reversed(self):
        out, _ = self.run_code('print(reversed([1, 2, 3])).')
        got = self.lines(out)
        self.assertIn('3', got[0])

    def test_repr(self):
        out, _ = self.run_code('print(repr("hello")).')
        self.assertOutputContains(out, 'hello')

    def test_input_function_not_crash(self):
        """input() should exist but we can't test interactively."""
        # Just verify it doesn't crash during parsing
        out, _ = self.run_code('fn f(): var x = input("prompt: ") ;')
        # Should parse fine (no execution since we don't call f)
        self.assertEqual(self.lines(out), [])


# ═════════════════════════════════════════════════════════════
#  DOT METHODS ON TYPES
# ═════════════════════════════════════════════════════════════

class TestDotMethods(TestInterpreter):
    """Dot-method dispatch on various types."""

    def test_type_method(self):
        out, _ = self.run_code('var x = 42.\nprint(x.type()).')
        self.assertFirstLine(out, 'int')

    def test_str_method(self):
        out, _ = self.run_code('var x = 42.\nprint(x.str()).')
        self.assertFirstLine(out, '42')

    def test_is_int_method(self):
        out, _ = self.run_code('var x = 42.\nprint(x.is_int()).')
        self.assertFirstLine(out, 'True')

    def test_is_string_method(self):
        out, _ = self.run_code('var x = "hi".\nprint(x.is_string()).')
        self.assertFirstLine(out, 'True')

    def test_is_none_method(self):
        out, _ = self.run_code('var x.\nprint(x.is_none()).')
        self.assertFirstLine(out, 'True')

    def test_is_list_method(self):
        out, _ = self.run_code('var x = [1, 2].\nprint(x.is_list()).')
        self.assertFirstLine(out, 'True')

    def test_to_double_method(self):
        out, _ = self.run_code('var x = 42.\nprint(x.toDouble()).')
        self.assertOutputContains(out, '42')


# ═════════════════════════════════════════════════════════════
#  NONE TYPE
# ═════════════════════════════════════════════════════════════

class TestNoneType(TestInterpreter):
    """None type behavior."""

    def test_none_literal(self):
        out, _ = self.run_code('print(None).')
        self.assertFirstLine(out, 'None')

    def test_var_default_is_none(self):
        out, _ = self.run_code('var x.\nprint(x).\nprint(x == None).')
        self.assertOutputExact(out, ['None', 'True'])

    def test_none_is_falsy(self):
        out, _ = self.run_code("""
var x.
if x:
    print("truthy")
else:
    print("falsy")
;
""")
        self.assertFirstLine(out, 'falsy')

    def test_function_no_give_is_none(self):
        out, _ = self.run_code("""
fn noop():
    var x = 1.
;
var result = noop().
print(result).
print(result == None).
""")
        self.assertOutputExact(out, ['None', 'True'])


# ═════════════════════════════════════════════════════════════
#  EDGE CASES & STRESS TESTS
# ═════════════════════════════════════════════════════════════

class TestEdgeCases(TestInterpreter):
    """Edge cases, weird syntax, and potential parser-breakers."""

    def test_deeply_nested_expression(self):
        out, _ = self.run_code('print(((((1 + 2) * 3) - 4) / 5) + 6).')
        got = self.lines(out)
        # ((((3)*3) - 4) / 5) + 6 = (9 - 4) / 5 + 6 = 1 + 6 = 7
        self.assertIn('7', got[0])

    def test_chained_string_methods(self):
        out, _ = self.run_code('print("  Hello World  ".strip().lower()).')
        self.assertFirstLine(out, 'hello world')

    def test_chained_list_operations(self):
        out, _ = self.run_code('print([3, 1, 2].sort().reverse()).')
        got = self.lines(out)
        # sort returns [1,2,3], reverse returns [3,2,1]
        self.assertIn('3', got[0])

    def test_empty_string_operations(self):
        out, _ = self.run_code('print(len("")).')
        self.assertFirstLine(out, '0')

    def test_string_with_numbers(self):
        out, _ = self.run_code('print("abc" + str(123) + "def").')
        self.assertFirstLine(out, 'abc123def')

    def test_boolean_in_arithmetic(self):
        out, _ = self.run_code('print(True + True).\nprint(False + 1).')
        self.assertOutputExact(out, ['2', '1'])

    def test_many_variables(self):
        code = ''
        for i in range(50):
            code += f'var v{i} = {i}.\n'
        code += 'var total = 0.\n'
        for i in range(50):
            code += f'total += v{i}.\n'
        code += 'print(total).\n'
        out, _ = self.run_code(code)
        # sum 0..49 = 1225
        self.assertFirstLine(out, '1225')

    def test_long_string(self):
        out, _ = self.run_code('print("a" * 100).')
        got = self.lines(out)
        self.assertEqual(len(got[0]), 100)

    def test_mixed_quotes_in_string(self):
        out, _ = self.run_code("print(\"it's\").")
        self.assertFirstLine(out, "it's")

    def test_single_quotes_with_double_inside(self):
        out, _ = self.run_code("print('he said \"hi\"').")
        self.assertFirstLine(out, 'he said "hi"')

    def test_expression_auto_print_none_suppressed(self):
        """None results should NOT be auto-printed by ExprStmt."""
        out, _ = self.run_code("""
fn noop(): pass ;
noop().
print("done").
""")
        self.assertOutputExact(out, ['done'])

    def test_multiple_statements_one_line(self):
        out, _ = self.run_code('var x = 1. var y = 2. print(x + y).')
        self.assertFirstLine(out, '3')

    def test_print_multiple_types(self):
        out, _ = self.run_code("""
print(42).
print(3.14).
print("hello").
print(True).
print(None).
print([1, 2]).
print({3, 4}).
""")
        got = self.lines(out)
        self.assertEqual(got[0], '42')
        self.assertIn('3.14', got[1])
        self.assertEqual(got[2], 'hello')
        self.assertEqual(got[3], 'True')
        self.assertEqual(got[4], 'None')

    def test_operator_chaining(self):
        out, _ = self.run_code('print(1 + 2 + 3 + 4 + 5).')
        self.assertFirstLine(out, '15')

    def test_multiplication_chain(self):
        out, _ = self.run_code('print(2 * 3 * 4).')
        self.assertFirstLine(out, '24')

    def test_mixed_operators(self):
        out, _ = self.run_code('print(10 + 5 * 2 - 3).')
        self.assertFirstLine(out, '17')

    def test_negative_loop_range(self):
        out, _ = self.run_code("""
var items = [].
for i in range(from 3 to 1 step -1):
    items.append(i).
;
print(items).
""")
        self.assertOutputContains(out, '3')
        self.assertOutputContains(out, '1')


# ═════════════════════════════════════════════════════════════
#  ERROR HANDLING
# ═════════════════════════════════════════════════════════════

class TestErrors(TestInterpreter):
    """Error messages and error paths."""

    def test_division_by_zero_error(self):
        out, _ = self.run_code('print(1 / 0).')
        self.assertOutputHasError(out, 'Division by zero')

    def test_unknown_function_error(self):
        out, _ = self.run_code('noSuchFunction().')
        self.assertOutputHasError(out, 'Unknown function')

    def test_wrong_arity_error(self):
        out, _ = self.run_code("""
fn f(a, b): give a + b ;
f(1).
""")
        self.assertOutputHasError(out, 'Unknown function')

    def test_unterminated_string_error(self):
        out, _ = self.run_code('print("unterminated).')
        self.assertOutputHasError(out, 'Unterminated string')

    def test_duplicate_param(self):
        out, _ = self.run_code('fn f(x, x): pass ;')
        self.assertOutputHasError(out, 'Duplicate parameter')

    def test_empty_function_body(self):
        out, _ = self.run_code('fn f(): ;')
        self.assertOutputHasError(out, 'Empty function body')

    def test_zero_step_error(self):
        out, _ = self.run_code("""
for i in range(from 1 to 10 step 0):
    print(i).
;
""")
        self.assertOutputHasError(out, 'Step cannot be zero')


# ═════════════════════════════════════════════════════════════
#  COMPLEX PROGRAMS
# ═════════════════════════════════════════════════════════════

class TestComplexPrograms(TestInterpreter):
    """Full programs combining multiple features."""

    def test_fizzbuzz(self):
        out, _ = self.run_code("""
for i in range(from 1 to 15):
    if i % 15 == 0:
        print("FizzBuzz")
    elif i % 3 == 0:
        print("Fizz")
    elif i % 5 == 0:
        print("Buzz")
    else:
        print(i)
    ;
;
""")
        got = self.lines(out)
        self.assertEqual(got[0], '1')
        self.assertEqual(got[1], '2')
        self.assertEqual(got[2], 'Fizz')
        self.assertEqual(got[3], '4')
        self.assertEqual(got[4], 'Buzz')
        self.assertEqual(got[14], 'FizzBuzz')

    def test_sum_of_squares(self):
        out, _ = self.run_code("""
fn sumOfSquares(n):
    var total = 0.
    for i in range(from 1 to n):
        total += i * i.
    ;
    give total
;
print(sumOfSquares(5)).
""")
        # 1 + 4 + 9 + 16 + 25 = 55
        self.assertFirstLine(out, '55')

    def test_string_processing(self):
        out, _ = self.run_code("""
var words = "hello world foo bar".split(" ").
for word in words:
    print(word.upper()).
;
""")
        self.assertOutputExact(out, ['HELLO', 'WORLD', 'FOO', 'BAR'])

    def test_function_composition(self):
        out, _ = self.run_code("""
fn dbl(x): give x * 2 ;
fn inc(x): give x + 1 ;
fn apply(x):
    give inc(dbl(x))
;
print(apply(5)).
""")
        # dbl(5) = 10, inc(10) = 11
        self.assertFirstLine(out, '11')

    def test_accumulator_pattern(self):
        out, _ = self.run_code("""
var total = 0.
var items = [10, 20, 30, 40, 50].
for item in items:
    total += item.
;
print(total).
""")
        self.assertFirstLine(out, '150')

    def test_grade_calculator(self):
        out, _ = self.run_code("""
fn grade(score):
    if score >= 90: give "A" ;
    if score >= 80: give "B" ;
    if score >= 70: give "C" ;
    if score >= 60: give "D" ;
    give "F"
;
print(grade(95)).
print(grade(85)).
print(grade(75)).
print(grade(65)).
print(grade(50)).
""")
        self.assertOutputExact(out, ['A', 'B', 'C', 'D', 'F'])

    def test_counter_with_while(self):
        out, _ = self.run_code("""
var count = 0.
var i = 1.
while i <= 100:
    if i % 7 == 0:
        count++.
    ;
    i++.
;
print(count).
""")
        # Numbers 1-100 divisible by 7: 7, 14, 21, 28, 35, 42, 49, 56, 63, 70, 77, 84, 91, 98 = 14
        self.assertFirstLine(out, '14')

    def test_pass_by_ref_accumulate(self):
        out, _ = self.run_code("""
fn addTo(@total, val):
    total = total + val.
;
var sum = 0.
addTo(sum, 10).
addTo(sum, 20).
addTo(sum, 30).
print(sum).
""")
        self.assertFirstLine(out, '60')

    def test_list_building_with_functions(self):
        out, _ = self.run_code("""
fn sumRange(n):
    var total = 0.
    for i in range(from 1 to n):
        total += i.
    ;
    give total
;
print(sumRange(5)).
""")
        # 1+2+3+4+5 = 15
        self.assertFirstLine(out, '15')

    def test_recursive_power(self):
        out, _ = self.run_code("""
fn power(base, exp):
    if exp == 0: give 1 ;
    give base * power(base, exp - 1)
;
print(power(2, 10)).
""")
        self.assertFirstLine(out, '1024')

    def test_overloaded_functions_program(self):
        out, _ = self.run_code("""
fn describe(x):
    give "one arg: " + str(x)
;
fn describe(x, y):
    give "two args: " + str(x) + ", " + str(y)
;
fn describe(x, y, z):
    give "three args: " + str(x) + ", " + str(y) + ", " + str(z)
;
print(describe(1)).
print(describe(1, 2)).
print(describe(1, 2, 3)).
""")
        self.assertOutputExact(out, [
            'one arg: 1',
            'two args: 1, 2',
            'three args: 1, 2, 3'
        ])


# ═════════════════════════════════════════════════════════════
#  MULTI-LINE & FORMATTING
# ═════════════════════════════════════════════════════════════

class TestMultiLine(TestInterpreter):
    """Multi-line code, indentation, block structure."""

    def test_multiline_function(self):
        out, _ = self.run_code("""
fn calculate(a, b, c):
    var sum = a + b + c.
    var avg = sum / 3.
    give avg
;
print(calculate(10, 20, 30)).
""")
        self.assertFirstLine(out, '20')

    def test_deeply_nested_blocks(self):
        out, _ = self.run_code("""
var result = 0.
for i in range(from 1 to 3):
    for j in range(from 1 to 3):
        if i == j:
            result += i * j.
        ;
    ;
;
print(result).
""")
        # i==j: (1,1)=1, (2,2)=4, (3,3)=9 → 14
        self.assertFirstLine(out, '14')

    def test_function_calling_function(self):
        out, _ = self.run_code("""
fn square(x): give x * x ;
fn sumSquares(a, b): give square(a) + square(b) ;
print(sumSquares(3, 4)).
""")
        # 9 + 16 = 25
        self.assertFirstLine(out, '25')


if __name__ == '__main__':
    unittest.main()
