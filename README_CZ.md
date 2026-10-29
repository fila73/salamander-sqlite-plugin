# SQLite Plugin pro Open Salamander

Výkonný prohlížeč SQLite databází pro souborového správce **Altap Salamander / Open Salamander**.

## Autoři a poděkování
- **Autoři**: Open Salamander Authors, Red Salamander Authors, Ondřej Kotas ([KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander))
- **Implementace Dark Mode**: Využívá podporu a host policy tmavého režimu z forku [KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander) od Ondřeje Kotase
- **Port do frameworku Salamander**: fila73

## Funkce a vlastnosti

- **Prohlížeč souborů (`F3` / Rychlý náhled)**:
  - Automatická detekce formátu SQLite 3 podle hlavičky souboru (`"SQLite format 3\000"`).
  - Podpora přípon: `.db`, `.sqlite`, `.sqlite3`, `.db3`, `.s3db`, `.sl3`, `.sqlite2`.
- **Prohlížení dat v tabulkách a pohledech**:
  - Rychlý virtuální seznam (`LVS_OWNERDATA`) s dvojitým bufferingem, mřížkou a výběrem celých řádků.
  - Stránkování dat (`<< První`, `< Předchozí`, `Další >`, `Poslední >>`).
  - Nastavitelná velikost stránky (50, 100, 200, 500, 1000, Vše).
  - Rychlé živé vyhledávání a filtrování řádků v tabulce.
  - Řazení podle sloupců (kliknutím na záhlaví sloupce vzestupně/sestupně).
  - Zřetelné zobrazení hodnot `NULL` (`[NULL]`) a binárních dat BLOB (`<BLOB X B>`).
  - Správné zarovnání textu a čísel.
- **Prohlížeč schématu a DDL**:
  - Zobrazení kompletního SQL DDL pro tabulku a indexy (`CREATE TABLE ...`, `CREATE INDEX ...`).
  - Přehled sloupců: název, datový typ, NOT NULL omezení, výchozí hodnota, primární klíč (PK).
  - Seznam cizích klíčů a indexů.
- **SQL Editor dotazů**:
  - Možnost spouštění libovolných `SELECT` dotazů s bezpečnostním omezením počtu řádků.
  - Výsledky se zobrazují přímo v gridu s měřením doby provádění v milisekundách.
- **Export a schránka**:
  - Kopírování buňky, řádku nebo všech řádků do schránky (jako text, CSV nebo TSV).
  - Export celé tabulky do standardního CSV souboru (s UTF-8 BOM).
- **Podpora tmavého režimu (Dark Mode)**:
  - Plná integrace s tmavým režimem ve forku [KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander) (`PluginDarkMode` / WinLib tématické jádro).
- **Lokalizace**:
  - Angličtina (`english.slg`) a čeština (`czech.slg`).

## Sestavení (Build)

### MSVC (MSBuild)
Vyžaduje Visual Studio 2022+ (C++ desktop workload) a SDK pluginů Open Salamanderu. Očekávané uspořádání: `salamander\plugins\salamander-sqlite-plugin` (tento repozitář) a SDK v `salamander\samandarin\src\plugins\shared`; SDK se najde automaticky, jinak zadejte `/p:SalamanderSdkDir=<cesta k adresáři obsahujícímu plugins\shared>`.

```cmd
MSBuild src\vcxproj\sqlite.vcxproj /p:Configuration=Release /p:Platform=x64
MSBuild src\vcxproj\lang_sqlite.vcxproj /p:Configuration=Release /p:Platform=x64
MSBuild src\vcxproj\lang_sqlite_cs.vcxproj /p:Configuration=Release /p:Platform=x64
```

Výstupní soubory v `build\Release_x64\`:
- `sqlite.spl` - Plugin DLL (x64, statické CRT)
- `lang\english.slg` - Anglický jazykový modul
- `lang\czech.slg` - Český jazykový modul

Pro ladicí sestavení použijte `/p:Configuration=Debug` (`build\Debug_x64\`). Projekty lze otevřít i ve Visual Studiu.

## Instalace do Salamanderu

1. Zkopírujte `sqlite.spl`, `english.slg` a `czech.slg` do složky `plugins/sqlite/` v instalaci Open Salamanderu.
2. Spusťte Open Salamander, otevřete **Plugins -> Plugins Manager -> Add...** a vyberte `sqlite.spl`.
3. Stiskněte `F3` na libovolném `.db` nebo `.sqlite` souboru pro okamžité zobrazení!
