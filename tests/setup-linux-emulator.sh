docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
docker run \
    --publish 8081:8081 \
    --publish 10250-10255:10250-10255 \
    --name linux-emulator \
    --replace \
    --detach \
    mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest