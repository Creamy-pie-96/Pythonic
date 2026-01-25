/**
 * @file graph_viewer.cpp
 * @brief Implementation of interactive graph viewer
 *
 * Provides a multithreaded, physics-based graph visualizer with View and Edit modes.
 */

#ifdef PYTHONIC_ENABLE_GRAPH_VIEWER

#include "pythonic/graph_viewer.hpp"
#include "pythonic/pythonicVars.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>
#include <future>
#include <queue>
#include <sstream>

namespace pythonic
{
    namespace viewer
    {

        // ============================================================================
        // HELPER FUNCTIONS
        // ============================================================================

        namespace
        {
            // Color utilities
            ImU32 make_color(float r, float g, float b, float a = 1.0f)
            {
                return ImColor(r, g, b, a);
            }

            ImU32 lerp_color(ImU32 c1, ImU32 c2, float t)
            {
                int r1 = (c1 >> 0) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = (c1 >> 16) & 0xFF, a1 = (c1 >> 24) & 0xFF;
                int r2 = (c2 >> 0) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = (c2 >> 16) & 0xFF, a2 = (c2 >> 24) & 0xFF;
                int r = r1 + (int)((r2 - r1) * t);
                int g = g1 + (int)((g2 - g1) * t);
                int b = b1 + (int)((b2 - b1) * t);
                int a = a1 + (int)((a2 - a1) * t);
                return IM_COL32(r, g, b, a);
            }

            float distance(float x1, float y1, float x2, float y2)
            {
                float dx = x2 - x1, dy = y2 - y1;
                return std::sqrt(dx * dx + dy * dy);
            }

            // GLSL Shader sources for glow effects
            const char *GLOW_VERTEX_SHADER = R"(
                #version 130
                in vec2 Position;
                in vec2 UV;
                in vec4 Color;
                out vec2 Frag_UV;
                out vec4 Frag_Color;
                uniform mat4 ProjMtx;
                void main() {
                    Frag_UV = UV;
                    Frag_Color = Color;
                    gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
                }
            )";

            const char *GLOW_FRAGMENT_SHADER = R"(
                #version 130
                in vec2 Frag_UV;
                in vec4 Frag_Color;
                out vec4 Out_Color;
                uniform sampler2D Texture;
                uniform float GlowIntensity;
                void main() {
                    vec4 col = Frag_Color * texture(Texture, Frag_UV.st);
                    // Add glow effect
                    float glow = GlowIntensity * col.a;
                    col.rgb += vec3(glow * 0.3);
                    Out_Color = col;
                }
            )";

        } // anonymous namespace

        // ============================================================================
        // GRAPHVIEWER IMPLEMENTATION (PIMPL)
        // ============================================================================

        class GraphViewer::Impl
        {
        public:
            pythonic::vars::var &graph_var_;
            ViewerConfig config_;
            std::atomic<bool> running_{false};
            std::atomic<bool> close_requested_{false};

            // Snapshots with double buffering
            GraphSnapshot front_snapshot_;
            GraphSnapshot back_snapshot_;
            std::mutex snapshot_mutex_;

            // Physics thread
            std::thread physics_thread_;
            std::atomic<bool> physics_running_{false};

            // Window
            GLFWwindow *window_ = nullptr;

            // Interaction state
            ViewerMode mode_ = ViewerMode::VIEW;
            int dragged_node_ = -1;
            bool dragging_camera_ = false;
            bool dragging_graph_center_ = false; // Drag entire graph via center handle (distinct from camera drag)
            double last_mouse_x_ = 0.0, last_mouse_y_ = 0.0;
            float camera_x_ = 0.0f, camera_y_ = 0.0f;
            float zoom_ = 1.0f;

            // Edge creation in edit mode
            int edge_start_node_ = -1;
            bool creating_edge_ = false;

            // Sidebar state
            bool sidebar_open_ = false;
            char new_node_label_[256] = "";
            char new_node_metadata_[1024] = "";

            // Signal system
            std::vector<Signal> signals_;
            std::mutex signals_mutex_;

            // Random generator for layout
            std::mt19937 rng_{std::random_device{}()};

            Impl(pythonic::vars::var &g) : graph_var_(g)
            {
                sync_from_graph();
            }

            ~Impl()
            {
                stop_physics();
                cleanup_window();
            }

            // ----------------------------------------------------------------
            // Graph synchronization
            // ----------------------------------------------------------------

            void sync_from_graph()
            {
                std::lock_guard<std::mutex> lock(snapshot_mutex_);

                back_snapshot_.nodes.clear();
                back_snapshot_.edges.clear();

                // Check if var holds a graph
                if (graph_var_.type() != "graph")
                {
                    // Create empty graph if var is not a graph
                    if (graph_var_.is<pythonic::vars::NoneType>() || graph_var_.type() == "none")
                    {
                        graph_var_ = pythonic::vars::graph(0);
                    }
                    else
                    {
                        throw std::runtime_error("var must contain a graph");
                    }
                }

                size_t n = graph_var_.node_count();
                back_snapshot_.node_count = n;

                // Initialize nodes with layered left-to-right layout (like main.cpp)
                // Try topological sort first for DAGs
                std::vector<size_t> layers(n, 0);
                bool has_layers = false;

                try
                {
                    // Run topological_sort in a separate thread and timeout if it takes too long.
                    std::packaged_task<pythonic::vars::var()> task([&]() { return graph_var_.topological_sort(); });
                    auto fut = task.get_future();
                    std::thread(std::move(task)).detach();

                    if (fut.wait_for(std::chrono::milliseconds(250)) == std::future_status::ready)
                    {
                        auto topo = fut.get();
                        // Assign layers based on longest path from source
                        std::vector<size_t> max_pred_layer(n, 0);

                        for (size_t i = 0; i < topo.len(); ++i)
                        {
                            size_t node_id = static_cast<size_t>(topo[i].toInt());
                            // Find max layer of all predecessors
                            size_t max_layer = 0;
                            for (size_t pred = 0; pred < n; ++pred)
                            {
                                auto edges = graph_var_.get_edges(pred);
                                for (size_t ei = 0; ei < edges.len(); ++ei)
                                {
                                    auto e = edges[ei];
                                    if (static_cast<size_t>(e["to"].toInt()) == node_id)
                                    {
                                        max_layer = std::max(max_layer, layers[pred] + 1);
                                    }
                                }
                            }
                            layers[node_id] = max_layer;
                        }
                        has_layers = true;
                    }
                    else
                    {
                        has_layers = false;
                    }
                }
                catch (...)
                {
                    // Has cycle, use circular layout
                    has_layers = false;
                }

                if (has_layers && n > 0)
                {
                    // Layered layout (left-to-right like main.cpp)
                    size_t max_layer = *std::max_element(layers.begin(), layers.end());
                    std::vector<std::vector<size_t>> layer_nodes(max_layer + 1);

                    for (size_t i = 0; i < n; ++i)
                    {
                        layer_nodes[layers[i]].push_back(i);
                    }

                    float start_x = 150.0f;
                    float spacing_x = 200.0f;
                    float spacing_y = 80.0f;
                    float center_y = config_.window_height / 2.0f;

                    for (size_t layer = 0; layer <= max_layer; ++layer)
                    {
                        size_t count = layer_nodes[layer].size();
                        float total_h = (count > 1) ? (count - 1) * spacing_y : 0;
                        float start_y = center_y - total_h / 2.0f;

                        for (size_t i = 0; i < count; ++i)
                        {
                            size_t node_id = layer_nodes[layer][i];

                            NodeState ns;
                            ns.node_id = node_id;
                            ns.x = ns.pinned_x = start_x + layer * spacing_x;
                            ns.y = ns.pinned_y = start_y + i * spacing_y;

                            try
                            {
                                auto &data = graph_var_.get_node_data(node_id);
                                ns.label = std::to_string(node_id);
                                ns.metadata_str = data.str();
                            }
                            catch (...)
                            {
                                ns.label = std::to_string(node_id);
                                ns.metadata_str = "";
                            }

                            back_snapshot_.nodes.push_back(ns);
                        }
                    }
                }
                else
                {
                    // Circular layout for cyclic graphs or empty
                    float center_x = config_.window_width / 2.0f;
                    float center_y = config_.window_height / 2.0f;

                    if (n == 0)
                    {
                        // No nodes
                    }
                    else if (n == 1)
                    {
                        NodeState ns;
                        ns.node_id = 0;
                        ns.x = ns.pinned_x = center_x;
                        ns.y = ns.pinned_y = center_y;
                        try
                        {
                            auto &data = graph_var_.get_node_data(0);
                            ns.label = std::to_string(0);
                            ns.metadata_str = data.str();
                        }
                        catch (...)
                        {
                            ns.label = std::to_string(0);
                            ns.metadata_str = "";
                        }
                        back_snapshot_.nodes.push_back(ns);
                    }
                    else
                    {
                        float radius = std::min(250.0f, std::min(center_x, center_y) * 0.7f);
                        for (size_t i = 0; i < n; ++i)
                        {
                            NodeState ns;
                            ns.node_id = i;
                            float angle = (2.0f * 3.14159f * i) / n;
                            ns.x = ns.pinned_x = center_x + radius * std::cos(angle);
                            ns.y = ns.pinned_y = center_y + radius * std::sin(angle);
                            try
                            {
                                auto &data = graph_var_.get_node_data(i);
                                ns.label = std::to_string(i);
                                ns.metadata_str = data.str();
                            }
                            catch (...)
                            {
                                ns.label = std::to_string(i);
                                ns.metadata_str = "";
                            }
                            back_snapshot_.nodes.push_back(ns);
                        }
                    }
                }

                // Get edges
                size_t edge_count = 0;
                for (size_t u = 0; u < n; ++u)
                {
                    auto edges = graph_var_.get_edges(u);
                    for (size_t ei = 0; ei < edges.len(); ++ei)
                    {
                        auto e = edges[ei];
                        size_t v = static_cast<size_t>(e["to"].toInt());
                        double w = e["weight"].toDouble();
                        bool dir = static_cast<bool>(e["directed"]);

                        // For undirected edges, only add once (u < v)
                        if (!dir && u > v)
                            continue;

                        EdgeState es;
                        es.from = u;
                        es.to = v;
                        es.weight = w;
                        es.directed = dir;
                        back_snapshot_.edges.push_back(es);
                        edge_count++;
                    }
                }
                back_snapshot_.edge_count = edge_count;

                // Copy to front
                front_snapshot_ = back_snapshot_;

                // Center camera on the graph
                center_camera_on_graph();
            }

            void center_camera_on_graph()
            {
                // NOTE: caller should hold `snapshot_mutex_` to avoid races.
                if (back_snapshot_.nodes.empty())
                    return;

                // Find graph bounds
                float min_x = 1e9, max_x = -1e9;
                float min_y = 1e9, max_y = -1e9;

                for (const auto &node : back_snapshot_.nodes)
                {
                    if (node.x < -1000)
                        continue; // Skip hidden nodes
                    min_x = std::min(min_x, node.x);
                    max_x = std::max(max_x, node.x);
                    min_y = std::min(min_y, node.y);
                    max_y = std::max(max_y, node.y);
                }

                // Calculate graph center
                float graph_center_x = (min_x + max_x) / 2.0f;
                float graph_center_y = (min_y + max_y) / 2.0f;

                // User wants: Y at window mid, X at leftmost node + padding
                camera_x_ = min_x - 100.0f;                                // Leftmost + padding
                camera_y_ = graph_center_y - config_.window_height / 2.0f; // Center vertically
            }

            void sync_to_graph()
            {
                // This syncs pinned positions back but structure changes
                // are applied immediately during edit operations
            }

            // ----------------------------------------------------------------
            // Window management
            // ----------------------------------------------------------------

            bool init_window()
            {
                if (!glfwInit())
                {
                    return false;
                }

                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

                if (config_.antialiasing)
                {
                    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA
                }

                window_ = glfwCreateWindow(
                    config_.window_width,
                    config_.window_height,
                    config_.window_title.c_str(),
                    nullptr, nullptr);

                if (!window_)
                {
                    glfwTerminate();
                    return false;
                }


                glfwMakeContextCurrent(window_);
                glfwSwapInterval(1); // VSync

                // Enable multisampling (after context is current)
                if (config_.antialiasing)
                {
                    glEnable(GL_MULTISAMPLE);
                }

                // ImGui setup
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO &io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

                ImGui::StyleColorsDark();
                ImGui_ImplGlfw_InitForOpenGL(window_, true);
                // Use a GLSL version compatible with requested context
                ImGui_ImplOpenGL3_Init("#version 330 core");

                return true;
            }

            void cleanup_window()
            {
                if (window_)
                {
                    ImGui_ImplOpenGL3_Shutdown();
                    ImGui_ImplGlfw_Shutdown();
                    ImGui::DestroyContext();
                    glfwDestroyWindow(window_);
                    glfwTerminate();
                    window_ = nullptr;
                }
            }

            // ----------------------------------------------------------------
            // Physics
            // ----------------------------------------------------------------

            void start_physics()
            {
                physics_running_ = true;
                physics_thread_ = std::thread([this]()
                                              { physics_loop(); });
            }

            void stop_physics()
            {
                physics_running_ = false;
                if (physics_thread_.joinable())
                    physics_thread_.join();
            }

            void physics_loop()
            {
                auto last_time = std::chrono::high_resolution_clock::now();

                while (physics_running_)
                {
                    auto now = std::chrono::high_resolution_clock::now();
                    float dt = std::chrono::duration<float>(now - last_time).count();
                    last_time = now;

                    // Limit dt to prevent explosion
                    dt = std::min(dt, 0.05f);

                    update_physics(dt);
                    update_signals(dt);

                    // Swap snapshots
                    {
                        std::lock_guard<std::mutex> lock(snapshot_mutex_);
                        std::swap(front_snapshot_, back_snapshot_);
                    }

                    // ~60 FPS physics
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }
            }

            void update_physics(float dt)
            {
                auto &nodes = back_snapshot_.nodes;
                auto &edges = back_snapshot_.edges;
                size_t n = nodes.size();

                if (n == 0)
                    return;

                // Reset forces
                for (auto &node : nodes)
                {
                    node.fx = 0.0f;
                    node.fy = 0.0f;
                }

                // Repulsion between all nodes
                for (size_t i = 0; i < n; ++i)
                {
                    for (size_t j = i + 1; j < n; ++j)
                    {
                        float dx = nodes[i].x - nodes[j].x;
                        float dy = nodes[i].y - nodes[j].y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist < 1.0f)
                            dist = 1.0f;

                        float force = config_.repulsion / (dist * dist);
                        float fx = force * dx / dist;
                        float fy = force * dy / dist;

                        nodes[i].fx += fx;
                        nodes[i].fy += fy;
                        nodes[j].fx -= fx;
                        nodes[j].fy -= fy;
                    }
                }

                // Attraction along edges
                for (const auto &edge : edges)
                {
                    auto &n1 = nodes[edge.from];
                    auto &n2 = nodes[edge.to];

                    float dx = n2.x - n1.x;
                    float dy = n2.y - n1.y;
                    float dist = std::sqrt(dx * dx + dy * dy);

                    float diff = dist - config_.ideal_distance;
                    float force = config_.attraction * diff;

                    float fx = force * dx / (dist + 0.01f);
                    float fy = force * dy / (dist + 0.01f);

                    n1.fx += fx;
                    n1.fy += fy;
                    n2.fx -= fx;
                    n2.fy -= fy;
                }

                // Integrate velocities and positions
                float drag_damp = 0.6f; // Higher damping when not dragging

                for (auto &node : nodes)
                {
                    if (node.is_dragging)
                    {
                        // When dragging, don't apply physics - user controls position
                        node.vx = 0;
                        node.vy = 0;
                        node.fx = 0;
                        node.fy = 0;
                        continue;
                    }

                    // Simple Euler with damping
                    node.vx = (node.vx + node.fx * dt) * config_.damping;
                    node.vy = (node.vy + node.fy * dt) * config_.damping;

                    // Apply layout gravity (pull to center)
                    // This keeps the graph centered without being too rigid
                    float center_x = config_.window_width / 2.0f; // Virtual center
                    float center_y = config_.window_height / 2.0f;
                    float dx = center_x - 0.0f - node.x; // Pinned to 0,0 relative? No, center around origin of simulation
                    // Let's pull towards the centroid of the graph
                    // But for now, just pull towards 0,0 if not pinned

                    // View mode spring-back or edit mode freedom
                    if (mode_ == ViewerMode::VIEW && config_.snap_to_pinned_in_view)
                    {
                        // Strong spring back to pinned
                        float k = 5.0f;
                        node.vx += (node.pinned_x - node.x) * k * dt;
                        node.vy += (node.pinned_y - node.y) * k * dt;
                        // Reduce velocity heavily when snapping
                        node.vx *= 0.8f;
                        node.vy *= 0.8f;
                    }

                    node.x += node.vx * dt;
                    node.y += node.vy * dt;

                    // Decay activation
                    node.activation *= 0.90f; // Faster decay (fixes "permanent shiny" bug)
                    if (node.activation < 0.01f)
                        node.activation = 0.0f;
                }

                // Decay edge activity
                for (auto &edge : edges)
                {
                    edge.activity *= 0.92f;
                }
            }

            void update_signals(float dt)
            {
                std::lock_guard<std::mutex> lock(signals_mutex_);

                auto &nodes = back_snapshot_.nodes;
                auto &edges = back_snapshot_.edges;

                std::vector<Signal> next_signals;
                std::vector<std::pair<size_t, int>> arrivals; // (node_id, wave)

                // Limit total signals to prevent infinite propagation in cycles
                const size_t MAX_SIGNALS = edges.size() * 3; // At most 3x edge count

                for (auto &sig : signals_)
                {
                    if (!sig.active)
                        continue;

                    sig.progress += config_.signal_speed * dt;

                    // Activate edge
                    for (auto &edge : edges)
                    {
                        if ((edge.from == sig.from && edge.to == sig.to) ||
                            (!edge.directed && edge.from == sig.to && edge.to == sig.from))
                        {
                            edge.activity = std::min(1.0f, edge.activity + 0.5f);
                        }
                    }

                    if (sig.progress < 1.0f)
                    {
                        next_signals.push_back(sig);
                    }
                    else
                    {
                        // Signal arrived
                        if (sig.to < nodes.size())
                        {
                            nodes[sig.to].activation = std::min(2.0f, nodes[sig.to].activation + sig.strength);
                            // Only propagate if wave count is reasonable and we haven't hit signal limit
                            if (sig.wave < 20 && next_signals.size() < MAX_SIGNALS)
                            {
                                arrivals.push_back({sig.to, sig.wave});
                            }
                        }
                    }
                }

                // Propagate from arrived signals (but respect limits)
                for (const auto &[node_id, wave] : arrivals)
                {
                    if (next_signals.size() >= MAX_SIGNALS)
                        break; // Stop propagating if we hit limit

                    // Get outgoing edges
                    for (const auto &edge : edges)
                    {
                        if (next_signals.size() >= MAX_SIGNALS)
                            break;

                        bool should_propagate = false;
                        size_t next_node = 0;

                        if (edge.directed)
                        {
                            if (edge.from == node_id)
                            {
                                should_propagate = true;
                                next_node = edge.to;
                            }
                        }
                        else
                        {
                            // Undirected: propagate both ways
                            if (edge.from == node_id)
                            {
                                should_propagate = true;
                                next_node = edge.to;
                            }
                            else if (edge.to == node_id)
                            {
                                should_propagate = true;
                                next_node = edge.from;
                            }
                        }

                        if (should_propagate)
                        {
                            Signal new_sig;
                            new_sig.from = node_id;
                            new_sig.to = next_node;
                            new_sig.progress = 0.0f;
                            new_sig.strength = 0.8f; // Decay strength
                            new_sig.wave = wave + 1;
                            new_sig.active = true;
                            next_signals.push_back(new_sig);
                        }
                    }
                }

                signals_ = std::move(next_signals);
                back_snapshot_.signals = signals_;
            }

            void trigger_signal_at(size_t node_id)
            {
                std::lock_guard<std::mutex> lock(signals_mutex_);

                auto &edges = back_snapshot_.edges;

                // Activate node
                if (node_id < back_snapshot_.nodes.size())
                {
                    back_snapshot_.nodes[node_id].activation = 2.0f;
                }

                // Create outgoing signals for directed, both ways for undirected
                for (const auto &edge : edges)
                {
                    if (edge.directed)
                    {
                        if (edge.from == node_id)
                        {
                            Signal sig;
                            sig.from = edge.from;
                            sig.to = edge.to;
                            sig.progress = 0.0f;
                            sig.strength = 1.0f;
                            sig.wave = 0;
                            sig.active = true;
                            signals_.push_back(sig);
                        }
                    }
                    else
                    {
                        if (edge.from == node_id)
                        {
                            Signal sig;
                            sig.from = edge.from;
                            sig.to = edge.to;
                            sig.progress = 0.0f;
                            sig.strength = 1.0f;
                            sig.wave = 0;
                            sig.active = true;
                            signals_.push_back(sig);
                        }
                        else if (edge.to == node_id)
                        {
                            Signal sig;
                            sig.from = edge.to;
                            sig.to = edge.from;
                            sig.progress = 0.0f;
                            sig.strength = 1.0f;
                            sig.wave = 0;
                            sig.active = true;
                            signals_.push_back(sig);
                        }
                    }
                }
            }

            // ----------------------------------------------------------------
            // Rendering
            // ----------------------------------------------------------------

            void render()
            {
                // Always read from front_snapshot_ for rendering consistency
                GraphSnapshot snapshot;
                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    snapshot = front_snapshot_;
                }

                ImDrawList *dl = ImGui::GetBackgroundDrawList();

                // Calculate graph center for handle
                float sum_x = 0.0f, sum_y = 0.0f;
                int n_count = 0;
                for (const auto &node : snapshot.nodes)
                {
                    sum_x += node.x;
                    sum_y += node.y;
                    n_count++;
                }
                float graph_center_x = n_count > 0 ? sum_x / n_count : 0.0f;
                float graph_center_y = n_count > 0 ? sum_y / n_count : 0.0f;

                // Apply camera transform
                auto transform = [&](float x, float y) -> ImVec2
                {
                    float tx = (x - camera_x_) * zoom_ + config_.window_width / 2.0f;
                    float ty = (y - camera_y_) * zoom_ + config_.window_height / 2.0f;
                    return ImVec2(tx, ty);
                };

                // Draw edges
                for (const auto &edge : snapshot.edges)
                {
                    if (edge.from >= snapshot.nodes.size() || edge.to >= snapshot.nodes.size())
                        continue;

                    const auto &n1 = snapshot.nodes[edge.from];
                    const auto &n2 = snapshot.nodes[edge.to];

                    ImVec2 p1 = transform(n1.x, n1.y);
                    ImVec2 p2 = transform(n2.x, n2.y);

                    // Get edge activity for tension effect
                    float activity = edge.activity;
                    float tension_boost = activity * 2.0f;

                    float width = (config_.edge_thickness + tension_boost) * zoom_;
                    ImU32 col;

                    if (edge.is_hovered || n1.is_hovered || n2.is_hovered)
                    {
                        col = IM_COL32(200, 100, 255, 255); // Purple
                    }
                    else
                    {
                        int alpha = (int)(100 + activity * 155);
                        alpha = std::min(255, alpha);
                        col = IM_COL32(100, 100, 120, alpha);
                    }

                    dl->AddLine(p1, p2, col, width);

                    // Arrow for directed edges
                    if (edge.directed)
                    {
                        float dx = p2.x - p1.x, dy = p2.y - p1.y;
                        float len = std::sqrt(dx * dx + dy * dy);
                        if (len > 1.0f)
                        {
                            dx /= len;
                            dy /= len;

                            // Calculate arrow position - place it just before the target node
                            float node_radius = config_.node_radius * zoom_;
                            float arrow_size = 10.0f * zoom_; // Larger arrows

                            // Position arrow just outside the target node circle
                            float arrow_pos = len - node_radius - arrow_size;
                            arrow_pos = std::max(len * 0.5f, arrow_pos); // At least halfway if nodes are close

                            float ax = p1.x + dx * arrow_pos;
                            float ay = p1.y + dy * arrow_pos;

                            ImVec2 p_tip(ax + dx * arrow_size, ay + dy * arrow_size);
                            ImVec2 p_left(ax - dy * arrow_size * 0.5f, ay + dx * arrow_size * 0.5f);
                            ImVec2 p_right(ax + dy * arrow_size * 0.5f, ay - dx * arrow_size * 0.5f);

                            dl->AddTriangleFilled(p_tip, p_left, p_right, col);
                        }
                    }
                }

                // Draw signals
                for (const auto &sig : snapshot.signals)
                {
                    if (sig.from >= snapshot.nodes.size() || sig.to >= snapshot.nodes.size())
                        continue;
                    const auto &n1 = snapshot.nodes[sig.from];
                    const auto &n2 = snapshot.nodes[sig.to];

                    float x = n1.x + (n2.x - n1.x) * sig.progress;
                    float y = n1.y + (n2.y - n1.y) * sig.progress;
                    ImVec2 pos = transform(x, y);

                    float radius = 5.0f * zoom_;
                    dl->AddCircleFilled(pos, radius, IM_COL32(255, 215, 0, 200));
                }

                // Draw nodes
                size_t node_idx = 0;
                for (const auto &node : snapshot.nodes)
                {
                    ImVec2 pos = transform(node.x, node.y);

                    // Visual radius based on activation
                    float base_rad = config_.node_radius * zoom_;
                    float visual_rad = base_rad + (node.activation * 3.0f * zoom_);

                    ImU32 fill_col;
                    ImU32 border_col = IM_COL32(200, 200, 200, 255);

                    // Color priority: edge creation source (blue) > selected (green) > hovered (purple) > normal (gray)
                    if (node_idx == edge_start_node_ && creating_edge_)
                    {
                        fill_col = IM_COL32(50, 150, 255, 255); // Blue for edge creation source
                        border_col = IM_COL32(100, 200, 255, 255);
                    }
                    else if (node.is_selected)
                        fill_col = IM_COL32(50, 180, 50, 255);
                    else if (node.is_hovered)
                        fill_col = IM_COL32(180, 100, 220, 255);
                    else
                        fill_col = IM_COL32(80, 80, 100, 255);

                    if (node.activation > 0.01f)
                    {
                        // Add glow using layers of circles
                        float intensity = std::min(node.activation, 1.0f);
                        int glow_alpha = (int)(intensity * 100);

                        // Outer glow
                        dl->AddCircleFilled(pos, visual_rad * 1.4f, IM_COL32(100, 100, 255, glow_alpha));

                        // Inner brightness
                        int boost = (int)(intensity * 100);
                        fill_col = IM_COL32(
                            std::min(255, (int)((fill_col & 0xFF) + boost)),
                            std::min(255, (int)(((fill_col >> 8) & 0xFF) + boost)),
                            std::min(255, (int)(((fill_col >> 16) & 0xFF) + boost)),
                            255);
                    }

                    dl->AddCircleFilled(pos, visual_rad, fill_col);
                    dl->AddCircle(pos, visual_rad, border_col, 0, 1.5f * zoom_);

                    // Label
                    if (zoom_ > 0.5f)
                    {
                        ImVec2 txt_pos(pos.x + visual_rad + 2, pos.y - 7);
                        dl->AddText(txt_pos, IM_COL32(255, 255, 255, 200), node.label.c_str());
                    }

                    node_idx++;
                }

                // Center Handle (only show when hovering or actively dragging it)
                if (n_count > 0 && mode_ == ViewerMode::VIEW)
                {
                    ImVec2 center_screen = transform(graph_center_x, graph_center_y);

                    // Check mouse distance
                    double mx, my;
                    glfwGetCursorPos(window_, &mx, &my);
                    float d = distance((float)mx, (float)my, center_screen.x, center_screen.y);

                    // Only show when hovering closely OR actively dragging the graph center
                    if (d < 20.0f || dragging_graph_center_)
                    {
                        float pulse = 0.5f + 0.3f * std::sin(ImGui::GetTime() * 3.0f);
                        int alpha = 100 + (int)(pulse * 100);

                        dl->AddCircleFilled(center_screen, 15.0f, IM_COL32(100, 150, 255, alpha));
                        dl->AddCircle(center_screen, 15.0f, IM_COL32(200, 200, 255, 255), 0, 2.0f);

                        // Crosshair
                        dl->AddLine(ImVec2(center_screen.x - 6, center_screen.y), ImVec2(center_screen.x + 6, center_screen.y), IM_COL32(255, 255, 255, 200));
                        dl->AddLine(ImVec2(center_screen.x, center_screen.y - 6), ImVec2(center_screen.x, center_screen.y + 6), IM_COL32(255, 255, 255, 200));
                    }
                }

                // Edge Creation Line
                if (creating_edge_)
                {
                    if (edge_start_node_ >= 0 && edge_start_node_ < snapshot.nodes.size())
                    {
                        const auto &n = snapshot.nodes[edge_start_node_];
                        ImVec2 p1 = transform(n.x, n.y);
                        double mx, my;
                        glfwGetCursorPos(window_, &mx, &my);
                        ImVec2 p2((float)mx, (float)my);

                        dl->AddLine(p1, p2, IM_COL32(50, 200, 50, 200), 2.0f);
                        dl->AddCircleFilled(p2, 4.0f, IM_COL32(50, 200, 50, 200));
                    }
                }
            }

            void render_ui()
            {
                // Control panel
                ImGui::Begin("Graph Viewer");

                // Mode toggle
                const char *mode_icon = (mode_ == ViewerMode::VIEW) ? "🔒 View Mode" : "✏️ Edit Mode";
                if (ImGui::Button(mode_icon))
                {
                    mode_ = (mode_ == ViewerMode::VIEW) ? ViewerMode::EDIT : ViewerMode::VIEW;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(Click to toggle)");

                ImGui::Separator();

                // Stats (read from front for consistency)
                size_t node_ct, edge_ct;
                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    node_ct = front_snapshot_.node_count;
                    edge_ct = front_snapshot_.edge_count;
                }
                ImGui::Text("Nodes: %zu", node_ct);
                ImGui::Text("Edges: %zu", edge_ct);
                ImGui::Text("Active Signals: %zu", signals_.size());

                ImGui::Separator();

                // Physics controls
                if (ImGui::CollapsingHeader("Physics"))
                {
                    ImGui::SliderFloat("Repulsion", &config_.repulsion, 10.0f, 500.0f);
                    ImGui::SliderFloat("Attraction", &config_.attraction, 0.01f, 0.5f);
                    ImGui::SliderFloat("Ideal Distance", &config_.ideal_distance, 50.0f, 400.0f);
                    ImGui::SliderFloat("Damping", &config_.damping, 0.5f, 0.99f);
                    ImGui::SliderFloat("Signal Speed", &config_.signal_speed, 0.5f, 5.0f);
                }

                // Layout controls
                if (ImGui::CollapsingHeader("Layout"))
                {
                    if (ImGui::Checkbox("Auto Topological Sort", &config_.auto_topo_sort))
                    {
                        // Changed - if enabled, do one layout now
                        if (config_.auto_topo_sort)
                        {
                            do_topological_relayout();
                        }
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(?)");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Automatically re-layout nodes when edges are added.\nDisable for large graphs to avoid lag.");
                    }

                    if (ImGui::Button("Relayout Now"))
                    {
                        do_topological_relayout();
                    }
                }

                // Visual controls
                if (ImGui::CollapsingHeader("Visuals"))
                {
                    ImGui::Checkbox("Antialiasing", &config_.antialiasing);
                    ImGui::Checkbox("Glow Effects", &config_.glow_enabled);
                    ImGui::SliderFloat("Node Radius", &config_.node_radius, 5.0f, 30.0f);
                    ImGui::SliderFloat("Edge Thickness", &config_.edge_thickness, 1.0f, 5.0f);
                }

                // Edit mode controls
                if (mode_ == ViewerMode::EDIT)
                {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Edit Mode Active");
                    ImGui::Text("Click node: Select");
                    ImGui::Text("Click node -> Click node: Add edge");
                    ImGui::Text("Click empty: Cancel edge creation");
                    ImGui::Text("Double-click empty: Add node");
                    ImGui::Text("Delete key: Remove selected");

                    if (ImGui::Button("Add Node"))
                    {
                        sidebar_open_ = true;
                    }

                    ImGui::SameLine();
                    int selected_idx;
                    {
                        std::lock_guard<std::mutex> lock(snapshot_mutex_);
                        selected_idx = back_snapshot_.selected_node;
                    }

                    if (selected_idx >= 0)
                    {
                        if (ImGui::Button("Delete Selected Node"))
                        {
                            delete_selected_node();
                        }
                    }
                    else
                    {
                        ImGui::BeginDisabled();
                        ImGui::Button("Delete Selected Node");
                        ImGui::EndDisabled();
                    }

                    if (ImGui::Button("Relayout (Topological)"))
                    {
                        do_topological_relayout();
                    }
                }

                ImGui::End();

                // Sidebar for node creation
                if (sidebar_open_)
                {
                    render_sidebar();
                }

                // Node info popup on hover (read from front_snapshot_)
                int hovered_idx;
                std::string hovered_label, hovered_meta;
                size_t hovered_id = 0;
                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    hovered_idx = front_snapshot_.hovered_node;
                    if (hovered_idx >= 0 && (size_t)hovered_idx < front_snapshot_.nodes.size())
                    {
                        const auto &node = front_snapshot_.nodes[hovered_idx];
                        hovered_id = node.node_id;
                        hovered_label = node.label;
                        hovered_meta = node.metadata_str;
                    }
                }
                if (hovered_idx >= 0 && !hovered_label.empty())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Node %zu", hovered_id);
                    if (!hovered_meta.empty())
                    {
                        ImGui::Separator();
                        ImGui::TextWrapped("%s", hovered_meta.c_str());
                    }
                    ImGui::EndTooltip();
                }
            }

            void render_sidebar()
            {
                ImGui::Begin("New Node", &sidebar_open_, ImGuiWindowFlags_AlwaysAutoResize);

                ImGui::InputText("Label", new_node_label_, sizeof(new_node_label_));
                ImGui::InputTextMultiline("Metadata", new_node_metadata_, sizeof(new_node_metadata_),
                                          ImVec2(200, 100));

                if (ImGui::Button("Create Node"))
                {
                    add_node_at_center();
                    sidebar_open_ = false;
                    new_node_label_[0] = '\0';
                    new_node_metadata_[0] = '\0';
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel"))
                {
                    sidebar_open_ = false;
                }

                ImGui::End();
            }

            // ----------------------------------------------------------------
            // Edit operations
            // ----------------------------------------------------------------

            void add_node_at_center()
            {
                // Add to actual graph
                size_t new_id;
                if (strlen(new_node_metadata_) > 0)
                {
                    pythonic::vars::var meta{std::string(new_node_metadata_)};
                    new_id = graph_var_.add_node(meta);
                }
                else
                {
                    new_id = graph_var_.add_node();
                }

                // Add to snapshot
                NodeState ns;
                ns.node_id = new_id;
                ns.label = strlen(new_node_label_) > 0 ? new_node_label_ : std::to_string(new_id);
                ns.metadata_str = new_node_metadata_;

                // Position near center of view (in world space)
                ns.x = ns.pinned_x = camera_x_;
                ns.y = ns.pinned_y = camera_y_;

                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    // CRITICAL: Add to BOTH snapshots so indices stay in sync
                    back_snapshot_.nodes.push_back(ns);
                    back_snapshot_.node_count++;
                    front_snapshot_.nodes.push_back(ns);
                    front_snapshot_.node_count++;
                }
            }

            void add_node_at_position(float x, float y)
            {
                size_t new_id = graph_var_.add_node();

                NodeState ns;
                ns.node_id = new_id;
                ns.label = std::to_string(new_id);
                ns.x = ns.pinned_x = x;
                ns.y = ns.pinned_y = y;

                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    // CRITICAL: Add to BOTH snapshots so indices stay in sync
                    back_snapshot_.nodes.push_back(ns);
                    back_snapshot_.node_count++;
                    front_snapshot_.nodes.push_back(ns);
                    front_snapshot_.node_count++;
                }
            }

            void add_edge(size_t from_idx, size_t to_idx)
            {
                // CRITICAL: from_idx/to_idx are snapshot vector indices, not node_ids!
                // Must map to actual graph node_ids before calling graph_var_.add_edge
                size_t from_id, to_id;
                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    if (from_idx >= back_snapshot_.nodes.size() || to_idx >= back_snapshot_.nodes.size())
                        return;
                    from_id = back_snapshot_.nodes[from_idx].node_id;
                    to_id = back_snapshot_.nodes[to_idx].node_id;

                    // Check for duplicate edges
                    for (const auto &e : back_snapshot_.edges)
                    {
                        if (e.from == from_idx && e.to == to_idx)
                        {
                            // Edge already exists
                            return;
                        }
                        // For undirected edges, also check reverse
                        if (!e.directed && e.from == to_idx && e.to == from_idx)
                        {
                            return;
                        }
                    }
                }

                // Add to actual graph using node_id (not index!)
                graph_var_.add_edge(from_id, to_id, 1.0, 0.0, true);

                EdgeState es;
                es.from = from_idx; // Store index for snapshot rendering
                es.to = to_idx;
                es.weight = 1.0;
                es.directed = true;

                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    // CRITICAL: Add to BOTH snapshots so edge is visible immediately
                    back_snapshot_.edges.push_back(es);
                    back_snapshot_.edge_count++;
                    front_snapshot_.edges.push_back(es);
                    front_snapshot_.edge_count++;
                }

                // Trigger auto topological sort if enabled
                if (config_.auto_topo_sort)
                {
                    do_topological_relayout();
                }
            }

            void delete_selected_node()
            {
                if (back_snapshot_.selected_node < 0)
                    return;

                // Note: Graph class may not support node removal
                // For now, we just hide it from the snapshot
                // Full implementation would need Graph::remove_node()

                size_t node_to_remove = back_snapshot_.selected_node;

                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);

                    // Remove edges connected to this node in BOTH snapshots
                    back_snapshot_.edges.erase(
                        std::remove_if(back_snapshot_.edges.begin(), back_snapshot_.edges.end(),
                                       [node_to_remove](const EdgeState &e)
                                       {
                                           return e.from == node_to_remove || e.to == node_to_remove;
                                       }),
                        back_snapshot_.edges.end());

                    front_snapshot_.edges.erase(
                        std::remove_if(front_snapshot_.edges.begin(), front_snapshot_.edges.end(),
                                       [node_to_remove](const EdgeState &e)
                                       {
                                           return e.from == node_to_remove || e.to == node_to_remove;
                                       }),
                        front_snapshot_.edges.end());

                    // Mark node as hidden in BOTH snapshots
                    if (node_to_remove < back_snapshot_.nodes.size())
                    {
                        back_snapshot_.nodes[node_to_remove].x = -10000;
                        back_snapshot_.nodes[node_to_remove].y = -10000;
                        back_snapshot_.nodes[node_to_remove].pinned_x = -10000;
                        back_snapshot_.nodes[node_to_remove].pinned_y = -10000;
                    }
                    if (node_to_remove < front_snapshot_.nodes.size())
                    {
                        front_snapshot_.nodes[node_to_remove].x = -10000;
                        front_snapshot_.nodes[node_to_remove].y = -10000;
                        front_snapshot_.nodes[node_to_remove].pinned_x = -10000;
                        front_snapshot_.nodes[node_to_remove].pinned_y = -10000;
                    }

                    back_snapshot_.selected_node = -1;
                    front_snapshot_.selected_node = -1;
                }
            }

            void do_topological_relayout()
            {
                // Get topological order and compute proper layers
                try
                {
                    auto topo = graph_var_.topological_sort();
                    size_t n = back_snapshot_.nodes.size();

                    // Compute layers based on longest path from source
                    std::vector<size_t> layers(n, 0);
                    for (size_t i = 0; i < topo.len(); ++i)
                    {
                        size_t node_id = static_cast<size_t>(topo[i].toInt());
                        size_t max_layer = 0;

                        // Find max layer of all predecessors
                        for (size_t pred = 0; pred < n; ++pred)
                        {
                            auto edges = graph_var_.get_edges(pred);
                            for (size_t ei = 0; ei < edges.len(); ++ei)
                            {
                                auto e = edges[ei];
                                if (static_cast<size_t>(e["to"].toInt()) == node_id)
                                {
                                    max_layer = std::max(max_layer, layers[pred] + 1);
                                }
                            }
                        }
                        layers[node_id] = max_layer;
                    }

                    // Group nodes by layer
                    size_t max_layer = *std::max_element(layers.begin(), layers.end());
                    std::vector<std::vector<size_t>> layer_nodes(max_layer + 1);
                    for (size_t i = 0; i < n; ++i)
                    {
                        layer_nodes[layers[i]].push_back(i);
                    }

                    // Layout parameters
                    float start_x = 150.0f;
                    float spacing_x = 200.0f;
                    float spacing_y = 80.0f;
                    float center_y = config_.window_height / 2.0f;

                    std::lock_guard<std::mutex> lock(snapshot_mutex_);

                    // Position nodes in layers
                    for (size_t layer = 0; layer <= max_layer; ++layer)
                    {
                        size_t count = layer_nodes[layer].size();
                        float total_h = (count > 1) ? (count - 1) * spacing_y : 0;
                        float start_y = center_y - total_h / 2.0f;

                        for (size_t i = 0; i < count; ++i)
                        {
                            size_t node_id = layer_nodes[layer][i];
                            if (node_id < back_snapshot_.nodes.size())
                            {
                                float new_x = start_x + layer * spacing_x;
                                float new_y = start_y + i * spacing_y;

                                back_snapshot_.nodes[node_id].pinned_x = new_x;
                                back_snapshot_.nodes[node_id].pinned_y = new_y;
                                back_snapshot_.nodes[node_id].x = new_x;
                                back_snapshot_.nodes[node_id].y = new_y;

                                // Update front snapshot too to avoid jitter
                                if (node_id < front_snapshot_.nodes.size())
                                {
                                    front_snapshot_.nodes[node_id].pinned_x = new_x;
                                    front_snapshot_.nodes[node_id].pinned_y = new_y;
                                    front_snapshot_.nodes[node_id].x = new_x;
                                    front_snapshot_.nodes[node_id].y = new_y;
                                }
                            }
                        }
                    }
                }
                catch (...)
                {
                    // Graph has cycle, can't topologically sort
                    // Just do force-directed layout, which is already running
                }
            }

            // ----------------------------------------------------------------
            // Input handling
            // ----------------------------------------------------------------

            void handle_input()
            {
                ImGuiIO &io = ImGui::GetIO();

                if (io.WantCaptureMouse || io.WantCaptureKeyboard)
                    return;

                double mx, my;
                glfwGetCursorPos(window_, &mx, &my);

                // Convert to world coordinates
                float world_x = (mx - config_.window_width / 2.0f) / zoom_ + camera_x_;
                float world_y = (my - config_.window_height / 2.0f) / zoom_ + camera_y_;

                // Find hovered node (read from front_snapshot_ for stable hover)
                int hovered = -1;
                float min_dist = config_.node_radius * 2.0f / zoom_;

                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    // Read positions from front_snapshot_ for stable hover detection
                    for (size_t i = 0; i < front_snapshot_.nodes.size(); ++i)
                    {
                        const auto &node = front_snapshot_.nodes[i];
                        float d = distance(world_x, world_y, node.x, node.y);
                        if (d < min_dist)
                        {
                            hovered = i;
                            min_dist = d;
                        }
                    }

                    // Update hover state in BOTH snapshots for consistent rendering
                    for (auto &node : back_snapshot_.nodes)
                        node.is_hovered = false;
                    for (auto &node : front_snapshot_.nodes)
                        node.is_hovered = false;

                    if (hovered >= 0)
                    {
                        if (hovered < back_snapshot_.nodes.size())
                            back_snapshot_.nodes[hovered].is_hovered = true;
                        if (hovered < front_snapshot_.nodes.size())
                            front_snapshot_.nodes[hovered].is_hovered = true;
                    }
                    back_snapshot_.hovered_node = hovered;
                    front_snapshot_.hovered_node = hovered;
                }

                // Mouse wheel zoom
                if (io.MouseWheel != 0)
                {
                    float zoom_factor = io.MouseWheel > 0 ? 1.1f : 0.9f;
                    zoom_ *= zoom_factor;
                    zoom_ = std::clamp(zoom_, 0.2f, 5.0f);
                }

                // Handle camera drag vs graph drag
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    if (hovered >= 0)
                    {
                        // Edit mode: check if creating edge
                        if (mode_ == ViewerMode::EDIT && edge_start_node_ >= 0 && edge_start_node_ != hovered)
                        {
                            // Second click - create edge from edge_start_node_ to hovered
                            add_edge(edge_start_node_, hovered);
                            edge_start_node_ = -1;
                            creating_edge_ = false;
                        }
                        else if (mode_ == ViewerMode::EDIT && edge_start_node_ < 0)
                        {
                            // First click - start edge creation
                            edge_start_node_ = hovered;
                            creating_edge_ = true;
                        }
                        else
                        {
                            // Start node drag
                            dragged_node_ = hovered;
                            // Set is_dragging in BOTH snapshots to avoid visual lag
                            std::lock_guard<std::mutex> lock(snapshot_mutex_);
                            if (dragged_node_ < back_snapshot_.nodes.size())
                                back_snapshot_.nodes[dragged_node_].is_dragging = true;
                            if (dragged_node_ < front_snapshot_.nodes.size())
                                front_snapshot_.nodes[dragged_node_].is_dragging = true;
                        }
                    }
                    else
                    {
                        // Clicking empty space cancels edge creation
                        if (mode_ == ViewerMode::EDIT)
                        {
                            edge_start_node_ = -1;
                            creating_edge_ = false;
                        }

                        // Check if hitting center handle (use front_snapshot_ for stability)
                        std::lock_guard<std::mutex> lock(snapshot_mutex_);
                        float sx = 0, sy = 0;
                        int c = 0;
                        for (const auto &n : front_snapshot_.nodes)
                        {
                            sx += n.x;
                            sy += n.y;
                            c++;
                        }
                        float cx = c > 0 ? sx / c : 0;
                        float cy = c > 0 ? sy / c : 0;

                        // Convert center to screen
                        float scr_cx = (cx - camera_x_) * zoom_ + config_.window_width / 2.0f;
                        float scr_cy = (cy - camera_y_) * zoom_ + config_.window_height / 2.0f;

                        float dist = distance((float)mx, (float)my, scr_cx, scr_cy);

                        if (dist < 20.0f && mode_ == ViewerMode::VIEW)
                        {
                            // Dragging the graph center handle
                            dragging_graph_center_ = true;
                        }
                        else
                        {
                            // Background drag = Pan camera
                            dragging_camera_ = true;
                        }
                    }
                    last_mouse_x_ = mx;
                    last_mouse_y_ = my;
                }

                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    float dx = (mx - last_mouse_x_);
                    float dy = (my - last_mouse_y_);

                    if (dragged_node_ >= 0 && !creating_edge_)
                    {
                        std::lock_guard<std::mutex> lock(snapshot_mutex_);
                        if (dragged_node_ < back_snapshot_.nodes.size())
                        {
                            auto &node = back_snapshot_.nodes[dragged_node_];
                            // Move node in world space
                            node.x += dx / zoom_;
                            node.y += dy / zoom_;

                            // CRITICAL: Also update front_snapshot_ immediately to reduce jitter
                            if (dragged_node_ < front_snapshot_.nodes.size())
                            {
                                front_snapshot_.nodes[dragged_node_].x = node.x;
                                front_snapshot_.nodes[dragged_node_].y = node.y;
                            }

                            if (mode_ == ViewerMode::EDIT)
                            {
                                node.pinned_x = node.x;
                                node.pinned_y = node.y;
                                if (dragged_node_ < front_snapshot_.nodes.size())
                                {
                                    front_snapshot_.nodes[dragged_node_].pinned_x = node.pinned_x;
                                    front_snapshot_.nodes[dragged_node_].pinned_y = node.pinned_y;
                                }
                            }
                        }
                    }
                    else if (dragging_graph_center_)
                    {
                        // Move all nodes (pinned + current) - graph center handle drag
                        std::lock_guard<std::mutex> lock(snapshot_mutex_);
                        float world_dx = dx / zoom_;
                        float world_dy = dy / zoom_;
                        for (auto &n : back_snapshot_.nodes)
                        {
                            n.x += world_dx;
                            n.y += world_dy;
                            n.pinned_x += world_dx;
                            n.pinned_y += world_dy;
                        }
                        // Also update front to reduce lag
                        for (auto &n : front_snapshot_.nodes)
                        {
                            n.x += world_dx;
                            n.y += world_dy;
                            n.pinned_x += world_dx;
                            n.pinned_y += world_dy;
                        }
                    }
                    else if (dragging_camera_)
                    {
                        // Pan camera view
                        camera_x_ -= dx / zoom_;
                        camera_y_ -= dy / zoom_;
                    }
                    last_mouse_x_ = mx;
                    last_mouse_y_ = my;
                }

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    if (dragged_node_ >= 0)
                    {
                        // Clear is_dragging in BOTH snapshots
                        std::lock_guard<std::mutex> lock(snapshot_mutex_);
                        if (dragged_node_ < back_snapshot_.nodes.size())
                            back_snapshot_.nodes[dragged_node_].is_dragging = false;
                        if (dragged_node_ < front_snapshot_.nodes.size())
                            front_snapshot_.nodes[dragged_node_].is_dragging = false;
                    }

                    dragged_node_ = -1;
                    dragging_camera_ = false;
                    dragging_graph_center_ = false;
                    // Don't clear creating_edge_ or edge_start_node_ - user might click another node
                }

                // Double click to add node (edit mode) or trigger signal (view mode)
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (mode_ == ViewerMode::EDIT)
                    {
                        if (hovered < 0)
                        {
                            // Add node at cursor
                            sidebar_open_ = true;
                            // Store position for when node is created
                        }
                    }
                    else
                    {
                        // View mode: trigger signal
                        if (hovered >= 0)
                        {
                            trigger_signal_at(hovered);
                        }
                    }
                }

                // Click to select (view mode: trigger signal)
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (mode_ == ViewerMode::VIEW && hovered >= 0)
                    {
                        trigger_signal_at(hovered);
                    }
                    else if (mode_ == ViewerMode::EDIT)
                    {
                        std::lock_guard<std::mutex> lock(snapshot_mutex_);
                        // Clear old selection in BOTH snapshots
                        for (auto &n : back_snapshot_.nodes)
                            n.is_selected = false;
                        for (auto &n : front_snapshot_.nodes)
                            n.is_selected = false;
                        if (hovered >= 0)
                        {
                            if (hovered < back_snapshot_.nodes.size())
                                back_snapshot_.nodes[hovered].is_selected = true;
                            if (hovered < front_snapshot_.nodes.size())
                                front_snapshot_.nodes[hovered].is_selected = true;
                            back_snapshot_.selected_node = hovered;
                            front_snapshot_.selected_node = hovered;
                        }
                        else
                        {
                            back_snapshot_.selected_node = -1;
                            front_snapshot_.selected_node = -1;
                        }
                    }
                }

                // Delete key
                if (glfwGetKey(window_, GLFW_KEY_DELETE) == GLFW_PRESS ||
                    glfwGetKey(window_, GLFW_KEY_BACKSPACE) == GLFW_PRESS)
                {
                    if (mode_ == ViewerMode::EDIT)
                    {
                        delete_selected_node();
                    }
                }

                // Escape to close
                if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                {
                    close_requested_ = true;
                }
            }

            // ----------------------------------------------------------------
            // Main loop
            // ----------------------------------------------------------------

            void run_loop(bool blocking)
            {
                if (!init_window())
                {
                    throw std::runtime_error("Failed to initialize window");
                }

                start_physics();
                running_ = true;

                while (!glfwWindowShouldClose(window_) && !close_requested_)
                {
                    glfwPollEvents();

                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplGlfw_NewFrame();
                    ImGui::NewFrame();

                    handle_input();
                    render();
                    render_ui();

                    ImGui::Render();

                    int display_w, display_h;
                    glfwGetFramebufferSize(window_, &display_w, &display_h);
                    glViewport(0, 0, display_w, display_h);
                    glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                    glfwSwapBuffers(window_);

                    if (!blocking)
                        break;
                }

                stop_physics();
                running_ = false;
            }
        };

        // ============================================================================
        // GRAPHVIEWER PUBLIC INTERFACE
        // ============================================================================

        GraphViewer::GraphViewer(pythonic::vars::var &graph_var)
            : impl_(std::make_unique<Impl>(graph_var))
        {
        }

        GraphViewer::~GraphViewer() = default;

        void GraphViewer::run(bool blocking)
        {
            impl_->run_loop(blocking);
        }

        void GraphViewer::request_close()
        {
            impl_->close_requested_ = true;
        }

        bool GraphViewer::is_running() const
        {
            return impl_->running_;
        }

        ViewerMode GraphViewer::get_mode() const
        {
            return impl_->mode_;
        }

        void GraphViewer::set_mode(ViewerMode mode)
        {
            impl_->mode_ = mode;
        }

        void GraphViewer::toggle_mode()
        {
            impl_->mode_ = (impl_->mode_ == ViewerMode::VIEW) ? ViewerMode::EDIT : ViewerMode::VIEW;
        }

        ViewerConfig &GraphViewer::config()
        {
            return impl_->config_;
        }

        void GraphViewer::trigger_signal(size_t node_id)
        {
            impl_->trigger_signal_at(node_id);
        }

        void GraphViewer::relayout()
        {
            impl_->do_topological_relayout();
        }

        // ============================================================================
        // CONVENIENCE FUNCTIONS
        // ============================================================================

        void show_graph(pythonic::vars::var &g, bool blocking)
        {
            GraphViewer viewer(g);
            viewer.run(blocking);
        }

        void show_graph(const pythonic::vars::var &g, bool blocking)
        {
            // Create a copy for viewing (can't modify const)
            pythonic::vars::var copy = g;
            GraphViewer viewer(copy);
            viewer.set_mode(ViewerMode::VIEW); // Force view mode
            viewer.run(blocking);
        }

    } // namespace viewer
} // namespace pythonic

#endif // PYTHONIC_ENABLE_GRAPH_VIEWER
