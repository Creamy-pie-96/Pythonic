// ScriptIt v2 — A scripting language powered by pythonic::vars::var
// Extension: .sit | Run: scriptit <file.sit> | REPL: scriptit

#include "scriptit_types.hpp"
#include "scriptit_methods.hpp"
#include "scriptit_builtins.hpp"
#include "perser.hpp"
#include "json_and_kernel.hpp"
// ═══════════════════════════════════════════════════════════
// ──── Execute Script Helper ────────────────────────────────
// ═══════════════════════════════════════════════════════════

void executeScript(const std::string &content)
{
    if (content.empty())
        return;
    try
    {
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(content);
        Parser parser(tokens);
        auto program = parser.parseProgram();
        Scope globalScope;
        globalScope.define("PI", var(3.14159265));
        globalScope.define("e", var(2.7182818));
        for (auto &stmt : program->statements)
            stmt->execute(globalScope);
    }
    catch (ReturnException &e)
    {
        std::cout << format_output(e.value) << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════
// ──── Main ─────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════

int main(int argc, char *argv[])
{
    if (argc > 1 && std::string(argv[1]) == "--kernel")
    {
        runKernel();
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--test")
    {
        std::string source =
            "var a = 10. \n"
            "--> Comment Test <-- \n"
            "fn add @(x, y): give(x+y). ; \n"
            "var result = add(a, 20). \n"
            "if result > 20: \n"
            "   result = result + 1. \n"
            "; \n"
            "var loopSum = 0. \n"
            "for i in range(from 1 to 5): \n"
            "   loopSum = loopSum + i. \n"
            "; ";
        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(source);
            Parser parser(tokens);
            auto program = parser.parseProgram();
            Scope global;
            global.define("PI", var(3.14159));
            for (auto &stmt : program->statements)
                stmt->execute(global);
            std::cout << "Result: " << global.get("result").str() << " (Expected 31)" << std::endl;
            std::cout << "LoopSum: " << global.get("loopSum").str() << " (Expected 15)" << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Test Failed: " << e.what() << std::endl;
        }
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--script")
    {
        std::string content((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
        executeScript(content);
        return 0;
    }

    if (argc > 1)
    {
        std::string filename = argv[1];
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
            return 1;
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        executeScript(content);
        return 0;
    }

    // ── Interactive REPL ──
    std::cout << "ScriptIt REPL v2 (powered by pythonic::var)" << std::endl;
    std::cout << "Type 'exit' to quit, 'clear' to clear screen, 'wipe' for fresh start." << std::endl;
    std::string line;
    Scope globalScope;
    globalScope.define("PI", var(3.14159265));
    globalScope.define("e", var(2.7182818));
    globalScope.define("ans", var(0));

    while (true)
    {
        std::cout << ">> ";
        if (!std::getline(std::cin, line) || line == "exit")
            break;
        if (line.empty())
            continue;
        if (line == "clear")
        {
#ifdef _WIN32
            std::system("cls");
#else
            std::system("clear");
#endif
            continue;
        }
        if (line == "wipe")
        {
#ifdef _WIN32
            std::system("cls");
#else
            std::system("clear");
#endif
            globalScope.clear();
            globalScope.define("PI", var(3.14159265));
            globalScope.define("e", var(2.7182818));
            globalScope.define("ans", var(0));
            std::cout << "Session wiped. All variables and functions cleared." << std::endl;
            continue;
        }
        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(line);
            Parser parser(tokens);
            auto program = parser.parseProgram();
            for (auto &stmt : program->statements)
                stmt->execute(globalScope);
        }
        catch (ReturnException &e)
        {
            std::cout << format_output(e.value) << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
    return 0;
}
