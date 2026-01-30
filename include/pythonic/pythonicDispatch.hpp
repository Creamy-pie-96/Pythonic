#pragma once

#include "pythonicVars.hpp"
#include "pythonicDispatchForwardDecls.hpp"
#include <array>
#include <functional>

namespace pythonic
{
    namespace dispatch
    {

        using pythonic::vars::TypeTag;
        using pythonic::vars::var;

        // Function pointer type for binary var operations
        using BinaryOpFunc = var (*)(const var &, const var &);

        constexpr int TypeTagCount = 19; // Update if you add more types to TypeTag

        template <>
        struct OpTable<struct Add>
        {
            static std::array<std::array<BinaryOpFunc, TypeTagCount>, TypeTagCount> table;
        };

        // Helper to get the function for a given type pair
        inline BinaryOpFunc get_add_func(TypeTag left, TypeTag right)
        {
            return OpTable<Add>::table[static_cast<int>(left)][static_cast<int>(right)];
        }

    } // namespace dispatch
} // namespace pythonic