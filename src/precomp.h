// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <dwmapi.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>

#define SALSDK_COMPATIBLE_WITH_VER 103

#include "versinfo.rh2"

#include "spl_com.h"
#include "spl_base.h"
#include "spl_arc.h"
#include "spl_gen.h"
#include "spl_fs.h"
#include "spl_menu.h"
#include "spl_thum.h"
#include "spl_view.h"
#include "spl_vers.h"
#include "spl_gui.h"

#include "dbg.h"
#include "mhandles.h"
#include "arraylt.h"
#define ENABLE_PROPERTYDIALOG
#include "winliblt.h"
#include "auxtools.h"
#include "plugindarkmode.h"

#include "sqlite.rh"
#include "sqlite.rh2"
#include "lang/lang.rh"

#include "sqlite/sqlite3.h"
#include "sqlite_engine.h"
#include "sqlitedarkmode.h"
#include "dialogs.h"
#include "viewer.h"
#include "sqlite.h"
