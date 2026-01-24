#pragma once

#include "pythonicVars.hpp"
#include "pythonicPrint.hpp"
#include "pythonicLoop.hpp"
#include "pythonicFunction.hpp"
#include "pythonicFile.hpp"
#include "pythonicMath.hpp"
#include "pythonicError.hpp"
#include "pythonicFastPath.hpp"
#include "graph_viewer.hpp"  // Always include - has internal #ifdef guards

namespace Pythonic
{
    using namespace pythonic::vars;
    using namespace pythonic::print;
    using namespace pythonic::loop;
    using namespace pythonic::math;
    using namespace pythonic::file;
    using namespace pythonic::func;
    using namespace pythonic::graph;
    using namespace pythonic::fast;
    using namespace pythonic::overflow;
    using namespace pythonic::error;
    using namespace pythonic::viewer;  // Graph viewer (enabled via PYTHONIC_ENABLE_GRAPH_VIEWER)

}

// Shortcut alias for even faster usage
namespace py = Pythonic;

