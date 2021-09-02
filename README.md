CosmosClient : Azure Cosmos REST-API Client for Modern C++
-------------------------------------------

<!-- badges -->
[![CodeQL](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/CosmosClient/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.CosmosClient?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionid=15&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.CosmosClient)
![](https://img.shields.io/github/v/tag/SiddiqSoft/CosmosClient)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/15)
![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/15)
<!-- end badges -->

![](docs/work-in-progress.png)


# Motivation

There is a need for a light-weight Azure Cosmos client that is crafted for C++ instead of "C" wrapper.

## Features
- C++20 Azure Cosmos REST API client.
  - Be instructive *and* functional.
  - Low-overhead--just not optimized.
  - C++20 sugarcoating
- JSON everywhere; configuration, I/O, results.
- Simple interface -- this means we limit the options you can fiddle
- Implement the SQL REST API
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
- While the code is pretty much C++20 the IO uses WinHTTP underneath. This 
- The build and tests are for Visual Studio 2019 under x64 using googletest.
- We switched to googletest from VSTest for the insane reason that enabiling ASAN was not compatible with VSTest.


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
    // No useless abstractions.
    // Just use the JSON object!
    if (auto [rc, doc] = cc.create(dbName, collectionName,
                                   {{"id", docId},  // We send the JSON document
                                    {"ttl", 360},   // inline for this example
                                    {"__pk", pkId}, // PKID is required
                                    {"func", __func__},
                                    {"source", "basic_tests.exe"}});
        201 == rc)
    {
        // ...do something
        // ...useful with doc..

        // Remove the just created document..
        auto [rcd, _] = cc.remove(dbName, collectionName, doc.value("id", docId), pkId);
    }
}
```

# Roadmap

0. Documentation
1. Async operations
2. Support C++20 modules
3. Co-routines
4. OpenSSL for non-Windows platforms (current implementation is Windows only)

<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>
