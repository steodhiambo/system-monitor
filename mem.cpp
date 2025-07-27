#include "header.h"
#include <sstream>
#include <algorithm>

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

// Get CPU usage for a specific process
double getProcessCPUUsage(int pid)
{
    string stat_path = "/proc/" + to_string(pid) + "/stat";
    ifstream stat_file(stat_path);

    if (stat_file.is_open()) {
        string line;
        getline(stat_file, line);

        istringstream iss(line);
        string temp;
        long long utime, stime;

        // Skip to utime and stime (positions 14 and 15)
        for (int i = 0; i < 13; i++) {
            iss >> temp;
        }
        iss >> utime >> stime;

        // This is a simplified calculation
        // In a real implementation, you'd need to track time differences
        return (utime + stime) / 100.0; // Convert from clock ticks
    }

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
