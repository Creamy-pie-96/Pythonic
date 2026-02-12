#pragma once

#include "pythonicVars.hpp"
#include "pythonicPrint.hpp"
#include "pythonicLoop.hpp"
#include "pythonicFunction.hpp"
#include "pythonicFile.hpp"
#include "pythonicMath.hpp"
#include "pythonicError.hpp"
#include "pythonicFastPath.hpp"
#include "pythonicDraw.hpp"
#include "pythonicMedia.hpp"
#include "pythonicLiveDraw.hpp"
#include "pythonicPlot.hpp"
#include "graph_viewer.hpp" // Always include - has internal #ifdef guards

#include "REPL/pythonicCalculator.hpp" // REPL Calculator

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
    using namespace pythonic::views;
    using namespace pythonic::draw;
    using namespace pythonic::media;
    using namespace pythonic::plot;
    using namespace pythonic::viewer; // Graph viewer (enabled via PYTHONIC_ENABLE_GRAPH_VIEWER)

    using namespace pythonic::calculator; // REPL Calculator

}

// Shortcut alias for even faster usage
namespace py = Pythonic;
