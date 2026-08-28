// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iostream>
#include <cassert>
#include <filesystem>
#include <cstdio>

#include "sqlite_engine.h"
#include "sqlite3.h"

namespace fs = std::filesystem;

void CreateTestDatabase(const char* dbPath)
{
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath, &db);
    assert(rc == SQLITE_OK);

    const char* sql = R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            email TEXT,
            score REAL DEFAULT 0.0,
            avatar BLOB,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE posts (
            post_id INTEGER PRIMARY KEY,
            user_id INTEGER REFERENCES users(id),
            title TEXT NOT NULL,
            body TEXT
        );

        CREATE INDEX idx_users_username ON users(username);
        CREATE INDEX idx_posts_user_id ON posts(user_id);

        CREATE VIEW v_user_posts AS
            SELECT u.id, u.username, p.title
            FROM users u
            JOIN posts p ON u.id = p.user_id;

        INSERT INTO users (username, email, score, avatar) VALUES
            ('alice', 'alice@example.com', 95.5, X'01020304'),
            ('bob', 'bob@example.com', 82.0, NULL),
            ('charlie', NULL, 64.25, X'DEADBEEF'),
            ('david', 'david@example.com', 77.0, NULL),
            ('eve', 'eve@example.com', 99.9, X'AABBCCDD');

        INSERT INTO posts (post_id, user_id, title, body) VALUES
            (1, 1, 'First Post', 'Hello World from Alice!'),
            (2, 1, 'Second Post', 'Another post from Alice.'),
            (3, 2, 'Bob Post', 'Hello from Bob.');
    )";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error in setup: " << (errMsg ? errMsg : "") << std::endl;
        sqlite3_free(errMsg);
    }
    assert(rc == SQLITE_OK);

    sqlite3_close(db);
}

int main()
{
    std::cout << "=== Running SQLite Engine Tests ===" << std::endl;

    const char* testDbPath = "test_sample.db";
    if (fs::exists(testDbPath))
    {
        fs::remove(testDbPath);
    }

    CreateTestDatabase(testDbPath);

    // Test 1: Signature detection
    std::cout << "[Test 1] Checking IsSqliteDatabase signature detection... ";
    assert(SqliteEngine::CSqliteEngine::IsSqliteDatabase(testDbPath) == true);
    assert(SqliteEngine::CSqliteEngine::IsSqliteDatabase("non_existent_file.xyz") == false);
    std::cout << "PASSED" << std::endl;

    // Test 2: Opening database
    std::cout << "[Test 2] Opening database... ";
    SqliteEngine::CSqliteEngine engine;
    std::string err;
    bool ok = engine.Open(testDbPath, err, true);
    if (!ok)
    {
        std::cerr << "Failed to open: " << err << std::endl;
    }
    assert(ok == true);
    assert(engine.IsOpen() == true);
    assert(engine.GetTables().size() == 3); // 2 tables + 1 view
    std::cout << "PASSED (Found " << engine.GetTables().size() << " tables/views)" << std::endl;

    // Test 3: Table schema inspection
    std::cout << "[Test 3] Inspecting schema of 'users' table... ";
    std::vector<SqliteEngine::ColumnInfo> cols;
    std::vector<SqliteEngine::IndexInfo> idxs;
    std::vector<SqliteEngine::ForeignKeyInfo> fks;
    std::string ddl;
    ok = engine.GetTableSchema("users", cols, idxs, fks, ddl, err);
    assert(ok == true);
    assert(cols.size() == 6);
    assert(cols[0].name == "id" && cols[0].isPrimaryKey == true);
    assert(cols[1].name == "username" && cols[1].notNull == true);
    assert(idxs.size() == 1);
    assert(idxs[0].name == "idx_users_username");
    assert(!ddl.empty());
    std::cout << "PASSED (Columns: " << cols.size() << ", Indexes: " << idxs.size() << ")" << std::endl;

    // Test 4: Foreign keys
    std::cout << "[Test 4] Inspecting foreign keys of 'posts' table... ";
    cols.clear(); idxs.clear(); fks.clear(); ddl.clear();
    ok = engine.GetTableSchema("posts", cols, idxs, fks, ddl, err);
    assert(ok == true);
    assert(fks.size() == 1);
    assert(fks[0].table == "users");
    assert(fks[0].from == "user_id");
    assert(fks[0].to == "id");
    std::cout << "PASSED (FK to table " << fks[0].table << ")" << std::endl;

    // Test 5: Paged data loading
    std::cout << "[Test 5] Loading paged table data (pageSize=2, offset=0)... ";
    auto page1 = engine.LoadTablePage("users", 2, 0);
    assert(page1.success == true);
    assert(page1.rows.size() == 2);
    assert(page1.totalRows == 5);
    assert(page1.hasMore == true);
    assert(page1.rows[0][1].text == "alice");
    assert(page1.rows[1][1].text == "bob");
    std::cout << "PASSED (Loaded " << page1.rows.size() << " rows)" << std::endl;

    // Test 6: Next page
    std::cout << "[Test 6] Loading next page (pageSize=2, offset=2)... ";
    auto page2 = engine.LoadTablePage("users", 2, 2);
    assert(page2.success == true);
    assert(page2.rows.size() == 2);
    assert(page2.rows[0][1].text == "charlie");
    assert(page2.rows[0][2].isNull == true); // charlie's email is NULL
    assert(page2.rows[1][1].text == "david");
    std::cout << "PASSED" << std::endl;

    // Test 7: Sorting
    std::cout << "[Test 7] Loading sorted data (sort by username DESC)... ";
    // Col 1 is username
    auto sortedPage = engine.LoadTablePage("users", 5, 0, 1, true);
    assert(sortedPage.success == true);
    assert(sortedPage.rows.size() == 5);
    assert(sortedPage.rows[0][1].text == "eve");
    assert(sortedPage.rows[4][1].text == "alice");
    std::cout << "PASSED (First: " << sortedPage.rows[0][1].text << ", Last: " << sortedPage.rows[4][1].text << ")" << std::endl;

    // Test 8: In-table search/filtering
    std::cout << "[Test 8] Loading filtered data (filter='alice')... ";
    auto filterPage = engine.LoadTablePage("users", 10, 0, -1, false, "alice");
    assert(filterPage.success == true);
    assert(filterPage.rows.size() == 1);
    assert(filterPage.rows[0][1].text == "alice");
    std::cout << "PASSED" << std::endl;

    // Test 9: Custom SQL Query Execution
    std::cout << "[Test 9] Executing custom SQL query... ";
    auto queryRes = engine.ExecuteQuery("SELECT u.username, COUNT(p.post_id) AS post_count FROM users u LEFT JOIN posts p ON u.id = p.user_id GROUP BY u.id ORDER BY post_count DESC;");
    assert(queryRes.success == true);
    assert(queryRes.rows.size() == 5);
    assert(queryRes.rows[0][0].text == "alice" && queryRes.rows[0][1].text == "2");
    std::cout << "PASSED (Query executed in " << queryRes.elapsedMs << " ms)" << std::endl;

    // Test 10: CSV export
    std::cout << "[Test 10] Exporting table to CSV... ";
    const char* exportPath = "test_export.csv";
    if (fs::exists(exportPath)) fs::remove(exportPath);
    ok = engine.ExportTableToCsv("users", exportPath, err);
    assert(ok == true);
    assert(fs::exists(exportPath));
    assert(fs::file_size(exportPath) > 0);
    std::cout << "PASSED (Export size: " << fs::file_size(exportPath) << " bytes)" << std::endl;

    engine.Close();

    // Clean up test files
    if (fs::exists(testDbPath)) fs::remove(testDbPath);
    if (fs::exists(exportPath)) fs::remove(exportPath);

    std::cout << "\n>>> ALL 10 TESTS PASSED SUCCESSFULLY! <<<\n" << std::endl;
    return 0;
}
