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
[nlohmann.json](https://github.com/nlohmann/json)<br/>![](https://img.shields.io/nuget/v/nlohmann.json)| Cosmos deals with JSON objects and this is one of the simplest, closest to `stdlib` structure libraries for C++.<br/>We have to make choices and this is our choice.<br/>The library is quite conformant and lends itself to general purpose use since it uses `<vector>` underneath it all.<br/>We leave time and experience (plus manpower) to optimize this library for us. So long as it works and we do not have to worry about some ugly JAVA-esque or C-style interface!
[restcl](https://github.com/SiddiqSoft/restcl)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)| This is our REST client.<br/>There are *many*, *many*, *many* clients out there and I'm not quite happy with their design.<br/>This implementation focuses on the "interface" as a thin wrapper  atop <a href="https://docs.microsoft.com/en-us/windows/win32/winhttp/about-winhttp">WinHTTP library</a>.
[AzureCppUtils](https://github.com/SiddiqSoft/azure-cpp-utils)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.AzureCppUtils) | This is library with helpers for encryption, base64 encode/decode and token generation for Cosmos.

_Unless otherwise noted, use the latest. We're quite aggressive updating dependencies._

# API

Namespace `siddiqsoft`

A single class [CosmosClient](#class-cosmosclient) implements the functionality.

Note that since this is a header, you'll also get the contents of the `azure-cpp-utils` and `restcl` as well as supporting code.
C++ modules are a solution to this problem but the implementation in VS 2019 v16.11.2 is flaky and I could not get it working!

## struct `CosmosResponseType`

This is the primary return type from the [CosmosClient](#struct-cosmosclient) functions.

Includes serializers for nlohmann::json via the macro `NLOHMANN_DEFINE_TYPE_INTRUSIVE`.

CosmosResponseType | Type  | Description
-------------------|----|---
`statusCode` | `uint32_t` | Holds the HTTP response Status Code or the system-error code (WinHTTP error code)
`document`   | `nlohmann::json` | Holds the response from the Cosmos response.

## struct `CosmosIterableResponseType`

Extends the [CosmosResponseType](#struct-cosmosresponsetype) by adding the `continuationToken` data member
to facilitate iteration for the methods [listDocuments](#listdocuments) and [query](#query).

CosmosIterableResponseType | Type  | Description
-------------------|----|---
`continuationToken` | `string` | Do not modify; this is the `x-ms-continuation` token from the response header and inpat for the `listDocuments` and `query` methods to obtain all of the documents.


## class `CosmosClient`

Implements the Cosmos SQL-API via REST. Omits the attachment API as of this version.

&nbsp; | Type           | Description
------:|:---------------|:-------------
`config` 🔒 | `nlohmann::json` | Configuration for the client
`serviceSettings` 🔒 | `nlohmann:json` | Azure region information from by `discoverRegion()` call via `configure()`.
`isConfigured` 🔒 | `atomic_bool` | Signalled when the `configuration()` has been successfully invoked.
`restClient` 🔒 | `WinHttpRESTClient` | The Rest Client is used for all operations against Cosmos.<br/>Multiple threads may use this class without issue as the underlying `send()` only uses the lone `HINTERNET` and the underlying WinHTTP library performs the connection-pooling.
`cnxn` 🔒 | `CosmosConnection` | Represents the Primary and optionally Secondary connection string.<br/>Holds the information on the current read/write enpoints and the encryption keys.<br/>This is un-important to the client and its implementation may change without affecting the user-facing API.
`CosmosClientUserAgentString` | `std::string` | The connection string used by the client against the Azure Cosmos Server.
`CosmosClient` ⎔ | | Default constructor.<br/>_Move constructors, assignment operators are not relevant and have been deleted._
`configuration` ⎔ | `const nlohmann::json&` | Return the as const the `config` object.
`configure` ⎔ | `CosmosClient&` | Configures the client by setting up the `cnxn` object, invokes `discoverRegions` to build the readable/writable locations for the region and prepares the current read/write locations.<br/>Do not invoke this method as it causes the underlying objects to be reset and will likely break any operations.
`discoverRegions` ⎔ | [`CosmosResponseType`](#struct-cosmosresponsetype) | Returns service configuration such as settings, regions, read and write locations.
`listDatabases` ⎔   | [`CosmosResponseType`](#struct-cosmosresponsetype) | Returns `Documents[]` containing the ids of the databases for this cosmos service endpoint.
`listCollections` ⎔ | [`CosmosResponseType`](#struct-cosmosresponsetype) | Returns `Documents[]` containing the ids of the collections in the given database.
`listDocuments` ⎔ |  [`CosmosIterableResponseType`](#struct-cosmositerableresponsetype) | Returns zero-or-more documents in the given collection.<br/>The client is responsible for repeatedly invoking this method to pull all items.
`create` ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Creates (add) single document to given collection in the database.
`upsert` ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Create of update a document in the given collection in the database.
`update` ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Update a document in the given collection in the database.
`remove` ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Remove a document matching the document id in the given collection.
`query` ⎔ |  [`CosmosIterableResponseType`](#struct-cosmositerableresponsetype) | Returns zero-or-more items matching the given search query and parameters.<br/>The client is responsible for repeatedly invoking this method to pull all items.
[`find`](#cosmosclientfind) ⎔ |  [`CosmosResponseType`](#struct-cosmosresponsetype) | Finds and returns a *single* document matching the given document id.
`to_json` ⎔ |  | Serializer for CosmosClient to a json object.

> There are also implementations for serializing via `std::format` using the json serializer.


### `CosmosClient::config`

### `CosmosClient::serviceSettings`

### `CosmosClient::isConfigured`

### `CosmosClient::restClient`

### `CosmosClient::cnxn`

### `CosmosClient::CosmosClient()`

Default constructor.

Move constructor and assignment operators and move operators have been deleted.


### `CosmosClient::configuration`

#### return

Const reference to the `config` object.

### `CosmosClient::configure`

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


### `CosmosClient::discoverRegions`

```cpp
CosmosResponseType discoverRegions();
```

### `CosmosClient::listDatabases`

```cpp
CosmosResponseType listDatabases();
```

### `CosmosClient::listCollections`

```cpp
CosmosResponseType listCollections(const std::string& dbName);
```

### `CosmosClient::listDocuments`

```cpp
CosmosIterableResponseType listDocuments(const std::string& dbName,
                                         const std::string& collName,
                                         const std::string& continuationToken);
```

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
                   {{{"name", "@v1"}, {"value", std::format("{}-", getpid())}}},
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

### `CosmosClient::create`

```cpp
CosmosResponseType create(const std::string& dbName,
                          const std::string& collName,
                          const nlohmann::json& doc);
```

### `CosmosClient::upsert`

```cpp
CosmosResponseType upsert(const std::string& dbName,
                          const std::string& collName,
                          const nlohmann::json& doc);
```

### `CosmosClient::update`

```cpp
CosmosResponseType update(const std::string& dbName,
                          const std::string& collName,
                          const std::string& docId,
                          const std::string& pkId);
```

### `CosmosClient::remove`

```cpp
uint32_t remove(const std::string& dbName,
                const std::string& collName,
                const std::string& docId,
                const std::string& pkId);
```

### `CosmosClient::query`

```cpp
CosmosIterableResponseType query(const std::string&    dbName,
                                 const std::string&    collName,
                                 const std::string&    pkId,
                                 const std::string&    queryStatement,
                                 const nlohmann::json& params            = {},
                                 const std::string&    continuationToken = {});
```

### `CosmosClient::find`

```cpp
CosmosResponseType find(const std::string& dbName,
                        const std::string& collName,
                        const std::string& docId,
                        const std::string& pkId);
```

# Tests

- The tests are written using Googletest framework instead of VSTest.
  - Enabling the address sanitizer threw errors enough to switch to googletest.
- AddressSanitizer is disabled for the test as it ends up hanging the multi-thread tests when using `std::latch` and/or `std::barrier`.
- The roll-up is not accurate depsite the fact that we've got 24 tests only 10 are reported!

# References

