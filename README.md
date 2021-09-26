CosmosClient : Azure Cosmos REST-API Client for Modern C++
-------------------------------------------

<!-- badges -->
[![CodeQL](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.CosmosClient?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionid=15&branchName=main)
![](https://img.shields.io/github/v/tag/SiddiqSoft/CosmosClient)
![](https://img.shields.io/nuget/v/SiddiqSoft.CosmosClient)
![](https://img.shields.io/nuget/dt/SiddiqSoft.CosmosClient)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/15)
<!--![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/15)-->
![](https://img.shields.io/github/license/siddiqsoft/CosmosClient)
<!-- end badges -->



# Motivation

There is a need for a light-weight Azure Cosmos client that is crafted for C++ instead of "C" wrapper.

## Features
- C++20 Azure Cosmos REST API client.
  - Be instructive *and* functional.
  - Low-overhead--just not optimized.
  - Leverage C++ sugarcoating (structured bindings and initializer lists).
- JSON everywhere; configuration, I/O, results.
- Simple interface -- this means we limit the options you can fiddle
- Implement the Cosmos SQL REST API
  - All of the responses are JSON documents as returned from Azure Cosmos. We do not abstract/encapsulate unless there is value.
  - Implement Azure REST API best practices.
- Single header
- Use Microsoft Win32 API for IO, encryption
  - Dependency on [nlohmann.json](https://github.com/nlohmann/json)
    - The best interface
  - Dependency on [azure-cpp-utils](https://github.com/siddiqsoft/azure-cpp-utils) our own library.
    - We do not implement any encryption functions. We only add a thin modern C++ coating on the underlying Win32 SDK.
  - Dependency on [restcl](https://github.com/siddiqsoft/restcl) our own library.
    - Again, we wrap a modern C++ sugar coat on the Win32 WinHTTP library. Nothing overly heavy or force you to use std::wstring or invent yet-another-string!
- Simplicity and ease of use--focus on your problem set and let the libraries evolve.

## Development Style

- Continuous improvement
  - During early stages, development is going to be private until a first release candidate source.
  - We break ABI often.
- Focus on end-use and ease-of-use over performance
- Build what is needed and add features subject to the above criteria

## Requirements
- While the code is pretty much C++20 the IO uses WinHTTP underneath.
- The build and tests are for Visual Studio 2019 under x64 using googletest.
- We switched to googletest from VSTest for the insane reason that enabiling ASAN was not compatible with VSTest.
- As of Visual Studio v16.11.2 the tag `/c++20` no longer includes `format` and our code is stuck at `/c++latest`.

# API / Usage

You can start saving your documents in Cosmos with just 3 lines of code!
- Declare the instance
- Configure with json via `configure`
- Make call to `create`

The detailed [API](https://siddiqsoft.github.io/CosmosClient/) is over at Github pages.

## Sample

```cpp
#include "nlohmann/json.hpp"
#include "siddiqsoft/azure-cosmos-restcl.hpp"

// See the example1 test for full source code
// Code here has been trimmed for brevity.
void example1(const std::string& p, const std::string& s)
{
    // You should maintain one instance for each Cosmos service
    // It is advised to maintain a single instance per runtime (exe/dll/service)
    siddiqsoft::CosmosClient cc;

    // If you provide valid information, this will configure by probing Azure for
    // region information and sets up the read and write locations.
    // We use JSON object as the primary configuration interface.
    // The items you provide are muxed with the defaults so you can provide as much
    // or as little as the following elements.
    cc.configure({{"partitionKeyNames", {"__pk"}},
                  {"connectionStrings", {p, s}}} );

    // This is all we need to worry about when creating a document!
    // Cosmos requires the "id" field be in the document so we perform
    // no checks for id nor the primaryKey field.
    // The goal is to get you the response from Cosmos with little more
    // than convenience overhead.
    // Minimal abstractions ("there are no zero-cost abstractions").
    // Just use the JSON object!
    // Here, we ask a document to be created and our lambda be invoked
    // on success or failure.
    cc.async({.operation  = siddiqsoft::CosmosOperation::create,
              .database   = dbName,
              .collection = collectionName,
              .document   = {{"id", docId},  // We send the JSON document
                             {"ttl", 360},   // inline for this example
                             {"__pk", pkId}, // PKID is required
                             {"func", __func__},
                             {"source", "basic_tests.exe"}},
              .onResponse = [&cc](auto const& ctx, auto const& resp){
                                if( 201 == resp.statusCode ) {
                                    // Document created..
                                    ..
                                    ..
                                    ..
                                    // You can even nest (this will queue into the worker
                                    // pool another async request.. Just make sure you
                                    // capture the cc instance.
                                    // The ctx contains the current context/arguments for this callback.
                                    cc.async({.operation    = siddiqsoft::CosmosOperation::remove,
                                              .database     = ctx.database,
                                              .collection   = ctx.collection,
                                              .id           = ctx.id, // or resp.document["id"] from create op
                                              .partitionKey = ctx.partitionKey,
                                              .onResponse   = [](auto const& ctx, auto const& resp){
                                                 // on remove of document..
                                              }}); // end of the internal async op
                                }
                                else {
                                   // handle failures..
                                }
                            }}); // end of the create async op
}
```

# Roadmap

0. [x] [Documentation](https://siddiqsoft.github.io/CosmosClient/)
1. [ ] Async operations with auto-retry (follow the read/write locations and toggle primary/secondary)
2. [ ] Support C++20 modules
3. [ ] Co-routines
4. [ ] OpenSSL for non-Windows platforms (current implementation is Windows only)

<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>
