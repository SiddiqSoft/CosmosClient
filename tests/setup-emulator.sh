docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331
docker run \
        --publish 8081:8081 \
        --publish 10250-10255:10250-10255 \
        --name=linux-emulator \
        --rm \
        --network=host \
        --detach \
        mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331
