#ifndef PYTHONIC_GRAPH_VIEWER_HPP
#define PYTHONIC_GRAPH_VIEWER_HPP

/**
 * @file graph_viewer.hpp
 * @brief Interactive graph viewer with View and Edit modes
 *
 * This header provides an interactive visualization and editing interface
 * for graphs stored in var. Features include:
 * - Force-directed physics layout
 * - Pan/zoom/drag interaction
 * - Signal flow animation
 * - View mode (read-only with topology preservation)
 * - Edit mode (add/remove nodes and edges)
 *
 * Requires: GLFW, OpenGL, ImGui
 * Enable with: cmake -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON
 */

#ifdef PYTHONIC_ENABLE_GRAPH_VIEWER

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>

// Forward declarations to avoid including heavy dependencies in header
struct GLFWwindow;

namespace pythonic
{
    namespace vars
    {
        class var; // Forward declaration
    }

    namespace viewer
    {
        // ============ ENUMS ============

        /**
         * @brief Viewer operation mode
         */
        enum class ViewerMode
        {
            VIEW, ///< Read-only mode: drag snaps back to pinned position
            EDIT  ///< Edit mode: can modify graph structure and topology
        };

        // ============ NODE/EDGE STATE ============

        /**
         * @brief State of a node in the viewer
         */
        struct NodeState
        {
            // Current position (can be dragged)
            float x = 0.0f;
            float y = 0.0f;

            // Pinned position (topology memory)
            float pinned_x = 0.0f;
            float pinned_y = 0.0f;

            // Physics
            float vx = 0.0f;
            float vy = 0.0f;
            float fx = 0.0f;
            float fy = 0.0f;

            // Visual state
            float activation = 0.0f;  ///< Glow intensity
            float glow_decay = 0.95f; ///< Decay factor per frame
            bool is_hovered = false;
            bool is_selected = false;
            bool is_dragging = false;

            // Metadata cache (from var graph)
            size_t node_id = 0;
            std::string label;
            std::string metadata_str;
        };

        /**
         * @brief State of an edge in the viewer
         */
        struct EdgeState
        {
            size_t from = 0;
            size_t to = 0;
            double weight = 1.0;
            bool directed = false;

            // Visual state
            float activity = 0.0f; ///< Edge glow when signal passes
            bool is_hovered = false;
            bool is_selected = false;
        };

        /**
         * @brief Signal flowing along an edge
         */
        struct Signal
        {
            size_t from = 0;
            size_t to = 0;
            float progress = 0.0f; ///< 0.0 = at source, 1.0 = at dest
            float strength = 1.0f;
            int wave = 0; ///< Wave number (outgoing=0, incoming=1, etc.)
            bool active = true;
        };

        // ============ GRAPH SNAPSHOT ============

        /**
         * @brief Thread-safe snapshot of graph state for rendering
         *
         * The physics thread updates this, and the render thread reads it.
         * Double-buffering prevents tearing.
         */
        class GraphSnapshot
        {
        public:
            std::vector<NodeState> nodes;
            std::vector<EdgeState> edges;
            std::vector<Signal> signals;

            // Graph metadata
            size_t node_count = 0;
            size_t edge_count = 0;
            bool is_connected = false;
            bool has_cycle = false;

            // Camera state
            float camera_x = 0.0f;
            float camera_y = 0.0f;
            float zoom = 1.0f;

            // Interaction state
            int hovered_node = -1;
            int selected_node = -1;
            int hovered_edge = -1;
            int selected_edge = -1;

            ViewerMode mode = ViewerMode::VIEW;
        };

        // ============ CONFIG ============

        /**
         * @brief Configuration for the graph viewer
         */
        struct ViewerConfig
        {
            // Window
            int window_width = 1280;
            int window_height = 800;
            std::string window_title = "Pythonic Graph Viewer";

            // Physics
            float repulsion = 150.0f;
            float attraction = 0.08f;
            float ideal_distance = 200.0f;
            float damping = 0.85f;
            float dt = 0.016f;

            // Signals
            float signal_speed = 2.0f;

            // Visual
            float node_radius = 15.0f;
            float edge_thickness = 2.0f;
            bool antialiasing = true;
            bool glow_enabled = true;

            // Behavior
            bool snap_to_pinned_in_view = true; ///< Nodes snap back in view mode
            bool auto_topo_sort = false;        ///< Auto-sort on topology change (disabled by default to avoid disruption during interactive editing)
        };

        // ============ GRAPH VIEWER CLASS ============

        /**
         * @brief Interactive graph viewer with View and Edit modes
         *
         * Thread-safe: physics runs on background thread, rendering on main thread.
         */
        class GraphViewer
        {
        public:
            /**
             * @brief Construct viewer for a graph var
             * @param graph_var Reference to var containing a graph
             */
            explicit GraphViewer(pythonic::vars::var &graph_var);

            /**
             * @brief Destructor - stops threads and cleans up
             */
            ~GraphViewer();

            // Non-copyable, non-movable
            GraphViewer(const GraphViewer &) = delete;
            GraphViewer &operator=(const GraphViewer &) = delete;
            GraphViewer(GraphViewer &&) = delete;
            GraphViewer &operator=(GraphViewer &&) = delete;

            /**
             * @brief Run the viewer
             * @param blocking If true, blocks until window is closed
             *
             * In blocking mode, this runs the main event loop.
             * Note: Must be called from main thread on most platforms.
             */
            void run(bool blocking = true);

            /**
             * @brief Request the viewer to close
             */
            void request_close();

            /**
             * @brief Check if viewer is still running
             */
            bool is_running() const;

            /**
             * @brief Get current viewer mode
             */
            ViewerMode get_mode() const;

            /**
             * @brief Set viewer mode
             */
            void set_mode(ViewerMode mode);

            /**
             * @brief Toggle between View and Edit modes
             */
            void toggle_mode();

            /**
             * @brief Get mutable configuration
             */
            ViewerConfig &config();

            /**
             * @brief Trigger signal flow from a node
             * @param node_id Node to start signal from
             */
            void trigger_signal(size_t node_id);

            /**
             * @brief Force topological sort and relayout
             */
            void relayout();

        private:
            // Implementation details (PIMPL pattern for ABI stability)
            class Impl;
            std::unique_ptr<Impl> impl_;
        };

        // ============ CONVENIENCE FUNCTION ============

        /**
         * @brief Show an interactive viewer for a graph
         *
         * @param g var containing a graph (or empty var to create new graph)
         * @param blocking If true, blocks until viewer is closed
         *
         * Usage:
         *   var g = graph(5);
         *   g.add_edge(0, 1);
         *   pythonic::viewer::show_graph(g);  // Opens viewer
         *   // After closing, g may have been modified
         */
        void show_graph(pythonic::vars::var &g, bool blocking = true);

        /**
         * @brief Show an interactive viewer for a graph (const version)
         *
         * Opens in View mode only since graph cannot be modified.
         */
        void show_graph(const pythonic::vars::var &g, bool blocking = true);

    } // namespace viewer

    // ============ NAMESPACE ALIASES ============

    // Make viewer accessible via pythonic::viewer or Pythonic::viewer
    // The Pythonic namespace alias is defined in pythonic.hpp

} // namespace pythonic

#else // PYTHONIC_ENABLE_GRAPH_VIEWER not defined

// Stub declarations when graph viewer is disabled
namespace pythonic
{
    namespace viewer
    {
        class GraphViewer
        {
        public:
            explicit GraphViewer(pythonic::vars::var &)
            {
                throw std::runtime_error(
                    "Graph viewer not enabled. Rebuild with -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON");
            }
        };

        inline void show_graph(pythonic::vars::var &, bool = true)
        {
            throw std::runtime_error(
                "Graph viewer not enabled. Rebuild with -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON");
        }

        inline void show_graph(const pythonic::vars::var &, bool = true)
        {
            throw std::runtime_error(
                "Graph viewer not enabled. Rebuild with -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON");
        }
    } // namespace viewer
} // namespace pythonic

#endif // PYTHONIC_ENABLE_GRAPH_VIEWER

#endif // PYTHONIC_GRAPH_VIEWER_HPP
