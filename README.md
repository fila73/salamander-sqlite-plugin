# SQLite Database Plugin for Open Salamander

A high-performance SQLite database viewer plugin for **Altap Salamander / Open Salamander**.

## Authors & Credits
- **Authors**: Open Salamander Authors, Red Salamander Authors, Ondřej Kotas ([KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander))
- **Dark Mode implementation**: Based on the Dark Mode host policy from [KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander) fork by Ondřej Kotas
- **Ported to Open Salamander framework by**: fila73

## Features

- **File Viewer (`F3` / Quick View)**:
  - Automatic detection of SQLite format 3 databases (`"SQLite format 3\000"` header check).
  - Associated file extensions: `.db`, `.sqlite`, `.sqlite3`, `.db3`, `.s3db`, `.sl3`, `.sqlite2`.
- **Interactive Data Grid View**:
  - Virtual ListView (`LVS_OWNERDATA`) with double buffering, grid lines, and full row selection.
  - Paged table reading with fast pagination (`<< First`, `< Prev`, `Next >`, `Last >>`).
  - Configurable page size (50, 100, 200, 500, 1000, All).
  - Live in-table search and filtering.
  - Column sorting (click column headers to sort ASC/DESC).
  - Clean rendering of `NULL` values (`[NULL]` in dimmed italic) and BLOB values (`<BLOB X B>`).
  - Correct text vs numeric alignment.
- **Schema & DDL Inspection**:
  - Full table and index DDL (`CREATE TABLE ...`, `CREATE INDEX ...`) in syntax viewer.
  - Column details list: name, declared type, not-null constraints, default values, primary keys.
  - Foreign key relations and index listings.
- **SQL Query Editor**:
  - Execute arbitrary `SELECT` queries with safety guards and row caps.
  - Results displayed in high-speed virtual grid with execution time reporting (ms).
- **Clipboard & Data Export**:
  - Copy cell, row, or all rows to clipboard (formatted as Text, CSV, or TSV).
  - Export entire table to standard CSV file (with UTF-8 BOM).
- **Dark Mode Support**:
  - Compatible with Dark Mode in the [KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander) fork (`PluginDarkMode` / WinLib theme engine).
- **Localization**:
  - English (`english.slg`) and Czech (`czech.slg`).

## Building

### MinGW-w64 (GCC)
```cmd
mingw32-make -f Makefile.mingw
```
Outputs:
- `sqlite.spl` - Plugin DLL
- `english.slg` - English language module
- `czech.slg` - Czech language module

### Visual Studio / MSVC
Open `src/vcxproj/sqlite.vcxproj` and build with Release/x64 or Debug/x64 configuration.

## Installation

1. Copy `sqlite.spl`, `english.slg`, and `czech.slg` to your Open Salamander `plugins/sqlite/` directory (or Open Salamander plugins folder).
2. Start Open Salamander, go to **Plugins -> Plugins Manager -> Add...** and select `sqlite.spl`.
3. Press `F3` on any `.db` or `.sqlite` file to view it!
