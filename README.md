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

# Objective

## Features
- C++20 Azure Cosmos REST API client.
- JSON everywhere; configuration, I/O, results.
- Simple interface -- this means we limit the options you can fiddle
- Implement the SQL REST API
- Single header
- Use Microsoft Win32 API for IO, encryption
- Dependency on [nlohmann.json](https://github.com/nlohmann/json)
- Simplicity and ease of use--focus on your problem set and let the libraries evolve.

## Development Style

- Continuous improvement
  - During early stages, development is going to be private until a first release candidate source.
  - We break ABI often.
- Focus on end-use and ease-of-use over performance
- Build what is needed and add features subject to the above criteria

## Requirements
- The build and tests are for Visual Studio 2019 under x64.

# API / Usage

- Use the nuget [SiddiqSoft.CosmosClient](https://www.nuget.org/packages/SiddiqSoft.CosmosClient/)
- Copy paste..whatever works.

## Sample

```cpp
#include "nlohmann/json.hpp"
#include "siddiqsoft/azure-cosmos-restcl.hpp"

// See the example1 test for full source code
// Code here has been trimmed for brevity.
void example1(const std::string& p, const std::string& s)
{
    siddiqsoft::CosmosClient cc;

    // If you provide valid information, this will configure by probing Azure for
    // region information and sets up the read and write locations.
    // We use JSON object as the primary configuration interface.
    // The items you provide are muxed with the defaults so you can provide as much
    // or as little as the following elements.
    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {p, s}}});

    // This is all we need to worry about when creating a document!
    // Cosmos requires the "id" field be in the document so we perform
    // no checks for id nor the primaryKey field.
    // The goal is to get you the response from Cosmos with little more
    // than convenience overhead.
    // No useless abstractions.
    // Just use the JSON object!
    if (auto [rc3, cDoc] = cc.create(dbName, collectionName,
                                     {{"id", docId},  // We send the JSON document
                                      {"ttl", 360},   // inline for this example
                                      {"__pk", pkId}, // PKID is required
                                      {"func", __func__},
                                      {"source", "basic_tests.exe"}});
        201 == rc3)
    {
        // ...do something
        // ...useful with cDoc..

        // Remove the just created document..
        auto [rc4, delDoc] = cc.remove(dbName, collectionName, cDoc.value("id", docId), pkId);
    }
}
```

# Testing

Testing requires that we declare the following environment variables (either from your local terminal or during the CI step.)

Environment Variable      | Purpose
--------------------------|:----------------
`CCTEST_PRIMARY_CS`   | The Primary Account Connection String from the Azure Portal for Cosmos.
`CCTEST_SECONDARY_CS` | The Secondary Account Connection String from the Azure Portal for Cosmos.


<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>
