#!/bin/bash
# CosmosClient - Build and Test Helper Script
# Usage: ./build_and_test.sh [debug|release] [macos|linux]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="${1:-debug}"
PLATFORM="${2:-macos}"
BUILD_DIR="build"
EMULATOR_NAME="cosmos-emulator"
EMULATOR_PORT="8081"

# Validate inputs
if [[ ! "$BUILD_TYPE" =~ ^(debug|release)$ ]]; then
    echo -e "${RED}Error: BUILD_TYPE must be 'debug' or 'release'${NC}"
    exit 1
fi

if [[ ! "$PLATFORM" =~ ^(macos|linux)$ ]]; then
    echo -e "${RED}Error: PLATFORM must be 'macos' or 'linux'${NC}"
    exit 1
fi

# Determine preset based on platform and build type
if [ "$PLATFORM" = "macos" ]; then
    PRESET="Apple-$(echo $BUILD_TYPE | sed 's/^./\U&/')"
else
    PRESET="Linux-Clang-$(echo $BUILD_TYPE | sed 's/^./\U&/')"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}CosmosClient Build & Test${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Platform: $PLATFORM"
echo "Build Type: $BUILD_TYPE"
echo "Preset: $PRESET"
echo ""

# Function to print section headers
print_section() {
    echo -e "${YELLOW}>>> $1${NC}"
}

# Function to check if emulator is running
check_emulator() {
    if docker ps | grep -q "$EMULATOR_NAME"; then
        return 0
    else
        return 1
    fi
}

# Function to start emulator
start_emulator() {
    print_section "Starting Azure Cosmos DB Emulator..."
    
    if check_emulator; then
        echo "Emulator already running"
        return 0
    fi
    
    docker run \
        --publish $EMULATOR_PORT:8081 \
        --publish 10250-10255:10250-10255 \
        --name $EMULATOR_NAME \
        --rm \
        --detach \
        mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest > /dev/null
    
    echo "Waiting for emulator to start..."
    sleep 60
    
    # Verify emulator is running
    if curl -s -k https://localhost:$EMULATOR_PORT/_explorer/index.html > /dev/null; then
        echo -e "${GREEN}✓ Emulator is ready${NC}"
    else
        echo -e "${RED}✗ Emulator failed to start${NC}"
        return 1
    fi
}

# Function to stop emulator
stop_emulator() {
    print_section "Stopping Azure Cosmos DB Emulator..."
    
    if check_emulator; then
        docker stop $EMULATOR_NAME > /dev/null 2>&1 || true
        echo -e "${GREEN}✓ Emulator stopped${NC}"
    else
        echo "Emulator not running"
    fi
}

# Function to configure build
configure_build() {
    print_section "Configuring CMake..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake --preset "$PRESET" .. || {
        echo -e "${RED}✗ CMake configuration failed${NC}"
        return 1
    }
    
    cd ..
    echo -e "${GREEN}✓ CMake configured${NC}"
}

# Function to build project
build_project() {
    print_section "Building project..."
    
    cmake --build "$BUILD_DIR" -j$(nproc) || {
        echo -e "${RED}✗ Build failed${NC}"
        return 1
    }
    
    echo -e "${GREEN}✓ Build successful${NC}"
}

# Function to run tests
run_tests() {
    print_section "Running tests..."
    
    cd "$BUILD_DIR"
    
    # Run validation tests (no emulator required)
    echo ""
    echo "Running validation tests (no emulator required)..."
    ctest --output-on-failure -R "Validation" || true
    
    # Check if emulator is running for integration tests
    if check_emulator; then
        echo ""
        echo "Running integration tests (requires emulator)..."
        ctest --output-on-failure --verbose || true
    else
        echo ""
        echo -e "${YELLOW}⚠ Emulator not running, skipping integration tests${NC}"
        echo "Start emulator with: docker run --publish 8081:8081 --publish 10250-10255:10250-10255 --name cosmos-emulator --rm --detach mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest"
    fi
    
    cd ..
    echo -e "${GREEN}✓ Tests completed${NC}"
}

# Function to show test results
show_results() {
    print_section "Test Results"
    
    if [ -f "$BUILD_DIR/Testing/TAG/*/Test.xml" ]; then
        echo "Test results available at:"
        echo "  $BUILD_DIR/Testing/TAG/*/Test.xml"
    fi
}

# Main execution
main() {
    # Check prerequisites
    print_section "Checking prerequisites..."
    
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}✗ CMake not found${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ CMake found${NC}"
    
    if ! command -v docker &> /dev/null; then
        echo -e "${RED}✗ Docker not found${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Docker found${NC}"
    
    echo ""
    
    # Build steps
    configure_build || exit 1
    echo ""
    
    build_project || exit 1
    echo ""
    
    # Start emulator for integration tests
    start_emulator || {
        echo -e "${YELLOW}⚠ Emulator failed to start, running validation tests only${NC}"
    }
    echo ""
    
    # Run tests
    run_tests
    echo ""
    
    # Show results
    show_results
    echo ""
    
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Build and test completed successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"
}

# Trap to cleanup on exit
cleanup() {
    echo ""
    read -p "Stop emulator? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        stop_emulator
    fi
}

trap cleanup EXIT

# Run main
main
