# CosmosClient - Developer Documentation Index

Welcome to the CosmosClient developer documentation! This guide will help you build, test, and develop the Azure Cosmos DB REST API client.

## 📚 Documentation Files

### Getting Started

1. **[QUICK_START.md](QUICK_START.md)** ⚡
   - **Best for**: Developers who want to get started immediately
   - **Time**: 5 minutes
   - **Contents**:
     - TL;DR commands for macOS and Linux
     - Common commands reference
     - Quick troubleshooting

2. **[DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md)** 📖
   - **Best for**: Comprehensive setup and development workflow
   - **Time**: 30 minutes to read, 1 hour to complete
   - **Contents**:
     - Detailed prerequisites
     - Platform-specific setup (macOS & Linux)
     - Building the project
     - Running tests
     - Troubleshooting guide
     - Development workflow tips

3. **[SCRIPTS_README.md](SCRIPTS_README.md)** 🔧
   - **Best for**: Automated build and test
   - **Time**: 2 minutes
   - **Contents**:
     - Helper script usage
     - Script features
     - Manual alternatives

---

## 🚀 Quick Start

### macOS

```bash
# Install tools
brew install cmake llvm docker

# Clone and setup
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive

# Build and test
./build_and_test.sh debug macos
```

### Linux

```bash
# Install tools
sudo apt-get update && sudo apt-get install -y cmake clang-15 docker.io
sudo usermod -aG docker $USER && newgrp docker

# Clone and setup
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive

# Build and test
./build_and_test.sh debug linux
```

---

## 📋 Common Tasks

### Build the Project

```bash
# Debug build
mkdir -p build && cd build
cmake --preset Apple-Debug ..
cmake --build .

# Release build
cmake --preset Apple-Release ..
cmake --build .

# Parallel build (faster)
cmake --build . -j$(nproc)
```

### Run Tests

```bash
# All tests (requires emulator)
ctest --output-on-failure --verbose

# Validation tests only (no emulator)
ctest --output-on-failure -R "Validation"

# Specific test
ctest --output-on-failure -R "CreateDocument"
```

### Manage Emulator

```bash
# Start
docker run --publish 8081:8081 --publish 10250-10255:10250-10255 \
  --name cosmos-emulator --rm --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest

# Stop
docker stop cosmos-emulator

# Check status
docker ps | grep cosmos-emulator
```

---

## 🏗️ Project Structure

```
CosmosClient/
├── include/
│   └── siddiqsoft/
│       └── cosmoscl.hpp          # Main header file
├── tests/
│   ├── testall.cpp               # Unified test suite (100+ tests)
│   ├── test_common.hpp           # Test utilities
│   ├── CMakeLists.txt            # Test build configuration
│   └── results/                  # Test output directory
├── build/                        # Build output (created by cmake)
├── CMakeLists.txt                # Main build configuration
├── CMakePresets.json             # CMake presets
├── DEVELOPER_BUILD_GUIDE.md      # Detailed build guide
├── QUICK_START.md                # Quick start guide
├── SCRIPTS_README.md             # Helper scripts guide
└── build_and_test.sh             # Automated build script
```

---

## 🧪 Test Organization

The unified test suite (`testall.cpp`) contains 100+ tests organized into categories:

### Validation Tests (No Emulator Required)
- Configuration validation
- Default settings
- JSON serialization
- Region discovery

### Connection Tests (No Emulator Required)
- Connection string parsing
- Connection rotation
- Primary/secondary failover

### Endpoint Tests (No Emulator Required)
- URI management
- Read/write URI rotation
- Fallback behavior

### Integration Tests (Requires Emulator)
- Database CRUD operations
- Collection CRUD operations
- Document CRUD operations
- Query operations
- Concurrent operations
- Data type handling
- Special character encoding
- Large document handling

---

## 🔧 Development Workflow

### 1. Setup (One-time)

```bash
# Install tools
brew install cmake llvm docker  # macOS
# or
sudo apt-get install cmake clang-15 docker.io  # Linux

# Clone repository
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive
```

### 2. Build

```bash
mkdir -p build && cd build
cmake --preset Apple-Debug ..
cmake --build . -j$(nproc)
```

### 3. Test

```bash
# Start emulator
docker run --publish 8081:8081 --publish 10250-10255:10250-10255 \
  --name cosmos-emulator --rm --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
sleep 60

# Run tests
ctest --output-on-failure --verbose

# Stop emulator
docker stop cosmos-emulator
```

### 4. Develop

```bash
# Make changes to source code
# Rebuild
cmake --build build/Apple-Debug -j$(nproc)

# Run specific tests
ctest --output-on-failure -R "YourTestName"
```

---

## 🐛 Troubleshooting

### Build Issues

| Problem | Solution |
|---------|----------|
| CMake not found | `brew install cmake` (macOS) or `sudo apt-get install cmake` (Linux) |
| Compiler not found | `brew install llvm` (macOS) or `sudo apt-get install clang-15` (Linux) |
| C++23 not supported | Update compiler to latest version |
| Out of memory | Reduce parallel jobs: `cmake --build . -j 2` |

### Test Issues

| Problem | Solution |
|---------|----------|
| Emulator not reachable | Check: `docker ps`, `docker logs cosmos-emulator` |
| Connection string not set | Set env vars: `export CCTEST_PRIMARY_CS=...` |
| Tests timeout | Increase timeout: `ctest --timeout 1800` |
| Docker permission denied | `sudo usermod -aG docker $USER && newgrp docker` |

See [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md#troubleshooting) for more troubleshooting tips.

---

## 📊 Test Statistics

- **Total Tests**: 100+
- **Test Categories**: 5 (Validation, Connection, Endpoint, Integration, Comprehensive)
- **Setup/Teardown**: Runs once for all tests
- **Database**: Single shared database
- **Collections**: 3 collections with 10 seeded documents each
- **Execution Time**: ~10 minutes (with emulator)

---

## 🔗 Related Documentation

- **[TEST_CONSOLIDATION_SUMMARY.md](TEST_CONSOLIDATION_SUMMARY.md)** - Test consolidation details
- **[PIPELINE_CONSOLIDATION_UPDATE.md](PIPELINE_CONSOLIDATION_UPDATE.md)** - CI/CD pipeline updates
- **[COMPLETE_CONSOLIDATION_SUMMARY.md](COMPLETE_CONSOLIDATION_SUMMARY.md)** - Complete consolidation overview

---

## 💡 Tips & Tricks

### Speed Up Builds

```bash
# Use all available cores
cmake --build . -j$(nproc)

# Or specify number of jobs
cmake --build . -j 8
```

### Run Specific Tests

```bash
# By category
ctest --output-on-failure -R "Validation"

# By pattern
ctest --output-on-failure -R "Query.*"

# Single test
ctest --output-on-failure -R "CreateDocument"
```

### Debug Tests

```bash
# With verbose output
ctest --output-on-failure --verbose

# With GDB (Linux)
gdb ./build/Linux-Clang-Debug/cosmoscl_tests

# With LLDB (macOS)
lldb ./build/Apple-Debug/cosmoscl_tests
```

### Generate Coverage Report

```bash
# Linux with Clang
cd build/Linux-Clang-Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -T Coverage
cat Testing/TAG/*/Coverage.xml
```

---

## 📞 Getting Help

1. **Check the documentation**
   - Start with [QUICK_START.md](QUICK_START.md)
   - Read [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md)

2. **Review troubleshooting**
   - See [DEVELOPER_BUILD_GUIDE.md#troubleshooting](DEVELOPER_BUILD_GUIDE.md#troubleshooting)

3. **Check build logs**
   - `cmake --build . --verbose`
   - `ctest --output-on-failure --verbose`

4. **Review GitHub Issues**
   - Check existing issues
   - Create a new issue with details

---

## 🎯 Next Steps

1. **New to the project?**
   - Start with [QUICK_START.md](QUICK_START.md)

2. **Need detailed setup?**
   - Read [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md)

3. **Want to automate?**
   - Use [build_and_test.sh](build_and_test.sh)

4. **Ready to develop?**
   - Follow the [Development Workflow](#-development-workflow) section

---

## 📝 Document Versions

| Document | Last Updated | Status |
|----------|--------------|--------|
| QUICK_START.md | 2024 | ✅ Current |
| DEVELOPER_BUILD_GUIDE.md | 2024 | ✅ Current |
| SCRIPTS_README.md | 2024 | ✅ Current |
| TEST_CONSOLIDATION_SUMMARY.md | 2024 | ✅ Current |
| PIPELINE_CONSOLIDATION_UPDATE.md | 2024 | ✅ Current |

---

## 📄 License

This project is licensed under the BSD 3-Clause License. See LICENSE file for details.

---

**Happy coding! 🚀**

For questions or issues, please refer to the documentation or create a GitHub issue.

