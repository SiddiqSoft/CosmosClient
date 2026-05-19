@echo off
echo "Check if the linux-emulator container is already running.."
docker ps --filter "name=linux-emulator" --filter "status=running" --quiet | findstr . >nul
if %errorlevel% equ 0 (
    echo "Docker container 'linux-emulator' is already running. Skipping start."
    exit /b 0
)
echo.
echo "Pulling the latest Azure Cosmos DB Linux Emulator image..."
docker pull mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:latest
echo.
echo "Starting the Azure Cosmos DB Linux Emulator container..."
docker run  --publish 8081:8081 --publish 10250-10255:10250-10255 --name linux-emulator --detach  --rm  mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-EN20260331
echo.
echo "Waiting for the emulator to initialize..."
timeout /t 30 /nobreak

@echo on