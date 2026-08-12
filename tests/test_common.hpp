#pragma once

#ifndef cosmoscl_test_common_hpp
#define cosmoscl_test_common_hpp

#include <optional>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <format>
#include <chrono>
#include <thread>
#include <print>
#include <source_location>
#include <print>

/*
 * Connection resolution order:
 *   1. Environment variables CCTEST_PRIMARY_CS / CCTEST_SECONDARY_CS  (live Azure Cosmos DB)
 *   2. Local Cosmos DB emulator at http://127.0.0.1:8081/             (Docker/Podman)
 *
 * The tests require either a live Azure Cosmos DB instance or the Docker/Podman emulator.
 * The emulator must be started before running tests (use setup-emulator.sh).
 */

static const std::string EMULATOR_CONNECTION_STRING =
        "AccountEndpoint=http://127.0.0.1:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;";
static const std::string        EMULATOR_KEY      = "C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==";
static const std::string        EMULATOR_ENDPOINT = "localhost:8081";

static const int                SEED_DOCUMENT_COUNT {10};
static std::string              testDBName          = "cosmoscl_test_DB";
static std::string              testDBName0         = "cosmoscl_DB_0";
static std::string              testDBName1         = "cosmoscl_DB_1";
static std::vector<std::string> testCollectionNames = {"cosmoscl_test_COLL_0", "cosmoscl_test_COLL_1", "cosmoscl_test_COLL_2"};
static std::string              testDocName0        = "cosmoscl_test_Doc0_";

// Colors for output
static const std::string RED = "\033[0;31m";
static const std::string BLU = "\033[0;34m";
static const std::string GRN = "\033[0;32m";
static const std::string YLW = "\033[1;33m";
static const std::string NOC = "\033[0m"; // No Color


class LF
{
    std::source_location m_location;
    std::string          m_function {};

public:
    LF(const std::source_location& location = std::source_location::current())
        : m_location(location)
    {
        std::println(std::cerr, "{}{} - STARTED - {}", BLU, m_function.empty() ? m_location.file_name() : m_function, NOC);
    }

    LF(const std::string& fn, const std::source_location& location = std::source_location::current())
        : m_location(location)
        , m_function(fn)
    {
        std::println(std::cerr, "{}{} - STARTED -{}", YLW, m_function.empty() ? m_location.file_name() : m_function, NOC);
    }

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args)
    {
        std::println(std::cerr, "{}{} - {}{}", YLW, m_function.empty() ? m_location.file_name() : m_function, std::format(fmt, std::forward<Args>(args)...), NOC);
    }

    template <typename... Args>
    void err(std::format_string<Args...> fmt, Args&&... args)
    {
        std::println(std::cerr, "{}{} - {}{}", RED, m_function.empty() ? m_location.file_name() : m_function, std::format(fmt, std::forward<Args>(args)...), NOC);
    }

    template <typename... Args>
    void msg(std::format_string<Args...> fmt, Args&&... args)
    {
        std::println(std::cerr, "{}{} - {}{}", NOC, m_function.empty() ? m_location.file_name() : m_function, std::format(fmt, std::forward<Args>(args)...), NOC);
    }

    ~LF() { std::println(std::cerr, "{}{} - COMPLETED - {}", BLU, m_function.empty() ? m_location.file_name() : m_function, NOC); }
};

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
 *   2. Emulator connection string at http://127.0.0.1:8081/
 *
 * @return std::pair<std::string, std::string>  primary, secondary connection strings
 */
static auto GetConnectionStrings() -> std::pair<std::string, std::string>
{
    LF   lf(__func__);

    auto pcs = std::getenv("CCTEST_PRIMARY_CS");
    auto scs = std::getenv("CCTEST_SECONDARY_CS");

    if (pcs) {
        // User provided explicit connection strings
        // std::print(std::cerr, "GetConnectionStrings: Using environment settings...primary={}  and secondary={}", pcs, scs);
        return std::make_pair(std::string(pcs), scs ? std::string(scs) : std::string(pcs));
    }

    // No env vars set — use emulator connection string
    lf.msg("GetConnectionStrings: Using environment settings...primary={}  and secondary={}", EMULATOR_CONNECTION_STRING, EMULATOR_CONNECTION_STRING);
    return std::make_pair(EMULATOR_CONNECTION_STRING, EMULATOR_CONNECTION_STRING);
}

/// @brief Quick connectivity probe: attempts discoverRegions on a throwaway client.
/// @return true when Cosmos DB (emulator or cloud) is reachable.
static bool IsCosmosReachable()
{
    LF                         lf(__func__);
    static std::optional<bool> cached;
    if (cached.has_value()) return *cached;

    lf.msg("IsCosmosReachable: Attempting to connect to Cosmos DB (emulator or cloud)...");

    // Try to connect with retries
    for (int attempt = 0; attempt < 5; ++attempt) {
        siddiqsoft::CosmosClient probe;
        probe.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetConnectionStrings()}});
        auto rc = probe.discoverRegions();
        lf.msg("IsCosmosReachable: discoverRegions attempt {} returned status code: {}", attempt + 1, rc.statusCode);

        if (rc.statusCode == 200) {
            lf.msg("IsCosmosReachable: Successfully connected to Cosmos DB");
            cached = true;
            return true;
        }

        if (attempt < 4) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    lf.err("IsCosmosReachable: Failed to connect to Cosmos DB after 5 attempts");
    cached = false;
    return false;
}

/// @brief Returns the active connection strings.
static auto GetActiveConnectionStrings() -> std::pair<std::string, std::string>
{
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
