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

nlohmann::json myDoc {{"id", "uniqueId"},
                      {"__pk", "partitionKey"},
                      {"ttl", 3600}};

CosmosClient cc;

cc.configure( {{ "connectionStrings", { primaryConnectionString} }} );

cc.create( "db", "collection", myDoc );
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
