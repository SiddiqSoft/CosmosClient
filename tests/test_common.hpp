#pragma once

#ifndef cosmoscl_test_common_hpp
#define cosmoscl_test_common_hpp

#include <optional>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <string>
#include <vector>
#include <format>
#include <chrono>
#include <thread>
#include <filesystem>
#include <print>

// POSIX headers for fork/exec/pipe
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>

/*
 * Connection resolution order:
 *   1. Environment variables CCTEST_PRIMARY_CS / CCTEST_SECONDARY_CS  (live Azure Cosmos DB)
 *   2. Local Cosmos DB emulator at http://127.0.0.1:8081/             (Docker)
 *   3. Mock Cosmos server (Python) at http://127.0.0.1:18081/         (auto-started)
 *
 * The mock server is a lightweight Python HTTP server that simulates the Cosmos DB REST API
 * with in-memory storage. It is started automatically when no other backend is available.
 */

static const std::string EMULATOR_CONNECTION_STRING = "AccountEndpoint=http://127.0.0.1:8081/;AccountKey=C2y6yDjf5/"
                                                      "R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;";
static const std::string EMULATOR_KEY               = "C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==";
static const std::string EMULATOR_ENDPOINT          = "localhost:8081";

/// @brief Connection string for the mock Cosmos server (uses a dummy base64 key)
static const std::string MOCK_CONNECTION_STRING = "AccountEndpoint=http://127.0.0.1:18081/;AccountKey=C2y6yDjf5/"
                                                  "R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;";

static const int                SEED_DOCUMENT_COUNT {10};
static std::string              testDBName          = std::format("cosmoscl_test_DB{}", __COUNTER__);
static std::string              testDBName0         = std::format("cosmoscl_DB_0");
static std::string              testDBName1         = std::format("cosmoscl_DB_1");
static std::vector<std::string> testCollectionNames = {std::format("cosmoscl_test_COLL{}", __COUNTER__),
                                                       std::format("cosmoscl_test_COLL{}", __COUNTER__),
                                                       std::format("cosmoscl_test_COLL{}", __COUNTER__)};
static std::string              testDocName0        = std::format("cosmoscl_test_Doc0_{}-", __COUNTER__);


#pragma region Mock Cosmos Server Management

/// @brief Manages the lifecycle of the mock Cosmos DB Python server process.
/// The server is started lazily on first use and killed when the process exits.
class MockCosmosServer
{
    pid_t  serverPid_ {0};
    bool   running_ {false};
    int    port_ {18081};

public:
    static MockCosmosServer& instance()
    {
        static MockCosmosServer inst;
        return inst;
    }

    ~MockCosmosServer() { stop(); }

    bool isRunning() const { return running_; }
    int  port() const { return port_; }

    /// @brief Start the mock server if not already running.
    /// @return true if the server is running after this call.
    bool start()
    {
        if (running_) return true;

        // Find the mock server script relative to the test executable
        // Try several candidate paths
        std::vector<std::string> candidates = {
                "../../tests/mock_cosmos_server.py",       // from build/Apple-Debug/
                "../tests/mock_cosmos_server.py",          // from build/
                "tests/mock_cosmos_server.py",             // from project root
                "mock_cosmos_server.py",                   // from tests/
        };

        // Also try using __FILE__ path
        {
            std::filesystem::path thisFile(__FILE__);
            auto                  dir = thisFile.parent_path();
            candidates.insert(candidates.begin(), (dir / "mock_cosmos_server.py").string());
        }

        std::string scriptPath;
        for (auto& c : candidates) {
            if (std::filesystem::exists(c)) {
                scriptPath = c;
                break;
            }
        }

        if (scriptPath.empty()) {
            std::print(std::cerr, "MockCosmosServer: Cannot find mock_cosmos_server.py\n");
            return false;
        }

        // Create a pipe to read the "READY" signal from the child
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            std::print(std::cerr, "MockCosmosServer: pipe() failed\n");
            return false;
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::print(std::cerr, "MockCosmosServer: fork() failed\n");
            close(pipefd[0]);
            close(pipefd[1]);
            return false;
        }

        if (pid == 0) {
            // Child process
            close(pipefd[0]); // Close read end
            // Redirect stdout to pipe write end
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            // Redirect stderr to /dev/null
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            // Exec python3
            execlp("python3", "python3", scriptPath.c_str(), std::to_string(port_).c_str(), nullptr);
            // If exec fails
            _exit(127);
        }

        // Parent process
        close(pipefd[1]); // Close write end
        serverPid_ = pid;

        // Wait for "READY" from the child (with timeout)
        char    buf[128] = {};
        fd_set  readfds;
        timeval tv;
        tv.tv_sec  = 5;
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(pipefd[0], &readfds);

        int sel = select(pipefd[0] + 1, &readfds, nullptr, nullptr, &tv);
        if (sel > 0) {
            ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n]  = '\0';
                running_ = (std::string(buf).find("READY") != std::string::npos);
            }
        }
        close(pipefd[0]);

        if (running_) {
            std::print(std::cerr, "MockCosmosServer: Started on port {} (pid {})\n", port_, serverPid_);
        }
        else {
            std::print(std::cerr, "MockCosmosServer: Failed to start (killing pid {})\n", serverPid_);
            kill(serverPid_, SIGTERM);
            serverPid_ = 0;
        }

        return running_;
    }

    void stop()
    {
        if (serverPid_ > 0) {
            kill(serverPid_, SIGTERM);
            // Reap the child
            int status = 0;
            waitpid(serverPid_, &status, WNOHANG);
            std::print(std::cerr, "MockCosmosServer: Stopped (pid {})\n", serverPid_);
            serverPid_ = 0;
            running_   = false;
        }
    }

private:
    MockCosmosServer() = default;
    MockCosmosServer(const MockCosmosServer&)            = delete;
    MockCosmosServer& operator=(const MockCosmosServer&) = delete;
};

#pragma endregion


///
// Helpers
///
#pragma region Test Suite Helpers
static siddiqsoft::CosmosClient testSuiteClient;

/**
 * @brief Get the Connection Strings object.
 *
 * Resolution order:
 *   1. CCTEST_PRIMARY_CS / CCTEST_SECONDARY_CS environment variables
 *   2. Emulator connection string (if emulator is reachable)
 *   3. Mock server connection string (auto-started)
 *
 * @return std::pair<std::string, std::string>  primary, secondary connection strings
 */
static auto GetConnectionStrings() -> std::pair<std::string, std::string>
{
    auto pcs = std::getenv("CCTEST_PRIMARY_CS");
    auto scs = std::getenv("CCTEST_SECONDARY_CS");

    if (pcs) {
        // User provided explicit connection strings
        return std::make_pair(std::string(pcs), scs ? std::string(scs) : std::string(pcs));
    }

    // No env vars set — try emulator first, then fall back to mock
    // We'll check connectivity in IsCosmosReachable; for now return emulator strings
    // and let the mock server be started if needed.
    return std::make_pair(EMULATOR_CONNECTION_STRING, EMULATOR_CONNECTION_STRING);
}

/// @brief Returns connection strings pointing to the mock server.
static auto GetMockConnectionStrings() -> std::pair<std::string, std::string>
{
    return std::make_pair(MOCK_CONNECTION_STRING, MOCK_CONNECTION_STRING);
}

/// @brief Quick connectivity probe: attempts discoverRegions on a throwaway client.
/// If the real Cosmos service (emulator or cloud) is not reachable, automatically
/// starts the mock server and reconfigures connection strings.
/// @return true when some Cosmos-compatible service is reachable.
static bool IsCosmosReachable()
{
    static std::optional<bool> cached;
    if (cached.has_value()) return *cached;

    // First, try the real connection strings
    {
        siddiqsoft::CosmosClient probe;
        probe.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetConnectionStrings()}});
        auto rc = probe.discoverRegions();
        if (rc.statusCode == 200) {
            cached = true;
            return true;
        }
    }

    // Real service not reachable — start mock server
    std::print(std::cerr, "IsCosmosReachable: Real Cosmos not reachable, starting mock server...\n");
    if (MockCosmosServer::instance().start()) {
        // Verify mock is responding
        siddiqsoft::CosmosClient probe;
        probe.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetMockConnectionStrings()}});
        auto rc = probe.discoverRegions();
        if (rc.statusCode == 200) {
            cached = true;
            return true;
        }
        std::print(std::cerr, "IsCosmosReachable: Mock server started but discoverRegions failed (status={})\n", rc.statusCode);
    }

    cached = false;
    return false;
}

/// @brief Returns the active connection strings — either real or mock.
/// Must be called after IsCosmosReachable() to ensure the mock is started if needed.
static auto GetActiveConnectionStrings() -> std::pair<std::string, std::string>
{
    if (MockCosmosServer::instance().isRunning()) {
        return GetMockConnectionStrings();
    }
    return GetConnectionStrings();
}


static auto TScreateDatabase(const std::string& dbName)
{
    return testSuiteClient.createDatabase({.database = dbName});
}

static auto TSfindDatabase(const std::string& dbName)
{
    return testSuiteClient.findDatabase({.database = dbName});
}

static auto TSdeleteDatabase(const std::string& dbName)
{
    return testSuiteClient.deleteDatabase({.database = dbName});
}

static auto TScreateCollection(const std::string& dbName, const std::string& collName)
{
    return testSuiteClient.createCollection({.database = dbName, .collection = collName});
}

static auto TScreateDocument(const std::string& dbName, const std::string& collName, const std::string& docName)
{
    return testSuiteClient.createDocument(
            {.database = dbName, .collection = collName, .document = {{"id", docName}, {"__pk", "siddiqsoft.com"}, {"source", __func__}}});
}

#pragma endregion

#endif
