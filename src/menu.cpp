// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

DWORD WINAPI CPluginInterfaceForMenuExt::GetMenuItemState(int id, DWORD eventMask)
{
    return 0;
}

BOOL WINAPI CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander,
                                                        HWND parent, int id, DWORD eventMask)
{
    if (id == CM_OPEN_SQLITE_DB)
    {
        OPENFILENAMEA ofn;
        char szFile[MAX_PATH] = "";
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = parent;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = LoadStr(IDS_OPEN_DB_FILTER);
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        ofn.lpstrTitle = LoadStr(IDS_OPEN_DB_TITLE);

        if (GetOpenFileNameA(&ofn))
        {
            InterfaceForViewer.ViewFile(szFile, -1, -1, -1, -1, SW_SHOWNORMAL, FALSE, FALSE, NULL, NULL, NULL, -1, -1);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
    return FALSE;
}

void WINAPI CPluginInterfaceForMenuExt::BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander)
{
    salamander->AddMenuItem(-1, LoadStr(IDS_OPEN_DB_TITLE), 0, CM_OPEN_SQLITE_DB, FALSE,
                            MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
}
