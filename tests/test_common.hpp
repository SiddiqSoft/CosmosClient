#pragma once

#ifndef cosmoscl_test_common_hpp
#define cosmoscl_test_common_hpp

/*
 * Required Environment Variables
 * CCTEST_PRIMARY_CS
 *
 * Optional Environment Variables
 * CCTEST_SECONDARY_CS
 *
 * Locally install podman running the Azure Cosmos Emulator
 * https://learn.microsoft.com/en-us/azure/cosmos-db/how-to-develop-emulator?tabs=podman-linux%2Ccsharp&pivots=api-nosql
 *
 * Runs on MacOS (Apple Silicon)
 * https://learn.microsoft.com/en-us/azure/cosmos-db/emulator-linux
 */
static const std::string        EMULATOR_CONNECTION_STRING = "AccountEndpoint=http://localhost:8081/;AccountKey=C2y6yDjf5/"
                                                             "R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;";
static const std::string        EMULATOR_KEY               = "C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==";
static const std::string        EMULATOR_ENDPOINT          = "localhost:8081";
static const uint               SEED_DOCUMENT_COUNT {10};
static std::string              testDBName          = std::format("cosmoscl_test_DB{}", __COUNTER__);
static std::string              testDBName0         = std::format("cosmoscl_DB_0");
static std::string              testDBName1         = std::format("cosmoscl_DB_1");
static std::vector<std::string> testCollectionNames = {std::format("cosmoscl_test_COLL{}", __COUNTER__),
                                                       std::format("cosmoscl_test_COLL{}", __COUNTER__),
                                                       std::format("cosmoscl_test_COLL{}", __COUNTER__)};
static std::string              testDocName0        = std::format("cosmoscl_test_Doc0_{}-", __COUNTER__);

///
// Helpers
///
#pragma region Test Suite Helpers
static siddiqsoft::CosmosClient testSuiteClient;

/**
 * @brief Get the Connection Strings object
 *
 * @return std::pair<std::string, std::string>
 */
static auto GetConnectionStrings() -> std::pair<std::string, std::string>
{
    auto pcs = std::getenv("CCTEST_PRIMARY_CS");
    auto scs = std::getenv("CCTEST_SECONDARY_CS");

    return std::make_pair(pcs ? std::string(pcs) : EMULATOR_CONNECTION_STRING, scs ? std::string(scs) : EMULATOR_CONNECTION_STRING);
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