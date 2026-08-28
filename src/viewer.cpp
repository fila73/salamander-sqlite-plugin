// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "viewer.h"
#include <commctrl.h>

#define IDC_COMBO_TABLES       103
#define IDC_COMBO_PAGESIZE     104
#define IDC_COMBO_MODE         105
#define IDC_EDIT_FILTER        106
#define IDC_STATUSBAR          107
#define IDC_LISTVIEW_DATA      108
#define IDC_LISTVIEW_SCHEMA    109
#define IDC_EDIT_SCHEMA_DDL    110
#define IDC_EDIT_SQL_QUERY     111
#define IDC_BTN_EXECUTE_SQL    112
#define IDC_LISTVIEW_SQL       113

static MENU_TEMPLATE_ITEM ViewerMenuTemplate[] =
{
    {MNTT_PB, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    // Files
    {MNTT_PB, IDS_MENU_FILES, MNTS_B | MNTS_I | MNTS_A, CML_VIEWER_FILES, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_FILES_OPEN, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_OPEN, -1, 0, NULL},
    {MNTT_SP, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_FILES_EXIT, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_EXIT, -1, 0, NULL},
    {MNTT_PE},

    // Edit
    {MNTT_PB, IDS_MENU_EDIT, MNTS_B | MNTS_I | MNTS_A, CML_VIEWER_EDIT, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_COPY, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_COPY, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_COPY_ALL, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_COPY_ALL, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_COPY_CSV, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_COPY_CSV, -1, 0, NULL},
    {MNTT_SP, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_EXPORT, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_EXPORT_CSV, -1, 0, NULL},
    {MNTT_PE},

    // View
    {MNTT_PB, IDS_MENU_VIEW, MNTS_B | MNTS_I | MNTS_A, CML_VIEWER_VIEW, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_REFRESH, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_REFRESH, -1, 0, NULL},
    {MNTT_SP, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_MODE_DATA, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_MODE_DATA, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_MODE_SCHEMA, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_MODE_SCHEMA, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_MODE_SQL, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_MODE_SQL, -1, 0, NULL},
    {MNTT_SP, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_FIRST_PAGE, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_FIRST_PAGE, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_PREV_PAGE, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_PREV_PAGE, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_NEXT_PAGE, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_NEXT_PAGE, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_LAST_PAGE, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_LAST_PAGE, -1, 0, NULL},
    {MNTT_SP, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EXEC_SQL, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_EXEC_SQL, -1, 0, NULL},
    {MNTT_PE},

    // Options
    {MNTT_PB, IDS_MENU_OPTIONS, MNTS_B | MNTS_I | MNTS_A, CML_VIEWER_OPTIONS, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_OPTIONS_CFG, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_CFG, -1, 0, NULL},
    {MNTT_PE},

    // Help
    {MNTT_PB, IDS_MENU_HELP, MNTS_B | MNTS_I | MNTS_A, CML_VIEWER_HELP, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_HELP_ABOUT, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_ABOUT, -1, 0, NULL},
    {MNTT_PE},

    {MNTT_PE}
};

static MENU_TEMPLATE_ITEM ViewerPopupMenuTemplate[] =
{
    {MNTT_PB, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_COPY, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_COPY, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_COPY_ALL, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_COPY_ALL, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_COPY_CSV, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_COPY_CSV, -1, 0, NULL},
    {MNTT_SP, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_EDIT_EXPORT, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_EXPORT_CSV, -1, 0, NULL},
    {MNTT_SP, -1, MNTS_B | MNTS_I | MNTS_A, 0, -1, 0, NULL},
    {MNTT_IT, IDS_MENU_REFRESH, MNTS_B | MNTS_I | MNTS_A, CM_VIEWER_REFRESH, -1, 0, NULL},
    {MNTT_PE}
};

CWindowQueue ViewerWindowQueue("SQLite Viewer Windows");
CThreadQueue ThreadQueue("SQLite Viewer Threads");
HACCEL ViewerAccels = NULL;

CViewerThread::CViewerThread(const char* name, int left, int top, int width, int height,
                             UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                             BOOL* lockOwner, HANDLE continueEvent, BOOL* success,
                             int enumFilesSourceUID, int enumFilesCurrentIndex)
    : CThread("SQLite Viewer")
    , m_left(left)
    , m_top(top)
    , m_width(width)
    , m_height(height)
    , m_showCmd(showCmd)
    , m_alwaysOnTop(alwaysOnTop)
    , m_returnLock(returnLock)
    , m_lock(lock)
    , m_lockOwner(lockOwner)
    , m_continueEvent(continueEvent)
    , m_success(success)
    , m_enumFilesSourceUID(enumFilesSourceUID)
    , m_enumFilesCurrentIndex(enumFilesCurrentIndex)
{
    lstrcpynA(m_name, name ? name : "", sizeof(m_name));
}

unsigned CViewerThread::Body()
{
    CViewerWindow* window = new CViewerWindow(m_enumFilesSourceUID, m_enumFilesCurrentIndex);
    if (!window)
    {
        if (m_success) *m_success = FALSE;
        SetEvent(m_continueEvent);
        return 0;
    }

    if (m_returnLock)
    {
        *m_lock = window->GetLock();
        *m_lockOwner = TRUE;
    }

    int w = (m_width > 100) ? m_width : 900;
    int h = (m_height > 100) ? m_height : 600;
    int x = (m_left >= 0) ? m_left : CW_USEDEFAULT;
    int y = (m_top >= 0) ? m_top : CW_USEDEFAULT;

    if (CfgSavePosition && CfgWindowPlacement.length == sizeof(WINDOWPLACEMENT))
    {
        WINDOWPLACEMENT place = CfgWindowPlacement;
        RECT monitorRect;
        RECT workRect;
        SalamanderGeneral->MultiMonGetClipRectByRect(&place.rcNormalPosition, &workRect, &monitorRect);
        OffsetRect(&place.rcNormalPosition, workRect.left - monitorRect.left,
                   workRect.top - monitorRect.top);
        SalamanderGeneral->MultiMonEnsureRectVisible(&place.rcNormalPosition, TRUE);
        x = place.rcNormalPosition.left;
        y = place.rcNormalPosition.top;
        w = place.rcNormalPosition.right - place.rcNormalPosition.left;
        h = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
        m_showCmd = place.showCmd;
    }

    HWND hwnd = window->CreateEx(m_alwaysOnTop ? WS_EX_TOPMOST : 0,
                                 CWINDOW_CLASSNAME2,
                                 LoadStr(IDS_PLUGINNAME),
                                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                 x, y, w, h,
                                 NULL, NULL, DLLInstance, window);

    if (hwnd == NULL)
    {
        if (m_returnLock && *m_lock != NULL)
            CloseHandle(*m_lock);
        delete window;
        if (m_success) *m_success = FALSE;
        SetEvent(m_continueEvent);
        return 0;
    }

    if (m_success) *m_success = TRUE;

    BOOL openFile = *m_success;
    SetEvent(m_continueEvent);
    m_continueEvent = NULL;
    m_lock = NULL;
    m_lockOwner = NULL;
    m_success = NULL;

    if (openFile)
    {
        window->OpenFile(m_name, FALSE);

        ShowWindow(window->HWindow, m_showCmd ? m_showCmd : SW_SHOWNORMAL);
        SetForegroundWindow(window->HWindow);
        UpdateWindow(window->HWindow);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
            {
                PostMessage(window->HWindow, WM_CLOSE, 0, 0);
                continue;
            }

            if (window->IsMenuBarMessage(&msg))
                continue;

            if (ViewerAccels && TranslateAccelerator(window->HWindow, ViewerAccels, &msg))
                continue;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (window != NULL)
        delete window;

    return 0;
}

CViewerWindow::CViewerWindow(int enumFilesSourceUID, int enumFilesCurrentIndex)
    : CWindow(ooStatic)
    , m_hLock(NULL)
    , m_enumFilesSourceUID(enumFilesSourceUID)
    , m_enumFilesCurrentIndex(enumFilesCurrentIndex)
    , m_mode(ViewerMode::Data)
    , m_pageSize(200)
    , m_rowOffset(0)
    , m_sortColumn(-1)
    , m_sortDesc(false)
    , m_hComboTables(NULL)
    , m_hComboPageSize(NULL)
    , m_hComboMode(NULL)
    , m_hEditFilter(NULL)
    , m_hStatusBar(NULL)
    , m_hListViewData(NULL)
    , m_hListViewSchema(NULL)
    , m_hEditSchemaDdl(NULL)
    , m_hEditSqlQuery(NULL)
    , m_hBtnExecuteSql(NULL)
    , m_hListViewSql(NULL)
    , m_hFontNormal(NULL)
    , m_hFontMono(NULL)
    , m_mainMenu(NULL)
    , m_menuBar(NULL)
{
    m_fileName[0] = 0;
    m_pageSize = CfgDefaultPageSize > 0 ? CfgDefaultPageSize : 200;
}

CViewerWindow::~CViewerWindow()
{
    if (m_hLock != NULL)
    {
        SetEvent(m_hLock);
        m_hLock = NULL;
    }

    if (m_hFontNormal) { DeleteObject(m_hFontNormal); m_hFontNormal = NULL; }
    if (m_hFontMono) { DeleteObject(m_hFontMono); m_hFontMono = NULL; }
    if (m_mainMenu) { SalamanderGUI->DestroyMenuPopup(m_mainMenu); m_mainMenu = NULL; }
    if (m_menuBar) { SalamanderGUI->DestroyMenuBar(m_menuBar); m_menuBar = NULL; }
}

HANDLE CViewerWindow::GetLock()
{
    if (m_hLock == NULL)
        m_hLock = CreateEvent(NULL, FALSE, FALSE, NULL);
    return m_hLock;
}

BOOL CViewerWindow::IsMenuBarMessage(CONST MSG* lpMsg)
{
    if (m_menuBar && m_menuBar->IsMenuBarMessage((MSG*)lpMsg))
        return TRUE;
    return FALSE;
}

void CViewerWindow::OpenFile(const char* name, BOOL setLock)
{
    if (setLock && m_hLock != NULL)
    {
        SetEvent(m_hLock);
        m_hLock = NULL;
    }

    lstrcpynA(m_fileName, name ? name : "", sizeof(m_fileName));

    std::string err;
    if (!m_engine.Open(m_fileName, err, CfgDirectOpen != FALSE))
    {
        char title[512];
        snprintf(title, sizeof(title), "%s - %s", LoadStr(IDS_PLUGINNAME), m_fileName);
        SetWindowTextA(HWindow, title);

        char msg[1024];
        snprintf(msg, sizeof(msg), "%s\n\n%s", LoadStr(IDS_NOT_A_SQLITE_DB), err.c_str());
        SalamanderGeneral->SalMessageBox(HWindow, msg, LoadStr(IDS_PLUGINNAME), MB_ICONERROR | MB_OK);
        return;
    }

    char title[512];
    snprintf(title, sizeof(title), "%s - [%s] (SQLite %s)",
             LoadStr(IDS_PLUGINNAME),
             m_fileName,
             m_engine.GetSqliteVersion().c_str());
    SetWindowTextA(HWindow, title);

    PopulateTablesCombo();

    if (!m_engine.GetTables().empty())
    {
        m_currentTable = m_engine.GetTables()[0].name;
        SendMessage(m_hComboTables, CB_SETCURSEL, 0, 0);
    }
    else
    {
        m_currentTable = "";
    }

    m_rowOffset = 0;
    m_sortColumn = -1;
    m_sortDesc = false;

    // Default query in SQL editor
    if (!m_currentTable.empty())
    {
        char sqlDef[256];
        snprintf(sqlDef, sizeof(sqlDef), "SELECT * FROM \"%s\" LIMIT 100;", m_currentTable.c_str());
        SetWindowTextA(m_hEditSqlQuery, sqlDef);
    }

    SwitchMode(m_mode);
    UpdateStatusBar();
}

bool CViewerWindow::CreateViewerControls()
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    m_hFontNormal = CreateFontIndirect(&ncm.lfMessageFont);
    m_hFontMono = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

    // Create Status Bar
    m_hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, "", HWindow, IDC_STATUSBAR);
    int sbParts[3] = { 260, 560, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 3, (LPARAM)sbParts);

    // Create Data ListView
    m_hListViewData = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS,
        0, 0, 100, 100, HWindow, (HMENU)IDC_LISTVIEW_DATA, DLLInstance, NULL);

    ListView_SetExtendedListViewStyle(m_hListViewData,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);

    // Create Schema ListView
    m_hListViewSchema = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        0, 0, 100, 100, HWindow, (HMENU)IDC_LISTVIEW_SCHEMA, DLLInstance, NULL);

    ListView_SetExtendedListViewStyle(m_hListViewSchema,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Create Schema DDL Edit
    m_hEditSchemaDdl = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
        0, 0, 100, 100, HWindow, (HMENU)IDC_EDIT_SCHEMA_DDL, DLLInstance, NULL);
    SendMessage(m_hEditSchemaDdl, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    // Create SQL Query controls
    m_hEditSqlQuery = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "SELECT * FROM sqlite_master;",
        WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
        0, 0, 100, 80, HWindow, (HMENU)IDC_EDIT_SQL_QUERY, DLLInstance, NULL);
    SendMessage(m_hEditSqlQuery, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    m_hBtnExecuteSql = CreateWindowEx(
        0, "BUTTON", LoadStr(IDS_SQL_EXECUTE_BTN),
        WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 140, 24, HWindow, (HMENU)IDC_BTN_EXECUTE_SQL, DLLInstance, NULL);
    SendMessage(m_hBtnExecuteSql, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hListViewSql = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS,
        0, 0, 100, 100, HWindow, (HMENU)IDC_LISTVIEW_SQL, DLLInstance, NULL);
    ListView_SetExtendedListViewStyle(m_hListViewSql,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);

    // Create Combos
    m_hComboTables = CreateWindowEx(
        0, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 180, 200, HWindow, (HMENU)IDC_COMBO_TABLES, DLLInstance, NULL);
    SendMessage(m_hComboTables, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hComboMode = CreateWindowEx(
        0, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 120, 120, HWindow, (HMENU)IDC_COMBO_MODE, DLLInstance, NULL);
    SendMessage(m_hComboMode, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hComboPageSize = CreateWindowEx(
        0, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 80, 150, HWindow, (HMENU)IDC_COMBO_PAGESIZE, DLLInstance, NULL);
    SendMessage(m_hComboPageSize, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hEditFilter = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        0, 0, 140, 22, HWindow, (HMENU)IDC_EDIT_FILTER, DLLInstance, NULL);
    SendMessage(m_hEditFilter, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    SendMessage(m_hEditFilter, EM_SETCUEBANNER, TRUE, (LPARAM)L"Filter rows...");

    PopulatePageSizeCombo();
    PopulateModeCombo();

    // Create WinLib Menu Bar
    m_mainMenu = SalamanderGUI->CreateMenuPopup();
    if (m_mainMenu)
    {
        m_mainMenu->LoadFromTemplate(HLanguage, ViewerMenuTemplate, NULL, NULL, NULL);
        m_menuBar = SalamanderGUI->CreateMenuBar(m_mainMenu, HWindow);
    }

    // Apply dark mode
    SqliteDarkMode::ApplyWindowTheme(HWindow);
    SqliteDarkMode::ApplyListViewTheme(m_hListViewData);
    SqliteDarkMode::ApplyListViewTheme(m_hListViewSchema);
    SqliteDarkMode::ApplyListViewTheme(m_hListViewSql);
    SqliteDarkMode::ApplyEditTheme(m_hEditSchemaDdl);
    SqliteDarkMode::ApplyEditTheme(m_hEditSqlQuery);
    SqliteDarkMode::ApplyEditTheme(m_hEditFilter);
    SqliteDarkMode::ApplyStatusBarTheme(m_hStatusBar);

    ViewerWindowQueue.Add(new CWindowQueueItem(HWindow));

    return true;
}

void CViewerWindow::PopulateTablesCombo()
{
    SendMessage(m_hComboTables, CB_RESETCONTENT, 0, 0);
    const auto& tables = m_engine.GetTables();
    for (const auto& t : tables)
    {
        char buf[256];
        if (t.isView)
            snprintf(buf, sizeof(buf), "[View] %s", t.name.c_str());
        else
            snprintf(buf, sizeof(buf), "%s (%lld rows)", t.name.c_str(), (long long)t.rowCount);
        SendMessageA(m_hComboTables, CB_ADDSTRING, 0, (LPARAM)buf);
    }
}

void CViewerWindow::PopulatePageSizeCombo()
{
    SendMessage(m_hComboPageSize, CB_RESETCONTENT, 0, 0);
    const char* sizes[] = { "50", "100", "200", "500", "1000", "All" };
    for (int i = 0; i < 6; ++i)
    {
        SendMessageA(m_hComboPageSize, CB_ADDSTRING, 0, (LPARAM)sizes[i]);
    }
    SendMessage(m_hComboPageSize, CB_SETCURSEL, 2, 0);
}

void CViewerWindow::PopulateModeCombo()
{
    SendMessage(m_hComboMode, CB_RESETCONTENT, 0, 0);
    SendMessageA(m_hComboMode, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEW_DATA_TAB));
    SendMessageA(m_hComboMode, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEW_SCHEMA_TAB));
    SendMessageA(m_hComboMode, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEW_SQL_TAB));
    SendMessage(m_hComboMode, CB_SETCURSEL, 0, 0);
}

void CViewerWindow::SwitchMode(ViewerMode newMode)
{
    m_mode = newMode;
    SendMessage(m_hComboMode, CB_SETCURSEL, (WPARAM)m_mode, 0);

    ShowWindow(m_hListViewData, (m_mode == ViewerMode::Data) ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hListViewSchema, (m_mode == ViewerMode::Schema) ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hEditSchemaDdl, (m_mode == ViewerMode::Schema) ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hEditSqlQuery, (m_mode == ViewerMode::Sql) ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnExecuteSql, (m_mode == ViewerMode::Sql) ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hListViewSql, (m_mode == ViewerMode::Sql) ? SW_SHOW : SW_HIDE);

    ShowWindow(m_hComboPageSize, (m_mode == ViewerMode::Data) ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hEditFilter, (m_mode == ViewerMode::Data) ? SW_SHOW : SW_HIDE);

    if (m_mode == ViewerMode::Data)
    {
        LoadCurrentTableData();
    }
    else if (m_mode == ViewerMode::Schema)
    {
        LoadSchemaData();
    }

    LayoutWindows();
    UpdateStatusBar();
}

void CViewerWindow::SetupDataListViewColumns()
{
    HWND hHeader = ListView_GetHeader(m_hListViewData);
    int colCount = Header_GetItemCount(hHeader);
    for (int i = colCount - 1; i >= 0; --i)
    {
        ListView_DeleteColumn(m_hListViewData, i);
    }

    LVCOLUMNA col;
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;

    for (size_t i = 0; i < m_currentPage.columnNames.size(); ++i)
    {
        char buf[256];
        if (!m_currentPage.columnTypes[i].empty())
            snprintf(buf, sizeof(buf), "%s (%s)", m_currentPage.columnNames[i].c_str(), m_currentPage.columnTypes[i].c_str());
        else
            snprintf(buf, sizeof(buf), "%s", m_currentPage.columnNames[i].c_str());

        col.pszText = buf;
        col.cx = 120;
        col.iSubItem = (int)i;
        ListView_InsertColumn(m_hListViewData, i, &col);
    }
}

void CViewerWindow::SetupSchemaListViewColumns()
{
    HWND hHeader = ListView_GetHeader(m_hListViewSchema);
    int colCount = Header_GetItemCount(hHeader);
    for (int i = colCount - 1; i >= 0; --i)
    {
        ListView_DeleteColumn(m_hListViewSchema, i);
    }

    LVCOLUMNA col;
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;

    const char* titles[] = { "#", "Name", "Type", "Not Null", "Default", "PK" };
    int widths[] = { 40, 160, 120, 70, 100, 50 };
    for (int i = 0; i < 6; ++i)
    {
        col.pszText = (char*)titles[i];
        col.cx = widths[i];
        col.iSubItem = i;
        ListView_InsertColumn(m_hListViewSchema, i, &col);
    }
}

void CViewerWindow::SetupSqlListViewColumns()
{
    HWND hHeader = ListView_GetHeader(m_hListViewSql);
    int colCount = Header_GetItemCount(hHeader);
    for (int i = colCount - 1; i >= 0; --i)
    {
        ListView_DeleteColumn(m_hListViewSql, i);
    }

    LVCOLUMNA col;
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;

    for (size_t i = 0; i < m_sqlResultPage.columnNames.size(); ++i)
    {
        char buf[256];
        if (!m_sqlResultPage.columnTypes[i].empty())
            snprintf(buf, sizeof(buf), "%s (%s)", m_sqlResultPage.columnNames[i].c_str(), m_sqlResultPage.columnTypes[i].c_str());
        else
            snprintf(buf, sizeof(buf), "%s", m_sqlResultPage.columnNames[i].c_str());

        col.pszText = buf;
        col.cx = 120;
        col.iSubItem = (int)i;
        ListView_InsertColumn(m_hListViewSql, i, &col);
    }
}

void CViewerWindow::LoadCurrentTableData()
{
    if (m_currentTable.empty() || !m_engine.IsOpen())
    {
        ListView_SetItemCountEx(m_hListViewData, 0, 0);
        return;
    }

    m_currentPage = m_engine.LoadTablePage(
        m_currentTable, m_pageSize, m_rowOffset, m_sortColumn, m_sortDesc, m_filterText);

    SetupDataListViewColumns();
    ListView_SetItemCountEx(m_hListViewData, (int)m_currentPage.rows.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    InvalidateRect(m_hListViewData, NULL, TRUE);

    UpdateStatusBar();
}

void CViewerWindow::LoadSchemaData()
{
    if (m_currentTable.empty() || !m_engine.IsOpen())
        return;

    std::vector<SqliteEngine::ColumnInfo> cols;
    std::vector<SqliteEngine::IndexInfo> idxs;
    std::vector<SqliteEngine::ForeignKeyInfo> fks;
    std::string ddl, err;
    m_engine.GetTableSchema(m_currentTable, cols, idxs, fks, ddl, err);

    SetupSchemaListViewColumns();
    ListView_DeleteAllItems(m_hListViewSchema);

    for (size_t i = 0; i < cols.size(); ++i)
    {
        LVITEMA item;
        item.mask = LVIF_TEXT;
        item.iItem = (int)i;
        item.iSubItem = 0;
        char cidBuf[32];
        snprintf(cidBuf, sizeof(cidBuf), "%d", cols[i].cid);
        item.pszText = cidBuf;
        ListView_InsertItem(m_hListViewSchema, &item);

        ListView_SetItemText(m_hListViewSchema, i, 1, (char*)cols[i].name.c_str());
        ListView_SetItemText(m_hListViewSchema, i, 2, (char*)cols[i].type.c_str());
        ListView_SetItemText(m_hListViewSchema, i, 3, (char*)(cols[i].notNull ? "YES" : "NO"));
        ListView_SetItemText(m_hListViewSchema, i, 4, (char*)cols[i].defaultValue.c_str());
        ListView_SetItemText(m_hListViewSchema, i, 5, (char*)(cols[i].isPrimaryKey ? "PK" : ""));
    }

    std::string fullDdl = "-- Table SQL DDL:\r\n" + ddl + "\r\n\r\n";
    if (!idxs.empty())
    {
        fullDdl += "-- Indexes:\r\n";
        for (const auto& idx : idxs)
        {
            fullDdl += idx.sql + ";\r\n";
        }
    }

    SetWindowTextA(m_hEditSchemaDdl, fullDdl.c_str());
}

void CViewerWindow::ExecuteCustomSql()
{
    int len = GetWindowTextLengthA(m_hEditSqlQuery);
    if (len <= 0) return;

    std::vector<char> buf(len + 1);
    GetWindowTextA(m_hEditSqlQuery, buf.data(), len + 1);
    std::string sql(buf.data());

    m_sqlResultPage = m_engine.ExecuteQuery(sql, CfgQueryRowCap > 0 ? CfgQueryRowCap : 5000);

    SetupSqlListViewColumns();
    ListView_SetItemCountEx(m_hListViewSql, (int)m_sqlResultPage.rows.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    InvalidateRect(m_hListViewSql, NULL, TRUE);

    UpdateStatusBar();
}

void CViewerWindow::OnDataListGetDispInfo(NMLVDISPINFOA* pdi)
{
    if (pdi->item.mask & LVIF_TEXT)
    {
        int row = pdi->item.iItem;
        int col = pdi->item.iSubItem;

        if (row >= 0 && row < (int)m_currentPage.rows.size() &&
            col >= 0 && col < (int)m_currentPage.rows[row].size())
        {
            pdi->item.pszText = (char*)m_currentPage.rows[row][col].text.c_str();
        }
        else
        {
            pdi->item.pszText = (char*)"";
        }
    }
}

void CViewerWindow::OnSqlListGetDispInfo(NMLVDISPINFOA* pdi)
{
    if (pdi->item.mask & LVIF_TEXT)
    {
        int row = pdi->item.iItem;
        int col = pdi->item.iSubItem;

        if (row >= 0 && row < (int)m_sqlResultPage.rows.size() &&
            col >= 0 && col < (int)m_sqlResultPage.rows[row].size())
        {
            pdi->item.pszText = (char*)m_sqlResultPage.rows[row][col].text.c_str();
        }
        else
        {
            pdi->item.pszText = (char*)"";
        }
    }
}

void CViewerWindow::OnColumnClick(int colIndex)
{
    if (m_mode != ViewerMode::Data) return;

    if (m_sortColumn == colIndex)
    {
        m_sortDesc = !m_sortDesc;
    }
    else
    {
        m_sortColumn = colIndex;
        m_sortDesc = false;
    }
    LoadCurrentTableData();
}

void CViewerWindow::OnFirstPage()
{
    m_rowOffset = 0;
    LoadCurrentTableData();
}

void CViewerWindow::OnPrevPage()
{
    if (m_pageSize <= 0) return;
    m_rowOffset -= m_pageSize;
    if (m_rowOffset < 0) m_rowOffset = 0;
    LoadCurrentTableData();
}

void CViewerWindow::OnNextPage()
{
    if (m_pageSize <= 0) return;
    if (m_rowOffset + m_pageSize < m_currentPage.totalRows)
    {
        m_rowOffset += m_pageSize;
        LoadCurrentTableData();
    }
}

void CViewerWindow::OnLastPage()
{
    if (m_pageSize <= 0 || m_currentPage.totalRows <= 0) return;
    m_rowOffset = ((m_currentPage.totalRows - 1) / m_pageSize) * m_pageSize;
    LoadCurrentTableData();
}

void CViewerWindow::OnPageSizeChanged()
{
    int sel = (int)SendMessage(m_hComboPageSize, CB_GETCURSEL, 0, 0);
    int sizes[] = { 50, 100, 200, 500, 1000, 0 };
    if (sel >= 0 && sel < 6)
    {
        m_pageSize = sizes[sel];
        m_rowOffset = 0;
        LoadCurrentTableData();
    }
}

void CViewerWindow::OnTableSelectionChanged()
{
    int sel = (int)SendMessage(m_hComboTables, CB_GETCURSEL, 0, 0);
    const auto& tables = m_engine.GetTables();
    if (sel >= 0 && sel < (int)tables.size())
    {
        m_currentTable = tables[sel].name;
        m_rowOffset = 0;
        m_sortColumn = -1;
        m_sortDesc = false;

        char sqlDef[256];
        snprintf(sqlDef, sizeof(sqlDef), "SELECT * FROM \"%s\" LIMIT 100;", m_currentTable.c_str());
        SetWindowTextA(m_hEditSqlQuery, sqlDef);

        if (m_mode == ViewerMode::Data)
            LoadCurrentTableData();
        else if (m_mode == ViewerMode::Schema)
            LoadSchemaData();
    }
}

void CViewerWindow::OnModeSelectionChanged()
{
    int sel = (int)SendMessage(m_hComboMode, CB_GETCURSEL, 0, 0);
    if (sel >= 0 && sel <= 2)
    {
        SwitchMode((ViewerMode)sel);
    }
}

void CViewerWindow::OnFilterChanged()
{
    int len = GetWindowTextLengthA(m_hEditFilter);
    std::vector<char> buf(len + 1);
    GetWindowTextA(m_hEditFilter, buf.data(), len + 1);
    m_filterText = buf.data();
    m_rowOffset = 0;
    LoadCurrentTableData();
}

void CViewerWindow::OnExportCsv()
{
    if (m_currentTable.empty()) return;

    OPENFILENAMEA ofn;
    char szFile[MAX_PATH];
    snprintf(szFile, sizeof(szFile), "%s.csv", m_currentTable.c_str());

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = HWindow;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = LoadStr(IDS_EXPORT_FILTER);
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "csv";

    if (GetSaveFileNameA(&ofn))
    {
        std::string err;
        if (m_engine.ExportTableToCsv(m_currentTable, szFile, err))
        {
            char msg[512];
            snprintf(msg, sizeof(msg), LoadStr(IDS_EXPORT_SUCCESS), szFile);
            SalamanderGeneral->SalMessageBox(HWindow, msg, LoadStr(IDS_PLUGINNAME), MB_ICONINFORMATION | MB_OK);
        }
        else
        {
            char msg[512];
            snprintf(msg, sizeof(msg), LoadStr(IDS_EXPORT_FAILED), err.c_str());
            SalamanderGeneral->SalMessageBox(HWindow, msg, LoadStr(IDS_PLUGINNAME), MB_ICONERROR | MB_OK);
        }
    }
}

void CViewerWindow::OnCopySelection()
{
    HWND hwndTarget = (m_mode == ViewerMode::Sql) ? m_hListViewSql : m_hListViewData;
    const SqliteEngine::QueryPage& page = (m_mode == ViewerMode::Sql) ? m_sqlResultPage : m_currentPage;

    int sel = ListView_GetNextItem(hwndTarget, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)page.rows.size()) return;

    std::string text;
    for (size_t c = 0; c < page.rows[sel].size(); ++c)
    {
        if (c > 0) text += "\t";
        text += page.rows[sel][c].text;
    }

    if (OpenClipboard(HWindow))
    {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
        if (hMem)
        {
            char* p = (char*)GlobalLock(hMem);
            memcpy(p, text.c_str(), text.length() + 1);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
        CloseClipboard();
    }
}

void CViewerWindow::OnCopyAllRows()
{
    const SqliteEngine::QueryPage& page = (m_mode == ViewerMode::Sql) ? m_sqlResultPage : m_currentPage;
    std::string tsv = SqliteEngine::CSqliteEngine::FormatRowsAsTsv(page, true);

    if (OpenClipboard(HWindow))
    {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, tsv.length() + 1);
        if (hMem)
        {
            char* p = (char*)GlobalLock(hMem);
            memcpy(p, tsv.c_str(), tsv.length() + 1);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
        CloseClipboard();
    }
}

void CViewerWindow::UpdateStatusBar()
{
    if (!m_hStatusBar) return;

    // Part 0: DB info
    char sb0[256];
    snprintf(sb0, sizeof(sb0), "Size: %.2f MB | %d tables | SQLite %s",
             (double)m_engine.GetFileSize() / (1024.0 * 1024.0),
             (int)m_engine.GetTables().size(),
             m_engine.GetSqliteVersion().c_str());
    SendMessageA(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)sb0);

    // Part 1: Row paging info
    char sb1[256];
    if (m_mode == ViewerMode::Data)
    {
        int64_t total = m_currentPage.totalRows;
        int64_t from = (total > 0) ? (m_rowOffset + 1) : 0;
        int64_t to = m_rowOffset + (int64_t)m_currentPage.rows.size();
        uint32_t pageNum = (m_pageSize > 0) ? (uint32_t)(m_rowOffset / m_pageSize + 1) : 1;
        uint32_t totalPages = (m_pageSize > 0 && total > 0) ? (uint32_t)((total + m_pageSize - 1) / m_pageSize) : 1;

        snprintf(sb1, sizeof(sb1), LoadStr(IDS_STATUS_ROWS_FMT), (long long)from, (long long)to, (long long)total, pageNum, totalPages);
    }
    else if (m_mode == ViewerMode::Schema)
    {
        snprintf(sb1, sizeof(sb1), "Schema / DDL Definition for %s", m_currentTable.c_str());
    }
    else if (m_mode == ViewerMode::Sql)
    {
        if (!m_sqlResultPage.error.empty())
            snprintf(sb1, sizeof(sb1), LoadStr(IDS_STATUS_QUERY_ERR), m_sqlResultPage.error.c_str());
        else
            snprintf(sb1, sizeof(sb1), "%lld rows returned", (long long)m_sqlResultPage.rows.size());
    }
    SendMessageA(m_hStatusBar, SB_SETTEXT, 1, (LPARAM)sb1);

    // Part 2: Timing
    char sb2[256];
    double ms = (m_mode == ViewerMode::Sql) ? m_sqlResultPage.elapsedMs : m_currentPage.elapsedMs;
    snprintf(sb2, sizeof(sb2), "Loaded in %.1f ms", ms);
    SendMessageA(m_hStatusBar, SB_SETTEXT, 2, (LPARAM)sb2);
}

void CViewerWindow::LayoutWindows()
{
    RECT rc;
    GetClientRect(HWindow, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
    RECT rcStatus;
    GetWindowRect(m_hStatusBar, &rcStatus);
    int statusHeight = rcStatus.bottom - rcStatus.top;

    int topBarHeight = 32;

    // Controls in top bar
    int x = 6;
    int comboTablesW = 200;
    int comboModeW = 120;
    int comboPageSizeW = 75;
    int editFilterW = 160;

    SetWindowPos(m_hComboTables, NULL, x, 5, comboTablesW, 200, SWP_NOZORDER);
    x += comboTablesW + 8;

    SetWindowPos(m_hComboMode, NULL, x, 5, comboModeW, 120, SWP_NOZORDER);
    x += comboModeW + 8;

    SetWindowPos(m_hComboPageSize, NULL, x, 5, comboPageSizeW, 150, SWP_NOZORDER);
    x += comboPageSizeW + 8;

    SetWindowPos(m_hEditFilter, NULL, x, 5, editFilterW, 22, SWP_NOZORDER);

    int clientY = topBarHeight + 6;
    int clientH = height - clientY - statusHeight;

    if (m_mode == ViewerMode::Data)
    {
        SetWindowPos(m_hListViewData, NULL, 0, clientY, width, clientH, SWP_NOZORDER);
    }
    else if (m_mode == ViewerMode::Schema)
    {
        int listH = clientH / 2;
        SetWindowPos(m_hListViewSchema, NULL, 0, clientY, width, listH, SWP_NOZORDER);
        SetWindowPos(m_hEditSchemaDdl, NULL, 0, clientY + listH + 2, width, clientH - listH - 2, SWP_NOZORDER);
    }
    else if (m_mode == ViewerMode::Sql)
    {
        int editH = 90;
        int btnH = 24;
        SetWindowPos(m_hEditSqlQuery, NULL, 4, clientY, width - 8, editH, SWP_NOZORDER);
        SetWindowPos(m_hBtnExecuteSql, NULL, 4, clientY + editH + 4, 160, btnH, SWP_NOZORDER);
        int listY = clientY + editH + btnH + 8;
        SetWindowPos(m_hListViewSql, NULL, 0, listY, width, height - listY - statusHeight, SWP_NOZORDER);
    }
}

void CViewerWindow::ShowContextMenu(const POINT& pt)
{
    CGUIMenuPopupAbstract* popup = SalamanderGUI->CreateMenuPopup();
    if (popup)
    {
        popup->LoadFromTemplate(HLanguage, ViewerPopupMenuTemplate, NULL, NULL, NULL);
        popup->Track(0, pt.x, pt.y, HWindow, NULL);
        SalamanderGUI->DestroyMenuPopup(popup);
    }
}

LRESULT CViewerWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        CreateViewerControls();
        return 0;

    case WM_SIZE:
        LayoutWindows();
        return 0;

    case WM_SETFOCUS:
        if (m_mode == ViewerMode::Data) SetFocus(m_hListViewData);
        else if (m_mode == ViewerMode::Schema) SetFocus(m_hListViewSchema);
        else if (m_mode == ViewerMode::Sql) SetFocus(m_hEditSqlQuery);
        return 0;

    case WM_USER_VIEWERCFGCHNG:
        InvalidateRect(HWindow, NULL, TRUE);
        return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostMessage(HWindow, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDC_COMBO_TABLES && code == CBN_SELCHANGE)
        {
            OnTableSelectionChanged();
            return 0;
        }
        else if (id == IDC_COMBO_PAGESIZE && code == CBN_SELCHANGE)
        {
            OnPageSizeChanged();
            return 0;
        }
        else if (id == IDC_COMBO_MODE && code == CBN_SELCHANGE)
        {
            OnModeSelectionChanged();
            return 0;
        }
        else if (id == IDC_EDIT_FILTER && code == EN_CHANGE)
        {
            OnFilterChanged();
            return 0;
        }
        else if (id == IDC_BTN_EXECUTE_SQL)
        {
            ExecuteCustomSql();
            return 0;
        }

        switch (id)
        {
        case CM_VIEWER_OPEN:
        {
            OPENFILENAMEA ofn;
            char szFile[MAX_PATH] = "";
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = HWindow;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = LoadStr(IDS_OPEN_DB_FILTER);
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            ofn.lpstrTitle = LoadStr(IDS_OPEN_DB_TITLE);
            if (GetOpenFileNameA(&ofn))
            {
                OpenFile(szFile, TRUE);
            }
            return 0;
        }

        case CM_VIEWER_EXIT:
            PostMessage(HWindow, WM_CLOSE, 0, 0);
            return 0;

        case CM_VIEWER_COPY:
            OnCopySelection();
            return 0;

        case CM_VIEWER_COPY_ALL:
        case CM_VIEWER_COPY_CSV:
            OnCopyAllRows();
            return 0;

        case CM_VIEWER_EXPORT_CSV:
            OnExportCsv();
            return 0;

        case CM_VIEWER_REFRESH:
            if (m_mode == ViewerMode::Sql)
                ExecuteCustomSql();
            else
            {
                std::string err;
                m_engine.RefreshTables(err);
                PopulateTablesCombo();
                LoadCurrentTableData();
            }
            return 0;

        case CM_VIEWER_EXEC_SQL:
            if (m_mode != ViewerMode::Sql)
                SwitchMode(ViewerMode::Sql);
            ExecuteCustomSql();
            return 0;

        case CM_VIEWER_FIRST_PAGE:
            OnFirstPage();
            return 0;

        case CM_VIEWER_PREV_PAGE:
            OnPrevPage();
            return 0;

        case CM_VIEWER_NEXT_PAGE:
            OnNextPage();
            return 0;

        case CM_VIEWER_LAST_PAGE:
            OnLastPage();
            return 0;

        case CM_VIEWER_MODE_DATA:
            SwitchMode(ViewerMode::Data);
            return 0;

        case CM_VIEWER_MODE_SCHEMA:
            SwitchMode(ViewerMode::Schema);
            return 0;

        case CM_VIEWER_MODE_SQL:
            SwitchMode(ViewerMode::Sql);
            return 0;

        case CM_VIEWER_FILTER_FOCUS:
            SetFocus(m_hEditFilter);
            return 0;

        case CM_VIEWER_CFG:
            OnConfiguration(HWindow);
            return 0;

        case CM_VIEWER_ABOUT:
            OnAbout(HWindow);
            return 0;
        }
        break;
    }

    case WM_NOTIFY:
    {
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm->idFrom == IDC_LISTVIEW_DATA)
        {
            if (pnm->code == LVN_GETDISPINFOA)
            {
                OnDataListGetDispInfo((NMLVDISPINFOA*)lParam);
                return TRUE;
            }
            else if (pnm->code == LVN_COLUMNCLICK)
            {
                NMLISTVIEW* pnmlv = (NMLISTVIEW*)lParam;
                OnColumnClick(pnmlv->iSubItem);
                return TRUE;
            }
            else if (pnm->code == NM_CUSTOMDRAW)
            {
                return SqliteDarkMode::HandleListViewCustomDraw(m_hListViewData, (LPNMLVCUSTOMDRAW)lParam);
            }
        }
        else if (pnm->idFrom == IDC_LISTVIEW_SQL)
        {
            if (pnm->code == LVN_GETDISPINFOA)
            {
                OnSqlListGetDispInfo((NMLVDISPINFOA*)lParam);
                return TRUE;
            }
            else if (pnm->code == NM_CUSTOMDRAW)
            {
                return SqliteDarkMode::HandleListViewCustomDraw(m_hListViewSql, (LPNMLVCUSTOMDRAW)lParam, true);
            }
        }
        else if (pnm->idFrom == IDC_LISTVIEW_SCHEMA)
        {
            if (pnm->code == NM_CUSTOMDRAW)
            {
                return SqliteDarkMode::HandleListViewCustomDraw(m_hListViewSchema, (LPNMLVCUSTOMDRAW)lParam);
            }
        }
        break;
    }

    case WM_CONTEXTMENU:
    {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        if (pt.x == -1 && pt.y == -1)
        {
            RECT rc;
            GetWindowRect(HWindow, &rc);
            pt.x = rc.left + 50;
            pt.y = rc.top + 50;
        }
        ShowContextMenu(pt);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(HWindow);
        return 0;

    case WM_DESTROY:
    {
        if (CfgSavePosition)
        {
            CfgWindowPlacement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(HWindow, &CfgWindowPlacement);
        }

        if (m_menuBar != NULL)
        {
            SalamanderGUI->DestroyMenuBar(m_menuBar);
            m_menuBar = NULL;
        }
        if (m_mainMenu != NULL)
        {
            SalamanderGUI->DestroyMenuPopup(m_mainMenu);
            m_mainMenu = NULL;
        }

        ViewerWindowQueue.Remove(HWindow);

        if (m_hLock != NULL)
        {
            SetEvent(m_hLock);
            m_hLock = NULL;
        }

        PostQuitMessage(0);
        break;
    }
    }

    return CWindow::WindowProc(uMsg, wParam, lParam);
}
