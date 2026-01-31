#!/bin/bash
# 1. Regenerate the dispatch code (after you've fixed the scripts)
python3 scripts/gen_forward_dec_func.py > include/pythonic/pythonicDispatchForwardDecls.hpp
python3 scripts/gen_dispatch_stubs.py > src/pythonicDispatchStubs.cpp
# 2. Build the project
mkdir -p build && cd build
cmake ..
make test_math test_overflow_ops
cd ..
# 3. Run the tests
./build/test_math
./build/test_overflow_ops