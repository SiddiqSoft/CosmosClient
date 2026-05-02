# CosmosClient - Quick Start Guide

## TL;DR - Get Started in 5 Minutes

### macOS

```bash
# 1. Install tools
brew install cmake llvm docker

# 2. Clone and setup
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive

# 3. Start emulator
docker run --publish 8081:8081 --publish 10250-10255:10250-10255 \
  --name cosmos-emulator --rm --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
sleep 60

# 4. Build
mkdir -p build && cd build
cmake --preset Apple-Debug ..
cmake --build .

# 5. Test
ctest --output-on-failure --verbose

# 6. Cleanup
docker stop cosmos-emulator
```

### Linux

```bash
# 1. Install tools
sudo apt-get update && sudo apt-get install -y cmake clang-15 docker.io
sudo usermod -aG docker $USER && newgrp docker

# 2. Clone and setup
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient
git submodule update --init --recursive

# 3. Start emulator
docker run --publish 8081:8081 --publish 10250-10255:10250-10255 \
  --name cosmos-emulator --rm --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
sleep 60

# 4. Build
mkdir -p build && cd build
cmake --preset Linux-Clang-Debug ..
cmake --build .

# 5. Test
ctest --output-on-failure --verbose

# 6. Cleanup
docker stop cosmos-emulator
```

---

## Common Commands

### Build

```bash
# Debug build
cmake --build build/Apple-Debug

# Release build
cmake --build build/Apple-Release

# Parallel build (faster)
cmake --build build/Apple-Debug -j$(nproc)
```

### Test

```bash
# All tests
ctest --output-on-failure --verbose

# Specific category
ctest --output-on-failure -R "Validation"

# Single test
ctest --output-on-failure -R "CreateDocument"
```

### Emulator

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

## Test Categories

- **Validation** - Configuration and setup tests (no emulator)
- **CosmosConnection** - Connection management tests (no emulator)
- **CosmosEndpoint** - Endpoint URI tests (no emulator)
- **CosmosIntegrationTests** - Full integration tests (requires emulator)
- **Comprehensive** - Comprehensive API tests (requires emulator)

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| CMake not found | `brew install cmake` (macOS) or `sudo apt-get install cmake` (Linux) |
| Compiler not found | `brew install llvm` (macOS) or `sudo apt-get install clang-15` (Linux) |
| Emulator not reachable | Check: `docker ps`, `docker logs cosmos-emulator`, `curl -k https://localhost:8081/` |
| Tests timeout | Increase timeout: `ctest --timeout 1800` |
| Docker permission denied | `sudo usermod -aG docker $USER && newgrp docker` |

---

## Full Documentation

See [DEVELOPER_BUILD_GUIDE.md](DEVELOPER_BUILD_GUIDE.md) for detailed instructions.

