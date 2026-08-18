# NuGet Package Integration

`CosmosClient` is available as a native C++ NuGet package for MSVC and Visual Studio.

---

## Package Installation

=== "Package Manager Console"

    ```powershell
    Install-Package SiddiqSoft.CosmosClient
    ```

=== "dotnet CLI"

    ```bash
    dotnet add package SiddiqSoft.CosmosClient
    ```

=== "nuget CLI"

    ```bash
    nuget install SiddiqSoft.CosmosClient
    ```

---

## Note on Dependencies

> **WARNING**
>
> The NuGet package provides the header files for `CosmosClient`. Transitively required packages (such as `restcl` and `nlohmann/json`) must be included in your build configuration or fetched via CPM/CMake.
