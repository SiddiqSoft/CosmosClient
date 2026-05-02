# Build & Test Helper Scripts

Helper scripts to simplify building and testing the CosmosClient project.

## Scripts

### `build_and_test.sh`

Automated build and test script with emulator management.

#### Location

```
CosmosClient/
└── tests/
    └── build_and_test.sh
```

#### Usage

```bash
# From project root
./tests/build_and_test.sh [debug|release] [macos|linux]

# Or from tests directory
cd tests
./build_and_test.sh [debug|release] [macos|linux]

# Examples
./tests/build_and_test.sh debug macos
./tests/build_and_test.sh release linux
./tests/build_and_test.sh debug linux

# With defaults (debug build, auto-detect platform)
./tests/build_and_test.sh
```

#### Features

- ✅ Automatic prerequisite checking
- ✅ CMake configuration
- ✅ Parallel build (uses all available cores)
- ✅ Automatic emulator startup
- ✅ Validation tests (no emulator required)
- ✅ Integration tests (with emulator)
- ✅ Colored output for easy reading
- ✅ Automatic cleanup on exit

#### Example Output

```
========================================
CosmosClient Build & Test
========================================
Project Root: /path/to/CosmosClient
Build Directory: /path/to/CosmosClient/build
Platform: macos
Build Type: debug
Preset: Apple-Debug

>>> Checking prerequisites...
✓ CMake found
✓ Docker found

>>> Configuring CMake...
✓ CMake configured

>>> Building project...
✓ Build successful

>>> Starting Azure Cosmos DB Emulator...
Waiting for emulator to start...
✓ Emulator is ready

>>> Running tests...
Running validation tests (no emulator required)...
Running integration tests (requires emulator)...
✓ Tests completed

>>> Test Results
Test results available at:
  /path/to/CosmosClient/build/Testing/TAG/*/Test.xml

========================================
Build and test completed successfully!
========================================
```

---

## Manual Build & Test

If you prefer to build and test manually, see [DEVELOPER_BUILD_GUIDE.md](../DEVELOPER_BUILD_GUIDE.md).

### Quick Manual Steps

```bash
# 1. Configure
mkdir -p build && cd build
cmake --preset Apple-Debug ..

# 2. Build
cmake --build .

# 3. Start emulator
docker run --publish 8081:8081 --publish 10250-10255:10250-10255 \
  --name cosmos-emulator --rm --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
sleep 60

# 4. Test
ctest --output-on-failure --verbose

# 5. Stop emulator
docker stop cosmos-emulator
```

---

## Troubleshooting

### Script won't run

```bash
# Make script executable
chmod +x tests/build_and_test.sh

# Run with bash explicitly
bash tests/build_and_test.sh
```

### CMake not found

```bash
# macOS
brew install cmake

# Linux
sudo apt-get install cmake
```

### Docker permission denied

```bash
# Add user to docker group
sudo usermod -aG docker $USER
newgrp docker
```

### Emulator won't start

```bash
# Check Docker is running
docker ps

# Check for existing emulator
docker ps -a | grep cosmos-emulator

# Remove old container if exists
docker rm cosmos-emulator

# Try starting again
docker run --publish 8081:8081 --publish 10250-10255:10250-10255 \
  --name cosmos-emulator --rm --detach \
  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
```

---

## Environment Variables

The scripts use these environment variables (optional):

```bash
# Build directory (default: build)
export BUILD_DIR=build

# Emulator name (default: cosmos-emulator)
export EMULATOR_NAME=cosmos-emulator

# Emulator port (default: 8081)
export EMULATOR_PORT=8081
```

---

## See Also

- [DEVELOPER_BUILD_GUIDE.md](../DEVELOPER_BUILD_GUIDE.md) - Detailed build instructions
- [QUICK_START.md](../QUICK_START.md) - Quick start guide
- [README.md](../README.md) - Project overview

