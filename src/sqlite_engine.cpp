// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sqlite_engine.h"

#include <fstream>
#include <iomanip>

namespace SqliteEngine
{

static const char SQLITE_SIGNATURE[] = "SQLite format 3\000";

CSqliteEngine::CSqliteEngine()
    : m_db(nullptr)
    , m_fileSize(0)
    , m_pageSize(4096)
{
    m_sqliteVersion = sqlite3_libversion();
}

CSqliteEngine::~CSqliteEngine()
{
    Close();
}

bool CSqliteEngine::TestFileAccess(const char* filePath, std::string& outError)
{
    if (!filePath || !*filePath)
    {
        outError = "File path is empty.";
        return false;
    }

    HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = GetLastError();
        char sysMsg[512] = {0};
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       sysMsg, sizeof(sysMsg), NULL);

        size_t len = strlen(sysMsg);
        while (len > 0 && (sysMsg[len - 1] == '\r' || sysMsg[len - 1] == '\n' || sysMsg[len - 1] == ' '))
        {
            sysMsg[--len] = '\0';
        }

        char buf[512];
        if (sysMsg[0] != '\0')
            snprintf(buf, sizeof(buf), "(%lu) %s", dwErr, sysMsg);
        else
            snprintf(buf, sizeof(buf), "Error %lu: Access denied or file is locked.", dwErr);

        outError = buf;
        return false;
    }

    char header[16] = {0};
    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, header, sizeof(header), &bytesRead, NULL);
    CloseHandle(hFile);

    if (!ok || bytesRead < 16)
    {
        outError = "File is too small or cannot read header.";
        return false;
    }

    if (memcmp(header, SQLITE_SIGNATURE, 16) != 0)
    {
        outError = "File does not contain a valid SQLite 3 signature header.";
        return false;
    }

    return true;
}

bool CSqliteEngine::IsSqliteDatabase(const char* filePath)
{
    std::string err;
    return TestFileAccess(filePath, err);
}

void CSqliteEngine::CleanTempSnapshot()
{
    if (!m_tempSnapshotPath.empty())
    {
        DeleteFileA(m_tempSnapshotPath.c_str());
        m_tempSnapshotPath.clear();
    }
}

bool CSqliteEngine::Open(const char* filePath, std::string& outError, bool directOpen)
{
    Close();

    if (!TestFileAccess(filePath, outError))
    {
        return false;
    }

    m_filePath = filePath;

    // Get file size
    WIN32_FILE_ATTRIBUTE_DATA fileAttr;
    if (GetFileAttributesExA(filePath, GetFileExInfoStandard, &fileAttr))
    {
        LARGE_INTEGER li;
        li.LowPart = fileAttr.nFileSizeLow;
        li.HighPart = fileAttr.nFileSizeHigh;
        m_fileSize = li.QuadPart;
    }
    else
    {
        m_fileSize = 0;
    }

    std::string pathToOpen = filePath;

    // Open database in immutable / read-only mode
    std::string uri = "file:" + pathToOpen + "?mode=ro&immutable=1";
    int rc = sqlite3_open_v2(uri.c_str(), &m_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL);
    if (rc != SQLITE_OK)
    {
        // Fallback to standard readonly open without immutable flag
        uri = "file:" + pathToOpen + "?mode=ro";
        rc = sqlite3_open_v2(uri.c_str(), &m_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL);
    }
    if (rc != SQLITE_OK)
    {
        // Standard open
        rc = sqlite3_open_v2(pathToOpen.c_str(), &m_db, SQLITE_OPEN_READONLY, NULL);
    }

    if (rc != SQLITE_OK || !m_db)
    {
        outError = m_db ? sqlite3_errmsg(m_db) : "Failed to open SQLite database.";
        Close();
        return false;
    }

    // Set read-only safety pragma
    sqlite3_exec(m_db, "PRAGMA query_only = ON;", NULL, NULL, NULL);

    // Read page size
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "PRAGMA page_size;", -1, &stmt, NULL) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            m_pageSize = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return RefreshTables(outError);
}

void CSqliteEngine::Close()
{
    if (m_db)
    {
        sqlite3_close_v2(m_db);
        m_db = nullptr;
    }
    m_tables.clear();
    m_filePath.clear();
    m_fileSize = 0;
    CleanTempSnapshot();
}

bool CSqliteEngine::RefreshTables(std::string& outError)
{
    m_tables.clear();
    if (!m_db)
    {
        outError = "Database is not open.";
        return false;
    }

    const char* sql =
        "SELECT name, type, sql FROM sqlite_master "
        "WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite_%' "
        "ORDER BY type ASC, name ASC;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        outError = sqlite3_errmsg(m_db);
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        TableInfo t;
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        const char* type = (const char*)sqlite3_column_text(stmt, 1);
        const char* ddl = (const char*)sqlite3_column_text(stmt, 2);

        t.name = name ? name : "";
        t.type = type ? type : "table";
        t.ddl = ddl ? ddl : "";
        t.isView = (t.type == "view");
        t.rowCount = QueryTableCount(t.name);

        m_tables.push_back(t);
    }

    sqlite3_finalize(stmt);
    return true;
}

std::string CSqliteEngine::EscapeIdentifier(const std::string& identifier)
{
    std::string escaped = "\"";
    for (char c : identifier)
    {
        if (c == '"')
            escaped += "\"\"";
        else
            escaped += c;
    }
    escaped += "\"";
    return escaped;
}

int64_t CSqliteEngine::QueryTableCount(const std::string& tableName, const std::string& filter)
{
    if (!m_db || tableName.empty())
        return 0;

    std::string sql = "SELECT COUNT(*) FROM " + EscapeIdentifier(tableName);
    if (!filter.empty())
    {
        // Add basic filter condition if needed
    }
    sql += ";";

    int64_t count = 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

bool CSqliteEngine::GetTableSchema(const std::string& tableName,
                                   std::vector<ColumnInfo>& outCols,
                                   std::vector<IndexInfo>& outIdxs,
                                   std::vector<ForeignKeyInfo>& outFks,
                                   std::string& outDdl,
                                   std::string& outError)
{
    outCols.clear();
    outIdxs.clear();
    outFks.clear();
    outDdl.clear();

    if (!m_db)
    {
        outError = "Database not open.";
        return false;
    }

    // 1. Get DDL from sqlite_master
    std::string ddlSql = "SELECT sql FROM sqlite_master WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, ddlSql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char* ddl = (const char*)sqlite3_column_text(stmt, 0);
            if (ddl) outDdl = ddl;
        }
        sqlite3_finalize(stmt);
    }

    // 2. PRAGMA table_info
    std::string pragmaColSql = "PRAGMA table_info(" + EscapeIdentifier(tableName) + ");";
    if (sqlite3_prepare_v2(m_db, pragmaColSql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ColumnInfo c;
            c.cid = sqlite3_column_int(stmt, 0);
            const char* colName = (const char*)sqlite3_column_text(stmt, 1);
            const char* colType = (const char*)sqlite3_column_text(stmt, 2);
            c.name = colName ? colName : "";
            c.type = colType ? colType : "";
            c.notNull = (sqlite3_column_int(stmt, 3) != 0);
            const char* defVal = (const char*)sqlite3_column_text(stmt, 4);
            c.defaultValue = defVal ? defVal : "";
            c.isPrimaryKey = (sqlite3_column_int(stmt, 5) != 0);
            outCols.push_back(c);
        }
        sqlite3_finalize(stmt);
    }

    // 3. PRAGMA index_list
    std::string pragmaIdxSql = "PRAGMA index_list(" + EscapeIdentifier(tableName) + ");";
    if (sqlite3_prepare_v2(m_db, pragmaIdxSql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            IndexInfo idx;
            const char* idxName = (const char*)sqlite3_column_text(stmt, 1);
            idx.name = idxName ? idxName : "";
            idx.unique = (sqlite3_column_int(stmt, 2) != 0);
            outIdxs.push_back(idx);
        }
        sqlite3_finalize(stmt);
    }

    // Index DDLs
    for (auto& idx : outIdxs)
    {
        std::string idxDdlSql = "SELECT sql FROM sqlite_master WHERE type='index' AND name = ?;";
        if (sqlite3_prepare_v2(m_db, idxDdlSql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, idx.name.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const char* isql = (const char*)sqlite3_column_text(stmt, 0);
                if (isql) idx.sql = isql;
            }
            sqlite3_finalize(stmt);
        }
    }

    // 4. PRAGMA foreign_key_list
    std::string pragmaFkSql = "PRAGMA foreign_key_list(" + EscapeIdentifier(tableName) + ");";
    if (sqlite3_prepare_v2(m_db, pragmaFkSql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ForeignKeyInfo fk;
            fk.id = sqlite3_column_int(stmt, 0);
            fk.seq = sqlite3_column_int(stmt, 1);
            const char* fkTable = (const char*)sqlite3_column_text(stmt, 2);
            const char* fkFrom = (const char*)sqlite3_column_text(stmt, 3);
            const char* fkTo = (const char*)sqlite3_column_text(stmt, 4);
            const char* onUpd = (const char*)sqlite3_column_text(stmt, 5);
            const char* onDel = (const char*)sqlite3_column_text(stmt, 6);

            fk.table = fkTable ? fkTable : "";
            fk.from = fkFrom ? fkFrom : "";
            fk.to = fkTo ? fkTo : "";
            fk.onUpdate = onUpd ? onUpd : "";
            fk.onDelete = onDel ? onDel : "";
            outFks.push_back(fk);
        }
        sqlite3_finalize(stmt);
    }

    return true;
}

QueryPage CSqliteEngine::LoadTablePage(const std::string& tableName,
                                       int pageSize,
                                       int64_t rowOffset,
                                       int sortColIndex,
                                       bool sortDesc,
                                       const std::string& filter)
{
    QueryPage page;
    page.rowOffset = rowOffset;

    if (!m_db || tableName.empty())
    {
        page.success = false;
        page.error = "Database is not open or table name is empty.";
        return page;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // First fetch columns info for table
    std::vector<ColumnInfo> cols;
    std::vector<IndexInfo> idxs;
    std::vector<ForeignKeyInfo> fks;
    std::string ddl, err;
    GetTableSchema(tableName, cols, idxs, fks, ddl, err);

    for (const auto& c : cols)
    {
        page.columnNames.push_back(c.name);
        page.columnTypes.push_back(c.type);
    }

    // Build SELECT query
    std::string sql = "SELECT * FROM " + EscapeIdentifier(tableName);

    // If filter is provided, create WHERE clause across all text/char columns
    bool hasWhere = false;
    if (!filter.empty())
    {
        std::string whereClause = "";
        for (const auto& c : cols)
        {
            if (!whereClause.empty())
                whereClause += " OR ";
            whereClause += "CAST(" + EscapeIdentifier(c.name) + " AS TEXT) LIKE ?";
        }
        if (!whereClause.empty())
        {
            sql += " WHERE (" + whereClause + ")";
            hasWhere = true;
        }
    }

    // Sorting
    if (sortColIndex >= 0 && sortColIndex < (int)cols.size())
    {
        sql += " ORDER BY " + EscapeIdentifier(cols[sortColIndex].name) + (sortDesc ? " DESC" : " ASC");
    }

    // Paging
    if (pageSize > 0)
    {
        sql += " LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(rowOffset);
    }
    sql += ";";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        page.success = false;
        page.error = sqlite3_errmsg(m_db);
        return page;
    }

    // Bind filter parameters
    if (hasWhere)
    {
        std::string pattern = "%" + filter + "%";
        for (int i = 1; i <= (int)cols.size(); ++i)
        {
            sqlite3_bind_text(stmt, i, pattern.c_str(), -1, SQLITE_TRANSIENT);
        }
    }

    int colCount = sqlite3_column_count(stmt);
    if (page.columnNames.empty())
    {
        for (int i = 0; i < colCount; ++i)
        {
            const char* cn = sqlite3_column_name(stmt, i);
            const char* ct = sqlite3_column_decltype(stmt, i);
            page.columnNames.push_back(cn ? cn : ("col_" + std::to_string(i)));
            page.columnTypes.push_back(ct ? ct : "");
        }
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::vector<CellValue> row;
        row.reserve(colCount);

        for (int i = 0; i < colCount; ++i)
        {
            CellValue cell;
            int type = sqlite3_column_type(stmt, i);
            if (type == SQLITE_NULL)
            {
                cell.isNull = true;
                cell.text = "";
            }
            else if (type == SQLITE_INTEGER)
            {
                cell.isNumeric = true;
                cell.text = std::to_string(sqlite3_column_int64(stmt, i));
            }
            else if (type == SQLITE_FLOAT)
            {
                cell.isNumeric = true;
                double val = sqlite3_column_double(stmt, i);
                char buf[64];
                snprintf(buf, sizeof(buf), "%.6g", val);
                cell.text = buf;
            }
            else if (type == SQLITE_BLOB)
            {
                cell.isBlob = true;
                cell.blobBytes = sqlite3_column_bytes(stmt, i);
                cell.text = "<BLOB " + std::to_string(cell.blobBytes) + " B>";
            }
            else
            {
                const char* txt = (const char*)sqlite3_column_text(stmt, i);
                cell.text = txt ? txt : "";
            }
            row.push_back(cell);
        }
        page.rows.push_back(row);
    }

    sqlite3_finalize(stmt);

    auto endTime = std::chrono::high_resolution_clock::now();
    page.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    // Total rows
    page.totalRows = QueryTableCount(tableName);
    page.hasMore = (pageSize > 0) && ((rowOffset + (int64_t)page.rows.size()) < page.totalRows);

    return page;
}

QueryPage CSqliteEngine::ExecuteQuery(const std::string& sql, int rowCap)
{
    QueryPage page;
    if (!m_db || sql.empty())
    {
        page.success = false;
        page.error = "No SQL query provided or database not open.";
        return page;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        page.success = false;
        page.error = sqlite3_errmsg(m_db);
        return page;
    }

    int colCount = sqlite3_column_count(stmt);
    for (int i = 0; i < colCount; ++i)
    {
        const char* cn = sqlite3_column_name(stmt, i);
        const char* ct = sqlite3_column_decltype(stmt, i);
        page.columnNames.push_back(cn ? cn : ("col_" + std::to_string(i)));
        page.columnTypes.push_back(ct ? ct : "");
    }

    int rowsFetched = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (rowCap > 0 && rowsFetched >= rowCap)
        {
            page.hasMore = true;
            break;
        }

        std::vector<CellValue> row;
        row.reserve(colCount);

        for (int i = 0; i < colCount; ++i)
        {
            CellValue cell;
            int type = sqlite3_column_type(stmt, i);
            if (type == SQLITE_NULL)
            {
                cell.isNull = true;
                cell.text = "";
            }
            else if (type == SQLITE_INTEGER)
            {
                cell.isNumeric = true;
                cell.text = std::to_string(sqlite3_column_int64(stmt, i));
            }
            else if (type == SQLITE_FLOAT)
            {
                cell.isNumeric = true;
                double val = sqlite3_column_double(stmt, i);
                char buf[64];
                snprintf(buf, sizeof(buf), "%.6g", val);
                cell.text = buf;
            }
            else if (type == SQLITE_BLOB)
            {
                cell.isBlob = true;
                cell.blobBytes = sqlite3_column_bytes(stmt, i);
                cell.text = "<BLOB " + std::to_string(cell.blobBytes) + " B>";
            }
            else
            {
                const char* txt = (const char*)sqlite3_column_text(stmt, i);
                cell.text = txt ? txt : "";
            }
            row.push_back(cell);
        }
        page.rows.push_back(row);
        rowsFetched++;
    }

    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        page.error = sqlite3_errmsg(m_db);
        if (page.rows.empty())
            page.success = false;
    }

    sqlite3_finalize(stmt);

    auto endTime = std::chrono::high_resolution_clock::now();
    page.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    page.totalRows = (int64_t)page.rows.size();

    return page;
}

std::string CSqliteEngine::EscapeCsvCell(const std::string& text)
{
    bool needQuotes = false;
    for (char c : text)
    {
        if (c == ',' || c == '"' || c == '\n' || c == '\r')
        {
            needQuotes = true;
            break;
        }
    }

    if (!needQuotes)
        return text;

    std::string res = "\"";
    for (char c : text)
    {
        if (c == '"')
            res += "\"\"";
        else
            res += c;
    }
    res += "\"";
    return res;
}

std::string CSqliteEngine::FormatRowsAsCsv(const QueryPage& page, bool includeHeaders)
{
    std::string csv;
    if (includeHeaders && !page.columnNames.empty())
    {
        for (size_t i = 0; i < page.columnNames.size(); ++i)
        {
            if (i > 0) csv += ",";
            csv += EscapeCsvCell(page.columnNames[i]);
        }
        csv += "\r\n";
    }

    for (const auto& row : page.rows)
    {
        for (size_t i = 0; i < row.size(); ++i)
        {
            if (i > 0) csv += ",";
            csv += EscapeCsvCell(row[i].text);
        }
        csv += "\r\n";
    }

    return csv;
}

std::string CSqliteEngine::FormatRowsAsTsv(const QueryPage& page, bool includeHeaders)
{
    std::string tsv;
    if (includeHeaders && !page.columnNames.empty())
    {
        for (size_t i = 0; i < page.columnNames.size(); ++i)
        {
            if (i > 0) tsv += "\t";
            tsv += page.columnNames[i];
        }
        tsv += "\r\n";
    }

    for (const auto& row : page.rows)
    {
        for (size_t i = 0; i < row.size(); ++i)
        {
            if (i > 0) tsv += "\t";
            tsv += row[i].text;
        }
        tsv += "\r\n";
    }

    return tsv;
}

bool CSqliteEngine::ExportTableToCsv(const std::string& tableName, const char* outFilePath, std::string& outError, const std::string& filter)
{
    if (!m_db || tableName.empty() || !outFilePath)
    {
        outError = "Invalid parameters for CSV export.";
        return false;
    }

    std::ofstream ofs(outFilePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
    {
        outError = "Cannot create output file.";
        return false;
    }

    // Write UTF-8 BOM
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    ofs.write((const char*)bom, sizeof(bom));

    // Get table schema
    std::vector<ColumnInfo> cols;
    std::vector<IndexInfo> idxs;
    std::vector<ForeignKeyInfo> fks;
    std::string ddl, err;
    GetTableSchema(tableName, cols, idxs, fks, ddl, err);

    // Header row
    for (size_t i = 0; i < cols.size(); ++i)
    {
        if (i > 0) ofs << ",";
        ofs << EscapeCsvCell(cols[i].name);
    }
    ofs << "\r\n";

    // Stream query
    std::string sql = "SELECT * FROM " + EscapeIdentifier(tableName) + ";";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        outError = sqlite3_errmsg(m_db);
        ofs.close();
        return false;
    }

    int colCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        for (int i = 0; i < colCount; ++i)
        {
            if (i > 0) ofs << ",";
            int type = sqlite3_column_type(stmt, i);
            if (type == SQLITE_NULL)
            {
                // empty string
            }
            else if (type == SQLITE_BLOB)
            {
                ofs << "<BLOB>";
            }
            else
            {
                const char* txt = (const char*)sqlite3_column_text(stmt, i);
                ofs << EscapeCsvCell(txt ? txt : "");
            }
        }
        ofs << "\r\n";
    }

    sqlite3_finalize(stmt);
    ofs.close();
    return true;
}

} // namespace SqliteEngine
