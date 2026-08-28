// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

struct sqlite3;
struct sqlite3_stmt;

namespace SqliteEngine
{

struct TableInfo
{
    std::string name;
    std::string type; // "table" or "view"
    std::string ddl;
    int64_t rowCount = 0;
    bool isView = false;
};

struct ColumnInfo
{
    int cid = 0;
    std::string name;
    std::string type;
    bool notNull = false;
    std::string defaultValue;
    bool isPrimaryKey = false;
};

struct IndexInfo
{
    std::string name;
    bool unique = false;
    std::string sql;
};

struct ForeignKeyInfo
{
    int id = 0;
    int seq = 0;
    std::string table;
    std::string from;
    std::string to;
    std::string onUpdate;
    std::string onDelete;
};

struct CellValue
{
    bool isNull = false;
    bool isBlob = false;
    std::string text;
    size_t blobBytes = 0;
    std::vector<uint8_t> blobData;
    std::string blobKind;
    bool isNumeric = false;
};

struct QueryPage
{
    std::vector<std::string> columnNames;
    std::vector<std::string> columnTypes;
    std::vector<std::vector<CellValue>> rows;
    int64_t rowOffset = 0;
    int64_t totalRows = 0;
    bool hasMore = false;
    double elapsedMs = 0.0;
    std::string error;
    bool success = true;
};

class CSqliteEngine
{
public:
    CSqliteEngine();
    ~CSqliteEngine();

    CSqliteEngine(const CSqliteEngine&) = delete;
    CSqliteEngine& operator=(const CSqliteEngine&) = delete;

    bool Open(const char* filePath, std::string& outError, bool directOpen = true);
    void Close();
    bool IsOpen() const { return m_db != nullptr; }

    const std::string& GetFilePath() const { return m_filePath; }
    int64_t GetFileSize() const { return m_fileSize; }
    const std::string& GetSqliteVersion() const { return m_sqliteVersion; }
    int GetPageSize() const { return m_pageSize; }
    const std::vector<TableInfo>& GetTables() const { return m_tables; }

    bool RefreshTables(std::string& outError);
    bool GetTableSchema(const std::string& tableName,
                        std::vector<ColumnInfo>& outCols,
                        std::vector<IndexInfo>& outIdxs,
                        std::vector<ForeignKeyInfo>& outFks,
                        std::string& outDdl,
                        std::string& outError);

    QueryPage LoadTablePage(const std::string& tableName,
                            int pageSize,
                            int64_t rowOffset,
                            int sortColIndex = -1,
                            bool sortDesc = false,
                            const std::string& filter = "");

    QueryPage ExecuteQuery(const std::string& sql, int rowCap = 5000);

    bool ExportTableToCsv(const std::string& tableName, const char* outFilePath, std::string& outError, const std::string& filter = "");

    static std::string FormatRowsAsCsv(const QueryPage& page, bool includeHeaders = true);
    static std::string FormatRowsAsTsv(const QueryPage& page, bool includeHeaders = true);
    static bool IsSqliteDatabase(const char* filePath);
    static bool TestFileAccess(const char* filePath, std::string& outError);
    static std::string FormatByteSize(size_t bytes);
    static std::string DetectBlobKind(const uint8_t* data, size_t size);
    static std::string FormatHexDump(const uint8_t* data, size_t size, size_t maxBytes = 65536);

private:
    sqlite3* m_db;
    std::string m_filePath;
    std::string m_tempSnapshotPath;
    int64_t m_fileSize;
    std::string m_sqliteVersion;
    int m_pageSize;
    std::vector<TableInfo> m_tables;

    static std::string EscapeIdentifier(const std::string& identifier);
    static std::string EscapeCsvCell(const std::string& text);
    int64_t QueryTableCount(const std::string& tableName, const std::string& filter = "");
    void CleanTempSnapshot();
};

} // namespace SqliteEngine
