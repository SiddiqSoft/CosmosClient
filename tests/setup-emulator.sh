#!/bin/bash
set -e

# Detect container runtime (prefer docker, fallback to podman)
if command -v docker &> /dev/null; then
    CONTAINER_RUNTIME="docker"
elif command -v podman &> /dev/null; then
    CONTAINER_RUNTIME="podman"
else
    echo "Error: Neither docker nor podman is installed"
    exit 1
fi

echo "Using container runtime: $CONTAINER_RUNTIME"

echo "Stopping any existing linux-emulator container..."
$CONTAINER_RUNTIME container stop linux-emulator 2>/dev/null || true
echo "Removing any existing linux-emulator container..."
$CONTAINER_RUNTIME container rm linux-emulator 2>/dev/null || true

echo "Pulling latest Azure Cosmos DB Emulator image..."
echo "The version vnext-EN20260331 support arm64"
$CONTAINER_RUNTIME pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331

echo "Starting Azure Cosmos DB Emulator..."
$CONTAINER_RUNTIME run \
        --publish 8081:8081 \
        --publish 10250-10255:10250-10255 \
        --name=linux-emulator \
        --rm \
        --detach \
        mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331

echo "Waiting for emulator to be ready..."
sleep 10

echo "Checking container status..."
$CONTAINER_RUNTIME ps | grep linux-emulator || echo "Warning: Container may not be running"

sleep 15
if curl -fsI https://localhost:8081 > /dev/null; then
  echo "Azure Cosmos DB Emulator is ready on https://localhost:8081"
else
  echo "Azure Cosmos DB Emulator is NOT ready"
fi