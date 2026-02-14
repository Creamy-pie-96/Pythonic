// ScriptIt v2 — A scripting language powered by pythonic::vars::var
// Extension: .sit | Run: scriptit <file.sit> | REPL: scriptit

#include "scriptit_types.hpp"
#include "scriptit_methods.hpp"
#include "scriptit_builtins.hpp"
#include "perser.hpp"
#include "json_and_kernel.hpp"
#include <unistd.h> // readlink for --notebook and --customize
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

    if (argc > 1 && std::string(argv[1]) == "--notebook")
    {
        // Launch the web notebook server
        // Try the installed scriptit-notebook launcher first, then fallback to local notebook.sh
        std::string notebookCmd;
        // Check for system-installed launcher
        if (std::system("command -v scriptit-notebook >/dev/null 2>&1") == 0)
        {
            notebookCmd = "scriptit-notebook";
        }
        else
        {
            // Fallback: find notebook.sh relative to this binary
            // argv[0] might be a path or just "scriptit" from PATH
            std::string selfPath;
            // Try /proc/self/exe on Linux
            char buf[4096] = {};
            ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
            if (len > 0)
            {
                buf[len] = '\0';
                selfPath = std::string(buf);
            }
            if (!selfPath.empty())
            {
                std::string dir = selfPath.substr(0, selfPath.rfind('/'));
                // If installed to /usr/local/bin, notebook files are in /usr/local/share/scriptit/notebook/
                std::string shareNotebook = dir + "/../share/scriptit/notebook/notebook_server.py";
                std::string localNotebook = dir + "/notebook.sh";
                // Also check REPL layout
                std::string replNotebook = dir + "/notebook/notebook_server.py";
                if (std::ifstream(shareNotebook).good())
                {
                    notebookCmd = "python3 \"" + shareNotebook + "\"";
                }
                else if (std::ifstream(replNotebook).good())
                {
                    notebookCmd = "python3 \"" + replNotebook + "\"";
                }
                else if (std::ifstream(localNotebook).good())
                {
                    notebookCmd = "bash \"" + localNotebook + "\"";
                }
            }
        }

        if (notebookCmd.empty())
        {
            std::cerr << "Error: Could not find notebook server.\n";
            std::cerr << "Make sure ScriptIt is installed system-wide (sudo cmake --install build_scriptit)\n";
            return 1;
        }

        // Forward remaining args (e.g. --port, notebook file)
        for (int i = 2; i < argc; i++)
        {
            notebookCmd += " ";
            notebookCmd += argv[i];
        }
        return std::system(notebookCmd.c_str());
    }

    if (argc > 1 && std::string(argv[1]) == "--customize")
    {
        // Launch the color customizer web server
        std::string customizerScript;
        std::string selfPath;
        char buf[4096] = {};
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len > 0)
        {
            buf[len] = '\0';
            selfPath = std::string(buf);
        }
        if (!selfPath.empty())
        {
            std::string dir = selfPath.substr(0, selfPath.rfind('/'));
            // Check various locations
            std::string paths[] = {
                dir + "/../share/scriptit/color_customizer/customizer_server.py", // system install
                dir + "/scriptit-vscode/color_customizer/customizer_server.py",   // REPL layout
                dir + "/color_customizer/customizer_server.py",                   // local
            };
            for (auto &p : paths)
            {
                if (std::ifstream(p).good())
                {
                    customizerScript = p;
                    break;
                }
            }
        }
        // Also try relative to CWD
        if (customizerScript.empty())
        {
            std::string cwdPaths[] = {
                "scriptit-vscode/color_customizer/customizer_server.py",
                "color_customizer/customizer_server.py",
            };
            for (auto &p : cwdPaths)
            {
                if (std::ifstream(p).good())
                {
                    customizerScript = p;
                    break;
                }
            }
        }

        if (customizerScript.empty())
        {
            std::cerr << "Error: Could not find the color customizer.\n";
            std::cerr << "Make sure ScriptIt is installed with the VS Code extension files.\n";
            return 1;
        }

        std::string cmd = "python3 \"" + customizerScript + "\"";
        // Forward --port if provided
        for (int i = 2; i < argc; i++)
        {
            cmd += " ";
            cmd += argv[i];
        }
        return std::system(cmd.c_str());
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
