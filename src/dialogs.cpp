// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "dialogs.h"

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, HWND hParent, CObjectOrigin origin)
    : CDialog(hInstance, resID, hParent, origin)
{
}

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, int helpID, HWND hParent, CObjectOrigin origin)
    : CDialog(hInstance, resID, helpID, hParent, origin)
{
}

INT_PTR CCommonDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG && Parent != NULL)
    {
        SalamanderGeneral->MultiMonCenterWindow(HWindow, Parent, TRUE);
    }
    return CDialog::DialogProc(uMsg, wParam, lParam);
}

void CCommonDialog::NotifDlgJustCreated()
{
    SalamanderGUI->ArrangeHorizontalLines(HWindow);
}

void CCommonPropSheetPage::NotifDlgJustCreated()
{
    SalamanderGUI->ArrangeHorizontalLines(HWindow);
}

CConfigPageViewer::CConfigPageViewer()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGEVIEWER, IDD_CFGPAGEVIEWER, PSP_HASHELP, NULL)
{
}

void CConfigPageViewer::Transfer(CTransferInfo& ti)
{
    int savePos = CfgSavePosition ? 1 : 0;
    int directOpen = CfgDirectOpen ? 1 : 0;
    ti.RadioButton(IDC_CFG_SAVEPOSONCLOSE, 1, savePos);
    ti.RadioButton(IDC_CFG_SETBYMAINWINDOW, 0, savePos);
    ti.CheckBox(IDC_CFG_DIRECTOPEN, directOpen);
    ti.EditLine(IDC_CFG_ROWCAP, CfgQueryRowCap, TRUE);
    if (ti.Type == ttDataFromWindow)
    {
        CfgSavePosition = (savePos != 0);
        CfgDirectOpen = (directOpen != 0);
    }
}

INT_PTR CConfigPageViewer::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        HWND hComboPage = GetDlgItem(HWindow, IDC_CFG_PAGESIZE);
        const char* sizes[] = { "50", "100", "200", "500", "1000", "All" };
        int sizeVals[] = { 50, 100, 200, 500, 1000, 0 };
        int selIdx = 2; // 200
        for (int i = 0; i < 6; ++i)
        {
            SendMessageA(hComboPage, CB_ADDSTRING, 0, (LPARAM)sizes[i]);
            if (CfgDefaultPageSize == sizeVals[i])
                selIdx = i;
        }
        SendMessage(hComboPage, CB_SETCURSEL, selIdx, 0);

        HWND hComboView = GetDlgItem(HWindow, IDC_CFG_DEFAULTVIEW);
        SendMessageA(hComboView, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEW_DATA_TAB));
        SendMessageA(hComboView, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEW_SCHEMA_TAB));
        SendMessageA(hComboView, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEW_SQL_TAB));
        SendMessage(hComboView, CB_SETCURSEL, (CfgDefaultView >= 0 && CfgDefaultView <= 2) ? CfgDefaultView : 0, 0);
    }
    else if (uMsg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK)
    {
        HWND hComboPage = GetDlgItem(HWindow, IDC_CFG_PAGESIZE);
        int sel = (int)SendMessage(hComboPage, CB_GETCURSEL, 0, 0);
        int sizeVals[] = { 50, 100, 200, 500, 1000, 0 };
        if (sel >= 0 && sel < 6)
            CfgDefaultPageSize = sizeVals[sel];

        HWND hComboView = GetDlgItem(HWindow, IDC_CFG_DEFAULTVIEW);
        CfgDefaultView = (int)SendMessage(hComboView, CB_GETCURSEL, 0, 0);
    }

    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

class CCenteredPropertyWindow : public CWindow
{
protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS* pos = (WINDOWPOS*)lParam;
            if (pos->flags & SWP_SHOWWINDOW)
            {
                HWND hParent = GetParent(HWindow);
                if (hParent != NULL)
                    SalamanderGeneral->MultiMonCenterWindow(HWindow, hParent, TRUE);
            }
            break;
        }
        case WM_APP + 1000:
        {
            DetachWindow();
            delete this;
            return 0;
        }
        }
        return CWindow::WindowProc(uMsg, wParam, lParam);
    }
};

static int CALLBACK CenterCallback(HWND HWindow, UINT uMsg, LPARAM lParam)
{
    if (uMsg == PSCB_INITIALIZED)
    {
        CCenteredPropertyWindow* wnd = new CCenteredPropertyWindow;
        if (wnd != NULL)
        {
            wnd->AttachToWindow(HWindow);
            if (wnd->HWindow == NULL)
                delete wnd;
            else
                PostMessage(wnd->HWindow, WM_APP + 1000, 0, 0);
        }
    }
    return 0;
}

static DWORD LastCfgPage = 0;

CConfigDialog::CConfigDialog(HWND parent)
    : CPropertyDialog(parent, HLanguage, LoadStr(IDS_CFG_TITLE),
                      LastCfgPage, PSH_USECALLBACK | PSH_NOAPPLYNOW | PSH_HASHELP,
                      NULL, &LastCfgPage, CenterCallback)
{
    Add(&PageViewer);
}

void OnConfiguration(HWND hParent)
{
    static BOOL InConfiguration = FALSE;
    if (InConfiguration)
    {
        SalamanderGeneral->SalMessageBox(hParent, LoadStr(IDS_CFG_ALREADY_OPENED), LoadStr(IDS_PLUGINNAME),
                                         MB_ICONINFORMATION | MB_OK);
        return;
    }
    InConfiguration = TRUE;
    if (CConfigDialog(hParent).Execute() == IDOK)
    {
        ViewerWindowQueue.BroadcastMessage(WM_USER_VIEWERCFGCHNG, 0, 0);
    }
    InConfiguration = FALSE;
}

void OnAbout(HWND hParent)
{
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "%s %s\n\n"
             "%s\n\n"
             "SQLite Library Version: %s\n\n"
             "%s",
             LoadStr(IDS_PLUGINNAME), VERSINFO_VERSION,
             LoadStr(IDS_PLUGIN_DESCRIPTION),
             sqlite3_libversion(),
             VERSINFO_COPYRIGHT);
    SalamanderGeneral->SalMessageBox(hParent, buf, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}
