# CMake & Submodules Integration

`CosmosClient` provides an `INTERFACE` library target `cosmoscl::cosmoscl` requiring C++23.

---

## Using CPM (Recommended)

[CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) is a cross-platform CMake dependency manager.

```cmake
# Add CPM.cmake to your CMake setup
include(path/to/CPM.cmake)

# Pull CosmosClient directly from GitHub
CPMAddPackage("gh:SiddiqSoft/CosmosClient#3.2.0")

# Link to target
target_link_libraries(your_target PRIVATE cosmoscl::cosmoscl)
```

---

## Using FetchContent

Standard CMake 3.14+ `FetchContent`:

```cmake
include(FetchContent)

FetchContent_Declare(
    CosmosClient
    GIT_REPOSITORY https://github.com/SiddiqSoft/CosmosClient.git
    GIT_TAG        v3.2.0
)

FetchContent_MakeAvailable(CosmosClient)

target_link_libraries(your_target PRIVATE cosmoscl::cosmoscl)
```

---

## Using Git Submodules

If you vendor dependencies into `3rdparty/`:

```bash
git submodule add https://github.com/SiddiqSoft/CosmosClient.git 3rdparty/CosmosClient
git submodule update --init --recursive
```

In `CMakeLists.txt`:

```cmake
add_subdirectory(3rdparty/CosmosClient)
target_link_libraries(your_target PRIVATE cosmoscl::cosmoscl)
```
