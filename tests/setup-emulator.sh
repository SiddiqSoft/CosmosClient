#!/bin/bash
set -e

echo "Stopping any existing linux-emulator container..."
docker container stop linux-emulator 2>/dev/null || true
echo "Removing any existing linux-emulator container..."
docker container rm linux-emulator 2>/dev/null || true

echo "Pulling latest Azure Cosmos DB Emulator image..."
docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331

echo "Starting Azure Cosmos DB Emulator..."
docker run \
        --publish 8081:8081 \
        --publish 10250-10255:10250-10255 \
        --name=linux-emulator \
        --rm \
        --network=host \
        --detach \
        mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331

echo "Waiting for emulator to be ready..."
sleep 10

echo "Checking container status..."
docker ps | grep linux-emulator || echo "Warning: Container may not be running"

echo "Azure Cosmos DB Emulator is ready on https://localhost:8081"
