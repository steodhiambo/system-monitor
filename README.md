# System Monitor

A comprehensive real-time system monitoring application for Linux systems, built with C++ and Dear ImGui. This application provides detailed insights into system performance, memory usage, process management, and network activity through an intuitive graphical interface.

## 🚀 Features

### System Information
- **Operating System Detection**: Displays current OS (Linux)
- **User Information**: Shows currently logged-in user
- **Hostname**: System hostname identification
- **CPU Information**: Detailed CPU model and specifications
- **Task Monitoring**: Real-time process counting (running, sleeping, stopped, zombie)

### Performance Monitoring
- **CPU Usage**: Real-time CPU percentage with interactive graphs
- **Thermal Monitoring**: Live temperature readings with responsive graphs
- **Fan Status**: Fan speed and status monitoring
- **Interactive Controls**: FPS sliders, Y-scale adjustment, animation toggle

### Memory & Process Management
- **RAM Usage**: Visual progress bars with detailed usage statistics
- **SWAP Usage**: Real-time swap memory monitoring
- **Disk Usage**: Storage space monitoring with accurate calculations
- **Process Table**: Comprehensive process list with PID, Name, State, CPU%, Memory%
- **Process Filtering**: Real-time search and filter capabilities
- **Multi-Selection**: Select multiple processes with Ctrl+click

### Network Monitoring
- **Interface Detection**: Automatic network interface discovery
- **IP Address Mapping**: Real-time IP address display for each interface
- **RX/TX Statistics**: Detailed receive and transmit data monitoring
- **Visual Representation**: Progress bars for network usage visualization
- **Comprehensive Tables**: Detailed network statistics (bytes, packets, errors, drops)

## 🛠️ Requirements

### System Requirements
- **Operating System**: Linux (Ubuntu, Fedora, Debian, etc.)
- **Architecture**: x86_64
- **Graphics**: OpenGL 3.0+ support
- **Display**: X11 or Wayland display server

### Dependencies
- **SDL2**: Graphics and window management
- **OpenGL**: 3D graphics rendering
- **Dear ImGui**: Immediate mode GUI framework
- **Standard C++ Libraries**: C++11 or later

## 📦 Installation

### Ubuntu/Debian
```bash
# Install SDL2 development libraries
sudo apt update
sudo apt install libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev

# Clone the repository
git clone <repository-url>
cd system-monitor

# Build the application
make

# Run the system monitor
./monitor
```

### Fedora/RHEL
```bash
# Install SDL2 development libraries
sudo dnf install SDL2-devel mesa-libGL-devel mesa-libGLU-devel

# Build and run
make
./monitor
```

### Arch Linux
```bash
# Install dependencies
sudo pacman -S sdl2 mesa

# Build and run
make
./monitor
```

## 🏗️ Building from Source

### Prerequisites
- GCC or Clang compiler with C++11 support
- Make build system
- SDL2 development headers
- OpenGL development headers

### Build Commands
```bash
# Clean build
make clean
make

# Clean all generated files
make clean-all

# Clean only test binaries
make clean-tests
```

### Build Configuration
The project uses a cross-platform Makefile that automatically detects:
- Operating system (Linux, macOS, Windows/MinGW)
- SDL2 configuration
- OpenGL loader setup

## 🎮 Usage

### Starting the Application
```bash
./monitor
```

### Interface Overview
The application features three main windows:

#### 1. System Monitor Window
- **System Information**: OS, user, hostname, CPU details
- **Tabbed Interface**: 
  - **CPU Tab**: Real-time CPU usage graphs with controls
  - **Thermal Tab**: Temperature monitoring with responsive graphs
  - **Fan Tab**: Fan speed and status monitoring

#### 2. Memory and Process Monitor Window
- **Memory Usage**: Visual progress bars for RAM, SWAP, and Disk
- **Process Table**: Sortable table with comprehensive process information
- **Process Controls**: Filter, search, and multi-select processes

#### 3. Network Monitor Window
- **Interface List**: All network interfaces with IP addresses
- **RX Tab**: Receive statistics with visual progress bars
- **TX Tab**: Transmit statistics with visual progress bars
- **Detailed Tables**: Comprehensive network statistics

### Interactive Controls
- **Graph Controls**: Adjust FPS (1-120), Y-scale (50-200%), toggle animation
- **Process Filtering**: Type to filter processes in real-time
- **Multi-Selection**: Hold Ctrl and click to select multiple processes
- **Real-time Updates**: All data refreshes automatically

## 📊 Data Sources

The application reads real-time data from Linux system files:
- `/proc/stat` - CPU and task statistics
- `/proc/meminfo` - Memory and swap information
- `/proc/cpuinfo` - CPU specifications
- `/proc/net/dev` - Network interface statistics
- `/proc/*/stat` - Individual process information
- `/sys/class/thermal/` - Temperature sensors
- `/proc/acpi/ibm/thermal` - ThinkPad thermal data (if available)

## 🧪 Testing

The project includes comprehensive test suites to verify functionality:

### Running Tests
```bash
# Compile and run all tests
g++ -o test_functions test_functions.cpp system.cpp mem.cpp network.cpp -std=c++11
./test_functions
```

### Test Coverage
- System information accuracy
- Memory calculation verification
- Network statistics validation
- Process enumeration testing
- GUI component functionality

## 🔧 Troubleshooting

### Common Issues

#### GUI Not Displaying
```bash
# Check display environment
echo $DISPLAY

# Ensure X11 is running
ps aux | grep X

# Install missing graphics libraries
sudo apt install libgl1-mesa-dev libglu1-mesa-dev
```

#### Permission Denied Errors
```bash
# Ensure executable permissions
chmod +x monitor

# Check file permissions
ls -la monitor
```

#### SDL2 Not Found
```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev

# Verify SDL2 installation
pkg-config --modversion sdl2
```

#### Compilation Errors
```bash
# Clean and rebuild
make clean-all
make

# Check compiler version
g++ --version
```

### Performance Optimization
- **Reduce FPS**: Lower graph FPS for better performance on older systems
- **Disable Animation**: Turn off graph animations to reduce CPU usage
- **Close Unused Tabs**: Focus on specific monitoring areas

## 📁 Project Structure

```
system-monitor/
├── main.cpp              # Main application and GUI implementation
├── system.cpp            # System information and CPU monitoring
├── mem.cpp               # Memory and process monitoring
├── network.cpp           # Network interface monitoring
├── header.h              # Function declarations and structures
├── Makefile              # Build configuration
├── README.md             # Project documentation
├── imgui/                # Dear ImGui library
│   └── lib/              # ImGui source files
├── test_*.cpp            # Test files (optional)
└── .gitignore            # Git ignore rules
```

## 🤝 Contributing

### Development Guidelines
1. **Code Style**: Follow existing C++ conventions
2. **Testing**: Add tests for new functionality
3. **Documentation**: Update README for new features
4. **Commits**: Use descriptive commit messages

### Adding New Features
1. **System Monitoring**: Add new data sources in appropriate .cpp files
2. **GUI Components**: Extend main.cpp with new interface elements
3. **Testing**: Create corresponding test files
4. **Documentation**: Update README and code comments

### Reporting Issues
- Include system information (OS, version, hardware)
- Provide error messages and logs
- Describe steps to reproduce the issue
- Include screenshots if GUI-related

## 📄 License

This project is open source. Please check the license file for specific terms and conditions.

## 🙏 Acknowledgments

- **Dear ImGui**: Excellent immediate mode GUI framework
- **SDL2**: Cross-platform multimedia library
- **Linux Community**: For comprehensive /proc filesystem documentation
- **Contributors**: All developers who helped improve this project

## 📞 Support

For support, questions, or feature requests:
- Create an issue in the project repository
- Check existing documentation and troubleshooting guides
- Review test files for usage examples

## 🔄 Version History

### Latest Version
- ✅ Complete system monitoring implementation
- ✅ Real-time data accuracy verified
- ✅ Professional GUI with visual representations
- ✅ Comprehensive testing suite
- ✅ Cross-platform build system
- ✅ Detailed documentation

---

**Built with ❤️ for the Linux community**
