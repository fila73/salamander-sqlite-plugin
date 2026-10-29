@echo off
rem Builds and runs the SQLite engine test with MSVC (run from any dir).
rem Needs a VS developer environment; set SalamanderSdkDir to override the SDK location.
setlocal
set ROOT=%~dp0..\
if not defined SalamanderSdkDir set SalamanderSdkDir=%ROOT%..\..\samandarin\src
if defined VSCMD_VER goto have_env
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" echo Run from a VS x64 developer prompt & exit /b 1
call "%VCVARS%" >nul
:have_env
set OUT=%ROOT%build\test\
if not exist "%OUT%" mkdir "%OUT%"
set SQ_DEFS=/DSQLITE_THREADSAFE=1 /DSQLITE_ENABLE_FTS5 /DSQLITE_ENABLE_RTREE /DSQLITE_ENABLE_JSON1 /DSQLITE_ENABLE_COLUMN_METADATA /DSQLITE_OMIT_LOAD_EXTENSION
cl /nologo /c /O2 /MT /w %SQ_DEFS% /Fo"%OUT%sqlite3.obj" "%ROOT%src\sqlite\sqlite3.c" || exit /b 1
cl /nologo /std:c++17 /EHsc /MT /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /DWIN32 /D_WINDOWS /DUNICODE_NOT_USED /DWINVER=0x0601 /D_WIN32_WINNT=0x0601 ^
  /I"%ROOT%src" /I"%ROOT%src\sqlite" /I"%SalamanderSdkDir%\plugins\shared" /I"%SalamanderSdkDir%" ^
  /Fo"%OUT%\" /Fe"%OUT%test_sqlite_engine.exe" "%ROOT%test\test_sqlite_engine.cpp" "%ROOT%src\sqlite_engine.cpp" "%OUT%sqlite3.obj" ^
  /link user32.lib advapi32.lib || exit /b 1
"%OUT%test_sqlite_engine.exe"
