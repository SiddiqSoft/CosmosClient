# CosmosClient Test Suite Documentation

## Overview

The `testall.cpp` file contains a comprehensive unified test suite for the Azure Cosmos DB REST API client library. All tests use a single test fixture (`CosmosIntegrationTests`) with shared setup and teardown, ensuring efficient resource management and test isolation.

---

## Test Organization

### Test Categories

The test suite is organized into **7 main categories**:

1. **Validation Tests** - Configuration and environment validation (no emulator required)
2. **Connection Tests** - Connection string parsing and rotation (no emulator required)
3. **Endpoint Tests** - URI endpoint handling and rotation (no emulator required)
4. **Integration Tests** - Basic CRUD operations (requires emulator)
5. **Comprehensive API Tests** - Advanced features and edge cases (requires emulator)
6. **Additional Azure Cosmos REST API Feature Tests** - Advanced query and system features (requires emulator)

---

## Test Fixture: CosmosIntegrationTests

### SetUpTestCase() - Runs Once Before All Tests

**Purpose**: Initialize the test environment with database and collections

**Steps**:
1. Check if Cosmos DB service is reachable
2. Configure the test client with connection strings and partition key names
3. Clean up any existing test database from previous runs
4. Create a fresh test database (`cosmoscl_DB_0`)
5. Create 3 test collections (`cosmoscl_test_COLL_0`, `cosmoscl_test_COLL_1`, `cosmoscl_test_COLL_2`)
6. Seed each collection with 10 test documents containing:
   - Unique ID (hex format with even/odd indicator)
   - TTL value (1360 seconds)
   - Partition key (`__pk`: "siddiqsoft.com")
   - Function name and source information

### TearDownTestCase() - Runs Once After All Tests

**Purpose**: Clean up test resources

**Steps**:
1. Delete the test database (`cosmoscl_DB_0`)
2. Verify deletion status (204 or 404 expected)

### SetUp() - Runs Before Each Test

**Purpose**: Verify Cosmos DB is reachable before each test

**Steps**:
1. Check if Cosmos DB service is reachable
2. Skip test if service is unavailable

### Helper Methods

- **GenerateDocId(prefix)**: Creates unique document IDs using current timestamp
- **CreateTestDocument(id, pk)**: Creates a standard test document with metadata

---

## Test Categories Explained

### 1. VALIDATION TESTS (No Emulator Required)

These tests validate configuration and environment setup without requiring a running Cosmos DB instance.

#### `checkEmulatorInfo`
- **Purpose**: Verify environment variables are set
- **Steps**:
  1. Retrieve connection strings from environment
  2. Assert primary connection string is not empty
  3. Assert secondary connection string is not empty
- **Expected Result**: Both connection strings are available

#### `configure_Defaults`
- **Purpose**: Verify default client configuration
- **Steps**:
  1. Create a new CosmosClient instance
  2. Check configuration contains required keys
  3. Verify API version is "2018-12-31"
- **Expected Result**: Default configuration is correct

#### `configure_check_json`
- **Purpose**: Verify client can be serialized to JSON
- **Steps**:
  1. Create a new CosmosClient instance
  2. Convert to JSON
  3. Verify JSON contains expected sections
- **Expected Result**: JSON serialization works correctly

#### `configure_1`
- **Purpose**: Verify client configuration with connection strings
- **Steps**:
  1. Get active connection strings
  2. Create and configure a CosmosClient
  3. Verify configuration is applied
  4. Check service settings (readable/writable locations)
- **Expected Result**: Client is properly configured

#### `discoverRegions`
- **Purpose**: Verify region discovery functionality
- **Steps**:
  1. Configure client with connection strings
  2. Call discoverRegions()
  3. Verify response status is 200
  4. Check readable and writable locations are populated
- **Expected Result**: Regions are discovered successfully

#### `discoverRegions_BadPrimary`
- **Purpose**: Verify failover when primary connection fails
- **Steps**:
  1. Configure with invalid primary and valid secondary
  2. Call discoverRegions() - expect failure
  3. Rotate connection to secondary
  4. Call discoverRegions() - expect success
  5. Rotate back to primary - expect failure
- **Expected Result**: Failover mechanism works correctly

---

### 2. CONNECTION TESTS (No Emulator Required)

These tests validate connection string parsing and connection rotation.

#### `test1_n` (CosmosConnection)
- **Purpose**: Parse connection string components
- **Steps**:
  1. Create connection from connection string
  2. Verify connection string is preserved
  3. Verify base URI is extracted correctly
  4. Verify encoded key is extracted correctly
  5. Verify host is parsed correctly
- **Expected Result**: Connection string is parsed correctly

#### `test2_n` (CosmosConnection)
- **Purpose**: Verify connection JSON serialization
- **Steps**:
  1. Create connection from connection string
  2. Convert to JSON
  3. Verify JSON has 4 elements
- **Expected Result**: JSON serialization works

#### `rotateConnection_1`
- **Purpose**: Verify connection rotation with primary and secondary
- **Steps**:
  1. Create connection with primary and secondary
  2. Verify initial connection is primary
  3. Rotate and verify secondary is active
  4. Rotate and verify primary is active again
  5. Rotate by 2 and verify secondary is active
  6. Rotate by 1 and verify primary is active
- **Expected Result**: Connection rotation works correctly

#### `rotateConnection_2`
- **Purpose**: Verify connection rotation with only primary
- **Steps**:
  1. Create connection with only primary
  2. Verify initial connection is primary
  3. Rotate multiple times
  4. Verify connection remains primary
- **Expected Result**: Single connection doesn't rotate

---

### 3. ENDPOINT TESTS (No Emulator Required)

These tests validate URI endpoint handling and rotation.

#### `test1_n` (CosmosEndpoint)
- **Purpose**: Parse endpoint from connection string
- **Steps**:
  1. Create endpoint from connection string
  2. Verify base URI is correct
  3. Verify encoded key is correct
  4. Verify host is parsed correctly
  5. Verify read/write URI rotation works
- **Expected Result**: Endpoint is parsed correctly

#### `test2_n` (CosmosEndpoint)
- **Purpose**: Verify endpoint with multiple read/write URIs
- **Steps**:
  1. Create endpoint from connection string
  2. Add multiple readable URIs
  3. Add multiple writable URIs
  4. Verify read URI rotation cycles through all URIs
  5. Verify write URI rotation cycles through all URIs
  6. Clear URIs and verify fallback to base URI
- **Expected Result**: URI rotation works correctly

---

### 4. INTEGRATION TESTS (Requires Emulator)

These tests validate basic CRUD operations against a running Cosmos DB instance.

#### `CreateDatabase`
- **Purpose**: Create, find, and delete a database
- **Steps**:
  1. Create a new database
  2. Verify status code is 201
  3. Find the database
  4. Verify status code is 200
  5. Delete the database
  6. Verify status code is 204
- **Expected Result**: Database lifecycle operations work

#### `CreateCollection`
- **Purpose**: Create collections in a database
- **Steps**:
  1. Create a database if it doesn't exist
  2. Create multiple collections
  3. Verify each collection creation returns 201
  4. Delete the database
- **Expected Result**: Collections are created successfully

#### `Example`
- **Purpose**: Complete CRUD example
- **Steps**:
  1. List databases
  2. Get first database
  3. List collections in database
  4. Get first collection
  5. Create a document with unique ID
  6. Delete the document
- **Expected Result**: Full CRUD cycle works

#### `ListDatabases`
- **Purpose**: List all databases
- **Steps**:
  1. Call listDatabases()
  2. Verify status code is 200
  3. Verify response contains "Databases" array
- **Expected Result**: Databases are listed

#### `ListCollections`
- **Purpose**: List collections in each database
- **Steps**:
  1. List all databases
  2. For each database, list its collections
  3. Verify each response contains "DocumentCollections" array
- **Expected Result**: Collections are listed for each database

#### `ListDocuments`
- **Purpose**: List documents with pagination
- **Steps**:
  1. Get first database and collection
  2. List documents with continuation token
  3. Iterate through pages (max 7 iterations)
  4. Verify each page has at least 1 document
- **Expected Result**: Document pagination works

#### `CreateDocument`
- **Purpose**: Create and delete a document
- **Steps**:
  1. Get database and collection
  2. Create a document with unique ID
  3. Verify status code is 201
  4. Delete the document
  5. Verify status code is 204
- **Expected Result**: Document creation and deletion work

#### `CreateDocument_MissingId`
- **Purpose**: Verify validation of required fields
- **Steps**:
  1. Attempt to create document without "id" field
  2. Expect std::invalid_argument exception
- **Expected Result**: Missing ID is caught

#### `CreateDocument_MissingPkId`
- **Purpose**: Verify partition key validation
- **Steps**:
  1. Attempt to create document without partition key
  2. Expect std::invalid_argument exception
- **Expected Result**: Missing partition key is caught

#### `FindDocument`
- **Purpose**: Create and retrieve a document
- **Steps**:
  1. Create a document
  2. Find the document by ID and partition key
  3. Verify status code is 200
  4. Verify document ID matches
  5. Delete the document
- **Expected Result**: Document retrieval works

#### `UpsertDocument`
- **Purpose**: Test upsert (insert or update) operation
- **Steps**:
  1. Upsert a new document (should insert, status 201)
  2. Upsert the same document with different data (should update, status 200)
  3. Attempt to create the same document (should fail with 409)
  4. Delete the document
- **Expected Result**: Upsert works for both insert and update

#### `UpdateDocument`
- **Purpose**: Update an existing document
- **Steps**:
  1. Create a document with initial data
  2. Modify the document
  3. Update the document
  4. Verify status code is 200
  5. Retrieve the document to verify changes
  6. Delete the document
- **Expected Result**: Document update works

#### `QueryDocument_odd` and `QueryDocument_even`
- **Purpose**: Query documents with parameters
- **Steps**:
  1. Query documents where source contains "odd" or "even"
  2. Use continuation tokens for pagination
  3. Collect all results
  4. Verify count matches expected (5 documents)
- **Expected Result**: Parameterized queries work with pagination

#### `MoveConstruct`
- **Purpose**: Verify move semantics
- **Steps**:
  1. Create vector of CosmosClient instances
  2. Add clients to vector
  3. Verify vector size is correct
- **Expected Result**: Move construction works

#### `ConfigureMulti`
- **Purpose**: Configure multiple clients concurrently
- **Steps**:
  1. Create 4 CosmosClient instances
  2. Configure each with connection strings
  3. Verify each client is properly configured
  4. Check service settings for each
- **Expected Result**: Multiple clients can be configured

#### `CreateDocumentThreaded`
- **Purpose**: Test concurrent document creation
- **Steps**:
  1. Create N threads (hardware concurrency)
  2. Each thread creates 15 documents
  3. Use barrier to synchronize thread start
  4. Each thread deletes its created documents
  5. Verify all documents were created and deleted
- **Expected Result**: Concurrent operations work correctly

---

### 5. COMPREHENSIVE API TESTS (Requires Emulator)

These tests validate advanced features and edge cases.

#### `InvalidConnectionStringFormat`
- **Purpose**: Verify invalid connection string handling
- **Steps**:
  1. Create endpoint with invalid connection string
  2. Verify endpoint is invalid (bool conversion returns false)
- **Expected Result**: Invalid connection strings are rejected

#### `ClientDefaultConfiguration`
- **Purpose**: Verify default client configuration
- **Steps**:
  1. Create new client
  2. Verify API version is "2018-12-31"
  3. Verify retry limit is 7
  4. Verify connection strings and partition keys are present
- **Expected Result**: Default configuration is correct

#### `CreateDatabaseBasic`
- **Purpose**: Create a database with unique name
- **Steps**:
  1. Generate unique database name
  2. Create database
  3. Verify status code is 201
  4. Verify database ID matches
  5. Delete database
- **Expected Result**: Database creation works

#### `CreateDatabaseDuplicate`
- **Purpose**: Verify duplicate database prevention
- **Steps**:
  1. Create a database
  2. Attempt to create same database again
  3. Verify first creation returns 201
  4. Verify second creation returns 409 (conflict)
  5. Delete database
- **Expected Result**: Duplicate databases are prevented

#### `ListDatabasesNotEmpty`
- **Purpose**: Verify database list is not empty
- **Steps**:
  1. List databases
  2. Verify status code is 200
  3. Verify Databases array exists
  4. Verify at least 1 database exists
- **Expected Result**: Database list contains test database

#### `FindDatabaseExists`
- **Purpose**: Find a specific database
- **Steps**:
  1. Find test database by name
  2. Verify status code is 200
  3. Verify database ID matches
- **Expected Result**: Database is found

#### `FindDatabaseNotFound`
- **Purpose**: Verify 404 for non-existent database
- **Steps**:
  1. Generate non-existent database name
  2. Attempt to find database
  3. Verify status code is 404
- **Expected Result**: Non-existent database returns 404

#### `DeleteDatabaseSuccess`
- **Purpose**: Delete a database
- **Steps**:
  1. Create a database
  2. Delete the database
  3. Verify status code is 204
  4. Attempt to find deleted database
  5. Verify status code is 404
- **Expected Result**: Database deletion works

#### `CreateCollectionBasic`
- **Purpose**: Create a collection
- **Steps**:
  1. Generate unique collection name
  2. Create collection
  3. Verify status code is 201
  4. Verify collection ID matches
- **Expected Result**: Collection creation works

#### `CreateCollectionDuplicate`
- **Purpose**: Verify duplicate collection prevention
- **Steps**:
  1. Create a collection
  2. Attempt to create same collection again
  3. Verify first creation returns 201
  4. Verify second creation returns 409
- **Expected Result**: Duplicate collections are prevented

#### `ListCollectionsNotEmpty`
- **Purpose**: Verify collection list is not empty
- **Steps**:
  1. List collections in test database
  2. Verify status code is 200
  3. Verify DocumentCollections array exists
  4. Verify at least 1 collection exists
- **Expected Result**: Collection list contains test collections

#### `CreateDocumentMinimal`
- **Purpose**: Create document with minimal fields
- **Steps**:
  1. Create document with only id and partition key
  2. Verify status code is 201
  3. Verify document ID matches
  4. Delete document
- **Expected Result**: Minimal document creation works

#### `CreateDocumentWithComplexStructure`
- **Purpose**: Create document with nested structures
- **Steps**:
  1. Create document with:
     - Nested objects (multiple levels)
     - Arrays
     - Boolean values
     - Floating point numbers
     - Null values
  2. Verify status code is 201
  3. Verify nested values are preserved
  4. Delete document
- **Expected Result**: Complex document structures work

#### `CreateDocumentMissingId`
- **Purpose**: Verify ID validation
- **Steps**:
  1. Attempt to create document without ID
  2. Expect std::invalid_argument exception
- **Expected Result**: Missing ID is caught

#### `CreateDocumentMissingPartitionKey`
- **Purpose**: Verify partition key validation
- **Steps**:
  1. Attempt to create document without partition key
  2. Expect std::invalid_argument exception
- **Expected Result**: Missing partition key is caught

#### `FindDocumentExists`
- **Purpose**: Find and retrieve a document
- **Steps**:
  1. Create a document
  2. Find the document
  3. Verify status code is 200
  4. Verify document ID matches
  5. Delete document
- **Expected Result**: Document retrieval works

#### `FindDocumentNotFound`
- **Purpose**: Verify 404 for non-existent document
- **Steps**:
  1. Attempt to find non-existent document
  2. Verify status code is 404
- **Expected Result**: Non-existent document returns 404

#### `UpdateDocumentBasic`
- **Purpose**: Update document fields
- **Steps**:
  1. Create a document
  2. Modify document fields
  3. Update the document
  4. Verify status code is 200
  5. Verify changes are persisted
  6. Delete document
- **Expected Result**: Document update works

#### `RemoveDocumentSuccess`
- **Purpose**: Delete a document
- **Steps**:
  1. Create a document
  2. Delete the document
  3. Verify status code is 204
  4. Attempt to find deleted document
  5. Verify status code is 404
- **Expected Result**: Document deletion works

#### `RemoveNonexistentDocument`
- **Purpose**: Verify 404 when deleting non-existent document
- **Steps**:
  1. Attempt to delete non-existent document
  2. Verify status code is 404
- **Expected Result**: Deleting non-existent document returns 404

#### `QueryDocumentsSimple`
- **Purpose**: Execute a simple query
- **Steps**:
  1. Query documents with WHERE clause
  2. Verify status code is 200
  3. Verify Documents array exists
- **Expected Result**: Simple query works

#### `QueryDocumentsWithPagination`
- **Purpose**: Query with continuation tokens
- **Steps**:
  1. Query documents with continuation token
  2. Iterate through pages (max 10)
  3. Accumulate document count
  4. Verify total count >= seeded documents
- **Expected Result**: Pagination works

#### `ListDocumentsBasic`
- **Purpose**: List documents in collection
- **Steps**:
  1. List documents
  2. Verify status code is 200
  3. Verify Documents array exists
  4. Verify at least 1 document exists
- **Expected Result**: Document listing works

#### `DiscoverRegionsSuccess`
- **Purpose**: Discover available regions
- **Steps**:
  1. Call discoverRegions()
  2. Verify status code is 200
  3. Verify writableLocations exists
  4. Verify readableLocations exists
- **Expected Result**: Region discovery works

#### Response Type Tests
- **CosmosResponseTypeSuccess**: Verify 200 status is success
- **CosmosResponseTypeCreated**: Verify 201 status is success
- **CosmosResponseTypeNoContent**: Verify 204 status is success
- **CosmosResponseTypeClientError**: Verify 400 status is failure
- **CosmosResponseTypeNotFound**: Verify 404 status is failure
- **CosmosResponseTypeServerError**: Verify 500 status is failure

#### Endpoint Tests
- **EndpointReadUriRotation**: Verify read URI cycles through all URIs
- **EndpointWriteUriRotation**: Verify write URI cycles through all URIs
- **EndpointFallbackToBaseUri**: Verify fallback when no URIs configured

#### Error Handling Tests
- **InvalidConfigurationMissingConnectionStrings**: Verify exception when connection strings missing
- **InvalidConfigurationMissingPartitionKeyNames**: Verify exception when partition keys missing

#### Data Type Tests
- **DocumentWithVariousDataTypes**: Test all JSON data types (strings, numbers, booleans, arrays, objects, null)
- **DocumentWithSpecialCharacters**: Test Unicode, escape sequences, special characters
- **DocumentWithLargeContent**: Test large arrays and strings

#### Bulk Operation Tests
- **BulkCreateDocuments**: Create 10 documents in sequence
- **BulkUpdateDocuments**: Update 5 documents in sequence

#### Concurrency Tests
- **ConcurrentDocumentCreation**: Create documents from 4 threads concurrently

---

### 6. ADDITIONAL AZURE COSMOS REST API FEATURE TESTS

These tests validate advanced Azure Cosmos DB features.

#### TTL and System Properties Tests

**DocumentTTLExpiration**
- **Purpose**: Verify TTL field support
- **Steps**:
  1. Create document with TTL=1 second
  2. Verify TTL field is preserved
- **Expected Result**: TTL field is stored

**DocumentWithTimestamp**
- **Purpose**: Verify _ts system property
- **Steps**:
  1. Create document
  2. Verify _ts field exists
  3. Verify _ts value is > 0
- **Expected Result**: Timestamp is automatically set

**DocumentWithETag**
- **Purpose**: Verify _etag system property
- **Steps**:
  1. Create document
  2. Verify _etag field exists
  3. Verify _etag is not empty
- **Expected Result**: ETag is automatically set

**DocumentWithRID**
- **Purpose**: Verify _rid system property
- **Steps**:
  1. Create document
  2. Verify _rid field exists
  3. Verify _rid is not empty
- **Expected Result**: Resource ID is automatically set

**DocumentWithSystemProperties**
- **Purpose**: Verify all system properties
- **Steps**:
  1. Create document
  2. Verify _rid, _self, _etag, _attachments, _ts all exist
- **Expected Result**: All system properties are present

#### Query Feature Tests

**QueryWithOrderBy**
- **Purpose**: Test ORDER BY clause
- **Steps**:
  1. Create 5 documents with priority values
  2. Query with ORDER BY priority DESC
  3. Verify status code is 200
  4. Verify Documents array exists
- **Expected Result**: ORDER BY works

**QueryWithAggregation**
- **Purpose**: Test COUNT and SUM aggregations
- **Steps**:
  1. Create 5 documents with numeric values
  2. Query with COUNT and SUM
  3. Verify status code is 200
  4. Verify aggregation results exist
- **Expected Result**: Aggregations work

**QueryWithDistinct**
- **Purpose**: Test DISTINCT keyword
- **Steps**:
  1. Create 6 documents with 3 unique categories
  2. Query with DISTINCT
  3. Verify status code is 200
- **Expected Result**: DISTINCT works

**QueryWithStringFunctions**
- **Purpose**: Test UPPER, LOWER, LENGTH functions
- **Steps**:
  1. Create document with text field
  2. Query using string functions
  3. Verify status code is 200
- **Expected Result**: String functions work

**QueryWithMathFunctions**
- **Purpose**: Test ROUND, FLOOR, CEILING functions
- **Steps**:
  1. Create document with numeric value
  2. Query using math functions
  3. Verify status code is 200
- **Expected Result**: Math functions work

**QueryWithArrayContains**
- **Purpose**: Test ARRAY_CONTAINS function
- **Steps**:
  1. Create document with array field
  2. Query using ARRAY_CONTAINS
  3. Verify status code is 200
- **Expected Result**: ARRAY_CONTAINS works

**QueryWithExists**
- **Purpose**: Test EXISTS function
- **Steps**:
  1. Create documents with and without optional field
  2. Query using EXISTS
  3. Verify status code is 200
- **Expected Result**: EXISTS works

**QueryWithIn**
- **Purpose**: Test IN operator
- **Steps**:
  1. Create documents with different status values
  2. Query using IN operator
  3. Verify status code is 200
- **Expected Result**: IN operator works

**QueryWithBetween**
- **Purpose**: Test BETWEEN operator
- **Steps**:
  1. Create documents with score values 10-100
  2. Query using BETWEEN 30 AND 70
  3. Verify status code is 200
- **Expected Result**: BETWEEN operator works

#### Upsert Tests

**UpsertDocumentInsert**
- **Purpose**: Test upsert as insert
- **Steps**:
  1. Upsert new document
  2. Verify status code is 201
  3. Verify operation field is "insert"
- **Expected Result**: Upsert inserts new documents

**UpsertDocumentUpdate**
- **Purpose**: Test upsert as update
- **Steps**:
  1. Create document
  2. Upsert same document with new data
  3. Verify status code is 200
  4. Verify operation field is "upsert"
- **Expected Result**: Upsert updates existing documents

#### Partition Key Tests

**PartitionKeyRangeQuery**
- **Purpose**: Query with specific partition key
- **Steps**:
  1. Create 3 documents with same partition key
  2. Query with specific partition key
  3. Verify status code is 200
- **Expected Result**: Partition key query works

**CrossPartitionQuery**
- **Purpose**: Query across all partitions
- **Steps**:
  1. Create 3 documents
  2. Query with "*" partition key
  3. Verify status code is 200
- **Expected Result**: Cross-partition query works

#### Pagination Tests

**QueryWithLimit**
- **Purpose**: Test TOP clause
- **Steps**:
  1. Create 10 documents
  2. Query with TOP 5
  3. Verify count <= 5
- **Expected Result**: TOP clause limits results

**QueryWithOffset**
- **Purpose**: Test OFFSET and LIMIT
- **Steps**:
  1. Create 10 documents
  2. Query with OFFSET 5 LIMIT 5
  3. Verify status code is 200
- **Expected Result**: OFFSET and LIMIT work

---

## Running the Tests

### Prerequisites

1. **Azure Cosmos DB Emulator** or **Live Azure Cosmos DB Instance**
2. **Environment Variables**:
   - `CCTEST_PRIMARY_CS`: Primary connection string
   - `CCTEST_SECONDARY_CS`: Secondary connection string (optional)

### Build and Run

```bash
# Configure with tests enabled
cmake --preset Apple-Debug  # or Windows-Debug, Linux-Clang-Debug, etc.

# Build
cmake --build --preset Apple-Debug

# Run tests
ctest --preset Apple-Debug
```

### Test Output

- **XML Results**: `tests/results/debug_test_detail.xml` or `release_test_detail.xml`
- **Console Output**: Detailed test execution logs
- **Timeout**: 1800 seconds (30 minutes) for entire suite

---

## Test Statistics

| Category | Count | Requires Emulator |
|----------|-------|-------------------|
| Validation | 6 | No |
| Connection | 5 | No |
| Endpoint | 2 | No |
| Integration | 20+ | Yes |
| Comprehensive API | 40+ | Yes |
| Additional Features | 20+ | Yes |
| **Total** | **90+** | - |

---

## Key Features Tested

✅ **Document Management** - CRUD operations
✅ **System Properties** - _ts, _etag, _rid, _self, _attachments
✅ **TTL** - Document expiration
✅ **Query Language** - SELECT, WHERE, ORDER BY, DISTINCT, aggregations, functions
✅ **Partition Keys** - Single and cross-partition queries
✅ **Data Types** - All JSON types
✅ **Error Handling** - Validation and error codes
✅ **Concurrency** - Multi-threaded operations
✅ **Pagination** - Continuation tokens
✅ **Upsert** - Insert or update semantics

---

## Notes

- All tests use a unified fixture with shared setup/teardown
- Tests run sequentially (CTEST_PARALLEL_LEVEL=1)
- Each test is independent and can be run individually
- Failed tests provide detailed error messages
- Test database is cleaned up after all tests complete
