docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-preview
docker run \
    --publish 8081:8081 \
    --publish 10250-10255:10250-10255 \
    --publish 1234:1234 \
    --name linux-emulator \
    --detach \
    mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-preview
