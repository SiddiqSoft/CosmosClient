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

/// @brief Add asynchrony to our library
#include "siddiqsoft/simple_pool.hpp"

/// @brief Add the json formatter
#include "siddiqsoft/formatters.hpp"

#include "siddiqsoft/TimeThis.hpp"


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
        uint32_t statusCode {};

        /// @brief Document from the server
        nlohmann::json document;

        /// @brief Represents the total time
        std::chrono::microseconds ttx {};

        /// @brief Checks if the response is successful based on the HTTP status code
        /// @return true iff the statusCode < 300
        bool success() const
        {
            return statusCode < 300;
        }
    };

    /// @brief Serializer for CosmosResponseType
    /// @param dest Destination json object
    /// @param src CosmosResponseType
    static void to_json(nlohmann::json& dest, CosmosResponseType const& src)
    {
        dest["statusCode"] = src.statusCode;
        dest["document"]   = src.document;
        dest["ttx"]        = src.ttx.count();
    }

    /// @brief Azure Cosmos Operations
    enum class CosmosOperation : uint16_t
    {
        discoverRegions = 0xA0,
        listDatabases   = 0xB1,
        listCollections = 0xB2,
        listDocuments   = 0xB3,
        create          = 0xC0,
        upsert          = 0xC1,
        update          = 0xC2,
        remove          = 0xC3,
        find            = 0xC4,
        query           = 0xE0,
        notset          = 0
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(CosmosOperation,
                                 {{CosmosOperation::discoverRegions, "discoverRegions"},
                                  {CosmosOperation::listDatabases, "listDatabases"},
                                  {CosmosOperation::listCollections, "listCollections"},
                                  {CosmosOperation::listDocuments, "listDocuments"},
                                  {CosmosOperation::create, "create"},
                                  {CosmosOperation::upsert, "upsert"},
                                  {CosmosOperation::update, "update"},
                                  {CosmosOperation::remove, "remove"},
                                  {CosmosOperation::find, "find"},
                                  {CosmosOperation::query, "query"},
                                  {CosmosOperation::notset, nullptr}});


    /// @brief Cosmos data extends the nlohmann::json and adds the callback
    /// @notes The fields may contain the following key-values
    /// operation:          "discoverRegions", "listDatabases", "listCollections", "listDocuments",
    ///                     "create", "upsert", "update", "remove", "find",
    ///                     "query"
    /// db:                 <database name>
    /// collection:         <collection name>
    /// docId:              <unique document id>
    /// partitionKey:       <parition key value>
    /// continuationToken   <present on listDocuments, query>
    /// queryString:        <query string>
    /// queryParameters     <json array query parameters>
    /// doc:                <json document contents to create,update,upsert>
    struct CosmosArgumentType
    {
        CosmosOperation operation {};
        std::string     database {};
        std::string     collection {};
        std::string     id {};
        std::string     partitionKey {};
        std::string     continuationToken {};
        std::string     queryStatement {};
        nlohmann::json  queryParameters;
        nlohmann::json  document;
        /// @brief The callback for this request. Invoked with the CosmosResponseType ref
        /// @note The callback is invoked with this argument and the response to allow propogation and context.
        std::function<void(CosmosArgumentType const&, CosmosResponseType const&)> onResponse {};

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(CosmosArgumentType,
                                       operation,
                                       database,
                                       collection,
                                       id,
                                       partitionKey,
                                       continuationToken,
                                       queryStatement,
                                       queryParameters,
                                       document);
    };


    /// @brief Alias to the callback for async operation
    /// The first parameter is the argument establishing the "context" for this response and the second
    /// parameter is the response from the requested operation.
    using CosmosAsyncCallbackType = std::function<void(CosmosArgumentType const&, CosmosResponseType const&)>;


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
    };


    /// @brief Serializer for CosmosIterableResponseType uses the serializer for CosmosResponseType
    /// @param dest Destination json object
    /// @param src CosmosIterableResponseType
    static void to_json(nlohmann::json& dest, CosmosIterableResponseType const& src)
    {
        to_json(dest, CosmosResponseType(src));
        dest["continuationToken"] = src.continuationToken;
    }


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
                {"libRetryLimit", 7},
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

        /// @brief The async worker pool
        simple_pool<CosmosArgumentType> asyncWorkers {std::bind_front(&CosmosClient::asyncDispatcher, this)};


        /// @brief The async dispatcher/driver
        /// @param req The queued request
        void asyncDispatcher(CosmosArgumentType& req)
        {
            switch (req.operation) {
                case CosmosOperation::discoverRegions: {
                    auto resp = discoverRegions();
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::listDatabases: {
                    auto resp = listDatabases();
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::listCollections: {
                    auto resp = listCollections(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::listDocuments: {
                    // This returns CosmosIterableResponseType and the client's handler is invoked for each block.
                    CosmosIterableResponseType resp = listDocuments(req);
                    if (req.onResponse) req.onResponse(req, resp);
                    if (resp.success() && !resp.continuationToken.empty()) {
                        req.continuationToken = resp.continuationToken;
#ifdef _DEBUG
                        std::cerr << std::format("....Status:{}  continueToken:{}  count:{}  ttx:{} requeue\n",
                                                 resp.statusCode,
                                                 resp.continuationToken,
                                                 resp.document.value("_count", 0),
                                                 resp.ttx);
#endif
                        // Queue the next instance.
                        // This approach allows for controlled shutdown as the thread is able to listen and handle
                        // stop requests.
                        asyncWorkers.queue(std::move(req));
                    }
                } break;

                case CosmosOperation::create: {
                    auto resp = createDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::upsert: {
                    auto resp = upsertDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::update: {
                    auto resp = updateDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::find: {
                    // Returns at most a single document or it is not found.
                    auto resp = findDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::remove: {
                    // The response is an error code so we will need to normalize into a response type for the callback.
                    auto rc = removeDocument(req);
                    if (req.onResponse) req.onResponse(req, {rc, nullptr});
                } break;

                case CosmosOperation::query: {
                    // This returns CosmosIterableResponseType and the client's handler is invoked for each block.
                    CosmosIterableResponseType resp = queryDocuments(req);
                    if (req.onResponse) req.onResponse(req, resp);
                    if (resp.success() && !resp.continuationToken.empty()) {
                        req.continuationToken = resp.continuationToken;
#ifdef _DEBUG
                        std::cerr << std::format("....Status:{}  continueToken:{}  count:{}  ttx:{} requeue\n",
                                                 resp.statusCode,
                                                 resp.continuationToken,
                                                 resp.document.value("_count", 0),
                                                 resp.ttx);
#endif
                        // Queue the next instance.
                        // This approach allows for controlled shutdown as the thread is able to listen and handle
                        // stop requests.
                        asyncWorkers.queue(std::move(req));
                    }
                } break;
            }
        }

    public:
        /// @brief This is the string used in the User-Agent header
        inline static const std::string CosmosClientUserAgentString {"SiddiqSoft.CosmosClient/0.10.0"};

        /// @brief Default constructor
        CosmosClient() { }

        /// @brief Move constructor
        /// @param src Other client instance
        CosmosClient(CosmosClient&& src) noexcept
            : config(std::move(src.config))
            , serviceSettings(std::move(src.serviceSettings))
            , restClient(std::move(src.restClient))
            , isConfigured(src.isConfigured.load())
            , cnxn(std::move(src.cnxn))
        {
        }


        auto& operator=(CosmosClient&& src) = delete;
        CosmosClient(const CosmosClient&)   = delete;
        auto& operator=(const CosmosClient&) = delete;


        /// @brief Gets the current configuration object
        /// @return Configuration json
        const nlohmann::json& configuration() const
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
                if (auto resp = discoverRegions(); resp.statusCode == 200 && !resp.document.empty()) {
                    serviceSettings = resp.document;
                    // Reconfigure/update the information such as the read location
                    cnxn.configure(serviceSettings);
                    // Mark as updated
                    isConfigured = true;
                }
            }

            return *this;
        }


        /// @brief Invokes the requested operation from threadpool
        /// @param arg The request payload. The json must contain at least "operation". The callback is required as member
        /// .onResponse in the op argument.
        void async(CosmosArgumentType&& op) noexcept(false)
        {
            // We need to perform some basic validations otherwise we cannot expect to throw within the callback as it would be
            // inefficient to throw for such basic validations.
            switch (op.operation) {
                case CosmosOperation::listDocuments:
                    if (op.collection.empty()) throw std::invalid_argument("op.collection required");
                case CosmosOperation::listCollections:
                    if (op.database.empty()) throw std::invalid_argument("op.database required");
                    break;
                    // Create and Upsert have same validation requirements
                case CosmosOperation::create:
                case CosmosOperation::upsert:
                    if (op.database.empty()) throw std::invalid_argument("op.database required");
                    if (op.collection.empty()) throw std::invalid_argument("op.collection required");
                    if (op.document.empty()) throw std::invalid_argument("op.document required");
                    if (op.document.value("id", "").empty()) throw std::invalid_argument("op.document[id] required");
                    if (!op.document.contains(config.at("/partitionKeyNames/0"_json_pointer)))
                        throw std::invalid_argument("op.document[] must contain partition key");
                    break;
                case CosmosOperation::update:
                    if (op.database.empty()) throw std::invalid_argument("op.database required");
                    if (op.collection.empty()) throw std::invalid_argument("op.collection required");
                    if (op.id.empty()) throw std::invalid_argument("op.id required");
                    if (op.partitionKey.empty()) throw std::invalid_argument("op.partitionKey required");
                    if (op.document.empty()) throw std::invalid_argument("op.document required");
                    break;
                    // Query has same requirement as remove and find except for id so we need to split its check
                case CosmosOperation::query:
                    if (op.database.empty()) throw std::invalid_argument("op.database required");
                    if (op.collection.empty()) throw std::invalid_argument("op.collection required");
                    if (op.partitionKey.empty()) throw std::invalid_argument("op.partitionKey required");
                    if (op.queryStatement.empty()) throw std::invalid_argument("op.queryStatement required");
                    break;
                    // Remove and find have same requirements
                case CosmosOperation::remove:
                case CosmosOperation::find:
                    if (op.database.empty()) throw std::invalid_argument("op.database required");
                    if (op.collection.empty()) throw std::invalid_argument("op.collection required");
                    if (op.id.empty()) throw std::invalid_argument("op.id required");
                    if (op.partitionKey.empty()) throw std::invalid_argument("op.partitionKey required");
                    break;
            }

            // Elementary checks..
            if (op.operation == CosmosOperation::notset)
                throw std::invalid_argument(std::format("{} requires op.operation be valid: {}", __func__, op));
            if (!op.onResponse) throw std::invalid_argument("async requires op.onResponse be valid callback");

            // We can now queue the request..
            asyncWorkers.queue(std::move(op));
        }


        /// @brief Discover the Regions for the current base Uri
        /// This method is invoked by the `configuration` method.
        /// @return Tuple of the status code and the json response (or empty)
        CosmosResponseType discoverRegions()
        {
            TimeThis tt {};
            auto     ts = DateUtils::RFC7231();
            ReqGet   req {cnxn.current().currentReadUri(),
                        {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "", "", ts)},
                         {"x-ms-date", ts},
                         {"x-ms-version", config["apiVersion"]}}};

            auto     resp = restClient.send(req);
            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count())};
        }

        /*
        void discoverRegions(std::function<void(CosmosResponseType&)> callback)
        {

                auto ts = DateUtils::RFC7231();

                restClient.send(
                        ReqGet(cnxn.current().currentReadUri(),
                               {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "", "", ts)},
                                {"x-ms-date", ts},
                                {"x-ms-version", config["apiVersion"]}}),
                        [&callback](const auto& req, const auto& resp) {
                            if (resp.success()) {
                                callback({resp.status().code, resp.success() ? std::move(resp["content"]) : nlohmann::json {}});
                            }
                            else if (!resp.success()) {
                            }
                    });

                if (auto resp = restClient.send(req); !resp.success()) {
    #ifdef _DEBUG
                    std::cerr << "Response (retryCount:" << retryCount << ") from " << req.uri.authority.host << " --> "
                              << resp.status().code << std::endl;
    #endif
                    if (resp.status().code == 12029) {
    #ifdef _DEBUG
                        std::cerr << "Rotate to " << cnxn.rotate().current().string() << std::endl;
    #else
                        cnxn.rotate();
    #endif
                        --retryCount;
                    }
                    else if (resp["response"]["status"] == 401) {
    #ifdef _DEBUG
                        std::cerr << "Rotate to " << cnxn.rotate().current().string() << std::endl;
    #else
                        cnxn.rotate();
    #endif
                        --retryCount;
                    }
                    else {
                        ret        = {resp.status().code, resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                        retryCount = 0;
                    }
                }
                else {
                    // Success..
                    ret        = {resp.status().code, resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
                    retryCount = 0;
                }
            } while (retryCount > 0);
        }
        */

        /// @brief List all databases for the given service
        /// Refer to https://docs.microsoft.com/en-us/rest/api/documentdb/documentdb-resource-uri-syntax-for-rest
        /// @return The json document response or error code
        CosmosResponseType listDatabases()
        {
            TimeThis tt {};

            // We need to add the same value to the header in the field x-ms-date as well as the Authorization field
            auto ts   = DateUtils::RFC7231();
            auto path = cnxn.current().currentReadUri() + "dbs";
            auto req  = ReqGet(path,
                              {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "dbs", "", ts)},
                               {"x-ms-date", ts},
                               {"x-ms-version", config["apiVersion"]}});

            auto resp = restClient.send(req);
            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count())};
        }


        /// @brief List the collections for the given database
        /// @param dbName Database name
        /// @return The json document from Cosmos contains the collections for the given
        CosmosResponseType listCollections(CosmosArgumentType const& ctx)
        {
            TimeThis tt {};
            auto     ts   = DateUtils::RFC7231();
            auto     path = std::format("{}dbs/{}/colls", cnxn.current().currentReadUri(), ctx.database);
            auto     auth = EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "colls", {"dbs/" + ctx.database}, ts);
            auto     req  = ReqGet(path, {{"Authorization", auth}, {"x-ms-date", ts}, {"x-ms-version", config["apiVersion"]}});
            auto     resp = restClient.send(req);
            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count())};
        }


        /// @brief List documents for the given database and collection
        /// @param dbName The database name
        /// @param collName The collectio nname
        /// @param [in] continuationToken Optional continuation token. This is used with the value `x-ms-continuation`.
        /// @return CosmosIterableResponseType contains the status code and if succesfull the contents and optionally the
        /// continuation token.
        /// @remarks The call returns 100 items and you must invoke the method again with the continuation token to fetch the
        /// next 100 items. It is not wise to use this method as it is expensive. Use the [find](find) method or the more
        /// flexible [query] method.
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
        CosmosIterableResponseType listDocuments(CosmosArgumentType const& ctx)
        {
            TimeThis tt {};
            auto     ts   = DateUtils::RFC7231();
            auto     path = std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentReadUri(), ctx.database, ctx.collection);
            nlohmann::json headers {
                    {"Authorization",
                     EncryptionUtils::CosmosToken<char>(
                             cnxn.current().Key, "GET", "docs", std::format("dbs/{}/colls/{}", ctx.database, ctx.collection), ts)},
                    {"x-ms-date", ts},
                    {"x-ms-version", config["apiVersion"]}};

            if (!ctx.continuationToken.empty()) headers["x-ms-continuation"] = ctx.continuationToken;

            auto req  = ReqGet(path, headers);
            auto resp = restClient.send(req);

            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count()),
                    resp["headers"].value("x-ms-continuation", "")};
        }


        /// @brief Create an entity in documentdb using the json object as the payload.
        /// @param dbName Database name
        /// @param collName Collection name
        /// @param doc The document must include the `id` and the partition key.
        /// @return status code and the created document as returned by Cosmos
        /// @see Example over at https://docs.microsoft.com/en-us/rest/api/documentdb/create-a-document
        CosmosResponseType createDocument(CosmosArgumentType const& ctx)
        {
            TimeThis tt {};

            if (ctx.document.value("id", "").empty()) throw std::invalid_argument("create - I need the uniqueid of the document");
            if (!ctx.document.contains(config.at("/partitionKeyNames/0"_json_pointer)))
                throw std::invalid_argument("create - I need the partitionId of the document");

            auto                ts        = DateUtils::RFC7231();
            std::string&        pkKeyName = config.at("/partitionKeyNames/0"_json_pointer).get_ref<std::string&>();
            auto                pkId      = ctx.document.value(pkKeyName, "");

            siddiqsoft::ReqPost req {
                    std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), ctx.database, ctx.collection),
                    {{"Authorization",
                      EncryptionUtils::CosmosToken<char>(cnxn.current().Key,
                                                         "POST",
                                                         "docs",
                                                         std::format("dbs/{}/colls/{}", ctx.database, ctx.collection),
                                                         ts)},
                     {"x-ms-date", ts},
                     {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                     {"x-ms-version", config["apiVersion"]},
                     {"x-ms-cosmos-allow-tentative-writes", "true"}},
                    ctx.document};
            auto resp = restClient.send(req);

            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count())};
        }


        /// @brief Insert or Update existing document for the given id
        /// @param dbName Database name
        /// @param collName Collection name
        /// @param doc The document must include the `id` and the partition key.
        /// @return status code and the created document as returned by Cosmos
        /// The status code is `201` when the document has been created
        /// The status code `200` represents a document that has been updated.
        CosmosResponseType upsertDocument(CosmosArgumentType const& ctx)
        {
            TimeThis tt {};

            if (ctx.document.value("id", "").empty()) throw std::invalid_argument("upsert - I need the uniqueid of the document");
            if (!ctx.document.contains(config.at("/partitionKeyNames/0"_json_pointer)))
                throw std::invalid_argument("upsert - I need the partitionId of the document");

            auto ts   = DateUtils::RFC7231();
            auto pkId = ctx.document.value(config.at("/partitionKeyNames/0"_json_pointer).get_ref<std::string&>(), "");

            siddiqsoft::ReqPost req {
                    std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), ctx.database, ctx.collection),
                    {{"Authorization",
                      EncryptionUtils::CosmosToken<char>(cnxn.current().Key,
                                                         "POST",
                                                         "docs",
                                                         std::format("dbs/{}/colls/{}", ctx.database, ctx.collection),
                                                         ts)},
                     {"x-ms-date", ts},
                     {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                     {"x-ms-documentdb-is-upsert", "true"},
                     {"x-ms-version", config["apiVersion"]},
                     {"x-ms-cosmos-allow-tentative-writes", "true"}},
                    ctx.document};

            auto resp = restClient.send(req);
            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count())};
        }


        /// @brief Update existing document
        /// @param dbName Database name
        /// @param collName Collection name
        /// @param docId Unique document id
        /// @param pkId Partition key
        /// @return Status code and Json document for the docId
        CosmosResponseType updateDocument(CosmosArgumentType const& ctx)
        {
            TimeThis tt {};
            auto     ts = DateUtils::RFC7231();

            if (ctx.id.empty()) throw std::invalid_argument("update - I need the docId of the document");
            if (ctx.partitionKey.empty()) throw std::invalid_argument("update - I need the pkId of the document");
            if (ctx.document.is_null() || ctx.document.size() == 0) throw std::invalid_argument("update - Need the document");

            siddiqsoft::ReqPut req {
                    std::format(
                            "{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), ctx.database, ctx.collection, ctx.id),
                    {{"Authorization",
                      EncryptionUtils::CosmosToken<char>(
                              cnxn.current().Key,
                              "PUT",
                              "docs",
                              std::format("dbs/{}/colls/{}/docs/{}", ctx.database, ctx.collection, ctx.id),
                              ts)},
                     {"x-ms-date", ts},
                     {"x-ms-documentdb-partitionkey", nlohmann::json {ctx.partitionKey}},
                     {"x-ms-version", config["apiVersion"]},
                     {"x-ms-cosmos-allow-tentative-writes", "true"}},
                    ctx.document};
            auto resp = restClient.send(req);
            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count())};
        }


        /// @brief Removes the document given the docId and pkId
        /// https://docs.microsoft.com/en-us/rest/api/documentdb/delete-a-document
        /// @param dbName Cosmos Database name
        /// @param collName Cosmos Collection Name
        /// @param docId Unique document Id
        /// @param pkId The partition Id
        /// @return Status code with empty json
        /// @remarks The remove operation returns no data beyond the status code.
        uint32_t removeDocument(CosmosArgumentType const& ctx)
        {
            auto ts = DateUtils::RFC7231();

            if (ctx.id.empty()) throw std::invalid_argument("remove - I need the docId of the document");
            if (ctx.partitionKey.empty()) throw std::invalid_argument("remove - I need the pkId of the document");

            siddiqsoft::ReqDelete req {
                    std::format(
                            "{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), ctx.database, ctx.collection, ctx.id),
                    {{"Authorization",
                      EncryptionUtils::CosmosToken<char>(
                              cnxn.current().Key,
                              "DELETE",
                              "docs",
                              std::format("dbs/{}/colls/{}/docs/{}", ctx.database, ctx.collection, ctx.id),
                              ts)},
                     {"x-ms-date", ts},
                     {"x-ms-documentdb-partitionkey", nlohmann::json {ctx.partitionKey}},
                     {"x-ms-version", config["apiVersion"]},
                     {"x-ms-cosmos-allow-tentative-writes", "true"}}};
            auto resp = restClient.send(req);

            return resp.status().code;
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
        CosmosIterableResponseType queryDocuments(CosmosArgumentType const& ctx)
        {
            TimeThis       tt {};
            auto           ts    = DateUtils::RFC7231();
            auto           count = 0;
            nlohmann::json headers {
                    {"Authorization",
                     EncryptionUtils::CosmosToken<char>(
                             cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", ctx.database, ctx.collection), ts)},
                    {"x-ms-date", ts},
                    {"x-ms-max-item-count", -1}, // -1: Let Cosmos figure out item count
                    {"x-ms-documentdb-isquery", "true"},
                    {"x-ms-version", config["apiVersion"]},
                    {"Content-Type", "application/query+json"}};

            if (ctx.queryStatement.empty()) throw std::invalid_argument("Missing queryStatement");

            if (ctx.partitionKey.starts_with("*")) {
                // Special case query with partitioned data set.
                headers["x-ms-documentdb-query-enablecrosspartition"] = "true";
                // This is required if the client does not provide partitionkey
                headers["x-ms-query-enable-crosspartition"] = "true";
            }
            else if (!ctx.partitionKey.empty()) {
                // Specific partition set by client.
                headers["x-ms-documentdb-partitionkey"] = nlohmann::json {ctx.partitionKey};
            }

            if (!ctx.continuationToken.empty()) {
                headers["x-ms-continuation"] = ctx.continuationToken;
            }

            ReqPost req {std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), ctx.database, ctx.collection),
                         headers,
                         !ctx.queryParameters.is_null() && ctx.queryParameters.is_array()
                                 ? nlohmann::json {{"query", ctx.queryStatement}, {"parameters", ctx.queryParameters}}
                                 : nlohmann::json {{"query", ctx.queryStatement}}};

            auto    resp = restClient.send(req);

            return {resp.status().code,                                 // status code
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context // document or empty json
                    std::chrono::microseconds(tt.elapsed().count()),
                    resp["headers"].value("x-ms-continuation", "")}; //  continuation token or empty
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
        CosmosResponseType findDocument(CosmosArgumentType const& ctx)
        {
            TimeThis tt {};
            auto     ts = DateUtils::RFC7231();

            if (ctx.id.empty()) throw std::invalid_argument("find - I need the docId of the document");
            if (ctx.partitionKey.empty()) throw std::invalid_argument("find - I need the pkId of the document");

            siddiqsoft::ReqGet req {
                    std::format(
                            "{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), ctx.database, ctx.collection, ctx.id),
                    {{"Authorization",
                      EncryptionUtils::CosmosToken<char>(
                              cnxn.current().Key,
                              "GET",
                              "docs",
                              std::format("dbs/{}/colls/{}/docs/{}", ctx.database, ctx.collection, ctx.id),
                              ts)},
                     {"x-ms-date", ts},
                     {"x-ms-documentdb-partitionkey", nlohmann::json {ctx.partitionKey}},
                     {"x-ms-version", config["apiVersion"]},
                     {"x-ms-cosmos-allow-tentative-writes", "true"}}};

            auto resp = restClient.send(req);
            return {resp.status().code,
                    resp.success() ? std::move(resp["content"]) : resp, // return error/io context
                    std::chrono::microseconds(tt.elapsed().count())};
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
        dest["workers"]         = src.asyncWorkers;
        dest["userAgentString"] = src.CosmosClientUserAgentString;
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


/// @brief Serializer for the CosmosArgumentType
template <>
struct std::formatter<siddiqsoft::CosmosArgumentType> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosArgumentType& s, FC& ctx)
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};

#endif // !AZURE_COSMOS_RESTCL_HPP
