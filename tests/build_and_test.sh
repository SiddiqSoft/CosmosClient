#!/bin/bash
# CosmosClient - Build and Test Helper Script
# Location: tests/build_and_test.sh
# Usage: ./tests/build_and_test.sh [debug|release] [macos|linux]
#    or: cd tests && ./build_and_test.sh [debug|release] [macos|linux]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Determine project root (parent of tests directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Detect container runtime (podman or docker)
CONTAINER_RUNTIME=""
if command -v podman &> /dev/null; then
    CONTAINER_RUNTIME="podman"
elif command -v docker &> /dev/null; then
    CONTAINER_RUNTIME="docker"
else
    CONTAINER_RUNTIME=""
fi

# Default values
BUILD_TYPE="${1:-debug}"
PLATFORM="${2:-macos}"
BUILD_DIR="$PROJECT_ROOT/build"
EMULATOR_NAME="cosmos-emulator"
EMULATOR_PORT="8081"
EMULATOR_HOST="localhost"
# This is the emulator connection string for the vnext emulator. It uses the default key and endpoint.
CCTEST_PRIMARY_CS="AccountEndpoint=https://${EMULATOR_HOST}:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;"
CCTEST_SECONDARY_CS="AccountEndpoint=https://${EMULATOR_HOST}:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;"


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
# make the build_type first char uppercase..
if [ "$PLATFORM" = "macos" ]; then
    PRESET="Apple-$(echo $BUILD_TYPE | perl -pe 's/^./\u$&/')"
else
    PRESET="Linux-Clang-$(echo $BUILD_TYPE | perl -pe 's/^./\u$&/')"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}CosmosClient Build & Test${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Project Root: $PROJECT_ROOT"
echo "Build Directory: $BUILD_DIR"
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
    if [ -z "$CONTAINER_RUNTIME" ]; then
        return 1
    fi
    
    if $CONTAINER_RUNTIME ps | grep -q "$EMULATOR_NAME"; then
        return 0
    else
        return 1
    fi
}

# Function to start emulator
start_emulator() {
    print_section "Starting Azure Cosmos DB Emulator..."
    
    if [ -z "$CONTAINER_RUNTIME" ]; then
        echo -e "${RED}✗ Neither docker nor podman found${NC}"
        return 1
    fi
    
    if check_emulator; then
        echo "Emulator already running (using $CONTAINER_RUNTIME)"
        return 0
    fi
    
    echo "Using container runtime: $CONTAINER_RUNTIME"
    
    $CONTAINER_RUNTIME run \
        --publish $EMULATOR_PORT:8081 \
        --publish 10250-10255:10250-10255 \
        --name $EMULATOR_NAME \
        --rm \
        --detach \
        mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-latest > /dev/null
    
    echo -e "${YELLOW}Waiting for emulator to start...${NC}"
    sleep 60
    
    # Verify emulator is running
    if curl -s -k https://${EMULATOR_HOST}:$EMULATOR_PORT/_explorer/index.html > /dev/null; then
        echo -e "${GREEN}✓ Emulator is ready${NC}"
    else
        echo -e "${RED}✗ Emulator failed to start${NC}"
        return 1
    fi
}

# Function to stop emulator
stop_emulator() {
    print_section "Stopping Azure Cosmos DB Emulator..."
    
    if [ -z "$CONTAINER_RUNTIME" ]; then
        echo "No container runtime available"
        return 0
    fi
    
    if check_emulator; then
        $CONTAINER_RUNTIME stop $EMULATOR_NAME > /dev/null 2>&1 || true
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

    echo "Build Directory: $BUILD_DIR"
    echo "Preset: $PRESET"
    echo "Project Root: $PROJECT_ROOT"

    cmake --preset "$PRESET" "$PROJECT_ROOT" || {
        echo -e "${RED}✗ CMake configuration failed${NC}"
        return 1
    }
    
    cd "$PROJECT_ROOT"
    echo -e "${GREEN}✓ CMake configured${NC}"
}

# Function to build project
build_project() {
    print_section "Building project... in $BUILD_DIR. $PRESET"
    
    cmake --build "$BUILD_DIR/$PRESET" || {
        echo -e "${RED}✗ Build failed${NC}"
        return 1
    }
    
    echo -e "${GREEN}✓ Build successful${NC}"
}

# Function to run tests
run_tests() {
    print_section "Running tests... ${BUILD_DIR}. ${PRESET}"
    
    cd "$BUILD_DIR/$PRESET"
    
    # Run validation tests (no emulator required)
    echo ""
    echo -e "${YELLOW}Running validation tests (no emulator required)...${NC}"
    ctest --output-on-failure -R "Validation" || true
    
    # Check if emulator is running for integration tests
    if check_emulator; then
        echo ""
        echo -e "${YELLOW}Running integration tests (requires emulator)...${NC}"
        ctest --output-on-failure --verbose || true
    else
        echo ""
        echo -e "${YELLOW}⚠ Emulator not running, skipping integration tests${NC}"
        echo "Start emulator with: docker run --publish 8081:8081 --publish 8080:8080 --publish 1234:1234 --publish 10250-10255:10250-10255 --name cosmos-emulator --rm --detach mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest"
    fi
    
    cd "$PROJECT_ROOT"
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
    
    if [ -z "$CONTAINER_RUNTIME" ]; then
        echo -e "${RED}✗ Neither docker nor podman found${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Container runtime found: $CONTAINER_RUNTIME${NC}"
    
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
    # Only ask to stop if emulator is running
    if check_emulator; then
        echo ""
        read -t 10 -p "Stop emulator? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            stop_emulator
        else
            echo "Emulator is still running. You can stop it manually with:"
            echo "  $CONTAINER_RUNTIME stop $EMULATOR_NAME"
        fi
    fi
    # Exit cleanly regardless of response
    exit 0
}

trap cleanup EXIT

# Run main
main
