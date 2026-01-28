#include <pythonic/pythonic.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <chrono>

using namespace py;
using namespace pythonic::fast;

// ==================================================================================
// HELPER MACROS / CONSTANTS
// ==================================================================================
const int IDX_X = 0;
const int IDX_Y = 1;
const int IDX_VX = 2;
const int IDX_VY = 3;
const int IDX_BIAS = 4;
const int IDX_ACT = 5;
const int IDX_LAYER = 6;
const int IDX_FX = 7;
const int IDX_FY = 8;

const int SIG_FROM = 0;
const int SIG_TO = 1;
const int SIG_PROG = 2;
const int SIG_STR = 3;
const int SIG_TYPE = 4;
const int SIG_ACTIVE = 5;

// Signal trail for comet effect
struct SignalTrail
{
    int from, to;
    float prog;
    int type;
    float str;
    float age; // 0.0 = newest, 1.0 = oldest
};

const int EDGE_U = 0;
const int EDGE_V = 1;
const int EDGE_W = 2;
const int EDGE_TYPE = 3;

// ==================================================================================
// SIMULATION CLASS
// ==================================================================================

class SynapseSim
{
public:
    var net;
    var nodes;
    var edge_cache;
    var signals;
    var config;

    CachedAdd add_op;
    CachedSub sub_op;
    CachedMul mul_op;
    CachedDiv div_op;

    // Native caches for performance
    std::mutex nodes_mutex;
    std::mutex signals_mutex;
    int num_threads = 6;

    // CRITICAL: Signal counter to avoid O(N) iteration every trigger
    std::atomic<int> signal_count{0};

    // Adjacency list for fast edge lookup - CRITICAL OPTIMIZATION
    std::vector<std::vector<int>> forward_edges;  // forward_edges[u] = list of edge indices
    std::vector<std::vector<int>> backward_edges; // backward_edges[v] = list of edge indices

    // Edge activity for visual tension effect
    std::vector<float> edge_activity;       // 0.0 to 1.0, decays over time
    std::vector<SignalTrail> signal_trails; // Trail history for comet effect

    // Topology Types
    enum Topology
    {
        TOP_SIMPLE,
        TOP_DEEP,
        TOP_DENSE,
        TOP_RANDOM,
        TOP_RESIDUAL,
        TOP_CUSTOM
    };
    int current_topology = TOP_SIMPLE;
    int dragged_node_idx = -1;
    bool dragging_graph = false; // Dragging entire graph via center handle
    double graph_drag_last_x = 0.0;
    double graph_drag_last_y = 0.0;
    int max_layer_idx = 0;

    // Custom topology string (e.g., "40-3-14-2")
    std::string custom_topology_str = "4-6-2";
    bool custom_use_residual = false; // Enable skip connections for custom topology

    // Phase Control
    enum Phase
    {
        PHASE_INPUT,
        PHASE_FORWARD,
        PHASE_BACKWARD
    };
    Phase phase = PHASE_INPUT;

    // Delayed buffers
    var backward_queue; // List of backward signals pending release

    SynapseSim()
    {
        config = dict();
        config["repulsion"] = 150.0;
        config["ideal_dist"] = 400.0;
        config["stiffness"] = 0.08;
        config["damping"] = 0.85;
        config["dt"] = 0.016;
        config["signal_speed"] = 2.5;
        config["decay"] = 0.95;
        config["auto_run"] = true;
        config["physics_on"] = true;

        reset_network();
    }

    void reset_network()
    {
        print("Resetting network... Topology:", current_topology);

        net = graph(0);
        nodes = list();
        edge_cache = list();
        signals = list();
        signal_count = 0; // Reset counter
        backward_queue = list();
        dragged_node_idx = -1;
        phase = PHASE_INPUT;

        // Clear adjacency lists
        forward_edges.clear();
        backward_edges.clear();

        // Define Layers based on Topology
        var layers = list();
        if (current_topology == TOP_SIMPLE)
        {
            layers.append(4);
            layers.append(6);
            layers.append(2);
        }
        else if (current_topology == TOP_DEEP)
        {
            layers.append(4);
            layers.append(8);
            layers.append(8);
            layers.append(6);
            layers.append(3);
        }
        else if (current_topology == TOP_DENSE)
        {
            layers.append(6);
            layers.append(12);
            layers.append(12);
            layers.append(4);
        }
        else if (current_topology == TOP_RESIDUAL)
        {
            // Deep residual network: 4-8-8-8-8-4
            layers.append(4);
            layers.append(8);
            layers.append(8);
            layers.append(8);
            layers.append(8);
            layers.append(4);
        }
        else if (current_topology == TOP_RANDOM)
        {
            layers.append(5);
            layers.append(7);
            layers.append(5);
        }
        else if (current_topology == TOP_CUSTOM)
        {
            // Parse custom topology string (e.g., "40-3-14-2")
            print("Parsing custom topology:", custom_topology_str.c_str());
            std::istringstream iss(custom_topology_str);
            std::string token;
            while (std::getline(iss, token, '-'))
            {
                try
                {
                    int count = std::stoi(token);
                    if (count > 0 && count < 1000)
                    {
                        layers.append(count);
                        print("  Added layer with", count, "nodes");
                    }
                }
                catch (...)
                {
                    print("  Failed to parse token:", token.c_str());
                }
            }
            // Fallback to simple if empty
            if (layers.len() == 0)
            {
                print("  No valid layers, using fallback");
                layers.append(4);
                layers.append(6);
                layers.append(2);
            }
        }

        // Manual Count
        int total_nodes = 0;
        int layer_count = 0;
        for (auto l : layers)
        {
            total_nodes += l.toInt();
            layer_count++;
        }
        print("Manual Layer Count:", layer_count);
        print("Total Nodes:", total_nodes);

        // Fix Max Layer
        if (current_topology != TOP_RANDOM)
        {
            max_layer_idx = layer_count - 1;
        }
        else
        {
            max_layer_idx = 3;
        }

        net = graph(total_nodes);
        net.reserve_edges_per_node(6);

        int current_idx = 0;
        float start_x = 100.0f;
        float spacing_x = 200.0f;

        // --- Node Generation ---
        if (current_topology != TOP_RANDOM)
        {
            int l = 0;
            for (auto layer_val : layers)
            {
                int count = layer_val.toInt();
                float total_h = (count - 1) * 80.0f;
                float start_y = 400.0f - total_h / 2.0f;

                print("Gen Layer", l, "nodes:", count);

                for (int i = 0; i < count; ++i)
                {
                    float px = start_x + l * spacing_x + (rand() % 30 - 15);
                    float py = start_y + i * 80.0f + (rand() % 30 - 15);

                    var node = list();
                    node.append(px);
                    node.append(py);
                    node.append(0.0);                            // vx
                    node.append(0.0);                            // vy
                    node.append(((rand() % 100) / 100.0) + 0.5); // bias
                    node.append(0.0);                            // act
                    node.append(l);                              // layer
                    node.append(0.0);                            // fx
                    node.append(0.0);                            // fy

                    nodes.append(node);
                    current_idx++;
                }
                l++;
            }
        }
        else
        {
            for (int i = 0; i < 30; ++i)
            {
                float px = 100 + rand() % 1000;
                float py = 100 + rand() % 600;
                int l = (px - 100) / 250;
                if (l < 0)
                    l = 0;
                if (l > 3)
                    l = 3;

                var node = list();
                node.append(px);
                node.append(py);
                node.append(0.0);
                node.append(0.0);
                node.append(1.0);
                node.append(0.0);
                node.append(l);
                node.append(0.0);
                node.append(0.0);

                nodes.append(node);
            }
        }

        // --- Edge Generation ---
        if (current_topology != TOP_RANDOM)
        {
            std::vector<int> layer_counts;
            for (auto v : layers)
                layer_counts.push_back(v.toInt());

            int layer_start = 0;
            for (int l = 0; l < (int)layer_counts.size() - 1; ++l)
            {
                int count_curr = layer_counts[l];
                int count_next = layer_counts[l + 1];
                int next_start = layer_start + count_curr;

                for (int i = 0; i < count_curr; ++i)
                {
                    int u = layer_start + i;
                    int connections_made = 0;

                    for (int j = 0; j < count_next; ++j)
                    {
                        int v = next_start + j;

                        // Ensure at least one connection + high probability for others
                        bool should_connect = (rand() % 100 < 95) ||
                                              (j == count_next - 1 && connections_made == 0);

                        if (should_connect)
                        {
                            connections_made++;
                            double w = 0.3 + (rand() % 70) / 100.0;
                            net.add_edge(u, v, w, 0.0, true);

                            var edge = list();
                            edge.append(u);
                            edge.append(v);
                            edge.append(w);
                            edge.append(0);
                            edge_cache.append(edge);
                        }
                    }

                    if (connections_made == 0)
                    {
                        print("WARNING: Node", u, "has no outgoing edges!");
                    }
                }
                layer_start = next_start;
            }

            // ADD SKIP CONNECTIONS for residual network
            if (current_topology == TOP_RESIDUAL || (current_topology == TOP_CUSTOM && custom_use_residual))
            {
                int layer_start = 0;
                for (int l = 0; l < (int)layer_counts.size() - 2; ++l) // Skip last layer
                {
                    int count_curr = layer_counts[l];
                    int count_skip = layer_counts[l + 2]; // Skip to layer l+2
                    int skip_start = layer_start + count_curr + layer_counts[l + 1];

                    for (int i = 0; i < count_curr; ++i)
                    {
                        int u = layer_start + i;
                        // Add skip connection with 60% probability
                        if ((rand() % 100) < 60)
                        {
                            int skip_target = skip_start + (rand() % count_skip);
                            double w = 0.3 + (rand() % 50) / 100.0; // Lighter weight
                            net.add_edge(u, skip_target, w, 0.0, true);

                            var skip_edge = list();
                            skip_edge.append(u);
                            skip_edge.append(skip_target);
                            skip_edge.append(w);
                            skip_edge.append(2); // type: 2 = skip connection
                            edge_cache.append(skip_edge);
                        }
                    }
                    layer_start += count_curr;
                }
            }
        }
        else
        {
            // Random Edges
            int n_len = 0;
            for (auto n : nodes)
                n_len++;
            for (int i = 0; i < n_len; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    int target = rand() % n_len;
                    if (target != i)
                    {
                        double w = 0.5;
                        var edge = list();
                        edge.append(i);
                        edge.append(target);
                        edge.append(w);
                        edge.append(0);
                        edge_cache.append(edge);
                    }
                }
            }
        }

        // Build adjacency lists for fast edge lookup - CRITICAL PERFORMANCE
        int n_len = 0;
        for (auto n : nodes)
            n_len++;
        forward_edges.resize(n_len);
        backward_edges.resize(n_len);

        int edge_idx = 0;
        for (auto e : edge_cache)
        {
            int u = e[EDGE_U].toInt();
            int v = e[EDGE_V].toInt();
            forward_edges[u].push_back(edge_idx);
            backward_edges[v].push_back(edge_idx);
            edge_idx++;
        }

        // Initialize edge activity tracking
        edge_activity.clear();
        edge_activity.resize(edge_idx, 0.0f);
        signal_trails.clear();

        // DEBUG CHECK
        int debug_n = 0;
        for (auto n : nodes)
            debug_n++;
        print("DEBUG: Nodes List Length after init:", debug_n);

        int debug_e = 0;
        for (auto e : edge_cache)
            debug_e++;
        print("DEBUG: Edges created:", debug_e);
    }

    // ----------------------------------------------------------------------
    // Simulation Logic
    // ----------------------------------------------------------------------

    void trigger_node(int idx, double strength, int type)
    {
        auto t_start = std::chrono::high_resolution_clock::now();

        var &node = nodes[idx];

        // Use native double for performance
        double act = node[IDX_ACT].toDouble();
        act += strength;
        if (act > 2.0)
            act = 2.0;
        node[IDX_ACT] = act;

        auto t1 = std::chrono::high_resolution_clock::now();

        // Use atomic counter instead of O(N) iteration - CRITICAL FIX!
        if (signal_count.load() > 4000)
            return;

        auto t2 = std::chrono::high_resolution_clock::now();

        // Use adjacency list for FAST edge lookup - NO MORE SCANNING ALL EDGES!
        const std::vector<int> &edges_to_check = (type == 0) ? forward_edges[idx] : backward_edges[idx];

        // OPTIMIZATION: For backward pass, only propagate 30% of the time to reduce signal spam
        bool should_propagate_backward = (type == 1 && (rand() % 100 < 30));
        if (type == 1 && !should_propagate_backward && edges_to_check.size() > 5)
        {
            // Update weights but don't create signals
            for (int edge_idx : edges_to_check)
            {
                var &e = edge_cache[edge_idx];
                double w = e[EDGE_W].toDouble();
                double new_w = std::clamp(w + ((rand() % 10 - 5) / 50.0), 0.1, 2.0);
                e[EDGE_W] = new_w;
            }
            return;
        }

        auto t3 = std::chrono::high_resolution_clock::now();

        // Create signals efficiently
        for (int edge_idx : edges_to_check)
        {
            var &e = edge_cache[edge_idx];

            int u = e[EDGE_U].toInt();
            int v = e[EDGE_V].toInt();
            double w = e[EDGE_W].toDouble();

            if (type == 0)
            {
                // Forward - create signal efficiently
                var sig = list();
                sig.append(u);
                sig.append(v);
                sig.append(0.0);
                sig.append(strength * w);
                sig.append(0);
                sig.append(true);
                signals.append(sig);
                signal_count++; // Increment counter
            }
            else
            {
                // Backward
                double new_w = std::clamp(w + ((rand() % 10 - 5) / 50.0), 0.1, 2.0);
                e[EDGE_W] = new_w;

                var sig = list();
                sig.append(v);
                sig.append(u);
                sig.append(0.0);
                sig.append(strength * 0.5);
                sig.append(1);
                sig.append(true);
                signals.append(sig);
                signal_count++; // Increment counter
            }
        }

        auto t_end = std::chrono::high_resolution_clock::now();

        if (type == 1)
        { // Only log backward pass
            auto d1 = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t_start).count();
            auto d2 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            auto d3 = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
            auto d4 = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t3).count();
            auto total = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

            if (total > 100)
            { // Only log slow calls
                std::cout << "BACK trigger " << idx << ": "
                          << "act=" << d1 << "us, "
                          << "cnt=" << d2 << "us, "
                          << "lkp=" << d3 << "us, "
                          << "sig=" << d4 << "us, "
                          << "TOT=" << total << "us, "
                          << "edg=" << edges_to_check.size() << std::endl;
            }
        }
    }

    void pulse_input()
    {
        int i = 0;
        for (auto node : nodes)
        { // value copy OK
            if (node[IDX_LAYER].toInt() == 0)
            {
                trigger_node(i, 1.5, 0);
            }
            i++;
        }
    }

    void update_signals(var dt)
    {
        double spd = config["signal_speed"].toDouble();
        double decay = config["decay"].toDouble();
        double dt_val = dt.toDouble();
        double spd_dt = spd * dt_val;

        // PHASE STATE MACHINE
        if (phase == PHASE_INPUT)
        {
            pulse_input(); // Triggers inputs
            phase = PHASE_FORWARD;
            backward_queue = list(); // Clear buffer
            return;
        }

        // --- PROCESS SIGNALS IN PARALLEL ---
        var processing = signals;
        signals = list();
        signal_count = 0; // Reset counter when clearing signals
        int active_count = 0;

        // Batch process signals - collect arrivals using pythonic list
        var arrivals = list(); // Each item: [target, strength, type]

        // Decay edge activity
        for (size_t i = 0; i < edge_activity.size(); ++i)
        {
            edge_activity[i] *= 0.92f; // Decay
        }

        // Age signal trails
        for (auto &trail : signal_trails)
        {
            trail.age += 0.15f;
        }
        // Remove old trails
        signal_trails.erase(
            std::remove_if(signal_trails.begin(), signal_trails.end(),
                           [](const SignalTrail &t)
                           { return t.age > 1.0f; }),
            signal_trails.end());

        for (auto s : processing)
        {
            if (s.type() != "list")
                continue;

            // Move - use native types
            double prog = s[SIG_PROG].toDouble();
            prog += spd_dt;
            s[SIG_PROG] = prog;

            int type = s[SIG_TYPE].toInt();
            int from = s[SIG_FROM].toInt();
            int to = s[SIG_TO].toInt();

            // Add trail for comet effect
            if ((int)(prog * 20.0) % 2 == 0) // Sample trail every ~5% progress
            {
                SignalTrail trail;
                trail.from = from;
                trail.to = to;
                trail.prog = prog;
                trail.type = type;
                trail.str = s[SIG_STR].toDouble();
                trail.age = 0.0f;
                signal_trails.push_back(trail);
            }

            // Activate edge for tension effect
            const std::vector<int> &edges = forward_edges[from];
            for (int edge_idx : edges)
            {
                var &e = edge_cache[edge_idx];
                if (e[EDGE_V].toInt() == to && edge_idx < (int)edge_activity.size())
                {
                    edge_activity[edge_idx] = std::min(1.0f, edge_activity[edge_idx] + 0.3f);
                    break;
                }
            }

            // Check if signal alive
            if (prog < 1.0)
            {
                signals.append(s);
                signal_count++; // Increment counter for kept signal
                active_count++;
            }
            else
            {
                // Signal Arrived - batch for processing
                int target = s[SIG_TO].toInt();
                double str = s[SIG_STR].toDouble();
                int t_layer = nodes[target][IDX_LAYER].toInt();

                var arrival = list();
                arrival.append(target);
                arrival.append(str);
                arrival.append(type);
                arrivals.append(arrival);

                // If Forward Reached End -> Queue Backward
                if (type == 0 && t_layer == max_layer_idx)
                { // Output layer
                    var b_sig = list();
                    b_sig.append(target); // dummy from
                    b_sig.append(str);    // strength
                    backward_queue.append(b_sig);
                }
            }
        }

        // Process all arrivals in batch
        for (auto arrival : arrivals)
        {
            int target = arrival[static_cast<size_t>(0)].toInt();
            double str = arrival[static_cast<size_t>(1)].toDouble();
            int type = arrival[static_cast<size_t>(2)].toInt();
            trigger_node(target, str, type);
        }

        // --- PHASE TRANSITIONS ---
        if (phase == PHASE_FORWARD)
        {
            bool forward_alive = false;
            for (auto s : signals)
            {
                if (s[SIG_TYPE].toInt() == 0)
                    forward_alive = true;
            }

            if (!forward_alive && active_count == 0)
            {
                // Switch to Backward
                phase = PHASE_BACKWARD;

                // Flush Backward Queue
                for (auto b : backward_queue)
                {
                    int target = b[static_cast<size_t>(0)].toInt();
                    double str = b[static_cast<size_t>(1)].toDouble();
                    trigger_node(target, str * 1.0, 1); // Launch Backward
                }
                backward_queue = list();
            }
        }
        else if (phase == PHASE_BACKWARD)
        {
            bool backward_alive = false;
            for (auto s : signals)
            {
                if (s[SIG_TYPE].toInt() == 1)
                    backward_alive = true;
            }

            if (!backward_alive && active_count == 0)
            {
                phase = PHASE_INPUT; // Loop back
            }
        }

        // Decay activation - use native types
        int n_count = 0;
        for (auto n : nodes)
            n_count++;
        for (int i = 0; i < n_count; ++i)
        {
            double act = nodes[i][IDX_ACT].toDouble();
            nodes[i][IDX_ACT] = act * decay;
        }
    }

    void update_physics(var dt)
    {
        // Safe count
        int count = 0;
        for (auto n : nodes)
            count++;

        double rep = config["repulsion"].toDouble();
        double stiff = config["stiffness"].toDouble();
        double damp = config["damping"].toDouble();
        double dt_val = dt.toDouble();

        // Reset Forces
        for (int i = 0; i < count; ++i)
        {
            nodes[i][IDX_FX] = 0.0;
            nodes[i][IDX_FY] = 0.0;
        }

        // Parallel Repulsion Calculation
        auto calc_repulsion = [&](int start, int end)
        {
            for (int i = start; i < end; ++i)
            {
                double xi = nodes[i][IDX_X].toDouble();
                double yi = nodes[i][IDX_Y].toDouble();
                double fxi = 0.0;
                double fyi = 0.0;

                for (int j = 0; j < count; ++j)
                {
                    if (i == j)
                        continue;
                    double xj = nodes[j][IDX_X].toDouble();
                    double yj = nodes[j][IDX_Y].toDouble();

                    double dx = xi - xj;
                    double dy = yi - yj;
                    double dist_sq = dx * dx + dy * dy;
                    double dist = std::sqrt(dist_sq);

                    // Distance-based force: repulse close, attract far
                    double ideal_dist = config["ideal_dist"].toDouble(); // Tunable ideal separation
                    double force_magnitude = 0.0;

                    if (dist < ideal_dist)
                    {
                        // Strong repulsion when too close
                        force_magnitude = (rep * 2.0) / (dist + 1.0);
                    }
                    else
                    {
                        // Attraction when too far
                        double excess = dist - ideal_dist;
                        force_magnitude = -1.0 * excess / dist; // Negative = attraction
                    }

                    fxi += force_magnitude * dx;
                    fyi += force_magnitude * dy;
                }
                nodes[i][IDX_FX] = nodes[i][IDX_FX].toDouble() + fxi;
                nodes[i][IDX_FY] = nodes[i][IDX_FY].toDouble() + fyi;
            }
        };

        // Launch threads for repulsion
        std::vector<std::thread> threads;
        int chunk = count / num_threads;
        for (int t = 0; t < num_threads; ++t)
        {
            int start = t * chunk;
            int end = (t == num_threads - 1) ? count : (t + 1) * chunk;
            threads.emplace_back(calc_repulsion, start, end);
        }
        for (auto &th : threads)
            th.join();

        // Springs - use native types
        int e_count = 0;
        for (auto e : edge_cache)
            e_count++;

        for (int i = 0; i < e_count; ++i)
        {
            var &e = edge_cache[i]; // Ref
            int u = e[EDGE_U].toInt();
            int v = e[EDGE_V].toInt();

            double dx = nodes[v][IDX_X].toDouble() - nodes[u][IDX_X].toDouble();
            double dy = nodes[v][IDX_Y].toDouble() - nodes[u][IDX_Y].toDouble();

            double f_spring_x = dx * stiff;
            double f_spring_y = dy * stiff;

            nodes[u][IDX_FX] = nodes[u][IDX_FX].toDouble() + f_spring_x;
            nodes[u][IDX_FY] = nodes[u][IDX_FY].toDouble() + f_spring_y;

            nodes[v][IDX_FX] = nodes[v][IDX_FX].toDouble() - f_spring_x;
            nodes[v][IDX_FY] = nodes[v][IDX_FY].toDouble() - f_spring_y;
        }

        // Integrate - use native types for speed
        double center_x = 640.0;
        double center_y = 400.0;

        for (int i = 0; i < count; ++i)
        {
            var &n = nodes[i]; // Ref

            // Mouse Drag Override
            if (i == dragged_node_idx)
            {
                n[IDX_FX] = 0.0;
                n[IDX_FY] = 0.0;
                n[IDX_VX] = 0.0;
                n[IDX_VY] = 0.0;
                continue;
            }

            double nx = n[IDX_X].toDouble();
            double ny = n[IDX_Y].toDouble();
            double nvx = n[IDX_VX].toDouble();
            double nvy = n[IDX_VY].toDouble();
            double nfx = n[IDX_FX].toDouble();
            double nfy = n[IDX_FY].toDouble();

            // Layer-based gravity for left-to-right flow
            int layer = n[IDX_LAYER].toInt();
            double target_x = 100.0 + (layer * 200.0); // Spread layers horizontally
            double target_y = center_y;

            double gx = (target_x - nx) * 0.04; // Stronger horizontal pull
            double gy = (target_y - ny) * 0.02; // Weaker vertical pull

            double fx = nfx + gx;
            double fy = nfy + gy;

            nvx = (nvx + fx * dt_val) * damp;
            nvy = (nvy + fy * dt_val) * damp;

            nx = nx + nvx * dt_val;
            ny = ny + nvy * dt_val;

            n[IDX_VX] = nvx;
            n[IDX_VY] = nvy;
            n[IDX_X] = nx;
            n[IDX_Y] = ny;
        }
    }

    // Wrapper for Test Runner
    void update()
    {
        if (config["auto_run"])
        {
            var dt = config["dt"];
            if (config["physics_on"])
                update_physics(dt);
            update_signals(dt);
        }
    }
};

// ==================================================================================
// MAIN
// ==================================================================================

int main(int argc, char **argv)
{
    srand(time(0));

    if (argc > 1 && std::string(argv[1]) == "--test")
    {
        SynapseSim sim;
        for (int i = 0; i < 60; ++i)
            sim.update();
        int n_check = 0;
        for (auto n : sim.nodes)
            n_check++;
        if (n_check > 0)
            return 0;
        return 1;
    }

    if (!glfwInit())
        return 1;
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = glfwCreateWindow(1280, 800, "Synapse - Pythonic FFNN", NULL, NULL);
    if (!window)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SynapseSim sim;

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImDrawList *dl = ImGui::GetBackgroundDrawList();

        // Update Sim
        sim.update_physics(0.016);
        sim.update_signals(0.016);

        // Get mouse position for hover detection
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        int hovered_node = -1;

        // Find hovered node
        int idx = 0;
        for (auto n : sim.nodes)
        {
            float x = n[IDX_X].toFloat();
            float y = n[IDX_Y].toFloat();
            float rad = 12.0f + n[IDX_ACT].toFloat() * 4.0f;
            float dx = mx - x;
            float dy = my - y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist < rad + 5.0f)
            {
                hovered_node = idx;
                break;
            }
            idx++;
        }

        // Render Edges
        int e_len = 0;
        for (auto e : sim.edge_cache)
        {
            int u = e[EDGE_U].toInt();
            int v = e[EDGE_V].toInt();
            float w = e[EDGE_W].toFloat();
            int edge_type = e[EDGE_TYPE].toInt();

            var &n1 = sim.nodes[u];
            var &n2 = sim.nodes[v];

            // Highlight edge if connected to hovered node
            bool is_hovered_edge = (hovered_node == u || hovered_node == v);

            // Get edge activity for tension effect
            float activity = (e_len < (int)sim.edge_activity.size()) ? sim.edge_activity[e_len] : 0.0f;
            float tension_boost = activity * 2.0f; // Boost thickness when active

            float th = w * 4.0f + 1.5f + tension_boost;
            ImU32 col;
            if (is_hovered_edge)
            {
                col = ImColor(200, 100, 255, 255); // Purple for hovered
            }
            else if (edge_type == 2) // Skip connection
            {
                int alpha = (int)(40 + w * 60 + activity * 100);  // Brighter when active
                col = ImColor(0, 180, 220, std::min(255, alpha)); // Cyan for skip edges
            }
            else
            {
                int alpha = (int)(60 + w * 80 + activity * 120);    // Brighter when active
                col = ImColor(100, 100, 120, std::min(255, alpha)); // Darker default
            }

            dl->AddLine({n1[IDX_X].toFloat(), n1[IDX_Y].toFloat()},
                        {n2[IDX_X].toFloat(), n2[IDX_Y].toFloat()}, col, th);
            e_len++;
        }

        // Render Nodes
        int n_len = 0;
        idx = 0;
        for (auto n : sim.nodes)
        {
            n_len++;
            float x = n[IDX_X].toFloat();
            float y = n[IDX_Y].toFloat();
            float act = n[IDX_ACT].toFloat();
            float bias = n[IDX_BIAS].toFloat();
            int layer = n[IDX_LAYER].toInt();

            float rad = 12.0f + act * 4.0f;

            ImVec4 col;

            // Hovered node
            if (idx == hovered_node)
            {
                col = {0.8f, 0.5f, 1.0f, 1.0f}; // Purple
            }
            // Default: ash grey
            else
            {
                col = {0.5f, 0.5f, 0.5f, 1.0f}; // Ash grey
            }

            // Light up when signal flows (activation)
            if (act > 0.1f)
            {
                float intensity = std::min(act, 1.0f);
                col.x += intensity * 0.5f;
                col.y += intensity * 0.5f;
                col.z = std::min(col.z + intensity * 0.3f, 1.0f);
            }

            dl->AddCircleFilled({x, y}, rad, ImColor(col));
            idx++;
        }

        // Render Signals
        int s_len = 0;
        for (auto s : sim.signals)
        {
            s_len++;
            int u = s[SIG_FROM].toInt();
            int v = s[SIG_TO].toInt();
            float t = s[SIG_PROG].toFloat();
            int type = s[SIG_TYPE].toInt();

            var &n1 = sim.nodes[u];
            var &n2 = sim.nodes[v];

            float x = n1[IDX_X].toFloat() + (n2[IDX_X].toFloat() - n1[IDX_X].toFloat()) * t;
            float y = n1[IDX_Y].toFloat() + (n2[IDX_Y].toFloat() - n1[IDX_Y].toFloat()) * t;

            if (type == 0)
            {
                // Forward signal - yellow/gold
                dl->AddCircleFilled({x, y}, 4.0f, ImColor(255, 215, 0, 200));
            }
            else
            {
                // Backward signal - red without plus sign
                dl->AddCircleFilled({x, y}, 5.0f, ImColor(255, 50, 50, 255));
            }
        }

        // Render Signal Trails (comet effect)
        for (const auto &trail : sim.signal_trails)
        {
            if (trail.from >= n_len || trail.to >= n_len)
                continue;

            var &n1 = sim.nodes[trail.from];
            var &n2 = sim.nodes[trail.to];

            float x = n1[IDX_X].toFloat() + (n2[IDX_X].toFloat() - n1[IDX_X].toFloat()) * trail.prog;
            float y = n1[IDX_Y].toFloat() + (n2[IDX_Y].toFloat() - n1[IDX_Y].toFloat()) * trail.prog;

            float fade = 1.0f - trail.age; // 1.0 = bright, 0.0 = faded
            float size = (trail.type == 0) ? 3.0f : 4.0f;
            size *= fade * 0.8f; // Shrink as it fades

            if (trail.type == 0)
            {
                // Forward trail - dimmer yellow
                dl->AddCircleFilled({x, y}, size, ImColor(255, 215, 0, (int)(120 * fade)));
            }
            else
            {
                // Backward trail - dimmer red
                dl->AddCircleFilled({x, y}, size, ImColor(255, 50, 50, (int)(180 * fade)));
            }
        }

        // Calculate graph center
        double graph_center_x = 640.0; // Default center
        double graph_center_y = 400.0;
        if (n_len > 0)
        {
            double sum_x = 0.0, sum_y = 0.0;
            for (auto n : sim.nodes)
            {
                sum_x += n[IDX_X].toDouble();
                sum_y += n[IDX_Y].toDouble();
            }
            graph_center_x = sum_x / n_len;
            graph_center_y = sum_y / n_len;
        }

        // Graph center handle - only visible when mouse is near
        double center_dx = mx - graph_center_x;
        double center_dy = my - graph_center_y;
        double center_dist = sqrt(center_dx * center_dx + center_dy * center_dy);
        bool show_center_handle = (center_dist < 80.0); // Show within 80px radius

        if (show_center_handle)
        {
            // Draw center handle with pulsing effect
            float pulse = 0.5f + 0.3f * sin(ImGui::GetTime() * 3.0f);
            int handle_alpha = 100 + (int)(pulse * 100);
            dl->AddCircleFilled({(float)graph_center_x, (float)graph_center_y}, 20.0f, ImColor(150, 150, 255, handle_alpha));
            dl->AddCircle({(float)graph_center_x, (float)graph_center_y}, 20.0f, ImColor(200, 200, 255, 255), 0, 2.0f);
            // Inner cross
            dl->AddLine({(float)graph_center_x - 8, (float)graph_center_y}, {(float)graph_center_x + 8, (float)graph_center_y}, ImColor(255, 255, 255, 200), 2.0f);
            dl->AddLine({(float)graph_center_x, (float)graph_center_y - 8}, {(float)graph_center_x, (float)graph_center_y + 8}, ImColor(255, 255, 255, 200), 2.0f);
        }

        // UI
        ImGui::Begin("Synapse Control");
        ImGui::Text("Network: %d Nodes, %d Edges", n_len, e_len);
        ImGui::Text("Active Signals: %d", s_len);

        // Status Display
        const char *status = "IDLE";
        if (sim.phase == SynapseSim::PHASE_FORWARD)
            status = "FORWARD (Thinking)";
        else if (sim.phase == SynapseSim::PHASE_BACKWARD)
            status = "BACKWARD (Learning)";
        else if (sim.phase == SynapseSim::PHASE_INPUT)
            status = "INPUT";

        ImGui::Text("Phase: %s", status);

        const char *topologies[] = {"Simple (4-6-2)", "Deep (4-8-8-6-3)", "Dense (6-12-12-4)", "Random Spaghetti", "Residual (4-8-8-8-8-4)", "Custom"};
        if (ImGui::Combo("Topology", &sim.current_topology, topologies, IM_ARRAYSIZE(topologies)))
        {
            sim.reset_network();
        }

        // Custom topology input
        if (sim.current_topology == SynapseSim::TOP_CUSTOM)
        {
            static char buf[128] = "";
            // Update buffer with current value only if empty
            if (buf[0] == '\0')
            {
                strcpy(buf, sim.custom_topology_str.c_str());
            }
            ImGui::Text("Format: num-num-num (e.g., 40-3-14-2)");
            if (ImGui::InputText("Layers##custom", buf, sizeof(buf)))
            {
                // Update on every change
                sim.custom_topology_str = std::string(buf);
            }
            ImGui::SameLine();
            if (ImGui::Button("Apply##topology"))
            {
                sim.custom_topology_str = std::string(buf);
                sim.reset_network();
            }
            // Checkbox for enabling residual connections
            if (ImGui::Checkbox("Use Skip Connections (Residual)", &sim.custom_use_residual))
            {
                sim.reset_network();
            }
            ImGui::TextWrapped("Skip connections add EXTRA edges that jump 1-2 layers ahead, creating residual paths for better gradient flow. This increases total edge count.");
        }

        if (ImGui::Button("Reset Network"))
            sim.reset_network();

        static float repulsion = 150.0f;
        if (ImGui::SliderFloat("Repulsion", &repulsion, 10, 1000))
            sim.config["repulsion"] = repulsion;
        static float ideal_dist = 400.0f;
        if (ImGui::SliderFloat("Ideal Distance", &ideal_dist, 50, 500))
            sim.config["ideal_dist"] = ideal_dist;
        static float speed = 2.5f;
        if (ImGui::SliderFloat("Signal Speed", &speed, 0.1, 10.0))
            sim.config["signal_speed"] = speed;

        ImGui::End();

        // Mouse Logic
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);

            if (ImGui::IsMouseClicked(0))
            {
                std::cout << "Mouse clicked at: " << mx << ", " << my << std::endl;

                // Calculate graph center
                double graph_center_x = 640.0;
                double graph_center_y = 400.0;
                if (n_len > 0)
                {
                    double sum_x = 0.0, sum_y = 0.0;
                    for (auto n : sim.nodes)
                    {
                        sum_x += n[IDX_X].toDouble();
                        sum_y += n[IDX_Y].toDouble();
                    }
                    graph_center_x = sum_x / n_len;
                    graph_center_y = sum_y / n_len;
                }

                double center_dx = mx - graph_center_x;
                double center_dy = my - graph_center_y;
                double center_dist = sqrt(center_dx * center_dx + center_dy * center_dy);

                // Check if clicking on center handle
                if (center_dist < 20.0)
                {
                    std::cout << "  Grabbed graph center handle" << std::endl;
                    sim.dragging_graph = true;
                    sim.graph_drag_last_x = mx;
                    sim.graph_drag_last_y = my;
                }
                else
                {
                    // Find closest node
                    int idx = 0;
                    float min_d = 1000.0f;
                    int closest = -1;
                    for (auto n : sim.nodes)
                    {
                        float nx = n[IDX_X].toFloat();
                        float ny = n[IDX_Y].toFloat();
                        float dx = mx - nx;
                        float dy = my - ny;
                        float d = sqrt(dx * dx + dy * dy);

                        if (idx < 5)
                        { // Debug first few nodes
                            std::cout << "  Node " << idx << " at (" << nx << ", " << ny << "), dx=" << dx << ", dy=" << dy << ", dist=" << d << std::endl;
                        }

                        if (d < 20.0f && d < min_d)
                        {
                            min_d = d;
                            closest = idx;
                        }
                        idx++;
                    }

                    std::cout << "  Closest node: " << closest << " (dist=" << min_d << ")" << std::endl;

                    if (closest != -1)
                    {
                        sim.dragged_node_idx = closest;
                    }
                }
            }

            if (ImGui::IsMouseDown(0) && sim.dragging_graph)
            {
                // Drag entire graph
                double offset_x = mx - sim.graph_drag_last_x;
                double offset_y = my - sim.graph_drag_last_y;

                int node_idx = 0;
                for (auto n : sim.nodes)
                {
                    double curr_x = sim.nodes[node_idx][IDX_X].toDouble();
                    double curr_y = sim.nodes[node_idx][IDX_Y].toDouble();
                    sim.nodes[node_idx][IDX_X] = curr_x + offset_x;
                    sim.nodes[node_idx][IDX_Y] = curr_y + offset_y;
                    node_idx++;
                }

                sim.graph_drag_last_x = mx;
                sim.graph_drag_last_y = my;
            }
            else if (ImGui::IsMouseDown(0) && sim.dragged_node_idx != -1)
            {
                var &n = sim.nodes[sim.dragged_node_idx];
                n[IDX_X] = (float)mx;
                n[IDX_Y] = (float)my;
                n[IDX_VX] = 0.0;
                n[IDX_VY] = 0.0;
            }

            if (ImGui::IsMouseReleased(0))
            {
                sim.dragged_node_idx = -1;
                sim.dragging_graph = false;
            }
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // Dark grey background
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
