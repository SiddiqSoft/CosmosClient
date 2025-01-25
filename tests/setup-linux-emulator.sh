podman pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
podman run \
    --publish 8081:8081 \
    --publish 10250-10255:10250-10255 \
    --name linux-emulator \
    --detach \
    mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest