docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-preview
docker run \
        --publish 8081:8081 \
        --publish 1234:1234 \
        --name linux-emulator \
        --replace \
        --detach \
        mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-preview
