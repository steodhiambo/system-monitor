#include "header.h"
#include <string.h>
#include <pwd.h>
#include <sstream>

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // unix system
    // for windoes maybe we must add the following
    // __cpuid(regs, 0);
    // regs is the array of 4 positions
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

// Get hostname of the computer
string getHostname()
{
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return string(hostname);
    }
    return "Unknown";
}

// Get logged in user
string getLoggedUser()
{
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return string(pw->pw_name);
    }
    return "Unknown";
}

// Read CPU stats from /proc/stat
CPUStats getCPUStats()
{
    CPUStats stats = {0};
    ifstream file("/proc/stat");
    string line;

    if (getline(file, line)) {
        istringstream iss(line);
        string cpu;
        iss >> cpu >> stats.user >> stats.nice >> stats.system >> stats.idle
            >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal
            >> stats.guest >> stats.guestNice;
    }

    return stats;
}

// Calculate CPU usage percentage
double calculateCPUUsage(const CPUStats& prev, const CPUStats& curr)
{
    long long prevIdle = prev.idle + prev.iowait;
    long long currIdle = curr.idle + curr.iowait;

    long long prevNonIdle = prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal;
    long long currNonIdle = curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal;

    long long prevTotal = prevIdle + prevNonIdle;
    long long currTotal = currIdle + currNonIdle;

    long long totalDiff = currTotal - prevTotal;
    long long idleDiff = currIdle - prevIdle;

    if (totalDiff == 0) return 0.0;

    return (double)(totalDiff - idleDiff) / totalDiff * 100.0;
}

// Get task counts from /proc/stat and process directories
vector<int> getTaskCounts()
{
    vector<int> counts(4, 0); // [running, sleeping, stopped, zombie]

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return counts;

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            string stat_path = "/proc/" + string(entry->d_name) + "/stat";
            ifstream stat_file(stat_path);

            if (stat_file.is_open()) {
                string line;
                if (getline(stat_file, line) && !line.empty()) {
                    // Find the state field - it's the 3rd field after PID and comm
                    // comm can contain spaces and parentheses, so we need to parse carefully
                    size_t first_paren = line.find('(');
                    size_t last_paren = line.rfind(')');

                    if (first_paren != string::npos && last_paren != string::npos && last_paren > first_paren) {
                        // Extract everything after the last parenthesis
                        string after_comm = line.substr(last_paren + 1);
                        istringstream iss(after_comm);
                        string state;
                        iss >> state; // First field after comm is the state

                        if (!state.empty()) {
                            char s = state[0];
                            switch (s) {
                                case 'R': counts[0]++; break; // Running
                                case 'S': case 'D': case 'I': counts[1]++; break; // Sleeping (including idle and uninterruptible)
                                case 'T': case 't': counts[2]++; break; // Stopped
                                case 'Z': counts[3]++; break; // Zombie
                                default:
                                    // Handle any other states as sleeping
                                    counts[1]++;
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }

    closedir(proc_dir);
    return counts;
}

// Get thermal temperature
double getThermalTemp()
{
    ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    if (temp_file.is_open()) {
        int temp_millidegrees;
        temp_file >> temp_millidegrees;
        return temp_millidegrees / 1000.0; // Convert to Celsius
    }
    return 0.0;
}

// Get fan status
string getFanStatus()
{
    ifstream fan_file("/proc/acpi/fan/FAN0/state");
    if (fan_file.is_open()) {
        string line;
        getline(fan_file, line);
        if (line.find("on") != string::npos) {
            return "Active";
        } else if (line.find("off") != string::npos) {
            return "Inactive";
        }
    }
    return "Unknown";
}

// Get fan speed (RPM)
int getFanSpeed()
{
    // Try different possible fan speed locations
    vector<string> fan_paths = {
        "/sys/class/hwmon/hwmon0/fan1_input",
        "/sys/class/hwmon/hwmon1/fan1_input",
        "/sys/class/hwmon/hwmon2/fan1_input"
    };

    for (const string& path : fan_paths) {
        ifstream fan_file(path);
        if (fan_file.is_open()) {
            int speed;
            fan_file >> speed;
            return speed;
        }
    }
    return 0;
}
