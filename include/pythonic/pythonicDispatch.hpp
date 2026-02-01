#pragma once

// #include "pythonicVars.hpp" // Removed to avoid cycle
#include "pythonicDispatchForwardDecls.hpp"
#include <array>
#include <functional>

// Forward declaration of TypeTag and var to avoid circular dependency
namespace pythonic
{
    namespace vars
    {

        enum class TypeTag : uint8_t;
        // Forward declaration
        class var;
    }
}
namespace pythonic
{
    namespace dispatch
    {

        using pythonic::vars::var;

        using pythonic::overflow::Overflow;

        // Function pointer type for binary var operations
        using BinaryOpFunc = var (*)(const var &, const var &, Overflow, bool);

        constexpr int TypeTagCount = 18; // Corrected count based on TypeTag enum (0 to 17)

        // Generic OpTable template for all operations
        template <typename Op>
        struct OpTable
        {
            static const std::array<std::array<BinaryOpFunc, TypeTagCount>, TypeTagCount> table;
        };

        // Op types
        struct Add
        {
        };
        struct Sub
        {
        };
        struct Mul
        {
        };
        struct Div
        {
        };
        struct Mod
        {
        };
        // Bitwise
        struct BitAnd
        {
        };
        struct BitOr
        {
        };
        struct BitXor
        {
        };
        struct ShiftLeft
        {
        };
        struct ShiftRight
        {
        };
        // Logical
        struct LogicalAnd
        {
        };
        struct LogicalOr
        {
        };
        // Relational (technically return bool wrapped in var, but signature is same)
        struct Eq
        {
        };
        struct Ne
        {
        };
        struct Gt
        {
        };
        struct Ge
        {
        };
        struct Lt
        {
        };
        struct Le
        {
        };

// Explicit specialization declarations
#include "pythonicDispatchDeclarations.hpp"

        // Helper to get function
        template <typename Op>
        inline BinaryOpFunc get_op_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right)
        {
            // Safely cast enum class to int for array indexing
            return OpTable<Op>::table[static_cast<std::underlying_type_t<pythonic::vars::TypeTag>>(left)][static_cast<std::underlying_type_t<pythonic::vars::TypeTag>>(right)];
        }

        // Convenience wrappers
        inline BinaryOpFunc get_add_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Add>(left, right); }
        inline BinaryOpFunc get_sub_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Sub>(left, right); }
        inline BinaryOpFunc get_mul_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Mul>(left, right); }
        inline BinaryOpFunc get_div_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Div>(left, right); }
        inline BinaryOpFunc get_mod_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Mod>(left, right); }
        inline BinaryOpFunc get_eq_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Eq>(left, right); }
        inline BinaryOpFunc get_ne_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Ne>(left, right); }
        inline BinaryOpFunc get_gt_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Gt>(left, right); }
        inline BinaryOpFunc get_ge_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Ge>(left, right); }
        inline BinaryOpFunc get_lt_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Lt>(left, right); }
        inline BinaryOpFunc get_le_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<Le>(left, right); }
        inline BinaryOpFunc get_band_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<BitAnd>(left, right); }
        inline BinaryOpFunc get_bor_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<BitOr>(left, right); }
        inline BinaryOpFunc get_bxor_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<BitXor>(left, right); }
        inline BinaryOpFunc get_shl_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<ShiftLeft>(left, right); }
        inline BinaryOpFunc get_shr_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<ShiftRight>(left, right); }
        inline BinaryOpFunc get_land_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<LogicalAnd>(left, right); }
        inline BinaryOpFunc get_lor_func(pythonic::vars::TypeTag left, pythonic::vars::TypeTag right) { return get_op_func<LogicalOr>(left, right); }

    } // namespace dispatch
} // namespace pythonic