#include "header.h"
#include <SDL.h>
#include <algorithm>
#include <map>
#include <cmath>

/*
NOTE : You are free to change the code as you wish, the main objective is to make the
       application work and pass the audit.

       It will be provided the main function with the following functions :

       - `void systemWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the system window on your screen
       - `void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the memory and processes window on your screen
       - `void networkWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the network window on your screen
*/

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h> // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h> // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h> // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h> // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE      // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h> // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE        // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h> // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Global variables for system window state
static CPUStats prevCPUStats = {0};
static GraphData cpuGraph(100);
static GraphData thermalGraph(100);
static GraphData fanGraph(100);
static bool firstRun = true;

// systemWindow, display information for the system monitorization
void systemWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // System Information Section
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Operating System: %s", getOsName());
    ImGui::Text("User: %s", getLoggedUser().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("CPU: %s", CPUinfo().c_str());

    // Task counts
    vector<int> taskCounts = getTaskCounts();
    ImGui::Text("Tasks: %d running, %d sleeping, %d stopped, %d zombie",
                taskCounts[0], taskCounts[1], taskCounts[2], taskCounts[3]);
    ImGui::Text("Total Tasks: %d", taskCounts[0] + taskCounts[1] + taskCounts[2] + taskCounts[3]);

    ImGui::Spacing();
    ImGui::Separator();

    // Tabbed section for CPU, Fan, Thermal
    if (ImGui::BeginTabBar("SystemTabs")) {

        // CPU Tab
        if (ImGui::BeginTabItem("CPU")) {
            CPUStats currentCPU = getCPUStats();
            double cpuUsage = 0.0;

            if (!firstRun) {
                cpuUsage = calculateCPUUsage(prevCPUStats, currentCPU);
            }
            prevCPUStats = currentCPU;
            firstRun = false;

            if (cpuGraph.animate && cpuGraph.shouldUpdate()) {
                cpuGraph.addValue(cpuUsage);
            }

            ImGui::Text("CPU Usage: %.1f%%", cpuUsage);

            // Graph controls
            ImGui::Checkbox("Animate", &cpuGraph.animate);
            ImGui::SliderFloat("FPS", &cpuGraph.fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y Scale", &cpuGraph.y_scale, 50.0f, 200.0f);

            // CPU Graph
            if (!cpuGraph.values.empty()) {
                ImGui::PlotLines("CPU Usage", cpuGraph.values.data(), cpuGraph.values.size(),
                               0, nullptr, 0.0f, cpuGraph.y_scale, ImVec2(0, 80));
            }

            ImGui::EndTabItem();
        }

        // Fan Tab
        if (ImGui::BeginTabItem("Fan")) {
            string fanStatus = getFanStatus();
            int fanSpeed = getFanSpeed();

            ImGui::Text("Fan Status: %s", fanStatus.c_str());

            if (fanSpeed >= 0) {
                ImGui::Text("Fan Speed: %d RPM", fanSpeed);
            } else {
                ImGui::Text("Fan Speed: Not Available");
            }

            if (fanGraph.animate && fanSpeed >= 0 && fanGraph.shouldUpdate()) {
                fanGraph.addValue(fanSpeed);
            }

            // Graph controls
            ImGui::Checkbox("Animate##Fan", &fanGraph.animate);
            ImGui::SliderFloat("FPS##Fan", &fanGraph.fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y Scale##Fan", &fanGraph.y_scale, 10.0f, 5000.0f);

            // Fan Speed Graph
            if (!fanGraph.values.empty()) {
                // Auto-adjust Y scale for low fan speeds
                float max_value = *max_element(fanGraph.values.begin(), fanGraph.values.end());
                float auto_scale = max(max_value * 1.2f, 50.0f); // At least 50 RPM scale

                ImGui::Text("Current: %.0f RPM, Max: %.0f RPM", fanGraph.values.back(), max_value);
                ImGui::PlotLines("Fan Speed", fanGraph.values.data(), fanGraph.values.size(),
                               0, nullptr, 0.0f, min(fanGraph.y_scale, auto_scale), ImVec2(0, 80));
            } else {
                ImGui::Text("No fan data available - check if animate is enabled");
            }

            ImGui::EndTabItem();
        }

        // Thermal Tab
        if (ImGui::BeginTabItem("Thermal")) {
            double temperature = getThermalTemp();

            ImGui::Text("Temperature: %.1f°C", temperature);

            if (thermalGraph.animate && thermalGraph.shouldUpdate()) {
                thermalGraph.addValue(temperature);
            }

            // Graph controls
            ImGui::Checkbox("Animate##Thermal", &thermalGraph.animate);
            ImGui::SliderFloat("FPS##Thermal", &thermalGraph.fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y Scale##Thermal", &thermalGraph.y_scale, 50.0f, 150.0f);

            // Temperature Graph
            if (!thermalGraph.values.empty()) {
                ImGui::PlotLines("Temperature", thermalGraph.values.data(), thermalGraph.values.size(),
                               0, nullptr, 0.0f, thermalGraph.y_scale, ImVec2(0, 80));
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// Global variables for memory window
static char processFilter[256] = "";
static vector<int> selectedProcesses;

// Helper function to format bytes
string formatMemoryBytes(long long bytes) {
    if (bytes >= 1024LL * 1024 * 1024) {
        double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);
        return to_string((int)round(gb)) + " GB";
    } else if (bytes >= 1024 * 1024) {
        double mb = (double)bytes / (1024.0 * 1024.0);
        return to_string((int)round(mb)) + " MB";
    } else if (bytes >= 1024) {
        double kb = (double)bytes / 1024.0;
        return to_string((int)round(kb)) + " KB";
    }
    return to_string(bytes) + " B";
}

// memoryProcessesWindow, display information for the memory and processes information
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Memory Usage Section
    ImGui::Separator();
    ImGui::Spacing();

    // RAM Usage
    MemoryInfo ramInfo = getMemoryInfo();
    ImGui::Text("Physical Memory (RAM)");
    ImGui::ProgressBar(ramInfo.percentage / 100.0f, ImVec2(0.0f, 0.0f),
                      (formatMemoryBytes(ramInfo.used) + " / " + formatMemoryBytes(ramInfo.total) +
                       " (" + to_string((int)ramInfo.percentage) + "%)").c_str());

    // SWAP Usage
    MemoryInfo swapInfo = getSwapInfo();
    ImGui::Text("Virtual Memory (SWAP)");
    if (swapInfo.total > 0) {
        ImGui::ProgressBar(swapInfo.percentage / 100.0f, ImVec2(0.0f, 0.0f),
                          (formatMemoryBytes(swapInfo.used) + " / " + formatMemoryBytes(swapInfo.total) +
                           " (" + to_string((int)swapInfo.percentage) + "%)").c_str());
    } else {
        ImGui::Text("No swap configured");
    }

    // Disk Usage
    DiskInfo diskInfo = getDiskInfo("/");
    ImGui::Text("Disk Usage (/)");
    ImGui::ProgressBar(diskInfo.percentage / 100.0f, ImVec2(0.0f, 0.0f),
                      (formatMemoryBytes(diskInfo.used) + " / " + formatMemoryBytes(diskInfo.total) +
                       " (" + to_string((int)diskInfo.percentage) + "%)").c_str());

    ImGui::Spacing();

    // Process Table Section
    ImGui::Text("Process Table");
    ImGui::Separator();

    // Filter input
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::InputText("##filter", processFilter, sizeof(processFilter));

    ImGui::Spacing();

    // Process table
    if (ImGui::BeginTable("ProcessTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                         ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {

        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("CPU %", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Memory %", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        vector<Proc> processes = getProcesses();
        string filterStr = string(processFilter);

        for (size_t i = 0; i < processes.size(); i++) {
            const Proc& proc = processes[i];

            // Apply filter
            if (!filterStr.empty() && proc.name.find(filterStr) == string::npos) {
                continue;
            }

            ImGui::TableNextRow();

            // PID column
            ImGui::TableSetColumnIndex(0);
            bool isSelected = std::find(selectedProcesses.begin(), selectedProcesses.end(), proc.pid) != selectedProcesses.end();

            if (ImGui::Selectable(to_string(proc.pid).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                if (ImGui::GetIO().KeyCtrl) {
                    // Multi-select with Ctrl
                    if (isSelected) {
                        selectedProcesses.erase(std::remove(selectedProcesses.begin(), selectedProcesses.end(), proc.pid),
                                              selectedProcesses.end());
                    } else {
                        selectedProcesses.push_back(proc.pid);
                    }
                } else {
                    // Single select
                    selectedProcesses.clear();
                    selectedProcesses.push_back(proc.pid);
                }
            }

            // Name column with anti-flickering
            ImGui::TableSetColumnIndex(1);
            static map<int, pair<string, double>> cached_names; // pid -> (name, last_update_time)
            static double last_name_update = 0.0;

            double current_time = SDL_GetTicks() / 1000.0;
            string display_name;

            // Update names every 2 seconds to prevent flickering
            if (current_time - last_name_update > 2.0 || cached_names.find(proc.pid) == cached_names.end()) {
                display_name = proc.name;
                cached_names[proc.pid] = make_pair(display_name, current_time);
                if (proc.pid == 1) last_name_update = current_time;
            } else {
                display_name = cached_names[proc.pid].first;
            }
            ImGui::Text("%s", display_name.c_str());

            // State column with anti-flickering
            ImGui::TableSetColumnIndex(2);
            static map<int, pair<char, double>> cached_states; // pid -> (state, last_update_time)
            static double last_state_update = 0.0;

            char display_state;

            // Update states every 2 seconds to prevent flickering
            if (current_time - last_state_update > 2.0 || cached_states.find(proc.pid) == cached_states.end()) {
                display_state = proc.state;
                cached_states[proc.pid] = make_pair(display_state, current_time);
                if (proc.pid == 1) last_state_update = current_time;
            } else {
                display_state = cached_states[proc.pid].first;
            }
            ImGui::Text("%c", display_state);

            // CPU % column with ultra-stable anti-flickering
            ImGui::TableSetColumnIndex(3);
            static map<int, pair<double, double>> cached_cpu; // pid -> (cpu_value, last_update_time)
            static map<int, double> stable_cpu; // pid -> stable display value
            static double last_cpu_update = 0.0;

            double cpu_value = 0.0;

            // Update CPU values every 3 seconds for maximum stability
            if (current_time - last_cpu_update > 3.0 || cached_cpu.find(proc.pid) == cached_cpu.end()) {
                double raw_cpu = getProcessCPUUsage(proc.pid);

                // Apply heavy stabilization
                if (stable_cpu.find(proc.pid) == stable_cpu.end()) {
                    // First time - use raw value
                    stable_cpu[proc.pid] = raw_cpu;
                } else {
                    // Apply very conservative smoothing to prevent flickering
                    double current_stable = stable_cpu[proc.pid];
                    double diff = abs(raw_cpu - current_stable);

                    // Only update if change is significant AND consistent
                    if (diff > 5.0) {
                        // Big change: use moderate smoothing
                        stable_cpu[proc.pid] = 0.3 * raw_cpu + 0.7 * current_stable;
                    } else if (diff > 1.0) {
                        // Small change: use very conservative smoothing
                        stable_cpu[proc.pid] = 0.1 * raw_cpu + 0.9 * current_stable;
                    }
                    // Changes < 1% are ignored to prevent flickering
                }

                // Round to whole numbers for maximum stability
                cpu_value = round(stable_cpu[proc.pid]);
                cached_cpu[proc.pid] = make_pair(cpu_value, current_time);

                if (proc.pid == 1) last_cpu_update = current_time;
            } else {
                cpu_value = cached_cpu[proc.pid].first;
            }

            ImGui::Text("%.0f", cpu_value);

            // Memory % column with anti-flickering
            ImGui::TableSetColumnIndex(4);
            static map<int, pair<double, double>> cached_memory; // pid -> (memory_value, last_update_time)
            static double last_memory_update = 0.0;

            double memory_value = 0.0;

            // Update memory every 2 seconds to prevent flickering
            if (current_time - last_memory_update > 2.0 || cached_memory.find(proc.pid) == cached_memory.end()) {
                memory_value = getProcessMemoryUsage(proc.pid);
                // Round to prevent micro-fluctuations
                memory_value = round(memory_value * 10.0) / 10.0;
                cached_memory[proc.pid] = make_pair(memory_value, current_time);
                if (proc.pid == 1) last_memory_update = current_time;
            } else {
                memory_value = cached_memory[proc.pid].first;
            }
            ImGui::Text("%.1f", memory_value);
        }

        ImGui::EndTable();
    }

    // Show selected processes info
    if (!selectedProcesses.empty()) {
        ImGui::Spacing();
        ImGui::Text("Selected processes: %zu", selectedProcesses.size());
        ImGui::SameLine();
        if (ImGui::Button("Clear Selection")) {
            selectedProcesses.clear();
        }
    }

    ImGui::End();
}

// Helper function to calculate progress for network usage (0-2GB scale)
float calculateNetworkProgress(long long bytes) {
    const long long maxBytes = 2LL * 1024 * 1024 * 1024; // 2GB
    return min(1.0f, (float)bytes / maxBytes);
}

// network, display information network information
void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Network Interfaces Section
    ImGui::Separator();
    ImGui::Spacing();

    vector<NetworkInterface> interfaces = getNetworkInterfaces();

    for (const auto& iface : interfaces) {
        ImGui::Text("Interface: %s", iface.name.c_str());
        ImGui::SameLine();
        ImGui::Text("IP: %s", iface.ip.c_str());
    }

    ImGui::Spacing();

    // Tabbed section for RX and TX tables
    if (ImGui::BeginTabBar("NetworkTabs")) {

        // RX Tab
        if (ImGui::BeginTabItem("RX (Receiver)")) {
            // Visual representation with progress bars
            ImGui::Spacing();

            for (const auto& iface : interfaces) {
                ImGui::Text("Interface: %s", iface.name.c_str());

                // Calculate progress (scale to 0-2GB range for visualization)
                double maxBytes = 2.0 * 1024 * 1024 * 1024; // 2GB max scale
                float progress = (float)(iface.rx_bytes / maxBytes);
                if (progress > 1.0f) progress = 1.0f;

                // Progress bar with current bytes
                ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f),
                                 (formatBytes(iface.rx_bytes) + " received").c_str());
                ImGui::Spacing();
            }

            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTable("RXTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_ScrollX | ImGuiTableFlags_Resizable)) {

                ImGui::TableSetupColumn("Interface", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Packets", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Errors", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Dropped", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("FIFO", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Compressed", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Multicast", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                for (const auto& iface : interfaces) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", iface.name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", formatBytes(iface.rx_bytes).c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%lld", iface.rx_packets);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%lld", iface.rx_errors);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%lld", iface.rx_dropped);

                    // Note: FIFO, Frame, Compressed, Multicast would need additional parsing
                    // For now, showing placeholder values
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("0");

                    ImGui::TableSetColumnIndex(6);
                    ImGui::Text("0");

                    ImGui::TableSetColumnIndex(7);
                    ImGui::Text("0");

                    ImGui::TableSetColumnIndex(8);
                    ImGui::Text("0");
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        // TX Tab
        if (ImGui::BeginTabItem("TX (Transmitter)")) {
            // Visual representation with progress bars
            ImGui::Spacing();

            for (const auto& iface : interfaces) {
                ImGui::Text("Interface: %s", iface.name.c_str());

                // Calculate progress (scale to 0-2GB range for visualization)
                double maxBytes = 2.0 * 1024 * 1024 * 1024; // 2GB max scale
                float progress = (float)(iface.tx_bytes / maxBytes);
                if (progress > 1.0f) progress = 1.0f;

                // Progress bar with current bytes
                ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f),
                                 (formatBytes(iface.tx_bytes) + " transmitted").c_str());
                ImGui::Spacing();
            }

            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTable("TXTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_ScrollX | ImGuiTableFlags_Resizable)) {

                ImGui::TableSetupColumn("Interface", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Packets", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Errors", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Dropped", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("FIFO", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Colls", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Carrier", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Compressed", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                for (const auto& iface : interfaces) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", iface.name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", formatBytes(iface.tx_bytes).c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%lld", iface.tx_packets);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%lld", iface.tx_errors);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%lld", iface.tx_dropped);

                    // Note: FIFO, Colls, Carrier, Compressed would need additional parsing
                    // For now, showing placeholder values
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("0");

                    ImGui::TableSetColumnIndex(6);
                    ImGui::Text("0");

                    ImGui::TableSetColumnIndex(7);
                    ImGui::Text("0");

                    ImGui::TableSetColumnIndex(8);
                    ImGui::Text("0");
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Spacing();

    // Network Usage Visualization Section
    ImGui::Text("Network Usage (0GB - 2GB scale)");
    ImGui::Separator();

    // RX Usage bars
    ImGui::Text("RX (Received):");
    for (const auto& iface : interfaces) {
        float progress = calculateNetworkProgress(iface.rx_bytes);
        string label = iface.name + " RX: " + formatBytes(iface.rx_bytes);
        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), label.c_str());
    }

    ImGui::Spacing();

    // TX Usage bars
    ImGui::Text("TX (Transmitted):");
    for (const auto& iface : interfaces) {
        float progress = calculateNetworkProgress(iface.tx_bytes);
        string label = iface.name + " TX: " + formatBytes(iface.tx_bytes);
        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), label.c_str());
    }

    ImGui::End();
}

// Main code
int main(int, char **)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char *name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // render bindings
    ImGuiIO &io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // background color
    // note : you are free to change the style of the application
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Initialize CPU measurements for all processes
    initializeCPUMeasurements();

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        {
            ImVec2 mainDisplay = io.DisplaySize;
            memoryProcessesWindow("== Memory and Processes ==",
                                  ImVec2((mainDisplay.x / 2) - 20, (mainDisplay.y / 2) + 50),
                                  ImVec2((mainDisplay.x / 2) + 10, 10));
            // --------------------------------------
            systemWindow("== System ==",
                         ImVec2((mainDisplay.x / 2) - 10, (mainDisplay.y / 2) + 50),
                         ImVec2(10, 10));
            // --------------------------------------
            networkWindow("== Network ==",
                          ImVec2(mainDisplay.x - 20, (mainDisplay.y / 2) - 40),
                          ImVec2(10, (mainDisplay.y / 2) + 50));
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
