# Návod na vývoj pluginů pro Open Salamander (Template & Best Practices)

Tento dokument slouží jako kompletní vzor a kuchařka pro tvorbu a údržbu pluginů pro **Open Salamander** (a Dark Mode fork **KRtkovo-eu-AI/salamander**), postavený na ověřených postupech z vývoje pluginu `salamander-sqlite-plugin`.

---

## 1. Architektura a adresářová struktura pluginu

Plugin do Salamandera se skládá z:
- **`*.spl`** – Samotná dynamická knihovna pluginu (Windows DLL přejmenovaná na `.spl`).
- **`lang/*.slg`** – Jazykové knihovny s přeloženými texty a dialogy (`english.slg`, `czech.slg`).

### Doporučená adresářová struktura:

```
salamander-sqlite-plugin/        # v salamander\plugins\
├── src/
│   ├── gdrive.cpp          # Hlavní vstup, DllMain, CPluginInterface, CPluginInterfaceForFS/Viewer
│   ├── gdrive.h            # Hlavičkový soubor pluginu
│   ├── gdrive.def          # Exportní definice (SalamanderPluginEntry2)
│   ├── gdrive.rc           # Hlavní resources (verze, ikony)
│   ├── gdrive.rh2          # Definice číselných ID resource a příkazů
│   ├── versinfo.rh2        # Hlavička pro verzi pluginu
│   ├── precomp.h           # Precompiled headers se Salamander SDK
│   ├── gdrivedarkmode.h    # Implementace Dark Mode podpory
│   ├── gdrivedarkmode.cpp
│   └── lang/
│       ├── lang.rh         # Společné resource ID pro jazyky
│       ├── lang_en.rc      # Anglické texty a dialogy
│       └── lang_cs.rc      # České texty a dialogy
├── test/                   # Unit testy pro nezávislou engine vrstvu
├── src/vcxproj/            # MSBuild: sqlite_common.props, sqlite.vcxproj, lang_sqlite*.vcxproj (MSVC)
├── README.md               # Dokumentace a návod k instalaci
└── .gitignore
```

---

## 2. Klíčové rozhraní Open Salamander Plugin SDK

Plugin komunikuje se Salamanderem přes virtuální C++ třídy definované v SDK `salamander\samandarin\src\plugins\shared\`:

### Hlavní rozhraní:
1. **`CPluginInterface`**:
   - `Init()` – Inicializace pluginu po načtení.
   - `Connect(HWND parent, CSalamanderConnectAbstract* salamander)` – Zde plugin registruje své schopnosti v Salamanderu (např. `salamander->AddFileSystem(...)`, `salamander->AddViewer(...)`, `salamander->AddIcon(...)`).
   - `LoadConfiguration(CSalamanderRegistryAbstract* registry, HKEY regKey)` – Načtení konfigurace z registru.
   - `SaveConfiguration(CSalamanderRegistryAbstract* registry, HKEY regKey)` – Uložení konfigurace.
   - `About(HWND parent)` a `Configuration(HWND parent)` – Standardní dialogy O aplikaci a Nastavení.

2. **`CPluginInterfaceForFS`** (pro souborové systémy / Google Drive):
   - Procházení adresářů (`OpenDirectory`, `ReadDirectory`, `CloseDirectory`).
   - Práce se soubory (`GetFileInfo`, `Copy`, `Move`, `Delete`, `CreateDirectory`).
   - Souborové operace na pozadí.

3. **`CPluginInterfaceForViewer`** (pro interní prohlížeč souborů):
   - `ViewFile(...)` – Spuštění samostatného vlákna prohlížeče (`CViewerThread`).

---

## 3. Zásadní pravidla a řešení známých chyb (Gotchas)

### ⚠️ 1. Statické linkování CRT (`/MT`)
Plugin se musí sestavit s `/MT` (`/MTd` v Debugu), aby nezávisel na `vcruntime`/`ucrt` DLL. Jinak na jiných počítačích Salamander zahlásí:
> `Chyba 126: Uvedený modul nebyl nalezen.`

**Řešení v `sqlite.vcxproj`**: `<RuntimeLibrary>MultiThreaded</RuntimeLibrary>` (Release) a `MultiThreadedDebug` (Debug).

Vždy zkontrolujte závislosti příkazem:
```powershell
dumpbin /dependents build\Release_x64\sqlite.spl
```
Musí obsahovat pouze standardní knihovny Windows (KERNEL32, USER32, GDI32, COMCTL32, ...), žádné `vcruntime*`/`msvcp*`/`api-ms-win-crt-*`.

---

### ⚠️ 2. Ošetření prázdného klíče registru při prvním spuštění (Chyba 6)
Při prvním spuštění, kdy v registru ještě neexistují klíče pluginu, předává Salamander do `LoadConfiguration` ukazatel `regKey == NULL`. Pokud se zavolá `registry->GetValue(regKey, ...)`, skončí to chybou:
> `(6) Neplatný popisovač. Registry value: Expected type...`

**Správná implementace**:
```cpp
void WINAPI CPluginInterface::LoadConfiguration(CSalamanderRegistryAbstract* registry, HKEY regKey)
{
    // Nastavit výchozí hodnoty
    CfgSavePosition = FALSE;
    CfgDefaultPageSize = 200;

    // Číst z registru POUZE pokud klíč existuje
    if (registry != NULL && regKey != NULL)
    {
        registry->GetValue(regKey, "SavePosition", REG_DWORD, &CfgSavePosition, sizeof(CfgSavePosition));
        // ...
    }
}
```

---

### ⚠️ 3. Automatická registrace a migrace konfigurace v `Connect()`
Aby uživatel nemusel ručně konfigurovat asociace v nastavení Salamandera, plugin se sám zaregistruje v metodě `Connect()`:

```cpp
void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    // 1. Zaregistrovat ikony pluginu v GUI Salamandera
    CGUIIconListAbstract* iconList = salamander->GetPluginIconList();
    if (iconList)
    {
        HICON hIcon = LoadIcon(DLLInstance, MAKEINTRESOURCE(IDI_PLUGIN));
        if (hIcon) iconList->AddIcon(hIcon);
    }

    // 2. Pro prohlížeče zaregistrovat podporované masky souborů (nebo AddFileSystem pro FS)
    salamander->AddViewer("*.gdrive;*.gdoc;*.gsheet", FALSE);

    // 3. Pro stávající uživatele zajistit aktualizaci konfigurace při změně verze
    if (ConfigVersion < 2)
    {
        salamander->AddViewer("*.gdrive;*.gdoc;*.gsheet", TRUE);
    }
}
```

---

### ⚠️ 4. Správné škálování pro High-DPI monitory (125 %, 150 %, 200 %)
Nikdy nenastavujte výšky ovládacích prvků (Edit boxy, lišty, tlačítka) pevnými pixely (např. 22px). Na monitorech s vyšším DPI se do nich nevejde kurzor ani text.

**Dynamický výpočet výšky**:
```cpp
int dpi = 96;
HDC hdc = GetDC(HWindow);
if (hdc) {
    dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(HWindow, hdc);
}
auto ScaleDpi = [dpi](int v) -> int { return MulDiv(v, dpi, 96); };

// Výšku Edit boxu sladit s výškou ComboBoxu:
RECT rcCombo = {0};
GetWindowRect(m_hCombo, &rcCombo);
int ctrlH = rcCombo.bottom - rcCombo.top;
if (ctrlH < ScaleDpi(26)) ctrlH = ScaleDpi(26);

// Nastavení pozice Edit boxu s přesnou výškou:
SetWindowPos(m_hEditFilter, NULL, x, ctrlY, ScaleDpi(170), ctrlH, SWP_NOZORDER);
```

---

### ⚠️ 5. Zamezení zobrazení prázdného okna při chybě otevření
Pokud selže otevření souboru/spojení (uzamčený soubor, výpadek sítě), nesmí prohlížeč zůstat viset na obrazovce jako prázdné šedé okno.

**V těle vlákna (`CViewerThread::Body`)**:
```cpp
if (!window->OpenFile(m_name, FALSE))
{
    // Chyba již byla uživateli zobrazena dialogem
    // Zničit okno a nevstupovat do message loopu:
    DestroyWindow(window->HWindow);
}
else
{
    ShowWindow(window->HWindow, SW_SHOWNORMAL);
    // standardní GetMessage smyčka...
}
```

---

## 4. Sestavení pomocí MSBuild (MSVC)

Projekty jsou v `src\vcxproj\`; společné cesty definuje `sqlite_common.props` (`SalamanderSdkDir` ve výchozím stavu `<repo>\..\..\samandarin\src`, přepsatelné přes `/p:SalamanderSdkDir=`). Výstup jde do `build\<Configuration>_<Platform>\`.

```cmd
MSBuild src\vcxproj\sqlite.vcxproj /p:Configuration=Release /p:Platform=x64
MSBuild src\vcxproj\lang_sqlite.vcxproj /p:Configuration=Release /p:Platform=x64
MSBuild src\vcxproj\lang_sqlite_cs.vcxproj /p:Configuration=Release /p:Platform=x64
```

- `sqlite.vcxproj` – `sqlite.spl`; sdílené zdroje SDK (`auxtools.cpp`, `dbg.cpp`, `mhandles.cpp`, `winliblt.cpp`, `plugindarkmode.cpp`) se kompilují přímo z `$(SalamanderSharedDir)`.
- `lang_sqlite.vcxproj` / `lang_sqlite_cs.vcxproj` – resource-only DLL `lang\english.slg` / `lang\czech.slg`.
- Každý nový `.cpp` ze `src\` musí být přidán do `sqlite.vcxproj`, jinak vzniknou nevyřešené externí symboly.

---

## 5. Šablona `gdrive.def`

```def
LIBRARY "gdrive.spl"
EXPORTS
    SalamanderPluginEntry2 @2
```

---

## 6. Správa Git repozitáře a GitHub Releases

### Inicializace a propojení s GitHubem:
```powershell
# 1. Nastavení identity autora
git config user.name "Filip Novák"
git config user.email "filip.novak@gmail.com"

# 2. Inicializace a commit
git init -b main
git add .
git commit -m "Initial commit of Google Drive plugin for Open Salamander"

# 3. Vytvoření veřejného repozitáře na GitHubu
gh repo create fila73/salamander-gdrive-plugin --public --source=. --remote=origin --push
```

### Balení a publikování nového Release (např. v0.1):
```powershell
# 1. Kompilace čistého sestavení
MSBuild -t:Rebuild src\vcxproj\sqlite.vcxproj /p:Configuration=Release /p:Platform=x64
MSBuild -t:Rebuild src\vcxproj\lang_sqlite.vcxproj /p:Configuration=Release /p:Platform=x64
MSBuild -t:Rebuild src\vcxproj\lang_sqlite_cs.vcxproj /p:Configuration=Release /p:Platform=x64

# 2. Příprava distribučního ZIP archivu
$stage = "release_stage"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Path "$stage\lang" | Out-Null
Copy-Item "build\Release_x64\sqlite.spl" -Destination "$stage\sqlite.spl"
Copy-Item "build\Release_x64\lang\english.slg" -Destination "$stage\lang\english.slg"
Copy-Item "build\Release_x64\lang\czech.slg" -Destination "$stage\lang\czech.slg"

$zipName = "salamander-gdrive-plugin-v0.1.zip"
if (Test-Path $zipName) { Remove-Item -Force $zipName }
Compress-Archive -Path "$stage\*" -DestinationPath $zipName
Remove-Item -Recurse -Force $stage

# 3. Publikování release přes GitHub CLI
gh release create v0.1 $zipName --title "v0.1" --notes "Initial release of Google Drive plugin for Open Salamander"
```

---

## 7. Instalace pluginu uživatelem

1. Stáhnout `salamander-gdrive-plugin-v0.1.zip` z GitHub Releases.
2. Rozbalit obsah archivu do podadresáře pluginů v Salamanderu:
   `C:\Program Files\Open Salamander\plugins\gdrive\` (nebo adresář Salamandera).
3. Struktura v adresáři musí být:
   ```
   plugins/gdrive/gdrive.spl
   plugins/gdrive/lang/english.slg
   plugins/gdrive/lang/czech.slg
   ```
4. V menu Salamandera: **Plugins → Plugins Manager... → Add...** vybrat `gdrive.spl`.
