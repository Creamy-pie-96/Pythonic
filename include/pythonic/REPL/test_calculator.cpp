#include "pythonicCalculator.hpp"
#include "../pythonicPrint.hpp"
using namespace pythonic::calculator;

int main(int argc, char **argv)
{
    // Simple argument handling for tests:
    // --debug           enable debug mode (if DEBUG exists)
    // --eval "<expr>"   evaluate expression and exit

    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--debug")
        {
// set DEBUG if available
#if defined(DEBUG)
            DEBUG = true;
#else
            // try namespaced symbol if present
            try
            {
                pythonic::calculator::DEBUG = true;
            }
            catch (...)
            {
            }
#endif
            continue;
        }

        if (a == "--eval" && i + 1 < argc)
        {
            std::string expr = argv[++i];
            Calculator calc;
            try
            {
                calc.process(expr);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                return 1;
            }
            return 0;
        }
    }

    // No eval flag: run interactive REPL
    calculator();
    return 0;
}