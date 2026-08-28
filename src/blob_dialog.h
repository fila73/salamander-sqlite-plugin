// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <string>
#include "sqlite_engine.h"

namespace BlobDialog
{
    void ShowBlobInspector(HWND hParent, const std::string& colName, const SqliteEngine::CellValue& cell);
}
