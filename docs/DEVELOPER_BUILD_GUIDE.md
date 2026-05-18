# CosmosClient - Developer Build & Test Guide

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [macOS Setup](#macos-setup)
3. [Linux Setup](#linux-setup)
4. [Building the Project](#building-the-project)
5. [Running Tests](#running-tests)
6. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Tools

All platforms require:
- **CMake** 3.20 or higher
- **C++23 compatible compiler**
- **Git**
- **Docker** (for Azure Cosmos DB Emulator)

### Environment Variables

Set these before building:

```bash
# Azure Cosmos DB connection strings (required for tests)
export CCTEST_PRIMARY_CS="AccountEndpoint=https://localhost:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPD8v+F7hHxJ0bLvngNcWL0rnA==;"
export CCTEST_SECONDARY_CS="AccountEndpoint=https://localhost:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPD8v+F7hHxJ0bLvngNcWL0rnA==;"
```

---

**NOTE**
The file `project-base.json` contains the project-specific settings for the common CMakePresets.json file. Make sure to change the value `project_BUILD_TESTS` where `project` is the name of your project.


## macOS Setup

### 1. Install Homebrew (if not already installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Install Required Tools

```bash
# Install CMake
brew install cmake

# Install LLVM (for C++23 support)
brew install llvm

# Install Docker Desktop
# Download from: https://www.docker.com/products/docker-desktop
# Or use Homebrew:
brew install --cask docker
```

### 3. Configure Compiler

Set up environment variables for LLVM:

```bash
# Add to ~/.zshrc or ~/.bash_profile
export CC=/usr/local/opt/llvm/bin/clang
export CXX=/usr/local/opt/llvm/bin/clang++
export LDFLAGS="-L/usr/local/opt/llvm/lib"
export CPPFLAGS="-I/usr/local/opt/llvm/include"
```

Then reload your shell:

```bash
source ~/.zshrc  # or ~/.bash_profile
```

### 4. Verify Installation

```bash
cmake --version
clang++ --version
docker --version
```

---

## Linux Setup

### Ubuntu/Debian

```bash
# Update package manager
sudo apt-get update

# Install CMake
sudo apt-get install -y cmake

# Install Clang (for C++23 support)
sudo apt-get install -y clang-15 clang++-15

# Install Docker
sudo apt-get install -y docker.io

# Add your user to docker group (to run without sudo)
sudo usermod -aG docker $USER
newgrp docker
```

### Fedora/RHEL

```bash
# Install CMake
sudo dnf install -y cmake

# Install Clang
sudo dnf install -y clang

# Install Docker
sudo dnf install -y docker

# Add your user to docker group
sudo usermod -aG docker $USER
newgrp docker
```

### 5. Verify Installation

```bash
cmake --version
clang++ --version
docker --version
```

---

## Building the Project

### Step 1: Clone the Repository

```bash
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
```

### Step 2: Initialize Git Submodules

```bash
git submodule update --init --recursive
```

### Step 3: Create Build Directory

```bash
mkdir -p build
cd build
```

### Step 4: Configure with CMake

#### macOS

```bash
# Debug build
cmake --preset Apple-Debug ..

# Or Release build
cmake --preset Apple-Release ..
```

#### Linux

```bash
# Debug build with Clang
cmake --preset Linux-Clang-Debug ..

# Or Release build with Clang
cmake --preset Linux-Clang-Release ..

# Or with GCC (if available)
cmake --preset Linux-GCC-Debug ..
cmake --preset Linux-GCC-Release ..
```

### Step 5: Build the Project

```bash
# Build using the preset
cmake --build . --config Debug

# Or specify the build directory
cmake --build build/Apple-Debug

# Or use make directly
cd build/Apple-Debug
make -j$(nproc)
```

### Build Output

The compiled executable will be located at:

```bash
# macOS
build/Apple-Debug/cosmoscl_tests

# Linux
build/Linux-Clang-Debug/cosmoscl_tests
```

---

## Running Tests

### Prerequisites for Tests

Before running tests, start the Azure Cosmos DB Emulator:

#### macOS

```bash
# Start Docker Desktop first
open -a Docker

# Wait for Docker to be ready, then start the emulator
docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
docker run \
  --publish 8081:8081 \
  --publish 10250-10255:10250-10255 \
  --name cosmos-emulator \
  --rm \
  --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest

# Wait 30-60 seconds for the emulator to start
sleep 60

# Verify the emulator is running
curl -k https://localhost:8081/_explorer/index.html
```

#### Linux

```bash
# Start the emulator
docker run \
  --publish 8081:8081 \
  --publish 10250-10255:10250-10255 \
  --name cosmos-emulator \
  --rm \
  --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest

# Wait 30-60 seconds for the emulator to start
sleep 60

# Verify the emulator is running
curl -k https://localhost:8081/_explorer/index.html
```

### Run All Tests

```bash
cd build/Apple-Debug  # or your build directory

# Run all tests with verbose output
ctest --output-on-failure --verbose

# Or run the test executable directly
./cosmoscl_tests
```

### Run Specific Test Categories

```bash
cd build/Apple-Debug

# Run only validation tests (no emulator required)
ctest --output-on-failure -R "Validation"

# Run only connection tests (no emulator required)
ctest --output-on-failure -R "CosmosConnection"

# Run only endpoint tests (no emulator required)
ctest --output-on-failure -R "CosmosEndpoint"

# Run only integration tests (requires emulator)
ctest --output-on-failure -R "CosmosIntegrationTests"

# Run comprehensive API tests
ctest --output-on-failure -R "Comprehensive"
```

### Run Specific Test

```bash
cd build/Apple-Debug

# Run a single test
ctest --output-on-failure -R "CreateDocument"

# Run with regex pattern
ctest --output-on-failure -R "Query.*"
```

### View Test Results

```bash
cd build/Apple-Debug

# Show test output
ctest --output-on-failure --verbose

# Generate XML report
ctest --output-on-failure -T Test

# View XML report
cat Testing/TAG/*/Test.xml
```

### Stop the Emulator

```bash
# Stop and remove the container
docker stop cosmos-emulator
docker rm cosmos-emulator

# Or just stop it
docker stop cosmos-emulator
```

---

## Complete Workflow Example

### macOS

```bash
# 1. Clone and setup
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive

# 2. Start Docker and Cosmos Emulator
open -a Docker
sleep 10  # Wait for Docker to start
docker run \
  --publish 8081:8081 \
  --publish 10250-10255:10250-10255 \
  --name cosmos-emulator \
  --rm \
  --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
sleep 60  # Wait for emulator to start

# 3. Configure and build
mkdir -p build
cd build
cmake --preset Apple-Debug ..
cmake --build . --config Debug

# 4. Run tests
ctest --output-on-failure --verbose

# 5. View results
cat Testing/TAG/*/Test.xml

# 6. Cleanup
cd ..
docker stop cosmos-emulator
```

### Linux

```bash
# 1. Clone and setup
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive

# 2. Start Cosmos Emulator
docker run \
  --publish 8081:8081 \
  --publish 10250-10255:10250-10255 \
  --name cosmos-emulator \
  --rm \
  --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
sleep 60  # Wait for emulator to start

# 3. Configure and build
mkdir -p build
cd build
cmake --preset Linux-Clang-Debug ..
cmake --build . --config Debug

# 4. Run tests
ctest --output-on-failure --verbose

# 5. View results
cat Testing/TAG/*/Test.xml

# 6. Cleanup
cd ..
docker stop cosmos-emulator
```

---

## Troubleshooting

### CMake Configuration Issues

#### "CMake not found"

```bash
# macOS
brew install cmake

# Linux (Ubuntu/Debian)
sudo apt-get install -y cmake

# Linux (Fedora/RHEL)
sudo dnf install -y cmake
```

#### "Compiler not found"

```bash
# macOS - Install LLVM
brew install llvm

# Linux (Ubuntu/Debian) - Install Clang
sudo apt-get install -y clang-15 clang++-15

# Linux (Fedora/RHEL) - Install Clang
sudo dnf install -y clang
```

#### "C++23 not supported"

Ensure you're using a recent compiler version:

```bash
# macOS
clang++ --version  # Should be 15.0 or higher

# Linux
clang++ --version  # Should be 15.0 or higher
```

### Build Issues

#### "Missing dependencies"

The project uses CPM (C++ Package Manager) to download dependencies automatically. If you encounter issues:

```bash
# Clear CMake cache and rebuild
rm -rf build
mkdir build
cd build
cmake --preset Apple-Debug ..
cmake --build .
```

#### "Out of memory during build"

Reduce parallel build jobs:

```bash
cmake --build . -j 2
```

### Test Issues

#### "Cosmos Emulator not reachable"

```bash
# Check if emulator is running
docker ps | grep cosmos-emulator

# Check emulator logs
docker logs cosmos-emulator

# Verify connectivity
curl -k https://localhost:8081/_explorer/index.html

# If not running, start it
docker run \
  --publish 8081:8081 \
  --publish 10250-10255:10250-10255 \
  --name cosmos-emulator \
  --rm \
  --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
```

#### "Connection string not set"

```bash
# Set environment variables
export CCTEST_PRIMARY_CS="AccountEndpoint=https://localhost:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPD8v+F7hHxJ0bLvngNcWL0rnA==;"
export CCTEST_SECONDARY_CS="AccountEndpoint=https://localhost:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPD8v+F7hHxJ0bLvngNcWL0rnA==;"

# Verify they're set
echo $CCTEST_PRIMARY_CS
```

#### "Tests timeout"

Increase the timeout:

```bash
ctest --output-on-failure --timeout 1800
```

#### "Docker permission denied"

```bash
# Add user to docker group
sudo usermod -aG docker $USER
newgrp docker

# Verify
docker ps
```

### Performance Issues

#### "Build is slow"

```bash
# Use more parallel jobs
cmake --build . -j$(nproc)

# Or specify number of jobs
cmake --build . -j 8
```

#### "Tests are slow"

```bash
# Run only specific tests
ctest --output-on-failure -R "Validation"

# Skip integration tests that require emulator
ctest --output-on-failure -R "Validation|Connection|Endpoint"
```

---

## Development Workflow Tips

### Quick Build & Test

```bash
# Create a script: build_and_test.sh
#!/bin/bash
set -e

cd build/Apple-Debug
cmake --build . -j$(nproc)
ctest --output-on-failure --verbose
```

### Watch for Changes

```bash
# Install watchman (macOS)
brew install watchman

# Or use entr (Linux)
sudo apt-get install -y entr

# Watch for changes and rebuild
find . -name "*.cpp" -o -name "*.hpp" | entr cmake --build build/Apple-Debug
```

### Debug Tests

```bash
# Run with GDB (Linux)
gdb ./build/Linux-Clang-Debug/cosmoscl_tests

# Run with LLDB (macOS)
lldb ./build/Apple-Debug/cosmoscl_tests

# Run specific test with debugging
ctest --output-on-failure -R "CreateDocument" --verbose
```

### Code Coverage

```bash
# Generate coverage report (Linux with Clang)
cd build/Linux-Clang-Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -T Coverage

# View coverage
cat Testing/TAG/*/Coverage.xml
```

---

## Additional Resources

- **CMake Documentation**: https://cmake.org/documentation/
- **Google Test Documentation**: https://google.github.io/googletest/
- **Azure Cosmos DB Emulator**: https://docs.microsoft.com/en-us/azure/cosmos-db/local-emulator
- **Docker Documentation**: https://docs.docker.com/

---

## Getting Help

If you encounter issues:

1. Check the [Troubleshooting](#troubleshooting) section
2. Review the project's GitHub Issues
3. Check the build logs: `cmake --build . --verbose`
4. Enable verbose testing: `ctest --output-on-failure --verbose`

---

## Summary

| Task | macOS | Linux |
|------|-------|-------|
| Install tools | `brew install cmake llvm` | `sudo apt-get install cmake clang-15` |
| Configure | `cmake --preset Apple-Debug ..` | `cmake --preset Linux-Clang-Debug ..` |
| Build | `cmake --build .` | `cmake --build .` |
| Start emulator | `docker run ...` | `docker run ...` |
| Run tests | `ctest --output-on-failure` | `ctest --output-on-failure` |
| Stop emulator | `docker stop cosmos-emulator` | `docker stop cosmos-emulator` |

