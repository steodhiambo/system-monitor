// To make sure you don't declare the function more than once by including the header multiple times.
#ifndef header_H
#define header_H

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
// lib to read from file
#include <fstream>
// for the name of the computer and the logged in user
#include <unistd.h>
#include <limits.h>
// this is for us to get the cpu information
// mostly in unix system
// not sure if it will work in windows
#include <cpuid.h>
// this is for the memory usage and other memory visualization
// for linux gotta find a way for windows
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
// for time and date
#include <ctime>
// ifconfig ip addresses
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

using namespace std;

struct CPUStats
{
    long long int user;
    long long int nice;
    long long int system;
    long long int idle;
    long long int iowait;
    long long int irq;
    long long int softirq;
    long long int steal;
    long long int guest;
    long long int guestNice;
};

// processes `stat`
struct Proc
{
    int pid;
    string name;
    char state;
    long long int vsize;
    long long int rss;
    long long int utime;
    long long int stime;
};

struct IP4
{
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks
{
    vector<IP4> ip4s;
};

struct TX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int frame;
    int compressed;
    int multicast;
};

struct RX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int colls;
    int carrier;
    int compressed;
};

// Core system information functions
string CPUinfo();
const char *getOsName();
string getHostname();
string getLoggedUser();
CPUStats getCPUStats();
double calculateCPUUsage(const CPUStats& prev, const CPUStats& curr);
vector<int> getTaskCounts(); // [running, sleeping, stopped, zombie]
double getThermalTemp();
string getFanStatus();
int getFanSpeed();

// UI state management for graphs
struct GraphData {
    vector<float> values;
    int max_values;
    bool animate;
    float fps;
    float y_scale;
    double last_update_time;

    GraphData(int max = 100) : max_values(max), animate(true), fps(60.0f), y_scale(100.0f), last_update_time(0.0) {}

    bool shouldUpdate() {
        double current_time = SDL_GetTicks() / 1000.0; // Get time in seconds
        double time_per_frame = 1.0 / fps;

        if (current_time - last_update_time >= time_per_frame) {
            last_update_time = current_time;
            return true;
        }
        return false;
    }

    void addValue(float value) {
        values.push_back(value);
        if ((int)values.size() > max_values) {
            values.erase(values.begin());
        }
    }
};

// Memory and process monitoring functions
struct MemoryInfo {
    long long total;
    long long available;
    long long used;
    double percentage;
};

struct DiskInfo {
    long long total;
    long long used;
    long long available;
    double percentage;
};

MemoryInfo getMemoryInfo();
MemoryInfo getSwapInfo();
DiskInfo getDiskInfo(const string& path = "/");
vector<Proc> getProcesses();
void initializeCPUMeasurements();
double getProcessCPUUsage(int pid);
double getProcessMemoryUsage(int pid);

// Network monitoring functions
struct NetworkInterface {
    string name;
    string ip;
    long long rx_bytes;
    long long tx_bytes;
    long long rx_packets;
    long long tx_packets;
    long long rx_errors;
    long long tx_errors;
    long long rx_dropped;
    long long tx_dropped;
};

vector<NetworkInterface> getNetworkInterfaces();
string formatBytes(long long bytes);

#endif
