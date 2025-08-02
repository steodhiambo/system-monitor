#include "header.h"
#include <sstream>
#include <algorithm>
#include <map>
#include <unistd.h>
#include <ctime>
#include <vector>
#include <cmath>
#include <sys/times.h>

// Get memory information from /proc/meminfo
MemoryInfo getMemoryInfo()
{
    MemoryInfo info = {0, 0, 0, 0.0};
    ifstream file("/proc/meminfo");
    string line;

    while (getline(file, line)) {
        istringstream iss(line);
        string key;
        long long value;
        string unit;

        if (iss >> key >> value >> unit) {
            if (key == "MemTotal:") {
                info.total = value * 1024; // Convert KB to bytes
            } else if (key == "MemAvailable:") {
                info.available = value * 1024;
            }
        }
    }

    info.used = info.total - info.available;
    if (info.total > 0) {
        info.percentage = (double)info.used / info.total * 100.0;
    }

    return info;
}

// Get swap information from /proc/meminfo
MemoryInfo getSwapInfo()
{
    MemoryInfo info = {0, 0, 0, 0.0};
    ifstream file("/proc/meminfo");
    string line;
    long long swapFree = 0;

    while (getline(file, line)) {
        istringstream iss(line);
        string key;
        long long value;
        string unit;

        if (iss >> key >> value >> unit) {
            if (key == "SwapTotal:") {
                info.total = value * 1024; // Convert KB to bytes
            } else if (key == "SwapFree:") {
                swapFree = value * 1024;
            }
        }
    }

    info.available = swapFree;
    info.used = info.total - info.available;
    if (info.total > 0) {
        info.percentage = (double)info.used / info.total * 100.0;
    }

    return info;
}

// Get disk information using statvfs
DiskInfo getDiskInfo(const string& path)
{
    DiskInfo info = {0, 0, 0, 0.0};
    struct statvfs stat;

    if (statvfs(path.c_str(), &stat) == 0) {
        // Calculate disk usage like 'df' command
        long long total_blocks = stat.f_blocks;
        long long free_blocks = stat.f_bfree;
        long long available_blocks = stat.f_bavail;
        long long block_size = stat.f_frsize;

        info.total = total_blocks * block_size;
        info.available = available_blocks * block_size;

        // Used = Total - Free (this matches df calculation)
        info.used = (total_blocks - free_blocks) * block_size;

        if (info.total > 0) {
            info.percentage = (double)info.used / info.total * 100.0;
        }
    }

    return info;
}

// Get list of processes from /proc
vector<Proc> getProcesses()
{
    vector<Proc> processes;
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return processes;

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            int pid = atoi(entry->d_name);

            // Read process stat file
            string stat_path = "/proc/" + string(entry->d_name) + "/stat";
            ifstream stat_file(stat_path);

            if (stat_file.is_open()) {
                Proc proc;
                proc.pid = pid;

                string line;
                getline(stat_file, line);

                // Parse stat file
                istringstream iss(line);
                string pid_str, comm, state_str;
                iss >> pid_str >> comm >> state_str;

                // Remove parentheses from command name
                if (comm.length() > 2) {
                    proc.name = comm.substr(1, comm.length() - 2);
                } else {
                    proc.name = comm;
                }

                proc.state = state_str[0];

                // Skip to the fields we need (vsize and rss are at positions 23 and 24)
                string temp;
                for (int i = 0; i < 20; i++) {
                    iss >> temp;
                }
                iss >> proc.vsize >> proc.rss;

                // Skip to utime and stime (positions 14 and 15 from start)
                istringstream iss2(line);
                for (int i = 0; i < 13; i++) {
                    iss2 >> temp;
                }
                iss2 >> proc.utime >> proc.stime;

                processes.push_back(proc);
            }
        }
    }

    closedir(proc_dir);
    return processes;
}

// Initialize CPU measurements for all processes (call this once at startup)
void initializeCPUMeasurements()
{
    vector<Proc> processes = getProcesses();
    for (const auto& proc : processes) {
        getProcessCPUUsage(proc.pid); // Initialize measurement
    }
}

// Get CPU usage for a specific process - NEW ACCURATE METHOD
double getProcessCPUUsage(int pid)
{
    static map<int, pair<long long, long long>> prev_process_times; // pid -> (total_time, timestamp)
    static map<int, pair<long long, long long>> prev_system_times;  // pid -> (total_system, timestamp)

    // Read process CPU time
    string stat_path = "/proc/" + to_string(pid) + "/stat";
    ifstream stat_file(stat_path);
    if (!stat_file.is_open()) {
        return 0.0;
    }

    string line;
    getline(stat_file, line);

    // Parse process stat
    istringstream iss(line);
    vector<string> fields;
    string field;
    while (iss >> field) {
        fields.push_back(field);
    }

    if (fields.size() < 17) {
        return 0.0;
    }

    // Get process CPU times (in clock ticks)
    long long utime = stoll(fields[13]);   // User time
    long long stime = stoll(fields[14]);   // System time
    long long cutime = stoll(fields[15]);  // Children user time
    long long cstime = stoll(fields[16]);  // Children system time

    long long process_total = utime + stime + cutime + cstime;

    // Read system CPU time from /proc/stat
    ifstream system_stat("/proc/stat");
    if (!system_stat.is_open()) {
        return 0.0;
    }

    string system_line;
    getline(system_stat, system_line);

    // Parse system CPU line: "cpu user nice system idle iowait irq softirq steal guest guest_nice"
    istringstream system_iss(system_line);
    string cpu_label;
    system_iss >> cpu_label; // Skip "cpu"

    long long system_total = 0;
    string time_str;
    while (system_iss >> time_str) {
        system_total += stoll(time_str);
    }

    // Get current timestamp in clock ticks
    long long current_ticks = times(nullptr);

    // Calculate CPU percentage
    if (prev_process_times.find(pid) != prev_process_times.end() &&
        prev_system_times.find(pid) != prev_system_times.end()) {

        long long prev_process = prev_process_times[pid].first;
        long long prev_system = prev_system_times[pid].first;
        long long prev_timestamp = prev_process_times[pid].second;

        long long process_diff = process_total - prev_process;
        long long system_diff = system_total - prev_system;
        long long time_diff = current_ticks - prev_timestamp;

        // Calculate CPU percentage like top does
        double cpu_percent = 0.0;
        if (system_diff > 0) {
            // Method 1: Based on system CPU time difference (most accurate)
            cpu_percent = (double)process_diff / (double)system_diff * 100.0;
        } else if (time_diff > 0) {
            // Method 2: Based on elapsed time (fallback)
            double process_seconds = (double)process_diff / sysconf(_SC_CLK_TCK);
            double elapsed_seconds = (double)time_diff / sysconf(_SC_CLK_TCK);
            cpu_percent = (process_seconds / elapsed_seconds) * 100.0;
        }

        // Store current values for next calculation
        prev_process_times[pid] = make_pair(process_total, current_ticks);
        prev_system_times[pid] = make_pair(system_total, current_ticks);

        // Clamp to reasonable range (0-400% for multi-core)
        if (cpu_percent < 0) cpu_percent = 0.0;
        if (cpu_percent > 400.0) cpu_percent = 400.0;

        return cpu_percent;
    }

    // First measurement - store baseline
    prev_process_times[pid] = make_pair(process_total, current_ticks);
    prev_system_times[pid] = make_pair(system_total, current_ticks);

    return 0.0;
}

// Get memory usage for a specific process
double getProcessMemoryUsage(int pid)
{
    string status_path = "/proc/" + to_string(pid) + "/status";
    ifstream status_file(status_path);

    if (status_file.is_open()) {
        string line;
        while (getline(status_file, line)) {
            if (line.find("VmRSS:") == 0) {
                istringstream iss(line);
                string key;
                long long value;
                string unit;

                if (iss >> key >> value >> unit) {
                    // Get total system memory for percentage calculation
                    MemoryInfo memInfo = getMemoryInfo();
                    if (memInfo.total > 0) {
                        return (double)(value * 1024) / memInfo.total * 100.0;
                    }
                }
                break;
            }
        }
    }

    return 0.0;
}
