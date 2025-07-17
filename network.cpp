#include "header.h"
#include <sstream>

// Get network interfaces with statistics
vector<NetworkInterface> getNetworkInterfaces()
{
    vector<NetworkInterface> interfaces;

    // Get IP addresses first
    map<string, string> interface_ips;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN);
                interface_ips[ifa->ifa_name] = string(ip_str);
            }
        }
        freeifaddrs(ifaddr);
    }

    // Read network statistics from /proc/net/dev
    ifstream dev_file("/proc/net/dev");
    string line;

    // Skip header lines
    getline(dev_file, line);
    getline(dev_file, line);

    while (getline(dev_file, line)) {
        istringstream iss(line);
        string interface_name;

        // Extract interface name (remove colon)
        iss >> interface_name;
        if (!interface_name.empty() && interface_name.back() == ':') {
            interface_name.pop_back();
        }

        NetworkInterface iface;
        iface.name = interface_name;
        iface.ip = interface_ips.count(interface_name) ? interface_ips[interface_name] : "N/A";

        // Read RX statistics
        iss >> iface.rx_bytes >> iface.rx_packets >> iface.rx_errors >> iface.rx_dropped;

        // Skip some fields
        long long temp;
        iss >> temp >> temp >> temp >> temp; // fifo, frame, compressed, multicast

        // Read TX statistics
        iss >> iface.tx_bytes >> iface.tx_packets >> iface.tx_errors >> iface.tx_dropped;

        interfaces.push_back(iface);
    }

    return interfaces;
}

// Format bytes to appropriate unit (B, KB, MB, GB)
string formatBytes(long long bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }

    // Format with appropriate precision
    ostringstream oss;
    if (unit_index == 0) {
        oss << (long long)size << " " << units[unit_index];
    } else {
        oss << fixed;
        oss.precision(2);
        oss << size << " " << units[unit_index];
    }

    return oss.str();
}
