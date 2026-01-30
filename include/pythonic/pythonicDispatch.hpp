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

        // Forward declarations for all (TypeTag, TypeTag) combinations for addition
        var add_none_none(const var &, const var &);
        var add_none_int(const var &, const var &);
        var add_none_float(const var &, const var &);
        var add_none_string(const var &, const var &);
        var add_none_bool(const var &, const var &);
        var add_none_double(const var &, const var &);
        var add_none_long(const var &, const var &);
        var add_none_long_long(const var &, const var &);
        var add_none_long_double(const var &, const var &);
        var add_none_uint(const var &, const var &);
        var add_none_ulong(const var &, const var &);
        var add_none_ulong_long(const var &, const var &);
        var add_none_list(const var &, const var &);
        var add_none_set(const var &, const var &);
        var add_none_dict(const var &, const var &);
        var add_none_orderedset(const var &, const var &);
        var add_none_ordereddict(const var &, const var &);
        var add_none_graph(const var &, const var &);

        var add_int_none(const var &, const var &);
        var add_int_int(const var &, const var &);
        var add_int_float(const var &, const var &);
        var add_int_string(const var &, const var &);
        var add_int_bool(const var &, const var &);
        var add_int_double(const var &, const var &);
        var add_int_long(const var &, const var &);
        var add_int_long_long(const var &, const var &);
        var add_int_long_double(const var &, const var &);
        var add_int_uint(const var &, const var &);
        var add_int_ulong(const var &, const var &);
        var add_int_ulong_long(const var &, const var &);
        var add_int_list(const var &, const var &);
        var add_int_set(const var &, const var &);
        var add_int_dict(const var &, const var &);
        var add_int_orderedset(const var &, const var &);
        var add_int_ordereddict(const var &, const var &);
        var add_int_graph(const var &, const var &);

        var add_float_none(const var &, const var &);
        var add_float_int(const var &, const var &);
        var add_float_float(const var &, const var &);
        var add_float_string(const var &, const var &);
        var add_float_bool(const var &, const var &);
        var add_float_double(const var &, const var &);
        var add_float_long(const var &, const var &);
        var add_float_long_long(const var &, const var &);
        var add_float_long_double(const var &, const var &);
        var add_float_uint(const var &, const var &);
        var add_float_ulong(const var &, const var &);
        var add_float_ulong_long(const var &, const var &);
        var add_float_list(const var &, const var &);
        var add_float_set(const var &, const var &);
        var add_float_dict(const var &, const var &);
        var add_float_orderedset(const var &, const var &);
        var add_float_ordereddict(const var &, const var &);
        var add_float_graph(const var &, const var &);

        var add_string_none(const var &, const var &);
        var add_string_int(const var &, const var &);
        var add_string_float(const var &, const var &);
        var add_string_string(const var &, const var &);
        var add_string_bool(const var &, const var &);
        var add_string_double(const var &, const var &);
        var add_string_long(const var &, const var &);
        var add_string_long_long(const var &, const var &);
        var add_string_long_double(const var &, const var &);
        var add_string_uint(const var &, const var &);
        var add_string_ulong(const var &, const var &);
        var add_string_ulong_long(const var &, const var &);
        var add_string_list(const var &, const var &);
        var add_string_set(const var &, const var &);
        var add_string_dict(const var &, const var &);
        var add_string_orderedset(const var &, const var &);
        var add_string_ordereddict(const var &, const var &);
        var add_string_graph(const var &, const var &);

        var add_bool_none(const var &, const var &);
        var add_bool_int(const var &, const var &);
        var add_bool_float(const var &, const var &);
        var add_bool_string(const var &, const var &);
        var add_bool_bool(const var &, const var &);
        var add_bool_double(const var &, const var &);
        var add_bool_long(const var &, const var &);
        var add_bool_long_long(const var &, const var &);
        var add_bool_long_double(const var &, const var &);
        var add_bool_uint(const var &, const var &);
        var add_bool_ulong(const var &, const var &);
        var add_bool_ulong_long(const var &, const var &);
        var add_bool_list(const var &, const var &);
        var add_bool_set(const var &, const var &);
        var add_bool_dict(const var &, const var &);
        var add_bool_orderedset(const var &, const var &);
        var add_bool_ordereddict(const var &, const var &);
        var add_bool_graph(const var &, const var &);

        var add_double_none(const var &, const var &);
        var add_double_int(const var &, const var &);
        var add_double_float(const var &, const var &);
        var add_double_string(const var &, const var &);
        var add_double_bool(const var &, const var &);
        var add_double_double(const var &, const var &);
        var add_double_long(const var &, const var &);
        var add_double_long_long(const var &, const var &);
        var add_double_long_double(const var &, const var &);
        var add_double_uint(const var &, const var &);
        var add_double_ulong(const var &, const var &);
        var add_double_ulong_long(const var &, const var &);
        var add_double_list(const var &, const var &);
        var add_double_set(const var &, const var &);
        var add_double_dict(const var &, const var &);
        var add_double_orderedset(const var &, const var &);
        var add_double_ordereddict(const var &, const var &);
        var add_double_graph(const var &, const var &);
        // The forward declarations for addition have been moved to pythonicDispatchForwardDecls.hpp

        var add_long_set(const var &, const var &);
        var add_long_dict(const var &, const var &);
        var add_long_orderedset(const var &, const var &);
        var add_long_ordereddict(const var &, const var &);
        var add_long_graph(const var &, const var &);

        var add_long_long_none(const var &, const var &);
        var add_long_long_int(const var &, const var &);
        var add_long_long_float(const var &, const var &);
        var add_long_long_string(const var &, const var &);
        var add_long_long_bool(const var &, const var &);
        var add_long_long_double(const var &, const var &);
        var add_long_long_long(const var &, const var &);
        var add_long_long_long_long(const var &, const var &);
        var add_long_long_long_double(const var &, const var &);
        var add_long_long_uint(const var &, const var &);
        var add_long_long_ulong(const var &, const var &);
        var add_long_long_ulong_long(const var &, const var &);
        var add_long_long_list(const var &, const var &);
        var add_long_long_set(const var &, const var &);
        var add_long_long_dict(const var &, const var &);
        var add_long_long_orderedset(const var &, const var &);
        var add_long_long_ordereddict(const var &, const var &);
        var add_long_long_graph(const var &, const var &);

        var add_long_double_none(const var &, const var &);
        var add_long_double_int(const var &, const var &);
        var add_long_double_float(const var &, const var &);
        var add_long_double_string(const var &, const var &);
        var add_long_double_bool(const var &, const var &);
        var add_long_double_double(const var &, const var &);
        var add_long_double_long(const var &, const var &);
        var add_long_double_long_long(const var &, const var &);
        var add_long_double_long_double(const var &, const var &);
        var add_long_double_uint(const var &, const var &);
        var add_long_double_ulong(const var &, const var &);
        var add_long_double_ulong_long(const var &, const var &);
        var add_long_double_list(const var &, const var &);
        var add_long_double_set(const var &, const var &);
        var add_long_double_dict(const var &, const var &);
        var add_long_double_orderedset(const var &, const var &);
        var add_long_double_ordereddict(const var &, const var &);
        var add_long_double_graph(const var &, const var &);

        var add_uint_none(const var &, const var &);
        var add_uint_int(const var &, const var &);
        var add_uint_float(const var &, const var &);
        var add_uint_string(const var &, const var &);
        var add_uint_bool(const var &, const var &);
        var add_uint_double(const var &, const var &);
        var add_uint_long(const var &, const var &);
        var add_uint_long_long(const var &, const var &);
        var add_uint_long_double(const var &, const var &);
        var add_uint_uint(const var &, const var &);
        var add_uint_ulong(const var &, const var &);
        var add_uint_ulong_long(const var &, const var &);
        var add_uint_list(const var &, const var &);
        var add_uint_set(const var &, const var &);
        var add_uint_dict(const var &, const var &);
        var add_uint_orderedset(const var &, const var &);
        var add_uint_ordereddict(const var &, const var &);
        var add_uint_graph(const var &, const var &);

        var add_ulong_none(const var &, const var &);
        var add_ulong_int(const var &, const var &);
        var add_ulong_float(const var &, const var &);
        var add_ulong_string(const var &, const var &);
        var add_ulong_bool(const var &, const var &);
        var add_ulong_double(const var &, const var &);
        var add_ulong_long(const var &, const var &);
        var add_ulong_long_long(const var &, const var &);
        var add_ulong_long_double(const var &, const var &);
        var add_ulong_uint(const var &, const var &);
        var add_ulong_ulong(const var &, const var &);
        var add_ulong_ulong_long(const var &, const var &);
        var add_ulong_list(const var &, const var &);
        var add_ulong_set(const var &, const var &);
        var add_ulong_dict(const var &, const var &);
        var add_ulong_orderedset(const var &, const var &);
        var add_ulong_ordereddict(const var &, const var &);
        var add_ulong_graph(const var &, const var &);

        var add_ulong_long_none(const var &, const var &);
        var add_ulong_long_int(const var &, const var &);
        var add_ulong_long_float(const var &, const var &);
        var add_ulong_long_string(const var &, const var &);
        var add_ulong_long_bool(const var &, const var &);
        var add_ulong_long_double(const var &, const var &);
        var add_ulong_long_long(const var &, const var &);
        var add_ulong_long_long_long(const var &, const var &);
        var add_ulong_long_long_double(const var &, const var &);
        var add_ulong_long_uint(const var &, const var &);
        var add_ulong_long_ulong(const var &, const var &);
        var add_ulong_long_ulong_long(const var &, const var &);
        var add_ulong_long_list(const var &, const var &);
        var add_ulong_long_set(const var &, const var &);
        var add_ulong_long_dict(const var &, const var &);
        var add_ulong_long_orderedset(const var &, const var &);
        var add_ulong_long_ordereddict(const var &, const var &);
        var add_ulong_long_graph(const var &, const var &);

        var add_list_none(const var &, const var &);
        var add_list_int(const var &, const var &);
        var add_list_float(const var &, const var &);
        var add_list_string(const var &, const var &);
        var add_list_bool(const var &, const var &);
        var add_list_double(const var &, const var &);
        var add_list_long(const var &, const var &);
        var add_list_long_long(const var &, const var &);
        var add_list_long_double(const var &, const var &);
        var add_list_uint(const var &, const var &);
        var add_list_ulong(const var &, const var &);
        var add_list_ulong_long(const var &, const var &);
        var add_list_list(const var &, const var &);
        var add_list_set(const var &, const var &);
        var add_list_dict(const var &, const var &);
        var add_list_orderedset(const var &, const var &);
        var add_list_ordereddict(const var &, const var &);
        var add_list_graph(const var &, const var &);

        var add_set_none(const var &, const var &);
        var add_set_int(const var &, const var &);
        var add_set_float(const var &, const var &);
        var add_set_string(const var &, const var &);
        var add_set_bool(const var &, const var &);
        var add_set_double(const var &, const var &);
        var add_set_long(const var &, const var &);
        var add_set_long_long(const var &, const var &);
        var add_set_long_double(const var &, const var &);
        var add_set_uint(const var &, const var &);
        var add_set_ulong(const var &, const var &);
        var add_set_ulong_long(const var &, const var &);
        var add_set_list(const var &, const var &);
        var add_set_set(const var &, const var &);
        var add_set_dict(const var &, const var &);
        var add_set_orderedset(const var &, const var &);
        var add_set_ordereddict(const var &, const var &);
        var add_set_graph(const var &, const var &);

        var add_dict_none(const var &, const var &);
        var add_dict_int(const var &, const var &);
        var add_dict_float(const var &, const var &);
        var add_dict_string(const var &, const var &);
        var add_dict_bool(const var &, const var &);
        var add_dict_double(const var &, const var &);
        var add_dict_long(const var &, const var &);
        var add_dict_long_long(const var &, const var &);
        var add_dict_long_double(const var &, const var &);
        var add_dict_uint(const var &, const var &);
        var add_dict_ulong(const var &, const var &);
        var add_dict_ulong_long(const var &, const var &);
        var add_dict_list(const var &, const var &);
        var add_dict_set(const var &, const var &);
        var add_dict_dict(const var &, const var &);
        var add_dict_orderedset(const var &, const var &);
        var add_dict_ordereddict(const var &, const var &);
        var add_dict_graph(const var &, const var &);

        var add_orderedset_none(const var &, const var &);
        var add_orderedset_int(const var &, const var &);
        var add_orderedset_float(const var &, const var &);
        var add_orderedset_string(const var &, const var &);
        var add_orderedset_bool(const var &, const var &);
        var add_orderedset_double(const var &, const var &);
        var add_orderedset_long(const var &, const var &);
        var add_orderedset_long_long(const var &, const var &);
        var add_orderedset_long_double(const var &, const var &);
        var add_orderedset_uint(const var &, const var &);
        var add_orderedset_ulong(const var &, const var &);
        var add_orderedset_ulong_long(const var &, const var &);
        var add_orderedset_list(const var &, const var &);
        var add_orderedset_set(const var &, const var &);
        var add_orderedset_dict(const var &, const var &);
        var add_orderedset_orderedset(const var &, const var &);
        var add_orderedset_ordereddict(const var &, const var &);
        var add_orderedset_graph(const var &, const var &);

        var add_ordereddict_none(const var &, const var &);
        var add_ordereddict_int(const var &, const var &);
        var add_ordereddict_float(const var &, const var &);
        var add_ordereddict_string(const var &, const var &);
        var add_ordereddict_bool(const var &, const var &);
        var add_ordereddict_double(const var &, const var &);
        var add_ordereddict_long(const var &, const var &);
        var add_ordereddict_long_long(const var &, const var &);
        var add_ordereddict_long_double(const var &, const var &);
        var add_ordereddict_uint(const var &, const var &);
        var add_ordereddict_ulong(const var &, const var &);
        var add_ordereddict_ulong_long(const var &, const var &);
        var add_ordereddict_list(const var &, const var &);
        var add_ordereddict_set(const var &, const var &);
        var add_ordereddict_dict(const var &, const var &);
        var add_ordereddict_orderedset(const var &, const var &);
        var add_ordereddict_ordereddict(const var &, const var &);
        var add_ordereddict_graph(const var &, const var &);

        var add_graph_none(const var &, const var &);
        var add_graph_int(const var &, const var &);
        var add_graph_float(const var &, const var &);
        var add_graph_string(const var &, const var &);
        var add_graph_bool(const var &, const var &);
        var add_graph_double(const var &, const var &);
        var add_graph_long(const var &, const var &);
        var add_graph_long_long(const var &, const var &);
        var add_graph_long_double(const var &, const var &);
        var add_graph_uint(const var &, const var &);
        var add_graph_ulong(const var &, const var &);
        var add_graph_ulong_long(const var &, const var &);
        var add_graph_list(const var &, const var &);
        var add_graph_set(const var &, const var &);
        var add_graph_dict(const var &, const var &);
        var add_graph_orderedset(const var &, const var &);
        var add_graph_ordereddict(const var &, const var &);
        var add_graph_graph(const var &, const var &);
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