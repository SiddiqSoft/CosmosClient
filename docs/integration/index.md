# Integration Overview

`CosmosClient` is designed for fast, seamless integration into modern C++ projects using CMake, CPM package manager, Git submodules, or NuGet.

---

## Integration Methods

| Method | Target Platform | Recommended For |
| :--- | :--- | :--- |
| [**CMake & CPM**](cmake.md) | Windows, Linux, macOS | Header-only dependency management in CMake projects |
| [**Dependencies Diagram**](dependencies.md) | All | Visual graph of core and system dependencies |
| [**NuGet Package**](nuget.md) | Windows (MSVC) | Visual Studio / MSBuild C++ projects |

---

## Quick Example (CMake with CPM)

```cmake
cmake_minimum_required(VERSION 3.29)
project(my_cosmos_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(pack/CMakeCommonHelpers.cmake) # or CPM.cmake

CPMAddPackage("gh:SiddiqSoft/CosmosClient#3.2.0")

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE cosmoscl::cosmoscl)
```
