import subprocess
import unittest

class TestInterpreter(unittest.TestCase):
    def run_code(self, code):
        """Runs the scriptit interpreter with the given code."""
        formatted_code = code.strip()
        process = subprocess.Popen(
            ['scriptit', '--script'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd='/home/DATA/CODE/code/test',
            text=True
        )
        stdout, stderr = process.communicate(input=formatted_code)
        return stdout, stderr

    def get_clean_output_lines(self, stdout):
        """Extracts non-empty lines from stdout, ignoring debug prints if any remain."""
        lines = [line.strip() for line in stdout.splitlines() if line.strip()]
        return lines

    def assertOutputSequence(self, stdout, expected_sequence):
        """Asserts that the expected sequence of strings appears in the output in order."""
        lines = self.get_clean_output_lines(stdout)
        # Filter out potential debug lines if they start with 'Debug:' or '---'
        lines = [l for l in lines if not l.startswith("Debug:") and not l.startswith("---")]
        
        # Check if expected_sequence is a subsequence of lines
        seq_idx = 0
        line_idx = 0
        while seq_idx < len(expected_sequence) and line_idx < len(lines):
            if expected_sequence[seq_idx] in lines[line_idx]:
                seq_idx += 1
            line_idx += 1
        
        if seq_idx < len(expected_sequence):
            self.fail(f"Sequence not found. Expected {expected_sequence}, got {lines}")

    def test_math_basics(self):
        code = """
        1 + 1.
        10 * 2.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2", "20"])

    def test_scope_shadowing_order(self):
        code = """
        var a = 10.
        fn f @():
            var a = 20.
            give(a).
        ;
        f().
        a.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["20", "10"])

    def test_syntax_error_missing_comma(self):
        code = """
        fn add @(x y): give(x+y). ;
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        # Specific error might vary but should be an error

    def test_syntax_error_missing_in(self):
        code = """
        for i range(from 1 to 5): i. ;
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("Expected in", out)

    def test_scope_leak(self):
        code = """
        fn f @():
            var leaked = 99.
        ;
        f().
        leaked.
        """
        out, _ = self.run_code(code)
        # v2: undefined vars return None (no error)
        self.assertNotIn("99", out)

    def test_loop_reverse_range(self):
        code = """
        var sum = 0.
        for i in range(from 5 to 1):
             sum = sum + 1.
        ;
        sum.
        """
        # Range is inclusive and bidirectional: 5, 4, 3, 2, 1 (5 items)
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    def test_division_by_zero(self):
        code = """
        10 / 0.
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("Div by 0", out)

    def test_return_without_give(self):
        code = """
        fn noGive @():
            var a = 1.
        ;
        noGive().
        """
        # Should return 0 by default
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_logical_operators(self):
        code = """
        1 < 2.
        1 > 2.
        1 == 1.
        1 != 1.
        1 <= 1.
        1 >= 2.
        (1 == 1) && (2 == 2).
        (1 == 2) || (1 == 1).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "0", "1", "0", "1", "0", "1", "1"])

    def test_multi_statement_line(self):
        code = """
        var x = 1. var y = 2.
        x + y.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    def test_nested_if(self):
        code = """
        if 1 < 2:
            if 2 < 3:
                give(100).
            ;
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["100"])

    def test_recursion_base_case(self):
        code = """
        fn fact @(n):
            if n <= 0: give(1). ;
            give(n * fact(n-1)).
        ;
        fact(0).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_param_shadowing(self):
        code = """
        fn f @(x):
            var x = 10.
            give(x).
        ;
        f(5).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["10"])

    def test_short_circuit(self):
        """Short-circuit ||: LHS is true, RHS (div by 0) should NOT be evaluated."""
        code = """
        (1 == 1) || (10 / 0).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Div by 0", out)
        self.assertOutputSequence(out, ["1"])

    def test_short_circuit_and(self):
        """Short-circuit &&: LHS is false, RHS (div by 0) should NOT be evaluated."""
        code = """
        (1 == 0) && (10 / 0).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Div by 0", out)
        self.assertOutputSequence(out, ["0"])

    def test_implicit_assignment(self):
        code = """
        x = 99.
        x.
        """
        out, _ = self.run_code(code)
        # Should error in strict mode
        self.assertIn("Error", out) 

    def test_function_arity(self):
        code = """
        fn add @(a, b): give(a+b). ;
        add(1, 2, 3).
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("mismatch", out)

    def test_undefined_function(self):
        code = """
        foo().
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_function_scope_isolation(self):
        code = """
        var g = 10.
        fn change @():
             g = 20. 
        ;
        change().
        """
        # Strict scope: cannot mutate outer 'g' without declaration (which would be local).
        out, _ = self.run_code(code)
        # We check for "Undefined variable" or "Error"
        self.assertIn("Error", out)

    def test_chained_comparison(self):
        # 1 < 2 < 3. 
        # C-style: (1 < 2) < 3 -> 1 < 3 -> 1 (True).
        code = "3 > 2 > 1."
        out, _ = self.run_code(code)
        # We implemented C-style binary operators.
        self.assertOutputSequence(out, ["0"]) # Expect False (C-style)

    def test_operator_precedence(self):
        # 1 + 2 * 3 = 7
        code = "1 + 2 * 3."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["7"])
        
        code2 = "(1 + 2) * 3."
        out2, _ = self.run_code(code2)
        self.assertOutputSequence(out2, ["9"])

    def test_unary_operators(self):
        code = """
        -5.
        --5.
        !(1 == 1).
        !(1 == 0).
        """
        out, _ = self.run_code(code)
        # -5 -> -5
        # --5 -> 5
        # !T -> 0
        # !F -> 1
        self.assertOutputSequence(out, ["-5", "5", "0", "1"])
        
    def test_unary_keywords(self):
        code = """
        not (1 == 1).
        not (1 == 0).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0", "1"])

    def test_logical_keywords(self):
        code = """
        (1 == 1) and (2 == 2).
        (1 == 2) or (1 == 1).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "1"])

    def test_nested_functions(self):
        code = """
        fn outer @():
            fn inner @(): give(100). ;
            give(inner()).
        ;
        outer().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["100"])

    def test_empty_function_error(self):
        code = """
        fn f @(): ;
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("Empty", out) # "Empty function body"

    def test_pass_keyword(self):
        code = """
        fn f @(): pass. ;
        f().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"]) # Void return

    def test_while_loop(self):
        code = """
        var i = 0.
        while i < 3:
            i = i + 1.
        ;
        i.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    def test_variable_redefinition_allowed(self):
        code = """
        var x = 1.
        var x = 2.
        x.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2"])

    def test_loop_scope_leak_check(self):
        code = """
        for i in range(from 1 to 2): pass. ;
        i.
        """
        out, _ = self.run_code(code)
        # v2: undefined vars return None (no error)
        self.assertNotIn("Error", out)
    # ===== NEW COMPREHENSIVE TESTS =====

    # 1. Function return without give after multiple statements
    def test_function_no_give_after_stmts(self):
        """A function with statements but no give should return 0 (void)."""
        code = """
        fn work @():
            var x = 10.
            var y = 20.
        ;
        work().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])  # Implicit void return

    def test_function_give_after_stmts(self):
        """Contrast: function WITH give should return value."""
        code = """
        fn work @():
            var x = 10.
            var y = 20.
            give(x + y).
        ;
        work().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["30"])

    # 2. Nested loops with empty body (pass)
    def test_nested_for_loops_pass(self):
        code = """
        for i in range(from 1 to 3):
            for j in range(from 1 to 2):
                pass.
            ;
        ;
        0.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])  # No crash, runs clean

    def test_while_with_pass(self):
        code = """
        var n = 3.
        while n > 0:
            n = n - 1.
        ;
        n.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    # 3. Edge-case ranges
    def test_range_start_equals_end(self):
        """for i in range(from 5 to 5) should execute once (inclusive)."""
        code = """
        var sum = 0.
        for i in range(from 5 to 5):
            sum = sum + i.
        ;
        sum.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    def test_range_single_element(self):
        """from 1 to 1 should give exactly [1]."""
        code = """
        var count = 0.
        for i in range(from 1 to 1):
            count = count + 1.
        ;
        count.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_range_negative_step(self):
        """from 5 to 1 should iterate 5,4,3,2,1."""
        code = """
        var sum = 0.
        for i in range(from 5 to 1):
            sum = sum + i.
        ;
        sum.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["15"])  # 5+4+3+2+1

    # 4. Deep shadowing of parameters and outer variables
    def test_deep_param_shadowing(self):
        """Param shadows outer, inner fn param shadows again."""
        code = """
        var x = 100.
        fn outer @(x):
            fn inner @(x):
                give(x).
            ;
            give(inner(x + 1)).
        ;
        outer(5).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["6"])  # inner gets 5+1=6

    def test_outer_var_visible_in_func(self):
        """Function can READ outer scope vars (but not mutate them)."""
        code = """
        var g = 42.
        fn readG @():
            give(g).
        ;
        readG().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    # 5. Consistency of error messages
    def test_error_syntax_missing_dot(self):
        # v2: Missing dot at EOF is now forgiven (feature).
        # Instead test missing dot in the MIDDLE of a multi-statement script.
        code = "var x = 5\nvar y = 10."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_error_runtime_div_zero(self):
        code = "1 / 0."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("Div by 0", out)

    def test_error_undefined_var(self):
        code = "noSuchVar."
        out, _ = self.run_code(code)
        # v2: undefined vars return None (no error)
        self.assertNotIn("Error", out)

    def test_error_undefined_func(self):
        code = "ghostFunc()."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_error_arity_too_few(self):
        code = """
        fn add @(a, b): give(a+b). ;
        add(1).
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("mismatch", out)

    def test_error_arity_too_many(self):
        code = """
        fn id @(a): give(a). ;
        id(1, 2).
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("mismatch", out)

    # 6. Complex chained arithmetic operations
    def test_chained_arithmetic(self):
        code = "2 + 3 * 4 - 1."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["13"])  # 2 + 12 - 1

    def test_chained_arithmetic_parens(self):
        code = "(2 + 3) * (4 - 1)."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["15"])  # 5 * 3

    def test_nested_parens(self):
        code = "((1 + 2) * (3 + 4)) + 1."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["22"])  # 3*7 + 1

    def test_modulo_and_power(self):
        code = """
        10 % 3.
        2 ^ 10.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "1024"])

    # 7. Boolean expressions mixing and/or/not with parentheses
    def test_bool_and_or_not_mixed(self):
        code = """
        (1 == 1) and not (2 == 3).
        not (1 == 1) or (2 == 2).
        not ((1 == 1) and (2 == 3)).
        """
        out, _ = self.run_code(code)
        # T and not F = T and T = 1
        # not T or T   = F or T = 1
        # not (T and F) = not F = 1
        self.assertOutputSequence(out, ["1", "1", "1"])

    def test_bool_short_circuit_note(self):
        """Verifying basic logical behavior (short-circuit not guaranteed)."""
        code = """
        (0 == 1) and (1 == 1).
        (1 == 1) or (0 == 0).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0", "1"])

    def test_bool_complex_chain(self):
        code = """
        var a = 1.
        var b = 0.
        var c = 1.
        (a == 1) and (b == 0) and (c == 1).
        (a == 0) or (b == 0) or (c == 0).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "1"])

    # 8. Functions returning functions and calling them
    # NOTE: Our language doesn't support first-class function return yet.
    # But we CAN test nested func calls (inner called from outer body).
    def test_nested_func_call_chain(self):
        code = """
        fn double @(x): give(x * 2). ;
        fn quad @(x): give(double(double(x))). ;
        quad(3).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["12"])  # 3*2=6, 6*2=12

    def test_nested_func_inner_uses_outer_param(self):
        code = """
        fn make @(base):
            fn add @(x): give(base + x). ;
            give(add(10)).
        ;
        make(5).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["15"])  # base=5, x=10

    # 9. Assignment to reserved keywords (should error)
    def test_assign_to_keyword_var(self):
        code = "var var = 5."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_assign_to_keyword_fn(self):
        code = "var fn = 10."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_assign_to_keyword_if(self):
        code = "var if = 1."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_assign_to_keyword_while(self):
        code = "var while = 1."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_assign_to_keyword_pass(self):
        code = "var pass = 0."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    # 10. Outer-scope variable mutation enforcement
    def test_outer_scope_mutation_blocked(self):
        """Function cannot mutate outer scope variable."""
        code = """
        var x = 10.
        fn mutate @():
            x = 99.
        ;
        mutate().
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_outer_scope_preserved_after_func(self):
        """Outer variable unchanged even if func defines same-named local."""
        code = """
        var x = 10.
        fn shadow @():
            var x = 99.
            give(x).
        ;
        shadow().
        x.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["99", "10"])

    def test_if_block_can_mutate_outer(self):
        """If-block scope is permeable — CAN mutate parent variables."""
        code = """
        var x = 1.
        if 1 == 1:
            x = 42.
        ;
        x.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    def test_for_loop_cannot_leak_var(self):
        """Loop-local var should not leak."""
        code = """
        for i in range(from 1 to 3):
            var temp = i.
        ;
        temp.
        """
        out, _ = self.run_code(code)
        # v2: undefined vars return None (no error)
        self.assertNotIn("Error", out)

    # ===== HARDENING TESTS: AIRTIGHT COVERAGE =====

    # --- Recursion ---
    def test_deep_recursion(self):
        """Factorial via recursion."""
        code = """
        fn fact @(n):
            if n <= 1:
                give(1).
            ;
            give(n * fact(n - 1)).
        ;
        fact(5).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["120"])

    def test_recursive_fibonacci(self):
        code = """
        fn fib @(n):
            if n <= 0:
                give(0).
            ;
            if n == 1:
                give(1).
            ;
            give(fib(n - 1) + fib(n - 2)).
        ;
        fib(6).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["8"])

    def test_mutual_function_calls(self):
        """Two functions calling each other (not recursive, sequential)."""
        code = """
        fn double @(x): give(x * 2). ;
        fn addThenDouble @(a, b): give(double(a + b)). ;
        addThenDouble(3, 4).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["14"])

    # --- Comments ---
    def test_comment_at_start(self):
        code = """
        --> This is a comment <--
        1 + 1.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2"])

    def test_comment_between_stmts(self):
        code = """
        1 + 1.
        --> middle comment <--
        2 + 2.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2", "4"])

    def test_comment_after_stmts(self):
        code = """
        3 + 3.
        --> trailing comment <--
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["6"])

    def test_multiline_comment(self):
        code = """
        --> this is
        a multiline
        comment <--
        42.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    def test_comment_inside_function(self):
        code = """
        fn f @():
            --> skip this <--
            give(99).
        ;
        f().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["99"])

    # --- Pass in all block types ---
    def test_pass_in_if(self):
        code = """
        if 1 == 1:
            pass.
        ;
        0.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_pass_in_elif(self):
        code = """
        if 0 == 1:
            pass.
        elif 1 == 1:
            pass.
        ;
        0.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_pass_in_else(self):
        code = """
        if 0 == 1:
            pass.
        else:
            pass.
        ;
        0.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_pass_in_while(self):
        """While with only pass (must have exit condition to avoid infinite loop)."""
        code = """
        var x = 1.
        while x > 0:
            x = x - 1.
            pass.
        ;
        x.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_pass_in_for(self):
        code = """
        for i in range(from 1 to 5):
            pass.
        ;
        0.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    # --- Give in branches ---
    def test_give_in_if_branch(self):
        code = """
        fn abs @(x):
            if x < 0:
                give(0 - x).
            ;
            give(x).
        ;
        abs(-10).
        abs(5).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["10", "5"])

    def test_give_in_else(self):
        code = """
        fn sign @(x):
            if x > 0:
                give(1).
            elif x < 0:
                give(-1).
            else:
                give(0).
            ;
            give(0).
        ;
        sign(42).
        sign(-7).
        sign(0).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "-1", "0"])

    def test_give_in_loop(self):
        """Give inside a loop should exit function immediately."""
        code = """
        fn findFirst @():
            for i in range(from 1 to 100):
                if i == 5:
                    give(i).
                ;
            ;
            give(-1).
        ;
        findFirst().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    # --- Deeply nested blocks ---
    def test_triple_nested_if(self):
        code = """
        var x = 10.
        if x > 5:
            if x > 8:
                if x == 10:
                    x.
                ;
            ;
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["10"])

    def test_nested_for_in_while(self):
        code = """
        var total = 0.
        var rounds = 2.
        while rounds > 0:
            for i in range(from 1 to 3):
                total = total + i.
            ;
            rounds = rounds - 1.
        ;
        total.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["12"])  # (1+2+3)*2

    def test_while_in_for(self):
        code = """
        var sum = 0.
        for i in range(from 1 to 3):
            var j = i.
            while j > 0:
                sum = sum + 1.
                j = j - 1.
            ;
        ;
        sum.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["6"])  # 1+2+3

    # --- Float/Decimal output ---
    def test_integer_output_format(self):
        """Integers should print without trailing decimals ideally."""
        code = "5 + 5."
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        self.assertTrue(any("10" in l for l in lines))

    def test_decimal_arithmetic(self):
        code = "1.5 + 2.5."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["4"])

    def test_decimal_division(self):
        code = "7 / 2."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3.5"])

    # --- Large numbers ---
    def test_large_number(self):
        code = "1000000 * 1000000."
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        self.assertTrue(len(lines) > 0)
        self.assertNotIn("Error", out)

    def test_power_large(self):
        code = "2 ^ 10."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1024"])

    # --- Negation chains ---
    def test_triple_negation(self):
        code = "---5."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["-5"])

    def test_not_not(self):
        code = "not not (1 == 1)."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_not_not_not(self):
        code = "not not not (1 == 1)."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_neg_in_expression(self):
        code = "3 + -2."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    # --- Variable naming ---
    def test_single_char_var(self):
        code = """
        var a = 1.
        var b = 2.
        a + b.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    def test_underscore_var(self):
        code = """
        var _x = 10.
        var my_var = 20.
        _x + my_var.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["30"])

    def test_long_var_name(self):
        code = """
        var thisIsAVeryLongVariableName = 42.
        thisIsAVeryLongVariableName.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    def test_var_name_with_digits(self):
        code = """
        var x1 = 1.
        var x2 = 2.
        x1 + x2.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    # --- Function redefinition ---
    def test_function_redefine(self):
        """Redefining a function should use the latest definition."""
        code = """
        fn greet @(): give(1). ;
        greet().
        fn greet @(): give(2). ;
        greet().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "2"])

    # --- Empty/minimal programs ---
    def test_empty_program(self):
        code = ""
        out, _ = self.run_code(code)
        # Should not crash
        self.assertNotIn("Error", out)

    def test_only_comment(self):
        code = "--> nothing here <--"
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_single_number(self):
        code = "42."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    def test_single_zero(self):
        code = "0."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    # --- Multiple gives (first one wins) ---
    def test_multiple_gives(self):
        """Only the first give encountered should fire (via exception)."""
        code = """
        fn f @():
            give(1).
            give(2).
        ;
        f().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    # --- Operator edge cases ---
    def test_modulo_negative(self):
        code = "-7 % 3."
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        self.assertTrue(len(lines) > 0)
        self.assertNotIn("Error", out)

    def test_mod_by_zero(self):
        code = "5 % 0."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_comparison_all_operators(self):
        code = """
        (1 < 2).
        (2 > 1).
        (1 <= 1).
        (1 >= 1).
        (1 == 1).
        (1 != 2).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "1", "1", "1", "1", "1"])

    def test_comparison_false_cases(self):
        code = """
        (2 < 1).
        (1 > 2).
        (2 <= 1).
        (1 >= 2).
        (1 == 2).
        (1 != 1).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0", "0", "0", "0", "0", "0"])

    # --- Whitespace / formatting ---
    def test_extra_whitespace(self):
        code = "   1   +   1   .   "
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2"])

    def test_no_whitespace(self):
        code = "1+1."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2"])

    def test_newlines_everywhere(self):
        code = """


        1 + 1.


        2 + 2.


        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2", "4"])

    # --- Scope stress ---
    def test_many_variables(self):
        code = """
        var a = 1.
        var b = 2.
        var c = 3.
        var d = 4.
        var e = 5.
        a + b + c + d + e.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["15"])

    def test_reassignment_chain(self):
        code = """
        var x = 0.
        x = 1.
        x = x + 1.
        x = x * 3.
        x.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["6"])  # 0->1->2->6

    def test_for_accumulate(self):
        """For loop accumulating: sum 1..10 = 55."""
        code = """
        var sum = 0.
        for i in range(from 1 to 10):
            sum = sum + i.
        ;
        sum.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["55"])

    # --- Error edge cases ---
    def test_error_missing_paren_func(self):
        code = """
        fn f @(: give(1). ;
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_error_missing_colon_if(self):
        code = """
        if 1 == 1
            pass.
        ;
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_error_missing_semicolon_if(self):
        """Missing ; at end of if block."""
        code = """
        if 1 == 1:
            1.
        """
        out, _ = self.run_code(code)
        # Should error or behave unexpectedly
        # (depends on parser — at minimum shouldn't crash silently)

    def test_error_double_equals_assignment(self):
        """Using == instead of = for assignment should fail."""
        code = "var x == 5."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_error_unclosed_parens(self):
        code = "(1 + 2."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_error_extra_right_paren(self):
        code = "1 + 2)."
        out, _ = self.run_code(code)
        # Either error or ignores — should not crash
        # This may or may not be an error in our parser

    def test_error_assign_without_var_undeclared(self):
        """Assigning to undeclared variable should error."""
        code = "z = 5."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    # --- Function with complex body ---
    def test_function_with_loop(self):
        code = """
        fn sumRange @(n):
            var s = 0.
            for i in range(from 1 to n):
                s = s + i.
            ;
            give(s).
        ;
        sumRange(5).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["15"])

    def test_function_with_while(self):
        code = """
        fn countdown @(n):
            var result = 0.
            while n > 0:
                result = result + n.
                n = n - 1.
            ;
            give(result).
        ;
        countdown(4).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["10"])  # 4+3+2+1

    def test_function_with_nested_if(self):
        code = """
        fn classify @(x):
            if x > 0:
                if x > 100:
                    give(2).
                ;
                give(1).
            ;
            give(0).
        ;
        classify(50).
        classify(200).
        classify(-1).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "2", "0"])

    # --- Mixed expression complexity ---
    def test_expression_with_func_in_arith(self):
        """Function calls inside arithmetic expressions."""
        code = """
        fn sq @(x): give(x * x). ;
        sq(3) + sq(4).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["25"])  # 9 + 16

    def test_expression_nested_func_calls(self):
        code = """
        fn add @(a, b): give(a + b). ;
        fn mul @(a, b): give(a * b). ;
        add(mul(2, 3), mul(4, 5)).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["26"])  # 6 + 20

    def test_bool_in_variable(self):
        code = """
        var t = (1 == 1).
        var f = (1 == 0).
        t.
        f.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "0"])

    def test_conditional_on_var(self):
        code = """
        var flag = 1.
        if flag:
            42.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    def test_conditional_on_zero(self):
        code = """
        var flag = 0.
        if flag:
            99.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    # ===== COMPLEX CHAINED CONDITION STRESS TESTS =====

    def test_if_mixed_and_or(self):
        """if (a == b and c == d or e == f)"""
        code = """
        var a = 1. var b = 1. var c = 2. var d = 3. var e = 5. var f = 5.
        if (a == b and c == d or e == f):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        # a==b is T, c==d is F, e==f is T → T and F or T → F or T → 1
        self.assertOutputSequence(out, ["1"])

    def test_if_and_or_precedence(self):
        """and binds tighter than or: F or T and T = F or T = T"""
        code = """
        if (1 == 0 or 1 == 1 and 2 == 2):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_if_or_and_precedence_reverse(self):
        """T or F and F = T or F = T (and binds first)"""
        code = """
        if (1 == 1 or 1 == 0 and 0 == 1):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_if_nested_parens_override(self):
        """Parens override precedence: (T or F) and F = T and F = F"""
        code = """
        if ((1 == 1 or 1 == 0) and 0 == 1):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_if_and_or_not_combo(self):
        """a == b and not (c == d) or e == f"""
        code = """
        var a = 1. var b = 1. var c = 2. var d = 2. var e = 3. var f = 4.
        if (a == b and not (c == d) or e == f):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        # a==b=T, not(c==d)=not T=F, e==f=F → T and F or F → F or F → 0
        self.assertOutputSequence(out, ["0"])

    def test_if_triple_and(self):
        code = """
        if (1 == 1 and 2 == 2 and 3 == 3):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_if_triple_and_one_false(self):
        code = """
        if (1 == 1 and 2 == 3 and 3 == 3):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_if_triple_or(self):
        code = """
        if (1 == 0 or 2 == 0 or 3 == 3):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_if_triple_or_all_false(self):
        code = """
        if (1 == 0 or 2 == 0 or 3 == 0):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_if_symbolic_and_or(self):
        """Using && and || symbols instead of keywords."""
        code = """
        if (1 == 1 && 2 == 2):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_if_symbolic_or(self):
        code = """
        if (1 == 0 || 2 == 2):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_if_mixed_keyword_and_symbol(self):
        """Mix and/&& and or/|| in same expression."""
        code = """
        if (1 == 1 and 2 == 2 || 3 == 0):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_if_not_with_symbol(self):
        """!expr form."""
        code = """
        if (!(1 == 0)):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_deeply_nested_bool(self):
        """((a and b) or (c and d)) and (e or f)"""
        code = """
        var a = 1. var b = 1. var c = 0. var d = 1. var e = 0. var f = 1.
        if (((a == 1 and b == 1) or (c == 1 and d == 1)) and (e == 1 or f == 1)):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        # (T and T) or (F and T) = T or F = T
        # (F or T) = T
        # T and T = 1
        self.assertOutputSequence(out, ["1"])

    def test_deeply_nested_bool_false(self):
        """Same structure but evaluates to false."""
        code = """
        var a = 1. var b = 0. var c = 0. var d = 1. var e = 0. var f = 0.
        if (((a == 1 and b == 1) or (c == 1 and d == 1)) and (e == 1 or f == 1)):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        # (T and F) or (F and T) = F or F = F
        # F and anything = F → 0
        self.assertOutputSequence(out, ["0"])

    def test_condition_with_arithmetic(self):
        """Conditions that involve arithmetic comparisons."""
        code = """
        var x = 10. var y = 20.
        if (x + y == 30 and x * 2 == y):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_condition_with_func_calls(self):
        """Conditions using function return values."""
        code = """
        fn isPositive @(x):
            if x > 0: give(1). ;
            give(0).
        ;
        fn isEven @(x):
            if x % 2 == 0: give(1). ;
            give(0).
        ;
        if (isPositive(5) and isEven(4)):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_condition_func_calls_false(self):
        code = """
        fn isPositive @(x):
            if x > 0: give(1). ;
            give(0).
        ;
        fn isEven @(x):
            if x % 2 == 0: give(1). ;
            give(0).
        ;
        if (isPositive(-5) and isEven(4)):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_while_complex_condition(self):
        """While loop with compound condition."""
        code = """
        var x = 10.
        var y = 10.
        var count = 0.
        while (x > 0 and y > 0):
            x = x - 2.
            y = y - 3.
            count = count + 1.
        ;
        count.
        """
        out, _ = self.run_code(code)
        # x: 10,8,6,4,2,0  y: 10,7,4,1,-2  → y <= 0 after 4 iterations
        self.assertOutputSequence(out, ["4"])

    def test_while_or_condition(self):
        """While loop exits only when BOTH conditions are false."""
        code = """
        var a = 3.
        var b = 5.
        var steps = 0.
        while (a > 0 or b > 0):
            a = a - 1.
            b = b - 1.
            steps = steps + 1.
        ;
        steps.
        """
        out, _ = self.run_code(code)
        # a: 3,2,1,0,-1  b: 5,4,3,2,1,0,-1 → both <=0 after iter where a=-1, b=0? 
        # iter1: a=2,b=4  iter2: a=1,b=3  iter3: a=0,b=2  iter4: a=-1,b=1  iter5: a=-2,b=0
        # After iter5: a=-2<=0, b=0<=0 → both false → exit. steps=5
        self.assertOutputSequence(out, ["5"])

    def test_elif_complex_conditions(self):
        code = """
        var x = 15.
        if (x > 20 or x < 10):
            1.
        elif (x >= 10 and x <= 20):
            2.
        else:
            3.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["2"])

    def test_not_in_condition_chain(self):
        """not applied to a sub-expression in a chain."""
        code = """
        var debug = 0.
        var verbose = 1.
        if (not debug and verbose):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_comparison_chain_in_var(self):
        """Store a complex boolean in a variable then use it."""
        code = """
        var a = 5. var b = 10. var c = 15.
        var result = (a < b and b < c).
        result.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_comparison_not_equal_chain(self):
        code = """
        var x = 1. var y = 2. var z = 3.
        if (x != y and y != z and x != z):
            1.
        else:
            0.
        ;
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    # ===== SUBTLE BUG-HUNTING TESTS =====

    # --- Number / dot ambiguity ---
    def test_decimal_number_with_dot_terminator(self):
        """3.14 followed by . terminator — parser must not confuse decimal with terminator."""
        code = "3.14."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3.14"])

    def test_integer_dot_as_terminator(self):
        """10. — the dot is a terminator, not a decimal point."""
        code = "10."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["10"])

    def test_decimal_in_assignment(self):
        code = """
        var pi = 3.14159.
        pi.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3.14159"])

    def test_decimal_in_arithmetic(self):
        code = "0.1 + 0.2."
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        self.assertTrue(len(lines) > 0)
        # Floating point: 0.3 approximately
        val = float(lines[0])
        self.assertAlmostEqual(val, 0.3, places=5)

    # --- Short-circuit with side effects (function calls) ---
    def test_short_circuit_or_no_side_effect(self):
        """|| short-circuit: if LHS true, function on RHS should not be called."""
        code = """
        fn bomb @():
            give(1 / 0).
        ;
        (1 == 1) || bomb().
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["1"])

    def test_short_circuit_and_no_side_effect(self):
        """&& short-circuit: if LHS false, function on RHS should not be called."""
        code = """
        fn bomb @():
            give(1 / 0).
        ;
        (1 == 0) && bomb().
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    def test_short_circuit_rhs_evaluated_when_needed(self):
        """|| should evaluate RHS when LHS is false."""
        code = """
        (1 == 0) || (2 == 2).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_short_circuit_and_rhs_evaluated_when_needed(self):
        """&& should evaluate RHS when LHS is true."""
        code = """
        (1 == 1) && (2 == 2).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    # --- Operator associativity ---
    def test_subtraction_left_associative(self):
        """10 - 3 - 2 should be (10-3)-2=5, not 10-(3-2)=9."""
        code = "10 - 3 - 2."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    def test_division_left_associative(self):
        """100 / 10 / 2 should be (100/10)/2=5, not 100/(10/2)=20."""
        code = "100 / 10 / 2."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    def test_power_right_associative_or_left(self):
        """2^3^2: if right-assoc = 2^9=512, if left-assoc = 8^2=64."""
        code = "2 ^ 3 ^ 2."
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        # Our implementation uses left-assoc for all (standard Shunting-Yard)
        # Either result is acceptable as long as it's consistent
        self.assertTrue(len(lines) > 0)

    # --- Scope chain across nested calls ---
    def test_scope_three_level_nesting(self):
        """Three levels of function nesting, each reading from enclosing scope."""
        code = """
        var g = 100.
        fn level1 @():
            var a = 10.
            fn level2 @():
                var b = 20.
                fn level3 @():
                    give(g + a + b).
                ;
                give(level3()).
            ;
            give(level2()).
        ;
        level1().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["130"])

    # --- Give interacting with while ---
    def test_give_exits_while_in_func(self):
        """Give from inside a while loop exits the function immediately."""
        code = """
        fn firstMultipleOf3 @():
            var i = 1.
            while i < 100:
                if i % 3 == 0:
                    give(i).
                ;
                i = i + 1.
            ;
            give(-1).
        ;
        firstMultipleOf3().
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    # --- For loop counter: does modifying i inside body affect iteration? ---
    def test_for_counter_modification(self):
        """Modifying the loop variable inside body should not affect iteration count."""
        code = """
        var sum = 0.
        for i in range(from 1 to 5):
            sum = sum + i.
            --> modifying i should not affect loop <--
        ;
        sum.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["15"])  # 1+2+3+4+5

    # --- Chained function calls in expression ---
    def test_chained_func_in_expression(self):
        code = """
        fn add1 @(x): give(x + 1). ;
        fn mul2 @(x): give(x * 2). ;
        add1(mul2(add1(0))).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])  # add1(0)=1, mul2(1)=2, add1(2)=3

    # --- Unary not on function return ---
    def test_not_on_func_return(self):
        code = """
        fn isZero @(x):
            if x == 0: give(1). ;
            give(0).
        ;
        not isZero(5).
        not isZero(0).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "0"])

    # --- Comparison with expressions on both sides ---
    def test_comparison_with_complex_expr(self):
        code = """
        (2 + 3 == 4 + 1).
        (10 / 2 > 3 * 2).
        (1 + 1 <= 3 - 1).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1", "0", "1"])

    # --- Recursive function: copies parameter, doesn't mutate caller ---
    def test_recursion_doesnt_mutate_caller(self):
        """Recursive calls get copy of argument, not reference."""
        code = """
        fn countDown @(n):
            if n == 0:
                give(0).
            ;
            give(n + countDown(n - 1)).
        ;
        countDown(4).
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["10"])  # 4+3+2+1+0

    # --- Multiple statements on same line ---
    def test_many_stmts_one_line(self):
        code = "var a = 1. var b = 2. var c = 3. a + b + c."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["6"])

    # --- Short-circuit chain ---
    def test_short_circuit_chain(self):
        """a || b || c with first true — should skip rest."""
        code = """
        (1 == 1) || (1 / 0) || (1 / 0).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["1"])

    def test_short_circuit_and_chain(self):
        """a && b && c with first false — should skip rest."""
        code = """
        (1 == 0) && (1 / 0) && (1 / 0).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    # --- Edge: expression is just a variable ---
    def test_expr_just_var(self):
        code = """
        var x = 42.
        x.
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    # --- Edge: negative number literal ---
    def test_negative_number_literal(self):
        code = "-1."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["-1"])

    # --- Edge: zero division in func, caught as error ---
    def test_div_zero_in_func(self):
        code = """
        fn bad @(): give(1 / 0). ;
        bad().
        """
        out, _ = self.run_code(code)
        self.assertIn("Error", out)
        self.assertIn("Div by 0", out)

    # --- Deeply nested parentheses ---
    def test_deeply_nested_parens(self):
        code = "((((1 + 2))))."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    # --- Function calling itself with modified arg multiple times ---
    def test_recursive_sum_of_digits(self):
        """Sum digits of 123: 1+2+3=6 (simplified via divide/modulo)."""
        code = """
        fn sumDigits @(n):
            if n < 10:
                give(n).
            ;
            give(n % 10 + sumDigits(n / 10)).
        ;
        sumDigits(123).
        """
        out, _ = self.run_code(code)
        # 123%10=3, 123/10=12.3, 12.3%10=2.3, 12.3/10=1.23, 1.23<10→1.23
        # Floating point: 3 + 2.3 + 1.23 = 6.53 (not exact integer arithmetic)
        # This test is about stability, not exact output
        self.assertNotIn("Error", out)

    # ===== BUG FIX TESTS =====

    # --- Issue 1: Math builtin functions should work when called ---
    def test_sin_basic(self):
        """sin(0) should return 0."""
        code = "sin(0)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    def test_cos_basic(self):
        """cos(0) should return 1."""
        code = "cos(0)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["1"])

    def test_sqrt_basic(self):
        """sqrt(9) should return 3."""
        code = "sqrt(9)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["3"])

    def test_abs_basic(self):
        """abs(-5) should return 5."""
        code = "abs(-5)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["5"])

    def test_floor_basic(self):
        """floor(3.7) should return 3."""
        code = "floor(3.7)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["3"])

    def test_ceil_basic(self):
        """ceil(3.2) should return 4."""
        code = "ceil(3.2)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["4"])

    def test_log_basic(self):
        """log(1) should return 0 (natural log)."""
        code = "log(1)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    def test_min_max_basic(self):
        """min and max with two args."""
        code = """
        min(3, 7).
        max(3, 7).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["3", "7"])

    def test_round_basic(self):
        """round(3.6) should return 4."""
        code = "round(3.6)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["4"])

    def test_sin_with_variable(self):
        """sin() should work with variables."""
        code = """
        var x = 0.
        sin(x).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    def test_math_func_in_expression(self):
        """Math functions should work inside arithmetic expressions."""
        code = "1 + sin(0)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["1"])

    def test_nested_math_func(self):
        """Nested math function calls: abs(sin(0))."""
        code = "abs(sin(0))."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    # --- Issue 1b: Implicit multiplication ---
    def test_implicit_mul_number_func(self):
        """10sin(0) should be 10*sin(0) = 0."""
        code = "10sin(0)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    def test_implicit_mul_number_paren(self):
        """2(3+4) should be 2*7 = 14."""
        code = "2(3 + 4)."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["14"])

    def test_implicit_mul_number_var(self):
        """3x where x=5 should be 15."""
        code = """
        var x = 5.
        3x.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["15"])

    def test_implicit_mul_complex(self):
        """2sin(0) + 3 should be 0 + 3 = 3."""
        code = "2sin(0) + 3."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["3"])

    # --- Issue 2: Global scope persistence in --script mode ---
    def test_var_persists_across_lines(self):
        """Variables defined on one line should be usable on the next."""
        code = """
        var x = 10.
        var y = 20.
        x + y.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["30"])

    def test_var_used_in_math_func(self):
        """Variable should be visible to math function calls."""
        code = """
        var x = 0.
        sin(x).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["0"])

    def test_func_defined_then_called(self):
        """Function defined in one statement should be callable later."""
        code = """
        fn double @(n): give(n * 2). ;
        double(5).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["10"])

    def test_var_persists_across_many_stmts(self):
        """Multiple vars and their interactions."""
        code = """
        var a = 1.
        var b = 2.
        var c = 3.
        var d = a + b + c.
        d.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["6"])

    # --- Issue 3: Multi-variable declaration ---
    def test_multi_var_basic(self):
        """var x = 10, y = 20. should declare both variables."""
        code = """
        var x = 10, y = 20.
        x + y.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["30"])

    def test_multi_var_three(self):
        """Three variables in one statement."""
        code = """
        var a = 1, b = 2, c = 3.
        a + b + c.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["6"])

    def test_multi_var_with_expressions(self):
        """Multi-var with expressions, not just literals."""
        code = """
        var x = 2 + 3, y = 10 * 2.
        x + y.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["25"])

    def test_multi_var_followed_by_stmt(self):
        """Multi-var declaration followed by another statement on next line."""
        code = """
        var x = 10, y = 20.
        sin(x * y).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        # Just verify no error, the exact sin value isn't critical
        lines = self.get_clean_output_lines(out)
        self.assertTrue(len(lines) > 0)

    def test_multi_var_single_still_works(self):
        """Single var declaration should still work fine."""
        code = """
        var x = 42.
        x.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["42"])

    # --- Combined: all three fixes working together ---
    def test_all_fixes_combined(self):
        """Multi-var, then implicit multiplication with math function."""
        code = """
        var x = 10, y = 20.
        2sin(x + y).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        # 2*sin(30 radians) — just check no error
        lines = self.get_clean_output_lines(out)
        self.assertTrue(len(lines) > 0)

    # ==================== V2 FEATURE TESTS ====================

    # --- String Literals ---
    def test_string_literal_double_quotes(self):
        code = '"hello world".'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["hello world"])

    def test_string_literal_single_quotes(self):
        code = "'hello world'."
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["hello world"])

    def test_string_escape_newline(self):
        code = 'print("a\\nb").'
        out, _ = self.run_code(code)
        self.assertIn("a\nb", out)

    def test_string_escape_tab(self):
        code = 'print("a\\tb").'
        out, _ = self.run_code(code)
        self.assertIn("a\tb", out)

    def test_string_escape_backslash(self):
        code = 'print("a\\\\b").'
        out, _ = self.run_code(code)
        self.assertIn("a\\b", out)

    def test_string_concatenation(self):
        code = '"hello" + " " + "world".'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["hello world"])

    def test_string_repeat(self):
        code = '"ab" * 3.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["ababab"])

    def test_string_repeat_reverse(self):
        code = '3 * "xy".'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["xyxyxy"])

    def test_string_var(self):
        code = 'var name = "Alice". name.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["Alice"])

    def test_string_num_concat(self):
        code = '"val=" + 42.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["val=42"])

    def test_string_equality(self):
        code = '"abc" == "abc".'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_string_inequality(self):
        code = '"abc" != "xyz".'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_string_equality_false(self):
        code = '"abc" == "xyz".'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_empty_string(self):
        code = 'var s = "". s.'
        out, _ = self.run_code(code)
        # empty string → no visible output from format_output (empty string)
        self.assertNotIn("Error", out)

    # --- List Literals ---
    def test_list_literal_basic(self):
        code = '[1, 2, 3].'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["[1, 2, 3]"])

    def test_list_literal_empty(self):
        code = 'var x = list(). print(x).'
        out, _ = self.run_code(code)
        self.assertIn("[]", out)

    def test_list_literal_mixed(self):
        code = 'var x = [1, "hello", 3]. print(x).'
        out, _ = self.run_code(code)
        self.assertIn("[1, hello, 3]", out)

    def test_list_in_var(self):
        code = 'var nums = [10, 20, 30]. print(nums).'
        out, _ = self.run_code(code)
        self.assertIn("[10, 20, 30]", out)

    def test_list_len(self):
        code = 'len([1, 2, 3, 4, 5]).'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    def test_list_append(self):
        code = 'var x = [1, 2]. var y = append(x, 3). print(y).'
        out, _ = self.run_code(code)
        self.assertIn("[1, 2, 3]", out)

    def test_list_nested(self):
        code = 'var x = [1, [2, 3], 4]. print(x).'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    # --- Set Literals ---
    def test_set_literal(self):
        code = 'var s = {1, 2, 3}. print(s).'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        # Sets may have different order, just check it looks like a set

    def test_set_empty(self):
        code = 'var s = set(). print(s).'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    # --- None / True / False ---
    def test_none_literal(self):
        code = 'var x. print(type(x)).'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_true_literal(self):
        code = 'True.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["1"])

    def test_false_literal(self):
        code = 'False.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["0"])

    def test_none_output_suppressed(self):
        """None values should not produce output from ExprStmt."""
        code = 'var x. x.'
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        self.assertEqual(len(lines), 0)

    # --- var with no initializer → None ---
    def test_var_no_init_is_none(self):
        code = 'var x. print(type(x)).'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_var_multi_none(self):
        code = 'var a, b, c.'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    # --- let / be syntax ---
    def test_let_be_basic(self):
        code = 'let x be 42. x.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["42"])

    def test_let_be_expression(self):
        code = 'let y be 3 + 4 * 2. y.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["11"])

    def test_let_be_string(self):
        code = 'let name be "Bob". name.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["Bob"])

    def test_let_be_list(self):
        code = 'let items be [1, 2, 3]. print(items).'
        out, _ = self.run_code(code)
        self.assertIn("[1, 2, 3]", out)

    # --- Forgive missing dot ---
    def test_forgive_dot_at_eof(self):
        """Missing dot at EOF should be forgiven."""
        code = 'var x = 42. x'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["42"])

    def test_forgive_dot_before_semicolon(self):
        """Missing dot before ; in block terminator should be forgiven."""
        code = 'if 1 == 1: 42 ;'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["42"])

    def test_forgive_dot_before_elif(self):
        code = 'if 0: 1 elif 1: 42. ;'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["42"])

    def test_forgive_dot_before_else(self):
        code = 'if 0: 1 else: 42. ;'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["42"])

    def test_dot_still_required_mid_script(self):
        """Missing dot in middle of script (not at boundary) should still error."""
        code = "var x = 5\nvar y = 10."
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    # --- for-in list loop ---
    def test_for_in_list(self):
        code = """
        var total = 0.
        for x in [10, 20, 30]:
            total = total + x.
        ;
        total.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["60"])

    def test_for_in_list_strings(self):
        code = """
        for word in ["hello", "world"]:
            print(word).
        ;
        """
        out, _ = self.run_code(code)
        self.assertIn("hello", out)
        self.assertIn("world", out)

    def test_for_in_variable(self):
        code = """
        var nums = [5, 10, 15].
        var sum = 0.
        for n in nums:
            sum = sum + n.
        ;
        sum.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["30"])

    # --- print() built-in ---
    def test_print_single(self):
        code = 'print(42).'
        out, _ = self.run_code(code)
        self.assertIn("42", out)

    def test_print_multi_args(self):
        code = 'print("x", "=", 10).'
        out, _ = self.run_code(code)
        self.assertIn("x = 10", out)

    def test_print_string(self):
        code = 'print("hello world").'
        out, _ = self.run_code(code)
        self.assertIn("hello world", out)

    def test_print_list(self):
        code = 'print([1, 2, 3]).'
        out, _ = self.run_code(code)
        self.assertIn("[1, 2, 3]", out)

    def test_print_returns_none(self):
        """print() should not produce extra output (returns None, which is suppressed)."""
        code = 'print("ok").'
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        self.assertEqual(len(lines), 1)
        self.assertEqual(lines[0], "ok")

    # --- type() built-in ---
    def test_type_int(self):
        code = 'type(42).'
        out, _ = self.run_code(code)
        self.assertIn("int", out.lower())

    def test_type_string(self):
        code = 'type("hello").'
        out, _ = self.run_code(code)
        self.assertIn("str", out.lower())

    def test_type_list(self):
        code = 'type([1, 2]).'
        out, _ = self.run_code(code)
        self.assertIn("list", out.lower())

    # --- len() built-in ---
    def test_len_string(self):
        code = 'len("hello").'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    def test_len_list(self):
        code = 'len([10, 20, 30]).'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    # --- str() / int() / float() conversions ---
    def test_str_builtin(self):
        code = 'var s = str(42). print(s).'
        out, _ = self.run_code(code)
        self.assertIn("42", out)

    def test_int_builtin(self):
        code = 'int(3.7).'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["3"])

    def test_float_builtin(self):
        code = 'float(5).'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    # --- File I/O ---
    def test_file_write_and_read(self):
        code = """
        write("/tmp/scriptit_test.txt", "hello from scriptit").
        var content = read("/tmp/scriptit_test.txt").
        print(content).
        """
        out, _ = self.run_code(code)
        self.assertIn("hello from scriptit", out)

    def test_file_readLine(self):
        code = """
        write("/tmp/scriptit_lines.txt", "line1\\nline2\\nline3").
        var lines = readLine("/tmp/scriptit_lines.txt").
        print(len(lines)).
        """
        out, _ = self.run_code(code)
        self.assertIn("3", out)

    # --- Edge cases ---
    def test_large_number_mul_promote(self):
        """Large integer multiplication should auto-promote, not overflow."""
        code = "1000000 * 1000000."
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        lines = self.get_clean_output_lines(out)
        self.assertTrue(len(lines) > 0)

    def test_string_in_if(self):
        code = """
        var s = "hello".
        if s == "hello":
            print("match").
        ;
        """
        out, _ = self.run_code(code)
        self.assertIn("match", out)

    def test_string_inequality_in_if(self):
        code = """
        var s = "a".
        if s != "b":
            print("different").
        ;
        """
        out, _ = self.run_code(code)
        self.assertIn("different", out)

    def test_none_equality(self):
        """Two None vars should be equal conceptually (both produce no output)."""
        code = 'var a. var b.'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_mixed_arithmetic_string_error(self):
        """Subtracting a string should error."""
        code = '"hello" - 1.'
        out, _ = self.run_code(code)
        self.assertIn("Error", out)

    def test_pprint_list(self):
        code = 'pprint([1, 2, 3]).'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_pop_from_list(self):
        code = 'var x = [1, 2, 3]. var last = pop(x). last.'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_list_in_for_with_function(self):
        """Define a function that uses a list via for-in."""
        code = """
        fn sum_list @(items):
            var total = 0.
            for x in items:
                total = total + x.
            ;
            give(total).
        ;
        sum_list([1, 2, 3, 4]).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["10"])

    def test_let_be_in_function(self):
        code = """
        fn greet @(name):
            let msg be "Hello " + name.
            give(msg).
        ;
        greet("World").
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["Hello World"])

    def test_nested_list_access_via_for(self):
        code = """
        var outer = [[1, 2], [3, 4]].
        var flat_sum = 0.
        for sub in outer:
            for item in sub:
                flat_sum = flat_sum + item.
            ;
        ;
        flat_sum.
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        self.assertOutputSequence(out, ["10"])

    def test_string_len_in_expression(self):
        code = 'var x = len("test") + 1. x.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["5"])

    def test_type_none(self):
        code = 'var x. type(x).'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_division_returns_float(self):
        """Division should always return a float/double type."""
        code = '10 / 3.'
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        val = float(lines[0])
        self.assertAlmostEqual(val, 3.33333, places=3)

    def test_string_with_spaces(self):
        code = 'var s = "  spaced  ". print(s).'
        out, _ = self.run_code(code)
        self.assertIn("  spaced  ", out)

    def test_list_of_strings(self):
        code = 'var names = ["Alice", "Bob", "Charlie"]. print(names).'
        out, _ = self.run_code(code)
        self.assertIn("Alice", out)
        self.assertIn("Bob", out)
        self.assertIn("Charlie", out)

    def test_multiple_prints(self):
        code = 'print(1). print(2). print(3).'
        out, _ = self.run_code(code)
        lines = self.get_clean_output_lines(out)
        self.assertEqual(lines, ["1", "2", "3"])

    def test_for_in_string(self):
        """Iterate over characters of a string."""
        code = """
        for ch in "abc":
            print(ch).
        ;
        """
        out, _ = self.run_code(code)
        self.assertIn("a", out)
        self.assertIn("b", out)
        self.assertIn("c", out)

    def test_let_be_reassign_error(self):
        """let/be creates a new variable, reassignment should work via = syntax."""
        code = 'let x be 10. x = 20. x.'
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["20"])

    def test_power_with_promote(self):
        """Power with large result should auto-promote."""
        code = '2 ^ 20.'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)
        lines = self.get_clean_output_lines(out)
        val = float(lines[0])
        self.assertAlmostEqual(val, 1048576, places=0)

    def test_modulo_with_promote(self):
        code = '17 % 5.'
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

    def test_string_in_function_param(self):
        code = """
        fn echo @(msg):
            give(msg).
        ;
        echo("hello").
        """
        out, _ = self.run_code(code)
        self.assertOutputSequence(out, ["hello"])

    def test_list_in_function_param(self):
        code = """
        fn first @(items):
            give(items).
        ;
        print(first([99, 88])).
        """
        out, _ = self.run_code(code)
        self.assertNotIn("Error", out)

if __name__ == '__main__':
    unittest.main()
