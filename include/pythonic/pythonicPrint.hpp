#pragma once
#include <sstream>
#include <iostream>
#include "pythonicVars.hpp"

namespace pythonic
{
    namespace print
    {

        using namespace pythonic::vars;

        // Forward declaration for recursive pretty printing
        inline std::string format_value(const var &v, int indent = 0, int indent_step = 2, bool top_level = true);

        // Helper to format a var value with optional indentation
        inline std::string format_value(const var &v, int indent, int indent_step, bool top_level)
        {
            std::string ind(indent, ' ');
            std::string inner_ind(indent + indent_step, ' ');

            // Use the type() method to determine container type
            std::string t = v.type();

            if (t == "list")
            {
                const auto &lst = v.get<List>();
                if (lst.empty())
                    return "[]";

                // Check if it's simple (no nested containers)
                bool simple = true;
                for (const auto &item : lst)
                {
                    std::string it = item.type();
                    if (it == "list" || it == "dict" || it == "set")
                    {
                        simple = false;
                        break;
                    }
                }

                if (simple && lst.size() <= 5)
                {
                    // Compact format for simple lists
                    return v.str();
                }

                // Pretty format for complex lists
                std::ostringstream ss;
                ss << "[\n";
                for (size_t i = 0; i < lst.size(); ++i)
                {
                    ss << inner_ind << format_value(lst[i], indent + indent_step, indent_step, false);
                    if (i < lst.size() - 1)
                        ss << ",";
                    ss << "\n";
                }
                ss << ind << "]";
                return ss.str();
            }
            else if (t == "dict")
            {
                const auto &dict = v.get<Dict>();
                if (dict.empty())
                    return "{}";

                // Check if it's simple
                bool simple = true;
                for (const auto &[k, val] : dict)
                {
                    std::string vt = val.type();
                    if (vt == "list" || vt == "dict" || vt == "set")
                    {
                        simple = false;
                        break;
                    }
                }

                if (simple && dict.size() <= 3)
                {
                    return v.str();
                }

                std::ostringstream ss;
                ss << "{\n";
                size_t i = 0;
                for (const auto &[k, val] : dict)
                {
                    ss << inner_ind << "\"" << k << "\": "
                       << format_value(val, indent + indent_step, indent_step, false);
                    if (i < dict.size() - 1)
                        ss << ",";
                    ss << "\n";
                    ++i;
                }
                ss << ind << "}";
                return ss.str();
            }
            else if (t == "set")
            {
                const auto &s = v.get<Set>();
                if (s.empty())
                    return "{}";

                bool simple = true;
                for (const auto &item : s)
                {
                    std::string it = item.type();
                    if (it == "list" || it == "dict" || it == "set")
                    {
                        simple = false;
                        break;
                    }
                }

                if (simple && s.size() <= 5)
                {
                    return v.str();
                }

                std::ostringstream ss;
                ss << "{\n";
                size_t i = 0;
                for (const auto &item : s)
                {
                    ss << inner_ind << format_value(item, indent + indent_step, indent_step, false);
                    if (i < s.size() - 1)
                        ss << ",";
                    ss << "\n";
                    ++i;
                }
                ss << ind << "}";
                return ss.str();
            }
            else if (t == "str")
            {
                // Strings get quotes in containers but not at top level
                if (!top_level)
                {
                    return "\"" + v.get<std::string>() + "\"";
                }
                return v.get<std::string>();
            }
            else
            {
                return v.str();
            }
        }

        // Pretty print helper for var types
        template <typename T>
        inline std::string to_print_str(const T &arg)
        {
            std::ostringstream ss;
            ss << arg;
            return ss.str();
        }

        // Specialization for var - use smart formatting
        template <>
        inline std::string to_print_str<var>(const var &arg)
        {
            return format_value(arg, 0, 2, true);
        }

        // Main print function - handles any types
        template <typename... Args>
        void print(const Args &...args)
        {
            std::ostringstream ss;
            ((ss << to_print_str(args) << ' '), ...);
            std::string out = ss.str();
            if (!out.empty())
                out.pop_back();
            std::cout << out << std::endl;
        }

        // pprint - force pretty print with configurable indent
        inline void pprint(const var &v, int indent_step = 2)
        {
            std::cout << v.pretty_str(0, indent_step) << std::endl;
        }

    } // namespace print
} // namespace pythonic