docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
docker run  --publish 8081:8081 --publish 10250-10255:10250-10255 --name linux-emulator --detach  --rm  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331
timeout /t 30 /nobreak >nul
