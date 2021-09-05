/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#ifndef AZURE_COSMOS_RESTCL_HPP
#define AZURE_COSMOS_RESTCL_HPP


#include <string>
#include <iostream>
#include <functional>
#include <format>

/// @brief The nlohmann.json provides for the primary interface for this class. While this may not be the most efficient, it is the
/// cleanest to use as a client.
#include "nlohmann/json.hpp"

/// @brief Provides for the Conversion utilities and the Cosmos token generation functionality
#include "siddiqsoft/azure-cpp-utils.hpp"

/// @brief Provides the all important Rest Client using WinHTTP
#include "siddiqsoft/restcl_winhttp.hpp"


namespace siddiqsoft
{
#pragma region CosmosEndpoint
    /// @brief Cosmos Connection String as available in the Azure Portal
    struct CosmosEndpoint
    {
        /// @brief The "home" or base Uri points to the home location.
        ///	For globally partitioned/availability zones, we use this to build the readable/writable
        std::basic_string<char> BaseUri {};

        /// @brief The Base64 encoded key
        std::basic_string<char> EncodedKey {};

        /// @brief The "binary" key decoded from the EncodedKey stored as std::string
        std::string Key {};

        /// @brief Read Locations for the region
        std::vector<std::basic_string<char>> ReadableUris {};

        /// @brief Current Read location within the ReadableUris
        size_t CurrentReadUriId {};

        /// @brief Write Locations for the region
        std::vector<std::basic_string<char>> WritableUris {};

        /// @brief Current Write Location within the WritableUris
        size_t CurrentWriteUriId {};

        /// @brief Default constructor
        CosmosEndpoint() = default;

        /// @brief Construct the connection string object from the Connection String obtained from the Azure Portal
        /// @param s Connection String obtained from the Azure Portal
        CosmosEndpoint(const std::basic_string<char>& s)
        {
            this->operator=(s);
        }

        /// @brief Operator assignment
        /// @param s Source string obtained from the Azure portal
        /// @return Self
        CosmosEndpoint& operator=(const std::basic_string<char>& s)
        {
            std::basic_string<char> MatchAccountEndpoint = "AccountEndpoint=";
            std::basic_string<char> MatchAccountKey      = ";AccountKey=";

            // The Azure Cosmos Connection string has the following format
            // AccountEndpoint=BaseUri;AccountKey=Key
            // The BaseUri is fully qualified name with port
            // The Key is the base64 encoded string representing the Cosmos key
            if (s.starts_with(MatchAccountEndpoint)) {
                if (auto posAccountKey = s.find(MatchAccountKey); posAccountKey != std::string::npos) {
                    // We have enough to extract both the uri and the key
                    BaseUri    = s.substr(MatchAccountEndpoint.length(), posAccountKey - MatchAccountEndpoint.length());
                    EncodedKey = s.substr(posAccountKey + MatchAccountKey.length(),
                                          s.length() - (posAccountKey + MatchAccountKey.length()));
                    // Make sure to strip off the trailing ; if present
                    if (EncodedKey.ends_with(";")) EncodedKey.resize(EncodedKey.length() - 1);
                    // Store the decoded key (only for std::string)
                    Key = Base64Utils::decode(EncodedKey);
                }
            }

            return *this;
        }


        /// @brief Checks if the BaseUri and the EncodedKey is non-empty
        operator bool() const
        {
            return !BaseUri.empty() && !EncodedKey.empty();
        }


        /// @brief Cast operator for string
        operator std::basic_string<char>() const
        {
            return string();
        }


        /// @brief Encodes the contents back into the original connection string from Azure Portal
        /// @return The rebuilt connection string must match that of the original connection string from the Azure Portal
        const std::basic_string<char> string() const
        {
            return std::format("AccountEndpoint={};AccountKey={};", BaseUri, EncodedKey);
        }


        /// @brief Current read endpoint
        /// @return Current read endpoint or the base Uri
        const auto& currentReadUri() const
        {
            if (!ReadableUris.empty() && CurrentReadUriId < ReadableUris.size()) return ReadableUris.at(CurrentReadUriId);
            return BaseUri;
        }

        /// @brief Current write endpoint
        /// @return Current write endpoint or the base Uri
        const auto& currentWriteUri() const
        {
            if (!WritableUris.empty() && CurrentWriteUriId < WritableUris.size()) return WritableUris.at(CurrentWriteUriId);
            return BaseUri;
        }

        /// @brief Increment the read endpoint to the next one in the list and if we reach the end, go back to start
        /// @return Self
        CosmosEndpoint& rotateReadUri()
        {
            if (ReadableUris.empty()) // If empty; use the baseUri
                CurrentReadUriId = 0;
            else if (ReadableUris.size() == 1) // Do not increment if there is only one item
                CurrentReadUriId = 0;
            else if (ReadableUris.size() >= 2) {
                // We have atleast two items
                // Increment..and..Go back to the top if we reach past the last element.
                if (++CurrentReadUriId >= ReadableUris.size()) CurrentReadUriId = 0;
            }

            return *this;
        }

        /// @brief Increment the write endpoint to the next one in the list and if we reach the end, go back to start
        /// @return Self
        CosmosEndpoint& rotateWriteUri()
        {
            if (WritableUris.empty()) // If empty, use the baseUri
                CurrentWriteUriId = 0;
            else if (WritableUris.size() == 1) // Do not increment if there is only one item
                CurrentWriteUriId = 0;
            else if (WritableUris.size() >= 2) {
                // We have atleast two items
                // Increment..and..Go back to the top if we reach past the last element.
                if (++CurrentWriteUriId >= WritableUris.size()) CurrentWriteUriId = 0;
            }

            return *this;
        }
    };


    /// @brief JSON serializer
    /// @tparam CharT We currently only support char (as the underlying json library does not support wchar_t)
    /// @param dest Destination json object
    /// @param src The source cosmos connection string
    static void to_json(nlohmann::json& dest, const CosmosEndpoint& src)
    {
        dest["baseUri"]           = src.BaseUri;
        dest["readUris"]          = src.ReadableUris;
        dest["currentReadUriId"]  = src.CurrentReadUriId;
        dest["writeUris"]         = src.WritableUris;
        dest["currentWriteUriId"] = src.CurrentWriteUriId;
        dest["key"]               = src.EncodedKey;
    }
#pragma endregion


#pragma region CosmosConnection
    /// @brief Represents the Cosmos cnxn
    struct CosmosConnection
    {
        /// @brief CurrentConnectionIdType
        enum class CurrentConnectionIdType : uint16_t
        {
            PrimaryConnection   = 1,
            SecondaryConnection = 2
        };

        /// @brief Serializer for the internal CurrentConnectionIdType
        NLOHMANN_JSON_SERIALIZE_ENUM(CurrentConnectionIdType,
                                     {{CurrentConnectionIdType::PrimaryConnection, "PrimaryConnection"},
                                      {CurrentConnectionIdType::SecondaryConnection, "SecondaryConnection"}});

        /// @brief Current Connection: 0=Not Set 1=Primary 2=Secondary
        CurrentConnectionIdType CurrentConnectionId {CurrentConnectionIdType::PrimaryConnection};

        /// @brief The Primary connection string from the Azure Portal
        CosmosEndpoint Primary {};

        /// @brief The Secondary connection string from the Azure Portal
        CosmosEndpoint Secondary {};

        /// @brief Default constructor
        CosmosConnection() = default;

        /// @brief Constructor with Primary and optional Secondary.
        /// @param p Primary Connection String from Azure portal
        /// @param s Secondary Connection String from Azure portal
        CosmosConnection(const std::basic_string<char>& p, const std::basic_string<char>& s = {})
        {
            configure({{"connectionStrings", {p, s}}});
        }

        /// @brief Configure the Primary and Secondary. Also resets the current connection to the "Primary"
        /// @param config JSON object with the following elements: `connectionStrings` array and `partitionKeyNames` array
        /// @return Self
        /// @remarks Resets the CurrentConnectionId to the Primary(1).
        CosmosConnection& configure(const nlohmann::json& config)
        {
            if (config.contains("connectionStrings") && config.at("connectionStrings").is_array()) {
                Primary   = config.value("/connectionStrings/0"_json_pointer, "");
                Secondary = config.value("/connectionStrings/1"_json_pointer, "");

                if (!Primary) throw std::invalid_argument("Primary must be present");
            }

            // If we have readLocations then load them up for the current connection
            if (config.contains("readableLocations")) {
                for (auto& item : config.at("readableLocations")) {
                    if (CurrentConnectionId == CurrentConnectionIdType::SecondaryConnection)
                        Secondary.ReadableUris.push_back(item.value("databaseAccountEndpoint", ""));
                    else
                        Primary.ReadableUris.push_back(item.value("databaseAccountEndpoint", ""));
                }
            }

            // If we have writeLocations then load them up for the current connection
            if (config.contains("writableLocations")) {
                for (auto& item : config.at("writableLocations")) {
                    if (CurrentConnectionId == CurrentConnectionIdType::SecondaryConnection)
                        Secondary.WritableUris.push_back(item.value("databaseAccountEndpoint", ""));
                    else
                        Primary.WritableUris.push_back(item.value("databaseAccountEndpoint", ""));
                }
            }

            return *this;
        }


        /// @brief Get the current active connection string
        /// @return Cosmos connection string
        /// @return Reference to the current active Connection Primary/Secondary
        const CosmosEndpoint& current() const
        {
            return (CurrentConnectionId == CurrentConnectionIdType::SecondaryConnection) ? std::ref(Secondary) : std::ref(Primary);
        }


        /// @brief Swaps the current connection by incrementing the current and if we hit past Secondary, we restart at Primary.
        /// @param c Maybe 0=Swap 1=Use Primary 2=Use Secondary
        /// @return Self
        CosmosConnection& rotate(const uint16_t c = 0)
        {
            if (c == 0) {
                // Swap between Primary and Secondary
                if (CurrentConnectionId == CurrentConnectionIdType::PrimaryConnection)
                    CurrentConnectionId = CurrentConnectionIdType::SecondaryConnection;

                else if (CurrentConnectionId == CurrentConnectionIdType::SecondaryConnection)
                    CurrentConnectionId = CurrentConnectionIdType::PrimaryConnection;
            }
            else if (c == 1) {
                CurrentConnectionId = CurrentConnectionIdType::PrimaryConnection;
            }
            else if (c == 2) {
                CurrentConnectionId = CurrentConnectionIdType::SecondaryConnection;
            }

            // If Secondary is empty; limit to Primary
            if ((CurrentConnectionId == CurrentConnectionIdType::SecondaryConnection) && Secondary.EncodedKey.empty())
                CurrentConnectionId = CurrentConnectionIdType::PrimaryConnection;

            return *this;
        }
    };

    /// @brief JSON serializer for the CosmosConnection object
    /// @param dest Output json object
    /// @param src The source CosmosConnection
    static void to_json(nlohmann::json& dest, const CosmosConnection& src)
    {
        dest["currentConnectionId"] = src.CurrentConnectionId;
        dest["primary"]             = src.Primary;
        dest["secondary"]           = src.Secondary;
        dest["currentConnection"]   = const_cast<CosmosConnection&>(src).current();
    }
#pragma endregion


#pragma region CosmosClient
    /// @brief The CosmosResponseType contains status code and the json content returned by the server.
    /// - `uint32_t` - Status Code from the server
    /// - `nlohmann::json` - Json contents from the server
    struct CosmosResponseType
    {
        /// @brief Status Code from the server
        uint32_t statusCode;

        /// @brief Document from the server
        nlohmann::json document;

        /// @brief Serializer to/from json
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(CosmosResponseType, statusCode, document);
    };


    /// @brief The CosmosIterableResponseType inherits from the CosmosResponseType and includes the continuation token
    /// - `std::string` - Continuation Token.
    ///
    /// @remarks The functions `CosmosClient::find`, `CosmosClient::query` and `CosmosClient::listDocuments` use this return type
    /// allowing the client to iterate through all of the entries.
    ///
    /// *Sample logic for continuation*
    /// ```cpp
    /// siddiqsoft::CosmosIterableResponseType irt {};
    /// // This is your destination container; note that the first element is null in this example
    /// nlohmann::json                         allDocs = nlohmann::json::array();
    /// // This is the running count of the total
    /// uint32_t                               allDocsCount {};
    /// // First, we query for all items that match our criteria (source=__func__)
    /// do {
    ///     irt = cc.query(dbName,
    ///                    collectionName,
    ///                    "*",
    ///                    "SELECT * FROM c WHERE contains(c.source, @v1)",
    ///                    {{{"name", "@v1"}, {"value", std::format("{}-", getpid())}}},
    ///                    irt.continuationToken);
    ///     if (200 == irt.statusCode &&
    ///         irt.document.contains("Documents") &&
    ///         !irt.document.at("Documents").is_null())
    ///     {
    ///         // Append to the current container
    ///         allDocs.insert(allDocs.end(),                     // dest
    ///                        irt.document["Documents"].begin(), // from
    ///                        irt.document["Documents"].end());
    ///         allDocsCount += irt.document.value("_count", 0);
    ///     }
    /// } while (!irt.continuationToken.empty());
    /// ```
    struct CosmosIterableResponseType : CosmosResponseType
    {
        /// @brief Continuation token from the server
        std::string continuationToken;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(CosmosIterableResponseType, statusCode, document, continuationToken);
    };


    /// @brief Cosmos Client
    /// Implements a stateful Cosmos Client using Cosmos SQL-API via REST API
    ///
    /// @details The client is thread-safe in the operations `create`, `find`, `upsert`, `query` and `remove`. The client should
    /// maintain a single instance as the underlying restcl uses connection pooling so you do not end up with needless sockets
    /// connecting to Azure.
    /// However, care must be taken if you wish to handle `401` and `rotate` the connection as the underlying re-discovery is
    /// currently not thread-safe.
    ///
    /// @see Documentation on the REST API from MS
    /// https://docs.microsoft.com/en-us/rest/api/cosmos-db/common-tasks-using-the-cosmosdb-rest-api
    /// @see Samples are here: https://docs.microsoft.com/en-us/azure/cosmos-db/sql-api-cpp-get-started
    /// @see Documentation is here: https://docs.microsoft.com/en-us/rest/api/documentdb/documentdb-resource-uri-syntax-for-rest
    class CosmosClient
    {
#if defined(COSMOSCLIENT_TESTING_MODE)
    public:
#else
    protected:
#endif
        /// @brief configuration object updated/merged with the client object
        /// This information is set by the call to `configure` and const accessed via `configuration`.
        /// @warning Guard against writers.
        nlohmann::json config {
                {"_typever", CosmosClientUserAgentString},
                {"apiVersion", "2018-12-31"}, // The API version for Cosmos REST API
                {"connectionStrings", {}},    // The Connection String from the Azure portal
                {"partitionKeyNames", {}}     // The partition key names is an array of partition key names
        };

        /// @brief Service Settings saved from discoverRegion
        /// This data is used within the method `configure` and used thereafter for informative purposes.
        /// @warning Guard against writers.
        nlohmann::json serviceSettings;

        /// @brief Used to signal first-time configuration
        std::atomic_bool isConfigured {false};

        /// @brief The REST client is initialized with the user agent
        /// @details This can be shared across multiple threads as the only method is `send` and they share minimal state
        /// information across threads.
        WinHttpRESTClient restClient {CosmosClientUserAgentString};

        /// @brief The connection object stores the Primary, Secondary connection strings as well as the read/write locations for
        /// the given Azure location.
        CosmosConnection cnxn {};

    public:
        /// @brief This is the string used in the User-Agent header
        inline static const std::string CosmosClientUserAgentString {"SiddiqSoft.CosmosClient/0.1.0"};

        /// @brief Default constructor
        CosmosClient() { }

#pragma region Disable Move constructors and move assignment operators
        CosmosClient(const CosmosClient&&) = delete;
        CosmosClient(const CosmosClient&)  = delete;
        auto operator=(const CosmosClient&&) = delete;
        auto operator=(const CosmosClient&) = delete;
#pragma endregion

        /// @brief Gets the current configuration object
        /// @return Configuration json
        const nlohmann::json& configuration()
        {
            return config;
        }


        /// @brief Re-configure the client. This will invoke the discoverRegions to populate the available regions and sets the
        /// primary write/read locations. If the source is empty (no arguments) then the current configuration is returned.
        /// Avoid repeated invocations!
        /// This method may throw if the underling call to `discoverRegions` fails.
        /// @param src A valid json object must comply with the defaults. If empty, the current configuration is returned without
        /// any changes.
        /// Must have the following elements:
        /// `{"connectionStrings": ["primary-connection-string"], "primaryKeyNames": ["field-name-of-partition-id"]}`
        /// @return Self
        CosmosClient& configure(const nlohmann::json& src = {}) noexcept(false)
        {
            if (!src.empty()) {
                // The minimum is that the ConnectionStrings exist with at least one element; a string from the Azure portal with
                // the Primary Connection String.
                if (!src.contains("connectionStrings")) throw std::invalid_argument("connectionStrings missing");
                // The current implementation requires that the Azure Cosmos service be configured with
                // partition keys--most non-trivial implementations use Geo-spatial deployments and partition keys
                // are required during setup.
                if (!src.contains("partitionKeyNames")) throw std::invalid_argument("partitionKeyNames missing");

                // Update our local configuration
                config.update(src);

                // We continue configuring..
                // After the update we must ensure that the requirements are still met: ConnectionStrings array
                if (!config["connectionStrings"].is_array()) throw std::invalid_argument("connectionStrings must be array");
                if (config["connectionStrings"].size() < 1)
                    throw std::invalid_argument("connectionStrings array must contain atleast primary element");

                // Update the database configuration
                cnxn.configure(config);

                // Discover the regions..
                auto [rc, regionInfo] = discoverRegions();
                if (rc == 200 && !regionInfo.empty()) {
                    serviceSettings = regionInfo;
                    // Reconfigure/update the information such as the read location
                    cnxn.configure(serviceSettings);
                    // Mark as updated
                    isConfigured = true;
                }
            }

            return *this;
        }


        /// @brief Discover the Regions for the current base Uri
        /// This method is invoked by the `configuration` method.
        /// @return Tuple of the status code and the json response (or empty)
        CosmosResponseType discoverRegions()
        {
            CosmosResponseType ret {0xFA17, {}};
            auto               ts = DateUtils::RFC7231();

            restClient.send(ReqGet(cnxn.current().currentReadUri(),
                                   {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "", "", ts)},
                                    {"x-ms-date", ts},
                                    {"x-ms-version", config["apiVersion"]}}),
                            [&ret](const auto& req, const auto& resp) {
                                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                            });
            return ret;
        }


        /// @brief List all databases for the given service
        /// Refer to https://docs.microsoft.com/en-us/rest/api/documentdb/documentdb-resource-uri-syntax-for-rest
        /// @return The json document response or error code
        CosmosResponseType listDatabases()
        {
            CosmosResponseType ret {0xFA17, {}};

            // We need to add the same value to the header in the field x-ms-date as well as the Authorization field
            auto ts   = DateUtils::RFC7231();
            auto path = cnxn.current().currentReadUri() + "dbs";

            restClient.send(ReqGet(path,
                                   {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "dbs", "", ts)},
                                    {"x-ms-date", ts},
                                    {"x-ms-version", config["apiVersion"]}}),
                            [&ret](const auto& req, const auto& resp) {
                                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                            });

            return ret;
        }

        /// @brief List the collections for the given database
        /// @param dbName Database name
        /// @return The json document from Cosmos contains the collections for the given
        CosmosResponseType listCollections(const std::string& dbName)
        {
            CosmosResponseType ret {0xFA17, {}};

            // We need to add the same value to the header in the field x-ms-date as well as the Authorization field
            auto ts   = DateUtils::RFC7231();
            auto path = std::format("{}dbs/{}/colls", cnxn.current().currentReadUri(), dbName);
            auto auth = EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "colls", {"dbs/" + dbName}, ts);

            restClient.send(ReqGet(path, {{"Authorization", auth}, {"x-ms-date", ts}, {"x-ms-version", config["apiVersion"]}}),
                            [&ret](const auto& req, const auto& resp) {
                                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                            });

            return ret;
        }

        /// @brief List documents for the given database and collection
        /// @param dbName The database name
        /// @param collName The collectio nname
        /// @param [in] continuationToken Optional continuation token. This is used with the value `x-ms-continuation`.
        /// @return CosmosIterableResponseType contains the status code and if succesfull the contents and optionally the
        /// continuation token.
        /// @remarks The call returns 100 items and you must invoke the method again with the continuation token to fetch the next
        /// 100 items. It is not wise to use this method as it is expensive. Use the [find](find) method or the more flexible
        /// [query] method.
        ///
        /// *Sample logic for continuation*
        /// ```cpp
        /// siddiqsoft::CosmosIterableResponseType irt {};
        /// // This is your destination container; note that the first element is null in this example
        /// nlohmann::json                         allDocs = nlohmann::json::array();
        /// // This is the running count of the total
        /// uint32_t                               allDocsCount {};
        /// // First, we query for all items that match our criteria (source=__func__)
        /// do {
        ///     irt = cc.query(dbName,
        ///                    collectionName,
        ///                    "*",
        ///                    "SELECT * FROM c WHERE contains(c.source, @v1)",
        ///                    {{{"name", "@v1"}, {"value", std::format("{}-", getpid())}}},
        ///                    irt.continuationToken);
        ///     if (200 == irt.statusCode &&
        ///         irt.document.contains("Documents") &&
        ///         !irt.document.at("Documents").is_null())
        ///     {
        ///         // Append to the current container
        ///         allDocs.insert(allDocs.end(),                     // dest
        ///                        irt.document["Documents"].begin(), // from
        ///                        irt.document["Documents"].end());
        ///         allDocsCount += irt.document.value("_count", 0);
        ///     }
        /// } while (!irt.continuationToken.empty());
        /// ```
        CosmosIterableResponseType
        listDocuments(const std::string& dbName, const std::string& collName, const std::string& continuationToken)
        {
            CosmosIterableResponseType ret {0xFA17, {}, {}};
            auto                       ts = DateUtils::RFC7231();
            auto           path = std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentReadUri(), dbName, collName);
            nlohmann::json headers {
                    {"Authorization",
                     EncryptionUtils::CosmosToken<char>(
                             cnxn.current().Key, "GET", "docs", std::format("dbs/{}/colls/{}", dbName, collName), ts)},
                    {"x-ms-date", ts},
                    {"x-ms-version", config["apiVersion"]}};

            if (!continuationToken.empty()) headers["x-ms-continuation"] = continuationToken;

            restClient.send(ReqGet(path, headers), [&ret, &continuationToken](const auto& req, const auto& resp) {
                ret = {std::get<0>(resp.status()),
                       resp.success() ? std::move(resp["content"]) : nlohmann::json {},
                       resp["headers"].value("x-ms-continuation", "")};
            });

            return ret;
        }


        /// @brief Create an entity in documentdb using the json object as the payload.
        /// @param dbName Database name
        /// @param collName Collection name
        /// @param doc The document must include the `id` and the partition key.
        /// @return status code and the created document as returned by Cosmos
        /// @see Example over at https://docs.microsoft.com/en-us/rest/api/documentdb/create-a-document
        CosmosResponseType create(const std::string& dbName, const std::string& collName, const nlohmann::json& doc)
        {
            if (doc.value("id", "").empty()) throw std::invalid_argument("create - I need the uniqueid of the document");
            if (doc.value(config.at("/partitionKeyNames/0"_json_pointer), "").empty())
                throw std::invalid_argument("create - I need the partitionId of the document");

            CosmosResponseType ret {0xFA17, {}};
            auto               ts   = DateUtils::RFC7231();
            auto               pkId = doc.value(config.at("/partitionKeyNames/0"_json_pointer), "");

            restClient.send(
                    siddiqsoft::ReqPost {
                            std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), dbName, collName),
                            {{"Authorization",
                              EncryptionUtils::CosmosToken<char>(
                                      cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", dbName, collName), ts)},
                             {"x-ms-date", ts},
                             {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                             {"x-ms-version", config["apiVersion"]},
                             {"x-ms-cosmos-allow-tentative-writes", "true"}},
                            doc},
                    [&ret](const auto& req, const auto& resp) {
                        ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                    });

            return ret;
        }


        /// @brief Insert or Update existing document for the given id
        /// @param dbName Database name
        /// @param collName Collection name
        /// @param doc The document must include the `id` and the partition key.
        /// @return status code and the created document as returned by Cosmos
        /// The status code is `201` when the document has been created
        /// The status code `200` represents a document that has been updated.
        CosmosResponseType upsert(const std::string& dbName, const std::string& collName, const nlohmann::json& doc)
        {
            if (doc.value("id", "").empty()) throw std::invalid_argument("upsert - I need the uniqueid of the document");
            if (doc.value(config.at("/partitionKeyNames/0"_json_pointer), "").empty())
                throw std::invalid_argument("upsert - I need the partitionId of the document");

            CosmosResponseType ret {0xFA17, {}};
            auto               ts   = DateUtils::RFC7231();
            auto               pkId = doc.value(config.at("/partitionKeyNames/0"_json_pointer), "");

            restClient.send(
                    siddiqsoft::ReqPost {
                            std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), dbName, collName),
                            {{"Authorization",
                              EncryptionUtils::CosmosToken<char>(
                                      cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", dbName, collName), ts)},
                             {"x-ms-date", ts},
                             {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                             {"x-ms-documentdb-is-upsert", "true"},
                             {"x-ms-version", config["apiVersion"]},
                             {"x-ms-cosmos-allow-tentative-writes", "true"}},
                            doc},
                    [&ret](const auto& req, const auto& resp) {
                        ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                    });

            return ret;
        }

        /// @brief Update existing document
        /// @param dbName Database name
        /// @param collName Collection name
        /// @param docId Unique document id
        /// @param pkId Partition key
        /// @return Status code and Json document for the docId
        CosmosResponseType update(const std::string&    dbName,
                                  const std::string&    collName,
                                  const std::string&    docId,
                                  const std::string&    pkId,
                                  const nlohmann::json& doc)
        {
            CosmosResponseType ret {0xFA17, {}};
            auto               ts = DateUtils::RFC7231();

            if (docId.empty()) throw std::invalid_argument("update - I need the docId of the document");
            if (pkId.empty()) throw std::invalid_argument("update - I need the pkId of the document");
            if (doc.is_null() || doc.size() == 0) throw std::invalid_argument("update - Need the document");

            restClient.send(
                    siddiqsoft::ReqPut {
                            std::format("{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), dbName, collName, docId),
                            {{"Authorization",
                              EncryptionUtils::CosmosToken<char>(cnxn.current().Key,
                                                                 "PUT",
                                                                 "docs",
                                                                 std::format("dbs/{}/colls/{}/docs/{}", dbName, collName, docId),
                                                                 ts)},
                             {"x-ms-date", ts},
                             {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                             {"x-ms-version", config["apiVersion"]},
                             {"x-ms-cosmos-allow-tentative-writes", "true"}},
                            doc},
                    [&ret](const auto& req, const auto& resp) {
                        ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                    });

            return ret;
        }


        /// @brief Removes the document given the docId and pkId
        /// https://docs.microsoft.com/en-us/rest/api/documentdb/delete-a-document
        /// @param dbName Cosmos Database name
        /// @param collName Cosmos Collection Name
        /// @param docId Unique document Id
        /// @param pkId The partition Id
        /// @return Status code with empty json
        /// @remarks The remove operation returns no data beyond the status code.
        uint32_t remove(const std::string& dbName, const std::string& collName, const std::string& docId, const std::string& pkId)
        {
            uint32_t ret {0xFA17};
            auto     ts = DateUtils::RFC7231();

            if (docId.empty()) throw std::invalid_argument("remove - I need the docId of the document");
            if (pkId.empty()) throw std::invalid_argument("remove - I need the pkId of the document");

            restClient.send(
                    siddiqsoft::ReqDelete {
                            std::format("{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), dbName, collName, docId),
                            {{"Authorization",
                              EncryptionUtils::CosmosToken<char>(cnxn.current().Key,
                                                                 "DELETE",
                                                                 "docs",
                                                                 std::format("dbs/{}/colls/{}/docs/{}", dbName, collName, docId),
                                                                 ts)},
                             {"x-ms-date", ts},
                             {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                             {"x-ms-version", config["apiVersion"]},
                             {"x-ms-cosmos-allow-tentative-writes", "true"}}},
                    [&ret](const auto& req, const auto& resp) { ret = std::get<0>(resp.status()); });

            return ret;
        }


        /// @brief Performs a query and continues an existing query if the continuation token exists
        /// CAUTION: If your query yields dozens or hundreds or thousands of documents, this method
        /// currently does not offer "streaming" or callbacks.
        /// It continues until the server reports no-more-continuation and returns the documents
        /// in a single response json.
        /// Memory and timing can be massive!
        /// @param dbName Database name
        /// @param collName Collection name
        /// @param pkId Primary key id: may be `*`, or the partition id value
        /// @param queryStatement The SQL API query string.
        /// @param params Optional json array with name-value objects.
        /// @param continuationToken Optional continuation token. This is used with the value `x-ms-continuation`
        /// to page through the query response.
        /// @return CosmosIterableResponseType @see CosmosIterableResponseType for details on the sample multi-query.
        /// @see https://docs.microsoft.com/en-us/rest/api/cosmos-db/q
        /// @see https://docs.microsoft.com/en-us/azure/cosmos-db/sql/sql-query-getting-started
        ///
        /// @remarks
        /// The response is paged so you will need to combine the results into
        /// delivering to the user a single json with array of `Documents` object and `_count`.
        ///
        /// *Output Sample*
        /// ```json
        /// {
        ///    "Documents": [
        ///       {
        ///          "__pk": "odd.siddiqsoft.com",
        ///          "_attachments": "attachments/",
        ///          "_etag": "\"7e0141d3-0000-0400-0000-612ec6f10000\"",
        ///          "_rid": "RP0wAM6H+R7-8VEAAAAAAQ==",
        ///          "_self": "dbs/RP0wAA==/colls/RP0wAM6H+R4=/docs/RP0wAM6H+R7-8VEAAAAAAQ==/",
        ///          "_ts": 1630455537,
        ///          "i": 1,
        ///          "id": "azure-cosmos-restcl.16304555368341392",
        ///          "odd": true,
        ///          "source": "23676-SiddiqSoft.CosmosClient/0.1.0",
        ///          "ttl": 360
        ///       },
        /// 	  ...
        /// 	  ...
        ///    ],
        ///    "_count": 2
        /// }
        /// ```
        CosmosIterableResponseType query(const std::string&    dbName,
                                         const std::string&    collName,
                                         const std::string&    pkId,
                                         const std::string&    queryStatement,
                                         const nlohmann::json& params            = {},
                                         const std::string&    continuationToken = {})
        {
            CosmosIterableResponseType ret {0xFA17, {}, {}};
            auto                       ts    = DateUtils::RFC7231();
            auto                       count = 0;
            nlohmann::json             headers {
                    {"Authorization",
                     EncryptionUtils::CosmosToken<char>(
                             cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", dbName, collName), ts)},
                    {"x-ms-date", ts},
                    {"x-ms-max-item-count", -1}, // -1: Let Cosmos figure out item count
                    {"x-ms-documentdb-isquery", "true"},
                    {"x-ms-version", config["apiVersion"]},
                    {"Content-Type", "application/query+json"}};

            if (queryStatement.empty()) throw std::invalid_argument("Missing queryStatement");

            if (pkId.starts_with("*")) {
                // Special case query with partitioned data set.
                headers["x-ms-documentdb-query-enablecrosspartition"] = "true";
                // This is required if the client does not provide partitionkey
                headers["x-ms-query-enable-crosspartition"] = "true";
            }
            else if (!pkId.empty()) {
                // Specific partition set by client.
                headers["x-ms-documentdb-partitionkey"] = nlohmann::json {pkId};
            }

            if (!continuationToken.empty()) {
                headers["x-ms-continuation"] = continuationToken;
            }

            restClient.send(ReqPost {std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), dbName, collName),
                                     headers,
                                     !params.is_null() && params.is_array()
                                             ? nlohmann::json {{"query", queryStatement}, {"parameters", params}}
                                             : nlohmann::json {{"query", queryStatement}}},
                            [&ret](const auto& req, const auto& resp) {
                                ret = {std::get<0>(resp.status()),                                      // status code
                                       resp.success() ? std::move(resp["content"]) : nlohmann::json {}, // document or empty json
                                       resp["headers"].value("x-ms-continuation", "")}; //  continuation token or empty
                            });
            return ret;
        }


        /// @brief Find a given document by id
        /// @param dbName The database name
        /// @param collName The collection name
        /// @param docId The unique id
        /// @param pkId Partition id
        /// @return CosmosResponseType - StatusCode and a single JSON document corresponding to the docId
        ///
        /// *sample document*
        /// ```json
        /// {
        ///   "__pk": "siddiqsoft.com",
        ///   "_attachments": "attachments/",
        ///   "_etag": "\"0c0124ff-0000-0400-0000-612ecb520000\"",
        ///   "_rid": "RP0wAM6H+R6g1FIAAAAADQ==",
        ///   "_self": "dbs/RP0wAA==/colls/RP0wAM6H+R4=/docs/RP0wAM6H+R6g1FIAAAAADQ==/",
        ///   "_ts": 1630456658,
        ///   "id": "azure-cosmos-restcl.16304566579088218",
        ///   "source": "basic_tests.exe",
        ///   "ttl": 360
        /// }
        /// ```
        /// @remarks
        /// We do not modify or abstract the contents.
        CosmosResponseType
        find(const std::string& dbName, const std::string& collName, const std::string& docId, const std::string& pkId)
        {
            CosmosResponseType ret {0xFA17, {}};
            auto               ts = DateUtils::RFC7231();

            if (docId.empty()) throw std::invalid_argument("find - I need the docId of the document");
            if (pkId.empty()) throw std::invalid_argument("find - I need the pkId of the document");

            restClient.send(
                    siddiqsoft::ReqGet {
                            std::format("{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), dbName, collName, docId),
                            {{"Authorization",
                              EncryptionUtils::CosmosToken<char>(cnxn.current().Key,
                                                                 "GET",
                                                                 "docs",
                                                                 std::format("dbs/{}/colls/{}/docs/{}", dbName, collName, docId),
                                                                 ts)},
                             {"x-ms-date", ts},
                             {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                             {"x-ms-version", config["apiVersion"]},
                             {"x-ms-cosmos-allow-tentative-writes", "true"}}},
                    [&ret](const auto& req, const auto& resp) {
                        ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                    });

            return ret;
        }


        /// @brief JSON serializer helper for CosmosClient
        /// @param dest Output json object
        /// @param src Reference to a CosmosClient instance
        friend void to_json(nlohmann::json& dest, const CosmosClient& src);
    };

    /// @brief JSON serializer helper for CosmosClient
    /// @param dest Output json object
    /// @param src Reference to a CosmosClient instance
    static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient& src)
    {
        dest["serviceSettings"] = src.serviceSettings;
        dest["database"]        = src.cnxn;
        dest["configuration"]   = src.config;
    }
#pragma endregion
} // namespace siddiqsoft


#pragma region Serializer CosmosEndpoint
/// @brief Serializer for CosmosEndpoint for the char type
/// @tparam CharT Either char or wchar_t
template <>
struct std::formatter<siddiqsoft::CosmosEndpoint> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosEndpoint& s, FC& ctx)
    {
        auto str = std::format("AccountEndpoint={};AccountKey={};", s.BaseUri, s.EncodedKey);
        return std::formatter<std::basic_string<char>>::format(str, ctx);
    }
};

/// @brief Ostream implementation for CosmosEndpoint object
/// @param os Detination ostream object
/// @param s Reference to CosmosEndpoint object
/// @return ostream object
static std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosEndpoint& s)
{
    os << std::basic_string<char>(s);
    return os;
}

#pragma endregion


#pragma region Serializer CosmosConnection
/// @brief Serializer for CosmosConnection for the char type
/// @tparam CharT Either char or wchar_t
template <>
struct std::formatter<siddiqsoft::CosmosConnection> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosConnection& s, FC& ctx)
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};

/// @brief Output stream write operator
/// @tparam CharT char or wchar_t
/// @param os The output stream
/// @param s The CosmosConnection object
/// @return The output stream
static std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosConnection& s)
{
    os << std::format("{}", s);
    return os;
}

#pragma endregion


#pragma region Serializer CosmosClient
/// @brief Serializer for CosmosClient for the char type
/// @tparam CharT Either char or wchar_t
template <>
struct std::formatter<siddiqsoft::CosmosClient> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosClient& s, FC& ctx)
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};

/// @brief Ostream writer for CosmosClient
/// Basically we dump the json object to the stream
/// @param os The output stream
/// @param s Reference to source CosmosClient instance
/// @return The output stream
static std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosClient& s)
{
    os << nlohmann::json(s).dump();
    return os;
}
#pragma endregion



/// @brief Serializer for the CosmosIterableResponseType
template <>
struct std::formatter<siddiqsoft::CosmosIterableResponseType> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosIterableResponseType& s, FC& ctx)
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};


/// @brief Serializer for the CosmosResponseType
template <>
struct std::formatter<siddiqsoft::CosmosResponseType> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosResponseType& s, FC& ctx)
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};

#endif // !AZURE_COSMOS_RESTCL_HPP
