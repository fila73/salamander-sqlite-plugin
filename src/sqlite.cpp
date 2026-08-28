// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

CPluginInterface PluginInterface;
CPluginInterfaceForViewer InterfaceForViewer;
CPluginInterfaceForMenuExt InterfaceForMenuExt;

const char* PluginNameEN = "SQLite Viewer";
const char* PluginNameShort = "SQLITE";

unsigned char* LowerCase = NULL;
unsigned char* UpperCase = NULL;

HINSTANCE DLLInstance = NULL;
HINSTANCE HLanguage = NULL;

CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
int SalamanderVersion = 0;
CSalamanderGUIAbstract* SalamanderGUI = NULL;

BOOL CfgSavePosition = FALSE;
WINDOWPLACEMENT CfgWindowPlacement = { sizeof(WINDOWPLACEMENT) };
int CfgDefaultPageSize = 200;
int CfgQueryRowCap = 5000;
int CfgDefaultView = 0;
BOOL CfgDirectOpen = TRUE;

static const char* CONFIG_SAVEPOS = "SavePosition";
static const char* CONFIG_WNDPLACEMENT = "WindowPlacement";
static const char* CONFIG_PAGESIZE = "DefaultPageSize";
static const char* CONFIG_ROWCAP = "QueryRowCap";
static const char* CONFIG_DEFAULTVIEW = "DefaultView";
static const char* CONFIG_DIRECTOPEN = "DirectOpen";

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX initCtrls;
        initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        initCtrls.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&initCtrls);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        SqliteDarkMode::ReleaseTheme();
    }
    return TRUE;
}

char* LoadStr(int resID)
{
    if (SalamanderGeneral != NULL && HLanguage != NULL)
        return SalamanderGeneral->LoadStr(HLanguage, resID);
    static char buf[1024];
    buf[0] = 0;
    LoadStringA(HLanguage ? HLanguage : DLLInstance, resID, buf, sizeof(buf));
    return buf;
}

extern "C" {

int WINAPI SalamanderPluginGetReqVer()
{
    return 103;
}

int WINAPI SalamanderPluginGetSDKVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();

    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), PluginNameEN);
    if (HLanguage == NULL)
        HLanguage = DLLInstance;

    if (SalamanderVersion < 102)
    {
        MessageBoxA(salamander->GetParentWindow(),
                    LoadStr(IDS_REQUIRE_SAL500),
                    PluginNameEN, MB_OK | MB_ICONERROR);
        return NULL;
    }

    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGeneral->GetLowerAndUpperCase(&LowerCase, &UpperCase);
    SalamanderGUI = salamander->GetSalamanderGUI();

    if (!InitializeWinLib(PluginNameEN, DLLInstance))
        return NULL;

    BOOL useDark = FALSE;
    if (SalamanderGeneral->GetConfigParameter(SALCFG_USEWINDOWSDARKMODE, &useDark, sizeof(useDark), NULL))
    {
        PluginDarkMode_SetHostPolicyAvailable(TRUE, useDark);
    }
    SqliteDarkMode::InitTheme();

    ViewerAccels = LoadAccelerators(DLLInstance, MAKEINTRESOURCE(IDA_ACCELERATORS));

    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME),
                                   FUNCTION_CONFIGURATION | FUNCTION_LOADSAVECONFIGURATION | FUNCTION_VIEWER,
                                   VERSINFO_VERSION_NO_PLATFORM,
                                   VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION),
                                   PluginNameShort,
                                   NULL,
                                   NULL);

    salamander->SetPluginHomePageURL(LoadStr(IDS_PLUGIN_HOME));

    return &PluginInterface;
}

} // extern "C"

//
// CPluginInterfaceForViewer
//

BOOL WINAPI CPluginInterfaceForViewer::CanViewFile(const char* name)
{
    if (!name || !*name)
        return FALSE;

    // Check SQLite header magic bytes first ("SQLite format 3\000")
    if (SqliteEngine::CSqliteEngine::IsSqliteDatabase(name))
        return TRUE;

    // Check extension
    const char* ext = strrchr(name, '.');
    if (ext)
    {
        if (_stricmp(ext, ".db") == 0 ||
            _stricmp(ext, ".sqlite") == 0 ||
            _stricmp(ext, ".sqlite3") == 0 ||
            _stricmp(ext, ".db3") == 0 ||
            _stricmp(ext, ".s3db") == 0 ||
            _stricmp(ext, ".sl3") == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL WINAPI CPluginInterfaceForViewer::ViewFile(const char* name, int left, int top, int width, int height,
                                              UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                              BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                              int enumFilesSourceUID, int enumFilesCurrentIndex)
{
    HANDLE contEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!contEvent)
        return FALSE;

    BOOL success = FALSE;
    CViewerThread* t = new CViewerThread(name, left, top, width, height,
                                         showCmd, alwaysOnTop, returnLock, lock,
                                         lockOwner, contEvent, &success,
                                         enumFilesSourceUID, enumFilesCurrentIndex);
    if (t != NULL)
    {
        if (t->Create(ThreadQueue) != NULL)
        {
            t = NULL;
            WaitForSingleObject(contEvent, INFINITE);
        }
        else
        {
            delete t;
        }
    }

    CloseHandle(contEvent);
    return success;
}

//
// CPluginInterface
//

void WINAPI CPluginInterface::About(HWND parent)
{
    OnAbout(parent);
}

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
    BOOL ret = ViewerWindowQueue.Empty();
    if (!ret && (force || SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_VIEWER_OPENWNDS),
                                                           LoadStr(IDS_PLUGINNAME),
                                                           MB_YESNO | MB_ICONQUESTION) == IDYES))
    {
        ret = ViewerWindowQueue.CloseAllWindows(force) || force;
    }

    if (ret)
    {
        if (!ThreadQueue.KillAll(force) && !force)
            ret = FALSE;
        else
        {
            if (ViewerAccels)
            {
                DestroyAcceleratorTable(ViewerAccels);
                ViewerAccels = NULL;
            }

            SqliteDarkMode::ReleaseTheme();
            ReleaseWinLib(DLLInstance);
        }
    }

    return ret;
}

static const char* CONFIG_VERSION = "ConfigVersion";
static const DWORD CURRENT_CONFIG_VERSION = 1;
DWORD ConfigVersion = CURRENT_CONFIG_VERSION;

void WINAPI CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    if (registry != NULL && regKey != NULL)
    {
        if (!registry->GetValue(regKey, CONFIG_VERSION, REG_DWORD, &ConfigVersion, sizeof(DWORD)))
            ConfigVersion = CURRENT_CONFIG_VERSION;

        DWORD dwVal = 0;
        if (registry->GetValue(regKey, CONFIG_SAVEPOS, REG_DWORD, &dwVal, sizeof(dwVal)))
            CfgSavePosition = (dwVal != 0);

        if (registry->GetValue(regKey, CONFIG_WNDPLACEMENT, REG_BINARY, &CfgWindowPlacement, sizeof(CfgWindowPlacement)))
        {
            if (CfgWindowPlacement.length != sizeof(WINDOWPLACEMENT))
                CfgSavePosition = FALSE;
        }

        if (registry->GetValue(regKey, CONFIG_PAGESIZE, REG_DWORD, &dwVal, sizeof(dwVal)))
            CfgDefaultPageSize = (int)dwVal;

        if (registry->GetValue(regKey, CONFIG_ROWCAP, REG_DWORD, &dwVal, sizeof(dwVal)))
            CfgQueryRowCap = (int)dwVal;

        if (registry->GetValue(regKey, CONFIG_DEFAULTVIEW, REG_DWORD, &dwVal, sizeof(dwVal)))
            CfgDefaultView = (int)dwVal;

        if (registry->GetValue(regKey, CONFIG_DIRECTOPEN, REG_DWORD, &dwVal, sizeof(dwVal)))
            CfgDirectOpen = (dwVal != 0);
    }
}

void WINAPI CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    if (registry != NULL && regKey != NULL)
    {
        DWORD v = CURRENT_CONFIG_VERSION;
        registry->SetValue(regKey, CONFIG_VERSION, REG_DWORD, &v, sizeof(DWORD));

        DWORD dwVal = CfgSavePosition ? 1 : 0;
        registry->SetValue(regKey, CONFIG_SAVEPOS, REG_DWORD, &dwVal, sizeof(dwVal));

        if (CfgSavePosition && CfgWindowPlacement.length == sizeof(WINDOWPLACEMENT))
        {
            registry->SetValue(regKey, CONFIG_WNDPLACEMENT, REG_BINARY, &CfgWindowPlacement, sizeof(CfgWindowPlacement));
        }

        dwVal = (DWORD)CfgDefaultPageSize;
        registry->SetValue(regKey, CONFIG_PAGESIZE, REG_DWORD, &dwVal, sizeof(dwVal));

        dwVal = (DWORD)CfgQueryRowCap;
        registry->SetValue(regKey, CONFIG_ROWCAP, REG_DWORD, &dwVal, sizeof(dwVal));

        dwVal = (DWORD)CfgDefaultView;
        registry->SetValue(regKey, CONFIG_DEFAULTVIEW, REG_DWORD, &dwVal, sizeof(dwVal));

        dwVal = CfgDirectOpen ? 1 : 0;
        registry->SetValue(regKey, CONFIG_DIRECTOPEN, REG_DWORD, &dwVal, sizeof(dwVal));
    }
}

void WINAPI CPluginInterface::Configuration(HWND parent)
{
    OnConfiguration(parent);
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
}

CPluginInterfaceForViewerAbstract* WINAPI CPluginInterface::GetInterfaceForViewer()
{
    return &InterfaceForViewer;
}

CPluginInterfaceForMenuExtAbstract* WINAPI CPluginInterface::GetInterfaceForMenuExt()
{
    return &InterfaceForMenuExt;
}
