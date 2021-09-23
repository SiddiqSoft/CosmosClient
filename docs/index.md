# Cosmos Client Library Documentation

<!-- badges -->
[![CodeQL](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.CosmosClient?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionid=15&branchName=main)
![](https://img.shields.io/github/v/tag/SiddiqSoft/CosmosClient)
![](https://img.shields.io/nuget/v/SiddiqSoft.CosmosClient)
![](https://img.shields.io/nuget/dt/SiddiqSoft.CosmosClient)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/15)
![](https://img.shields.io/github/license/siddiqsoft/CosmosClient)
<!-- end badges -->

# Getting started

- Use Visual Studio 2019 v16.11.2 or newer.
- This is Windows-only code; you can still use it for reference and to base your own improved version!
- Use the Nuget package!
- Make sure you use `c++latest` as the `<format>` is no longer in the `c++20` option pending ABI resolution.
- Read the examples! Notably [queryDocuments_thread](../tests/test.cpp) where I demonstrate how you can use threads and `<latch>` and `<barrier>`.

## Dependencies

We use [NuGet](https://nuget.org) for dependencies. Why? Simply put, it is the *easiest* source for obtaining packages. Publishing your own packages is superior to the manual and as-yet-immature [vcpkg](https://vcpkg.io/en/index.html). _They want me to [git clone](https://vcpkg.io/en/getting-started.html?platform=windows) the thing and build it first..._ _NuGet, comparatively, gives you a first-class experience and writing your own packages is a breeze! Sure, it does not work for non-Windows and I'll have to eventually tackle CMake._

Package     | Comments
-----------:|:----------
[nlohmann.json](https://github.com/nlohmann/json)<br/>![](https://img.shields.io/nuget/v/nlohmann.json)| This is one of the simplest JSON libraries for C++.<br/>We have to make choices and this is our choice: clean, simple and elegant over efficiency. Our use-case <br/>The library is quite conformant and lends itself to general purpose use since it uses `<vector>` underneath it all.<br/>We leave time and experience (plus manpower) to optimize this library for us. So long as it works and we do not have to worry about some ugly JAVA-esque or C-style interface!
[restcl](https://github.com/SiddiqSoft/restcl)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)| This is our REST client.<br/>There are *many*, *many*, *many* clients out there and I'm not quite happy with their design.<br/>This implementation focuses on the "interface" as a thin wrapper  atop <a href="https://docs.microsoft.com/en-us/windows/win32/winhttp/about-winhttp">WinHTTP library</a>.
[AzureCppUtils](https://github.com/SiddiqSoft/azure-cpp-utils)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.AzureCppUtils) | This is library with helpers for encryption, base64 encode/decode and token generation for Cosmos.

_Unless otherwise noted, use the latest. We're quite aggressive updating dependencies._

<hr/>

# API

## namespace `siddiqsoft`

A single class [CosmosClient](#class-cosmosclient) implements the functionality.

Note that since this is a header, you'll also get the contents of the `azure-cpp-utils` and `restcl` as well as supporting code.
C++ modules are a solution to this problem but the implementation in VS 2019 v16.11.2 is flaky and I could not get it working!

## struct `CosmosArgumentType`

This structure is used to specify arguments to the [`queue`]() operations and as a parameter to the callback for async completion callback.

CosmosArgumentType | Type | Description
-------------------|----------------|---------------------
**`operation`** | `std::string` | Mandatory; one of the following (corresponds to the method):<br/>`discoverRegion`, `listDatabases`, `listCollections`,<br/>`create`, `upsert`, `update`, `remove`,</br>`listDocuments`, `find`, `query`
`database` | `std::string` | Database name
`collection` | `std::string` | Collection name
`docId` | `std::string` | The unique document id. Required for operations: `find`, `update`, `remove`
`partitionKey` | `std::string` | Required for operaions: `update`, `find`, `remove`, `query`.<br/>In the case of `query`, this may be `*` to indicate cross-partition query.
`continuationToken` | `std::string` | Used when there would be more than 100 items requrned by the server for operations: `listDocuments`, `find` and `query`.<br/>If you find this field in the response then you must use iteration to fetch the rest of the documents.
`queryString` | `std::string` | The query string. May include tokens with values in the queryParameters json
`queryParameters` | `nlohmann::json` | An array of key-value arguments matching the tokens in the queryString
`doc` | `nlohmann::json` | The document to create/upsert/update
`onResponse` | function | Callback/lambda to receive the response for the given async request.<br/>Invoked with const reference to the argument structure and the response.

> Take a look at structured bindings in C++17.


### Signature

```cpp
    struct CosmosArgumentType
    {
        std::string    operation {};
        std::string    database {};
        std::string    collection {};
        std::string    docId {};
        std::string    partitionKey {};
        std::string    continuationToken {};
        std::string    queryString {};
        nlohmann::json queryParameters;
        nlohmann::json doc;

        std::function<void(CosmosArgumentType const&, CosmosResponseType const&)> onResponse {};
    };
```

## struct `CosmosResponseType`

This is the primary return type from the [CosmosClient](#struct-cosmosclient) functions.

Includes serializers for nlohmann::json via the macro `NLOHMANN_DEFINE_TYPE_INTRUSIVE`.

CosmosResponseType | Type  | Description
-------------------|----|---
`statusCode` | `uint32_t` | Holds the HTTP response Status Code or the system-error code (WinHTTP error code)
`document`   | `nlohmann::json` | Holds the response from the Cosmos response.

Azure Cosmos REST API [status codes](https://docs.microsoft.com/en-us/rest/api/cosmos-db/http-status-codes-for-cosmosdb) has the full list with detailed explanations.

statusCode | Comment
----------:|:--------------
 `200`    | Success. [`listDatabases`](#cosmosclient-listdatabases), [`listCollections`](#cosmosclient-listcollections), [`listDocuments`](#cosmosclient-listdocuments), [`find`](#cosmosclient-find), [`query`](#cosmosclient-query), [`upsert`](#cosmosclient-upsert)
 `201`    | Document created. [`create`](#cosmosclient-create), [`upsert`](#cosmosclient-upsert)
 `204`    | Document removed. [`remove`](#cosmosclient-remove)
 `401`    | Authorization failure.
 `404`    | Document Not found.
 `409`    | Document already exists. [`create`](#cosmosclient-create)


*Output Sample*

The json document typically contains two elements: `_count` and `Documents` which is an array of documents from the functions listDocuments(), listCollections(), listDocuments(), query().

```json
{
  "Documents": [
     {
        "__pk": "PARTITION-KEY-VALUE",
        "_attachments": "attachments/",
        "_etag": "\"7e0141d3-0000-0400-0000-612ec6f10000\"",
        "_rid": "RP0wAM6H+R7-8VEAAAAAAQ==",
        "_self": "dbs/RP0wAA==/colls/RP0wAM6H+R4=/docs/RP0wAM6H+R7-8VEAAAAAAQ==/",
        "_ts": 1630455537,
        "i": 1,
        "id": "UNIQUE-DOCUMENT-ID",
     },
    ...
    ...
  ],
  "_count": 2
}
```

*Output Sample*

For operations on single documents `create()`, `find()`, `update()` return the document as stored (with Cosmos injected housekeeping data).

```json
{
    "__pk": "PARTITION-KEY-VALUE",
    "_attachments": "attachments/",
    "_etag": "\"7e0141d3-0000-0400-0000-612ec6f10000\"",
    "_rid": "RP0wAM6H+R7-8VEAAAAAAQ==",
    "_self": "dbs/RP0wAA==/colls/RP0wAM6H+R4=/docs/RP0wAM6H+R7-8VEAAAAAAQ==/",
    "_ts": 1630455537,
    "i": 1,
    "id": "UNIQUE-DOCUMENT-ID",
}
```


## struct `CosmosIterableResponseType`

Extends the [CosmosResponseType](#struct-cosmosresponsetype) by adding the `continuationToken` data member
to facilitate iteration for the methods [listDocuments](#listdocuments) and [query](#query).

CosmosIterableResponseType | Type  | Description
-------------------|----|---
`continuationToken` | `string` | Do not modify; this is the `x-ms-continuation` token from the response header and inpat for the `listDocuments` and `query` methods to obtain all of the documents.


## using `CosmosAsyncCallbackType`

```cpp
    using CosmosAsyncCallbackType = std::function<void(CosmosArgumentType const& context, CosmosResponseType const& response)>;
```

 | Type  | Description
-------------------|----|---
`context` | [`CosmosArgumentType const&`](#struct-cosmosargumenttype) | Const-reference to the original request.
`response`   | [`CosmosResponseType const&`](#struct-cosmosresponsetype) | Const-reference to the response from the server.

<hr/>

## class `CosmosClient`

Implements the Cosmos SQL-API via REST. Omits the attachment API as of this version.

```cpp
    class CosmosClient
    {
    protected:
        nlohmann::json                  config;
        nlohmann::json                  serviceSettings;
        std::atomic_bool                isConfigured;
        WinHttpRESTClient               restClient;
        CosmosConnection                cnxn;
        simple_pool<CosmosArgumentType> asyncWorkers;

        void asyncDispatcher(CosmosArgumentType&);

    public:
        CosmosClient();
        CosmosClient(CosmosClient&&);

        auto& operator=(CosmosClient&&)      = delete;
        CosmosClient(const CosmosClient&)    = delete;
        auto& operator=(const CosmosClient&) = delete;

        const nlohmann::json& configuration()
        CosmosClient& configure(const nlohmann::json&) noexcept(false);
        void queue(CosmosArgumentType&& op, CosmosAsyncCallbackType);

        CosmosResponseType          discoverRegions()
        CosmosResponseType          listDatabases()
        CosmosResponseType          listCollections(CosmosArgumentType const& ctx)
        CosmosIterableResponseType  listDocuments(CosmosArgumentType const& ctx)
        CosmosResponseType          createDocument(CosmosArgumentType const& ctx)
        CosmosResponseType          upsertDocument(CosmosArgumentType const& ctx)
        CosmosResponseType          updateDocument(CosmosArgumentType const& ctx)
        uint32_t                    removeDocument(CosmosArgumentType const& ctx)
        CosmosIterableResponseType  queryDocuments(CosmosArgumentType const& ctx)
        CosmosResponseType          findDocument(CosmosArgumentType const& ctx)

        friend void to_json(nlohmann::json& dest, const CosmosClient& src);
    }
```

### Member variables
&nbsp; | Type           | Description
------:|:---------------|:-------------
`config` 🔒 | `nlohmann::json` | Configuration for the client
`serviceSettings` 🔒 | `nlohmann:json` | Azure region information from by `discoverRegion()` call via `configure()`.
`isConfigured` 🔒 | `atomic_bool` | Signalled when the `configuration()` has been successfully invoked.
`restClient` 🔒 | `WinHttpRESTClient` | The Rest Client is used for all operations against Cosmos.<br/>Multiple threads may use this class without issue as the underlying `send()` only uses the lone `HINTERNET` and the underlying WinHTTP library performs the connection-pooling.
`cnxn` 🔒 | `CosmosConnection` | Represents the Primary and optionally Secondary connection string.<br/>Holds the information on the current read/write enpoints and the encryption keys.<br/>This is un-important to the client and its implementation may change without affecting the user-facing API.
`CosmosClientUserAgentString` | `std::string` | The connection string used by the client against the Azure Cosmos Server.
`asyncWorkers` | `simple_pool<CosmosArgumentType>` | Implements asynchrony to the cosmos library.

### Member Functions
&nbsp; | Returns           | Description
------:|:---------------|:-------------
[`CosmosClient`](#cosmosclientcosmosclient) ⎔ | | Default constructor.<br/>_Move constructors, assignment operators are not relevant and have been deleted._
[`configuration`](#cosmosclientconfiguration) ⎔ | `const nlohmann::json&` | Return the as const the `config` object.
[`configure`](#cosmosclientconfigure) ⎔ | `CosmosClient&` | Configures the client by setting up the `cnxn` object, invokes `discoverRegions` to build the readable/writable locations for the region and prepares the current read/write locations.<br/>Do not invoke this method as it causes the underlying objects to be reset and will likely break any operations.
[`discoverRegions`](#cosmosclientdiscoverregions) ⎔ | [`CosmosResponseType`](#struct-cosmosresponsetype) | Returns service configuration such as settings, regions, read and write locations.
[`listDatabases`](#cosmosclientlistdatabases) ⎔   | [`CosmosResponseType`](#struct-cosmosresponsetype) | Returns `Documents[]` containing the ids of the databases for this cosmos service endpoint.
[`listCollections`](#cosmosclientlistcollections) ⎔ | [`CosmosResponseType`](#struct-cosmosresponsetype) | Returns `Documents[]` containing the ids of the collections in the given database.
[`listDocuments`](#cosmosclientlistdocuments) ⎔ |  [`CosmosIterableResponseType`](#struct-cosmositerableresponsetype) | Returns zero-or-more documents in the given collection.<br/>The client is responsible for repeatedly invoking this method to pull all items.
[`create`](#cosmosclientcreate) ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Creates (add) single document to given collection in the database.
[`upsert`](#cosmosclientupsert) ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Create of update a document in the given collection in the database.
[`update`](#cosmosclientupdate) ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Update a document in the given collection in the database.
[`remove`](#cosmosclientremove) ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Remove a document matching the document id in the given collection.
[`query`](#cosmosclientquery) ⎔ |  [`CosmosIterableResponseType`](#struct-cosmositerableresponsetype) | Returns zero-or-more items matching the given search query and parameters.<br/>The client is responsible for repeatedly invoking this method to pull all items.
[`find`](#cosmosclientfind) ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Finds and returns a *single* document matching the given document id.
[`async`](#cosmosclientasync) ⎔ |   | Finds and returns a *single* document matching the given document id.
`to_json` ⎔ |  | Serializer for CosmosClient to a json object.
`asyncDispatcher` |  | Implements the dispatch from the queue and recovery/retry logic.

> There are also implementations for serializing via `std::format` using the json serializer.


### `CosmosClient::config`

`nlohmann::json` - Object stores the various configuration elements for this client instance.

The following elements are required:
- `apiVersion` - Defaults to `2018-12-31`; you should not provide this value as the library is mated to the specific version.
- `connectionStrings` - An array of one or two connection strings you'd get from the Azure Cosmos Keys portal. These may be "read-write" or "Read-only" values depending on your application use.<br/>If you use read-only keys then you will get errors invoking `create`, `upsert`, `update`.
- `partitionKeyNames` - An array of one or more partition key fields that must be present in each document. This is also configured in the Azure Portal.

**Sample/default**
```cpp
nlohmann::json config {
        {"_typever", CosmosClientUserAgentString},
        {"apiVersion", "2018-12-31"},
        {"connectionStrings", {}},
        {"partitionKeyNames", {}}
};
```

### `CosmosClient::serviceSettings`

Service Settings saved from [`discoverRegions()`](#cosmosclientdiscoverregions)

This data is used within the method `configure` and used thereafter for informative purposes.

> Guard against writers.

### `CosmosClient::isConfigured`

`atomic_bool` - Indicates that the client has been configured.

### `CosmosClient::restClient`

The I/O utility class from [restcl](https://github.com/siddiqsoft/restcl).

### `CosmosClient::cnxn`

This is the core data-structure and holds the read, write endpoints, primary and secondary connections and the encryption key.

It is not intended to be used by the client and its implementation is subject to change.


### `CosmosClient::CosmosClient()`

Empty default constructor. All of the move constructor and assignment operators and move operators have been deleted.


### `CosmosClient::configuration()`

Returns the current config json object as `const`.

#### return

Const reference to the `config` object.


### `CosmosClient::configure()`

```cpp
CosmosClient& configure(const nlohmann::json& src = {});
```

Re-configure the client. This will invoke the discoverRegions to populate the available regions and sets the
primary write/read locations. If the source is empty (no arguments) then the current configuration is returned.

Avoid repeated invocations!

This method may throw if the underling call to `discoverRegions` fails.

#### param `src`
A valid json object must comply with the defaults. If empty, the current configuration is returned without
any changes.

Must have the following elements:
```json
{
 "connectionStrings": ["primary-connection-string"],
 "primaryKeyNames": ["field-name-of-partition-id"]
}
```

#### return

Reference to the CosmosClient object.


### `CosmosClient::async`

Perform any supported operation using the `simple_pool` which uses a `std::deque` with a threadpool to perform the IO and recovery for simple errors such as `401` and IO transient errors.

```cpp
    void async(CosmosArgumentType&& op,
               CosmosAsyncCallbackType callback);
```
#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`op` | [`CosmosArgumentType&&`](#struct-cosmosargumenttype) | The request to be executed asynchronously.
`callback` | [`CosmosAsyncCallbackType`](#using-cosmosasynccallbacktype) | The callback invoked when the operation completes (success or failure).<br/>The callback includes the original request as well as the response from the server.

#### examples

A simple `async` call uses structured binding introduced in C++17 
```cpp
    std::atomic_bool         passTest = false;
    std::string              priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string              secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    siddiqsoft::CosmosClient cc;

    // Configure
    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});
    // Make an async call
    cc.queue({.operation = "listDatabases"},
             [](auto const& ctx, auto const& resp) {
                // Print the list of databases..
                std::cout << std::format("List of databases\n{}\n", resp.document.dump(3));
             });
```

This is a complete example (without error checking) to illustrate how you can nest operations repeatedly and have them complete asynchronously!

```cpp
    std::atomic_bool passTest = false;
    std::string priConnStr    = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr    = std::getenv("CCTEST_SECONDARY_CS");

    siddiqsoft::CosmosClient cc;

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});
    // #1..Start by listing the databases
    cc.queue({.operation = "listDatabases"},
             [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType const& resp) {
                 // #2..On completion of listDatabases, get the collections..
                 cc.queue({.operation = "listCollections",
                           .database = resp.document.value("/Databases/0/id"_json_pointer, "")},
                          [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType const& resp) {
                              // #3..On successful listCollections, lets create a document..
                              auto docId          = std::format("azure-cosmos-restcl.{}",
                                                                std::chrono::system_clock().now().time_since_epoch().count());
                              auto pkId           = "siddiqsoft.com";
                              // #3..Create the document..
                              cc.queue({.operation    = "create",
                                        .database     = ctx.database,
                                        .collection   = resp.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                                        .docId        = docId,
                                        .partitionKey = pkId,
                                        .doc          = { {"id", docId},
                                                         {"ttl", 360},
                                                         {"__pk", pkId},
                                                         {"func", __func__},
                                                         {"source", "basic_tests.exe"}}},
                                       [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx,
                                                        siddiqsoft::CosmosResponseType const& resp) {
                                           // #4..We created the document..
                                           // ...
                                           // #4..Remove the document
                                           cc.queue({.operation    = "remove",
                                                     .database           = ctx.database,
                                                     .collection   = ctx.collection,
                                                     .docId        = resp.document.value("id", ctx.docId),
                                                     .partitionKey = ctx.partitionKey},
                                                    [&cc, &passTest](auto const& ctx, auto const& resp) {
                                                        // #5..All done. the document should be gone.
                                                        passTest = true;
                                                        passTest.notify_all();
                                                    });
                                       });
                          });
             });

    // Holds until the test completes
    passTest.wait(false);
```
> **NOTE**
> 
> When using lambda captures, it is critical that you avoid using `&` and specify the values you'd like to capture explicitly.
> Using the `&` causes heap errors and slows down as the compiler generates an ever larger heap for you.
> 
> Also, if you're using AddressSanitier, it will complain. The above example uses explicit capture and enables AddressSanitizer without issue.

### `CosmosClient::discoverRegions()`

```cpp
CosmosResponseType discoverRegions();
```

Discover the Regions for the current base Uri

This method is invoked by the `configuration` method.

#### return

[`CosmosResponseType`](#struct-cosmosresponsetype)


### `CosmosClient::listDatabases()`

```cpp
CosmosResponseType listDatabases();
```

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
&nbsp; | &nbsp; | &nbsp;

List all databases for the given service

Refer to https://docs.microsoft.com/en-us/rest/api/documentdb/documentdb-resource-uri-syntax-for-rest

#### return

[`CosmosResponseType`](#struct-cosmosresponsetype)


### `CosmosClient::listCollections()`

```cpp
CosmosResponseType listCollections(const std::string& dbName);
```

List all collections for the given database.

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.

#### return

[`CosmosResponseType`](#struct-cosmosresponsetype)


### `CosmosClient::listDocuments()`

```cpp
CosmosIterableResponseType listDocuments(const std::string& dbName,
                                         const std::string& collName,
                                         const std::string& continuationToken);
```

Returns all of the documents for the given collection.

This can be very slow and you must use the continuationToken to repeatedly call the method to complete your fetch. See example below.

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.
`collName` | `std::string` | Collection name.
`continuationToken` | `std::string` | Optional. The first invocation would not have this value. However, subsequent invocations will include this value from the response to iterate through the documents.

#### return

[`CosmosIterableResponseType`](#struct-cosmositerableresponsetype)

#### Iterate Example

Refer to the test `listDocuments` for working example.

```cpp
siddiqsoft::CosmosIterableResponseType irt {};
// This is your destination container; note that the first element is null in this example
nlohmann::json                         allDocs = nlohmann::json::array();
// This is the running count of the total
uint32_t                               allDocsCount {};
// First, we query for all items that match our criteria (source=__func__)
do {
    irt = cc.query(dbName,
                   collectionName,
                   "*", // across all partitions
                   "SELECT * FROM c WHERE contains(c.source, @v1)",
                   { { {"name", "@v1"},
                       {"value", std::format("{}-", getpid())}
                     } },
                   irt.continuationToken);
    if (200 == irt.statusCode &&
        irt.document.contains("Documents") &&
        !irt.document.at("Documents").is_null())
    {
        // Append to the current container
        allDocs.insert(allDocs.end(),                     // dest
                       irt.document["Documents"].begin(), // from
                       irt.document["Documents"].end());
        allDocsCount += irt.document.value("_count", 0);
    }
} while (!irt.continuationToken.empty());
```

### `CosmosClient::create()`

```cpp
CosmosResponseType create(const std::string& dbName,
                          const std::string& collName,
                          const nlohmann::json& doc);
```

Create document in the given collection.

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.
`collName` | `std::string` | Collection name.
`doc` | `nlohmann::json` | The document to insert in the collection

#### return

[`CosmosResponseType`](#struct-cosmosresponsetype)


### `CosmosClient::upsert()`

Update or Insert document in the given collection.

```cpp
CosmosResponseType upsert(const std::string& dbName,
                          const std::string& collName,
                          const nlohmann::json& doc);
```

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.
`collName` | `std::string` | Collection name.
`doc` | `nlohmann::json` | The document to insert or update

#### return

[`CosmosResponseType`](#struct-cosmosresponsetype)


### `CosmosClient::update()`

```cpp
CosmosResponseType update(const std::string& dbName,
                          const std::string& collName,
                          const std::string& docId,
                          const std::string& pkId,
                          const nlohmann::json& doc);
```

Update an existing document

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.
`collName` | `std::string` | Collection name.
`docId` | `std::string` | The document id.
`pkId` | `std::string` | Partition key.
`doc` | `nlohmann::json` | The document to update

#### return

[`CosmosResponseType`](#struct-cosmosresponsetype)


### `CosmosClient::remove()`

```cpp
uint32_t remove(const std::string& dbName,
                const std::string& collName,
                const std::string& docId,
                const std::string& pkId);
```

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.
`collName` | `std::string` | Collection name.
`docId` | `std::string` | The document id.
`pkId` | `std::string` | Partition key.


#### return

[`CosmosResponseType`](#struct-cosmosresponsetype)


### `CosmosClient::query()`

```cpp
CosmosIterableResponseType query(const std::string&    dbName,
                                 const std::string&    collName,
                                 const std::string&    pkId,
                                 const std::string&    queryStatement,
                                 const nlohmann::json& params            = {},
                                 const std::string&    continuationToken = {});
```

Performs a query and continues an existing query if the continuation token exists

CAUTION: If your query yields dozens or hundreds or thousands of documents, this method
currently does not offer "streaming" or callbacks.
It continues until the server reports no-more-continuation and returns the documents
in a single response json.
Memory and timing can be massive!

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.
`collName` | `std::string` | Collection name.
`pkId` | `std::string` | Partition key. You can use `*` to indicate query across partitions.
`queryStatement` | `std::string` | The SQL API query string.<br/>`"SELECT * FROM c WHERE contains(c.source, @v1)"`
`params` | `nlohmann::json` | Optional json array with name-value objects.<br/>`[{"name": "@v1", "value": "hello.world"}]`
`continuationToken` | `std::string` | Optional continuation token. This is used with the value `x-ms-continuation` to page through the query response.

**remarks**

The response is paged so you will need to combine the results into
delivering to the user a single json with array of `Documents` object and `_count`.

See [Iterate example](#iterate-example) on the contents and how to fetch all documents by iterating over the results.

[Queries in Cosmos](https://docs.microsoft.com/en-us/rest/api/cosmos-db/q)

#### return

[`CosmosIterableResponseType`](#struct-cosmositerableresponsetype)


### `CosmosClient::find()`

```cpp
CosmosResponseType find(const std::string& dbName,
                        const std::string& collName,
                        const std::string& docId,
                        const std::string& pkId);
```

Removes the document given the docId and pkId

See https://docs.microsoft.com/en-us/rest/api/documentdb/delete-a-document

#### params

Parameter  | Type            | Description
----------:|-----------------|----------------------
`dbName` | `std::string` | Database name.
`collName` | `std::string` | Collection name.
`docId` | `std::string` | The document id.
`pkId` | `std::string` | Partition key. You can use `*` to indicate query across partitions.


#### return

Returns `uint32_t` with `204` on successful removal or `404` if document does not exist.

<hr/>

# Tests

- The tests are written using Googletest framework instead of VSTest.
  - Enabling the address sanitizer threw errors enough to switch to googletest.
- AddressSanitizer is disabled for the test as it ends up hanging the multi-thread tests when using `std::latch` and/or `std::barrier`.
- The roll-up is not accurate depsite the fact that we've got 24 tests only 10 are reported!

# References

