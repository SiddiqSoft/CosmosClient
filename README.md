# CosmosClient: Azure Cosmos REST-API Client for Modern C++

<!-- badges -->
[![CodeQL](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.CosmosClient?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionid=15&branchName=main)
![](https://img.shields.io/github/v/tag/SiddiqSoft/CosmosClient)
![](https://img.shields.io/nuget/v/SiddiqSoft.CosmosClient)
![](https://img.shields.io/nuget/dt/SiddiqSoft.CosmosClient)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/15)
![](https://img.shields.io/github/license/siddiqsoft/CosmosClient)
<!-- end badges -->

---

## 🎯 Quick Links

**New to the project?** Start here:
- 📖 **[START_HERE.md](START_HERE.md)** - Main entry point for all developers
- ⚡ **[QUICK_START.md](QUICK_START.md)** - Get started in 5 minutes
- 📚 **[DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)** - Find what you need

**Want to build and test?**
- 🔧 **[DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md)** - Complete build guide
- 🚀 **[SCRIPTS_README.md](SCRIPTS_README.md)** - Automated build script

**Need API documentation?**
- 📖 **[API Documentation](https://siddiqsoft.github.io/CosmosClient/)** - Full API reference

---

## 📋 Overview

A lightweight, modern C++ client for Azure Cosmos DB REST API. Designed for C++ developers who want a simple, functional interface without unnecessary abstractions.

### Key Features

- **C++23 Native** - Modern C++ with structured bindings and initializer lists
- **JSON-First** - Configuration, I/O, and results all use JSON
- **Simple Interface** - Minimal abstractions, maximum clarity
- **Full Cosmos SQL API** - Complete REST API implementation
- **Single Header** - Easy integration
- **Cross-Platform** - Windows, macOS, and Linux support
- **Async Operations** - Built-in async/await support
- **Zero-Cost Abstractions** - Only pay for what you use

### Dependencies

- [nlohmann/json](https://github.com/nlohmann/json) - JSON library
- [SplitUri](https://github.com/SiddiqSoft/SplitUri) - URI parsing utilities
- [StringHelpers](https://github.com/siddiqsoft/StringHelpers) - String manipulation utilities
- [AzureCppUtils](https://github.com/SiddiqSoft/AzureCppUtils) - Azure utilities
- [string2map](https://github.com/SiddiqSoft/string2map) - String to map conversion
- [RunOnEnd](https://github.com/SiddiqSoft/RunOnEnd) - RAII scope guard utility
- [asynchrony](https://github.com/SiddiqSoft/asynchrony) - Async/await support
- [timethis](https://github.com/SiddiqSoft/timethis) - Timing utilities
- [restcl](https://github.com/SiddiqSoft/restcl) - REST client library

---

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive
```

### Build

```bash
# macOS
mkdir -p build && cd build
cmake --preset Apple-Debug ..
cmake --build .

# Linux
mkdir -p build && cd build
cmake --preset Linux-Clang-Debug ..
cmake --build .
```

### Usage Example

```cpp
#include "nlohmann/json.hpp"
#include "siddiqsoft/cosmoscl.hpp"

int main() {
    // Create client instance
    siddiqsoft::CosmosClient cc;

    // Configure with connection strings
    cc.configure({
        {"partitionKeyNames", {"__pk"}},
        {"connectionStrings", {primaryCS, secondaryCS}}
    });

    // Create a document
    auto response = cc.createDocument({
        .database   = "mydb",
        .collection = "mycoll",
        .document   = {
            {"id", "doc1"},
            {"__pk", "partition1"},
            {"name", "My Document"},
            {"value", 42}
        }
    });

    if (response.statusCode == 201) {
        std::cout << "Document created!" << std::endl;
    }

    return 0;
}
```

---

## 📚 Documentation

### For Developers

| Document | Purpose | Time |
|----------|---------|------|
| [START_HERE.md](START_HERE.md) | Main entry point | 2 min |
| [QUICK_START.md](QUICK_START.md) | 5-minute quick start | 5 min |
| [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md) | Comprehensive guide | 30 min |
| [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md) | Navigation hub | 5 min |
| [SCRIPTS_README.md](SCRIPTS_README.md) | Automated scripts | 2 min |

### For Understanding Changes

| Document | Purpose | Time |
|----------|---------|------|
| [TEST_CONSOLIDATION_SUMMARY.md](TEST_CONSOLIDATION_SUMMARY.md) | Test consolidation | 10 min |
| [PIPELINE_CONSOLIDATION_UPDATE.md](PIPELINE_CONSOLIDATION_UPDATE.md) | CI/CD updates | 10 min |
| [COMPLETE_CONSOLIDATION_SUMMARY.md](COMPLETE_CONSOLIDATION_SUMMARY.md) | Complete overview | 15 min |

### API Reference

- **[Full API Documentation](https://siddiqsoft.github.io/CosmosClient/)** - Complete API reference

---

## 🛠️ Development

### Prerequisites

**macOS:**
```bash
brew install cmake llvm docker
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install cmake clang-15 docker.io
```

### Build & Test

```bash
# Automated build and test
./build_and_test.sh debug macos  # or linux

# Manual build
mkdir -p build && cd build
cmake --preset Apple-Debug ..
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure --verbose
```

### Test Suite

- **100+ Tests** - Comprehensive test coverage
- **5 Categories** - Validation, Connection, Endpoint, Integration, Comprehensive
- **Unified Setup** - Single shared database for all tests
- **No Emulator Required** - Validation tests run without Azure Cosmos Emulator

#### Windows Docker Tests

Docker-based tests are **disabled on Windows** in the CI/CD pipeline. This is because the Azure Cosmos DB Emulator Docker image is only available for Linux containers, and Windows agents cannot run Linux containers without additional virtualization overhead. Windows builds focus on compilation and packaging validation. For full integration testing, use macOS or Linux environments.

For detailed testing information, see [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md#running-tests).

---

## 📖 API Usage

### Basic Operations

```cpp
// Create document
auto resp = cc.createDocument({
    .database   = "db",
    .collection = "coll",
    .document   = {{"id", "1"}, {"__pk", "pk1"}}
});

// Find document
auto resp = cc.findDocument({
    .database     = "db",
    .collection   = "coll",
    .id           = "1",
    .partitionKey = "pk1"
});

// Update document
auto resp = cc.updateDocument({
    .database     = "db",
    .collection   = "coll",
    .id           = "1",
    .partitionKey = "pk1",
    .document     = updatedDoc
});

// Delete document
auto resp = cc.removeDocument({
    .database     = "db",
    .collection   = "coll",
    .id           = "1",
    .partitionKey = "pk1"
});

// Query documents
auto resp = cc.queryDocuments({
    .database       = "db",
    .collection     = "coll",
    .partitionKey   = "*",
    .queryStatement = "SELECT * FROM c WHERE c.status = @status",
    .queryParameters = {{{"name", "@status"}, {"value", "active"}}}
});
```

### Async Operations

```cpp
cc.async({
    .operation  = siddiqsoft::CosmosOperation::create,
    .database   = "db",
    .collection = "coll",
    .document   = {{"id", "1"}, {"__pk", "pk1"}},
    .onResponse = [](auto const& ctx, auto const& resp) {
        if (resp.statusCode == 201) {
            std::cout << "Document created!" << std::endl;
        }
    }
});
```

For complete API documentation, see [https://siddiqsoft.github.io/CosmosClient/](https://siddiqsoft.github.io/CosmosClient/).

---

## 🏗️ Architecture

### Design Principles

- **Simplicity First** - Minimal abstractions, maximum clarity
- **JSON Everywhere** - Configuration, I/O, and results use JSON
- **Modern C++** - Leverage C++23 features
- **Zero-Cost** - Only pay for what you use
- **Functional** - Focus on solving problems, not performance

### Project Structure

```
CosmosClient/
├── include/
│   └── siddiqsoft/
│       └── cosmoscl.hpp          # Main header
├── tests/
│   ├── testall.cpp               # Unified test suite (100+ tests)
│   ├── test_common.hpp           # Test utilities
│   └── CMakeLists.txt            # Test configuration
├── CMakeLists.txt                # Build configuration
├── CMakePresets.json             # CMake presets
└── [Documentation files]         # See below
```

---

## 📚 Documentation Files

### Getting Started
- **[START_HERE.md](START_HERE.md)** - Main entry point
- **[QUICK_START.md](QUICK_START.md)** - 5-minute quick start
- **[DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md)** - Comprehensive guide
- **[DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)** - Navigation hub

### Development
- **[SCRIPTS_README.md](SCRIPTS_README.md)** - Automated scripts
- **[build_and_test.sh](build_and_test.sh)** - Build script

### Project Information
- **[TEST_CONSOLIDATION_SUMMARY.md](TEST_CONSOLIDATION_SUMMARY.md)** - Test consolidation
- **[PIPELINE_CONSOLIDATION_UPDATE.md](PIPELINE_CONSOLIDATION_UPDATE.md)** - CI/CD updates
- **[COMPLETE_CONSOLIDATION_SUMMARY.md](COMPLETE_CONSOLIDATION_SUMMARY.md)** - Complete overview

---

## 🔄 Development Workflow

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
# Make changes
# Rebuild
cmake --build build/Apple-Debug -j$(nproc)

# Run specific tests
ctest --output-on-failure -R "YourTestName"
```

For detailed instructions, see [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md).

---

## 🐛 Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| CMake not found | `brew install cmake` (macOS) or `sudo apt-get install cmake` (Linux) |
| Compiler not found | `brew install llvm` (macOS) or `sudo apt-get install clang-15` (Linux) |
| Emulator not reachable | Check: `docker ps`, `docker logs cosmos-emulator` |
| Tests timeout | Increase timeout: `ctest --timeout 1800` |
| Docker permission denied | `sudo usermod -aG docker $USER && newgrp docker` |

For more troubleshooting, see [DEVELOPER_BUILD_GUIDE.md#troubleshooting](DEVELOPER_BUILD_GUIDE.md#troubleshooting).

---

## 🗺️ Roadmap

- [x] Documentation
- [x] Unified test suite (100+ tests)
- [x] Cross-platform support (Windows, macOS, Linux)
- [ ] Async operations with auto-retry
- [ ] OpenSSL for non-Windows platforms

---

## 📊 Project Statistics

- **100+ Tests** - Comprehensive test coverage
- **5 Test Categories** - Validation, Connection, Endpoint, Integration, Comprehensive
- **Single Database** - Unified test setup
- **Cross-Platform** - Windows, macOS, Linux
- **Modern C++** - C++23 features

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests: `ctest --output-on-failure --verbose`
5. Submit a pull request

For development guidelines, see [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md).

---

## 📄 License

This project is licensed under the BSD 3-Clause License. See [LICENSE](LICENSE) file for details.

---

## 📞 Support

- **Documentation**: See [START_HERE.md](START_HERE.md)
- **API Reference**: [https://siddiqsoft.github.io/CosmosClient/](https://siddiqsoft.github.io/CosmosClient/)
- **Issues**: [GitHub Issues](https://github.com/SiddiqSoft/CosmosClient/issues)
- **Discussions**: [GitHub Discussions](https://github.com/SiddiqSoft/CosmosClient/discussions)

---

## 🎯 Quick Navigation

**I want to...**

- **Build and test RIGHT NOW** → [QUICK_START.md](QUICK_START.md)
- **Understand the complete process** → [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md)
- **Use automated scripts** → [SCRIPTS_README.md](SCRIPTS_README.md)
- **Find specific documentation** → [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)
- **Understand the API** → [API Documentation](https://siddiqsoft.github.io/CosmosClient/)

---

<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>
