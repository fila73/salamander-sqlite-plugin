// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sqlite_engine.h"

enum class ViewerMode
{
    Data,
    Schema,
    Sql
};

class CViewerWindow;

class CViewerThread : public CThread
{
public:
    char m_name[MAX_PATH];
    int m_left;
    int m_top;
    int m_width;
    int m_height;
    UINT m_showCmd;
    BOOL m_alwaysOnTop;
    BOOL m_returnLock;
    HANDLE* m_lock;
    BOOL* m_lockOwner;
    HANDLE m_continueEvent;
    BOOL* m_success;
    int m_enumFilesSourceUID;
    int m_enumFilesCurrentIndex;

    CViewerThread(const char* name, int left, int top, int width, int height,
                  UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                  BOOL* lockOwner, HANDLE continueEvent, BOOL* success,
                  int enumFilesSourceUID, int enumFilesCurrentIndex);

    virtual unsigned Body() override;
};

class CViewerWindow : public CWindow
{
public:
    CViewerWindow(int enumFilesSourceUID, int enumFilesCurrentIndex);
    ~CViewerWindow();

    bool OpenFile(const char* name, BOOL setLock = TRUE);
    HANDLE GetLock();

    BOOL IsMenuBarMessage(CONST MSG* lpMsg);

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    char m_fileName[MAX_PATH];
    HANDLE m_hLock;
    int m_enumFilesSourceUID;
    int m_enumFilesCurrentIndex;

    SqliteEngine::CSqliteEngine m_engine;
    SqliteEngine::QueryPage m_currentPage;
    SqliteEngine::QueryPage m_sqlResultPage;

    ViewerMode m_mode;
    std::string m_currentTable;
    int m_pageSize;
    int64_t m_rowOffset;
    int m_sortColumn;
    bool m_sortDesc;
    std::string m_filterText;

    // Controls
    HWND m_hComboTables;
    HWND m_hComboPageSize;
    HWND m_hComboMode;
    HWND m_hEditFilter;
    HWND m_hStatusBar;

    // Main view controls
    HWND m_hListViewData;
    HWND m_hListViewSchema;
    HWND m_hEditSchemaDdl;
    HWND m_hEditSqlQuery;
    HWND m_hBtnExecuteSql;
    HWND m_hListViewSql;

    HFONT m_hFontNormal;
    HFONT m_hFontMono;

    // WinLib Menu bar
    CGUIMenuPopupAbstract* m_mainMenu;
    CGUIMenuBarAbstract* m_menuBar;

    bool CreateViewerControls();
    void LayoutWindows();
    void UpdateStatusBar();
    void PopulateTablesCombo();
    void PopulatePageSizeCombo();
    void PopulateModeCombo();

    void SwitchMode(ViewerMode newMode);
    void LoadCurrentTableData();
    void LoadSchemaData();
    void ExecuteCustomSql();

    void OnFirstPage();
    void OnPrevPage();
    void OnNextPage();
    void OnLastPage();
    void OnPageSizeChanged();
    void OnTableSelectionChanged();
    void OnModeSelectionChanged();
    void OnFilterChanged();
    void OnExportCsv();
    void OnCopySelection();
    void OnCopyAllRows();

    void SetupDataListViewColumns();
    void SetupSchemaListViewColumns();
    void SetupSqlListViewColumns();

    void OnDataListGetDispInfo(NMLVDISPINFOA* pdi);
    void OnSchemaListGetDispInfo(NMLVDISPINFOA* pdi);
    void OnSqlListGetDispInfo(NMLVDISPINFOA* pdi);
    void OnColumnClick(int colIndex);

    void ShowContextMenu(const POINT& pt);
    void OnInspectCurrentCell();
};

extern CWindowQueue ViewerWindowQueue;
extern CThreadQueue ThreadQueue;
extern HACCEL ViewerAccels;
