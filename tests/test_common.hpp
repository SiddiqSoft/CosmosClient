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

#include "siddiqsoft/ScopeTrace.hpp"

/*
 * Connection resolution order:
 *   1. Environment variables CCTEST_PRIMARY_CS / CCTEST_SECONDARY_CS  (live Azure Cosmos DB)
 *   2. Local Cosmos DB emulator at http://127.0.0.1:8081/             (Docker/Podman)
 *
 * The tests require either a live Azure Cosmos DB instance or the Docker/Podman emulator.
 * The emulator must be started before running tests (use setup-emulator.sh).
 */

static const std::string EMULATOR_CONNECTION_STRING =
        "AccountEndpoint=http://localhost:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;";
static const std::string        EMULATOR_KEY      = "C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==";
static const std::string        EMULATOR_ENDPOINT = "localhost:8081";

static const int                SEED_DOCUMENT_COUNT {10};
static std::string              testDBName          = "cosmoscl_test_DB";
static std::string              testDBName0         = "cosmoscl_DB_0";
static std::string              testDBName1         = "cosmoscl_DB_1";
static std::vector<std::string> testCollectionNames = {"cosmoscl_test_COLL_0", "cosmoscl_test_COLL_1", "cosmoscl_test_COLL_2"};
static std::string              testDocName0        = "cosmoscl_test_Doc0_";

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
    siddiqsoft::ScopeTrace st;

    auto                   pcs = std::getenv("CCTEST_PRIMARY_CS");
    auto                   scs = std::getenv("CCTEST_SECONDARY_CS");

    if (pcs) {
        // User provided explicit connection strings
        // std::print(std::cerr, "GetConnectionStrings: Using environment settings...primary={}  and secondary={}", pcs, scs);
        return std::make_pair(std::string(pcs), scs ? std::string(scs) : std::string(pcs));
    }

    // No env vars set — use emulator connection string
    st.msg("GetConnectionStrings: Using environment settings...primary={}  and secondary={}", EMULATOR_CONNECTION_STRING, EMULATOR_CONNECTION_STRING);
    return std::make_pair(EMULATOR_CONNECTION_STRING, EMULATOR_CONNECTION_STRING);
}

/// @brief Quick connectivity probe: attempts discoverRegions on a throwaway client.
/// @return true when Cosmos DB (emulator or cloud) is reachable.
static bool IsCosmosReachable()
{
    siddiqsoft::ScopeTrace st(__func__);
    static std::optional<bool> cached;
    if (cached.has_value()) return *cached;

    st.msg("IsCosmosReachable: Attempting to connect to Cosmos DB (emulator or cloud)...");

    // Try to connect with retries
    for (int attempt = 0; attempt < 5; ++attempt) {
        siddiqsoft::CosmosClient probe;
        probe.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetConnectionStrings()}});
        auto rc = probe.discoverRegions();
        st.msg("IsCosmosReachable: discoverRegions attempt {} returned status code: {}", attempt + 1, rc.statusCode);

        if (rc.statusCode == 200) {
            st.msg("IsCosmosReachable: Successfully connected to Cosmos DB");
            cached = true;
            return true;
        }

        if (attempt < 4) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    st.err("IsCosmosReachable: Failed to connect to Cosmos DB after 5 attempts");
    cached = false;
    return false;
}

static auto CheckEmulatorLivenessProbe() -> bool
{
    siddiqsoft::ScopeTrace st(__func__);
    auto [primaryCS, secondaryCS] = GetConnectionStrings();
    if (primaryCS == EMULATOR_CONNECTION_STRING) {
        // Emulator connection string is being used, check if emulator is reachable
        auto wrc = siddiqsoft::GetRESTClient();
    }
    return true; // Not using emulator, assume reachable
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
