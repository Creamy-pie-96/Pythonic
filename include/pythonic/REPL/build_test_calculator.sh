#!/bin/bash
rm -fr test_calculator &&
g++ -std=c++20 -o test_calculator -DHAVE_READLINE test_calculator.cpp -lreadline -lncurses || exit 1
clear &&
if [[ "$1" == "--test" ]]; then
    printf "%s\n" \
    "var a = PI*2, b = sin(a) + e" \
    "var c = (a^2 + b^2) / (1 + cos(a/2))" \
    "var d = sqrt(abs(c - 100)) + log10(b + 1)" \
    "sqrt(d) + deg"
    ./test_calculator <<'EOF'
var a = PI*2, b = sin(a) + e
var c = (a^2 + b^2) / (1 + cos(a/2))
var d = sqrt(abs(c - 100)) + log10(b + 1)
sqrt(d) + deg
EOF
else
    ./test_calculator
fi