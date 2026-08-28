// SPDX-FileCopyrightText: 2026 Open Salamander Authors & Red Salamander Authors
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "blob_dialog.h"
#include "sqlitedarkmode.h"

#include <gdiplus.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")

namespace BlobDialog
{

enum class BlobViewMode
{
    Hex,
    Text,
    Image
};

struct BlobInspectorState
{
    std::string colName;
    SqliteEngine::CellValue cell;
    BlobViewMode mode = BlobViewMode::Hex;

    Gdiplus::Bitmap* pImageBitmap = nullptr;
    UINT imgWidth = 0;
    UINT imgHeight = 0;

    HWND hwndMain = NULL;
    HWND hwndInfoLabel = NULL;
    HWND hwndBtnHex = NULL;
    HWND hwndBtnText = NULL;
    HWND hwndBtnImage = NULL;
    HWND hwndEditView = NULL;
    HWND hwndImageCanvas = NULL;
    HWND hwndBtnSave = NULL;
    HWND hwndBtnCopy = NULL;
    HWND hwndBtnClose = NULL;

    HFONT hFontNormal = NULL;
    HFONT hFontMono = NULL;
    std::string hexText;
    std::string plainText;
};

static LRESULT CALLBACK ImageCanvasProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BlobInspectorState* state = (BlobInspectorState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!state) return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        // Double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // Background
        COLORREF bgCol = SqliteDarkMode::IsDarkMode() ? RGB(26, 26, 26) : RGB(240, 240, 240);
        HBRUSH hBrBg = CreateSolidBrush(bgCol);
        FillRect(memDC, &rc, hBrBg);
        DeleteObject(hBrBg);

        if (state->pImageBitmap && state->imgWidth > 0 && state->imgHeight > 0)
        {
            Gdiplus::Graphics g(memDC);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

            // Compute fitted rect
            float scaleX = (float)(w - 20) / (float)state->imgWidth;
            float scaleY = (float)(h - 20) / (float)state->imgHeight;
            float scale = (scaleX < scaleY) ? scaleX : scaleY;
            if (scale > 1.0f) scale = 1.0f; // don't upscale beyond 100%

            int drawW = (int)(state->imgWidth * scale);
            int drawH = (int)(state->imgHeight * scale);
            int drawX = (w - drawW) / 2;
            int drawY = (h - drawH) / 2;

            // Draw image border / shadow
            RECT rcImg = { drawX - 1, drawY - 1, drawX + drawW + 1, drawY + drawH + 1 };
            HBRUSH hBrBorder = CreateSolidBrush(SqliteDarkMode::IsDarkMode() ? RGB(60, 60, 60) : RGB(180, 180, 180));
            FrameRect(memDC, &rcImg, hBrBorder);
            DeleteObject(hBrBorder);

            g.DrawImage(state->pImageBitmap, drawX, drawY, drawW, drawH);
        }
        else
        {
            // Placeholder text
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, SqliteDarkMode::IsDarkMode() ? RGB(140, 140, 140) : RGB(100, 100, 100));
            SelectObject(memDC, state->hFontNormal);
            const char* msg = "No image data or unsupported image format.";
            DrawTextA(memDC, msg, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static void LayoutBlobDialog(BlobInspectorState* state)
{
    if (!state || !state->hwndMain) return;

    RECT rc;
    GetClientRect(state->hwndMain, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    int dpi = 96;
    HDC hdc = GetDC(state->hwndMain);
    if (hdc)
    {
        dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(state->hwndMain, hdc);
    }
    if (dpi <= 0) dpi = 96;

    auto ScaleDpi = [dpi](int v) -> int { return MulDiv(v, dpi, 96); };

    int margin = ScaleDpi(10);
    int btnH = ScaleDpi(26);
    int topH = ScaleDpi(30);

    // Top info label
    SetWindowPos(state->hwndInfoLabel, NULL, margin, margin, width - margin * 2, topH, SWP_NOZORDER);

    // Mode Buttons
    int btnW = ScaleDpi(90);
    int modeX = margin;
    int modeY = margin + topH + ScaleDpi(6);

    SetWindowPos(state->hwndBtnHex, NULL, modeX, modeY, btnW, btnH, SWP_NOZORDER);
    modeX += btnW + ScaleDpi(6);

    SetWindowPos(state->hwndBtnText, NULL, modeX, modeY, btnW, btnH, SWP_NOZORDER);
    modeX += btnW + ScaleDpi(6);

    if (state->hwndBtnImage)
    {
        SetWindowPos(state->hwndBtnImage, NULL, modeX, modeY, btnW, btnH, SWP_NOZORDER);
    }

    // Bottom buttons
    int bottomBtnY = height - margin - btnH;
    int rightBtnW = ScaleDpi(130);
    int saveX = margin;
    SetWindowPos(state->hwndBtnSave, NULL, saveX, bottomBtnY, rightBtnW, btnH, SWP_NOZORDER);

    int copyX = saveX + rightBtnW + ScaleDpi(8);
    int copyW = ScaleDpi(100);
    SetWindowPos(state->hwndBtnCopy, NULL, copyX, bottomBtnY, copyW, btnH, SWP_NOZORDER);

    int closeW = ScaleDpi(90);
    int closeX = width - margin - closeW;
    SetWindowPos(state->hwndBtnClose, NULL, closeX, bottomBtnY, closeW, btnH, SWP_NOZORDER);

    // Main content area
    int contentY = modeY + btnH + ScaleDpi(8);
    int contentH = bottomBtnY - contentY - ScaleDpi(8);
    int contentW = width - margin * 2;

    if (contentH < 50) contentH = 50;

    SetWindowPos(state->hwndEditView, NULL, margin, contentY, contentW, contentH, SWP_NOZORDER);
    SetWindowPos(state->hwndImageCanvas, NULL, margin, contentY, contentW, contentH, SWP_NOZORDER);
}

static void UpdateBlobViewMode(BlobInspectorState* state)
{
    if (!state) return;

    bool isHex = (state->mode == BlobViewMode::Hex);
    bool isText = (state->mode == BlobViewMode::Text);
    bool isImage = (state->mode == BlobViewMode::Image);

    SendMessage(state->hwndBtnHex, BM_SETCHECK, isHex ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(state->hwndBtnText, BM_SETCHECK, isText ? BST_CHECKED : BST_UNCHECKED, 0);
    if (state->hwndBtnImage)
        SendMessage(state->hwndBtnImage, BM_SETCHECK, isImage ? BST_CHECKED : BST_UNCHECKED, 0);

    if (isImage)
    {
        ShowWindow(state->hwndEditView, SW_HIDE);
        ShowWindow(state->hwndImageCanvas, SW_SHOW);
        InvalidateRect(state->hwndImageCanvas, NULL, TRUE);
    }
    else
    {
        ShowWindow(state->hwndImageCanvas, SW_HIDE);
        ShowWindow(state->hwndEditView, SW_SHOW);

        if (isHex)
        {
            SetWindowTextA(state->hwndEditView, state->hexText.c_str());
        }
        else
        {
            SetWindowTextA(state->hwndEditView, state->plainText.c_str());
        }
    }
}

static void OnSaveBlobToFile(BlobInspectorState* state)
{
    if (!state) return;

    const uint8_t* pData = nullptr;
    size_t dataSize = 0;
    if (state->cell.isBlob)
    {
        pData = state->cell.blobData.data();
        dataSize = state->cell.blobData.size();
    }
    else
    {
        pData = (const uint8_t*)state->cell.text.data();
        dataSize = state->cell.text.size();
    }

    if (!pData || dataSize == 0)
    {
        MessageBoxA(state->hwndMain, "No data to save.", "Save BLOB", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::string ext = "bin";
    std::string filter = "Binary Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0\0";

    if (state->cell.blobKind == "PNG")
    {
        ext = "png";
        filter = "PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0\0";
    }
    else if (state->cell.blobKind == "JPEG")
    {
        ext = "jpg";
        filter = "JPEG Image (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0\0";
    }
    else if (state->cell.blobKind == "GIF")
    {
        ext = "gif";
        filter = "GIF Image (*.gif)\0*.gif\0All Files (*.*)\0*.*\0\0";
    }
    else if (state->cell.blobKind == "BMP")
    {
        ext = "bmp";
        filter = "Bitmap Image (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0\0";
    }
    else if (state->cell.blobKind == "PDF")
    {
        ext = "pdf";
        filter = "PDF Document (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0\0";
    }
    else if (state->cell.blobKind == "ZIP")
    {
        ext = "zip";
        filter = "ZIP Archive (*.zip)\0*.zip\0All Files (*.*)\0*.*\0\0";
    }
    else if (state->cell.blobKind == "JSON")
    {
        ext = "json";
        filter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0\0";
    }
    else if (state->cell.blobKind == "Text" || !state->cell.isBlob)
    {
        ext = "txt";
        filter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
    }

    OPENFILENAMEA ofn;
    char szFile[MAX_PATH];
    snprintf(szFile, sizeof(szFile), "%s_data.%s", state->colName.c_str(), ext.c_str());

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = state->hwndMain;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = ext.c_str();

    if (GetSaveFileNameA(&ofn))
    {
        HANDLE hFile = CreateFileA(szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD bytesWritten = 0;
            WriteFile(hFile, pData, (DWORD)dataSize, &bytesWritten, NULL);
            CloseHandle(hFile);

            char msg[512];
            snprintf(msg, sizeof(msg), "Saved %lu bytes successfully to:\n%s", bytesWritten, szFile);
            MessageBoxA(state->hwndMain, msg, "Save BLOB", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            char msg[512];
            snprintf(msg, sizeof(msg), "Failed to create file: %s (Error %lu)", szFile, GetLastError());
            MessageBoxA(state->hwndMain, msg, "Save BLOB Error", MB_OK | MB_ICONERROR);
        }
    }
}

static void OnCopyBlob(BlobInspectorState* state)
{
    if (!state) return;

    std::string textToCopy;
    if (state->mode == BlobViewMode::Hex)
        textToCopy = state->hexText;
    else
        textToCopy = state->plainText;

    if (textToCopy.empty()) return;

    if (OpenClipboard(state->hwndMain))
    {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, textToCopy.size() + 1);
        if (hMem)
        {
            char* p = (char*)GlobalLock(hMem);
            if (p)
            {
                memcpy(p, textToCopy.c_str(), textToCopy.size() + 1);
                GlobalUnlock(hMem);
                SetClipboardData(CF_TEXT, hMem);
            }
        }
        CloseClipboard();
    }
}

static LRESULT CALLBACK BlobInspectorWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BlobInspectorState* state = (BlobInspectorState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_CREATE:
    {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        state = (BlobInspectorState*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        state->hwndMain = hwnd;

        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        state->hFontNormal = CreateFontIndirect(&ncm.lfMessageFont);
        state->hFontMono = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

        // Top info text
        char infoText[512];
        if (state->cell.isBlob)
        {
            std::string sizeStr = SqliteEngine::CSqliteEngine::FormatByteSize(state->cell.blobBytes);
            if (state->imgWidth > 0 && state->imgHeight > 0)
            {
                snprintf(infoText, sizeof(infoText),
                         "Column: %s  |  Type: BLOB (%s Image)  |  Size: %s (%zu bytes)  |  Dimensions: %u x %u px",
                         state->colName.c_str(), state->cell.blobKind.c_str(), sizeStr.c_str(), state->cell.blobBytes,
                         state->imgWidth, state->imgHeight);
            }
            else
            {
                snprintf(infoText, sizeof(infoText),
                         "Column: %s  |  Type: BLOB%s%s  |  Size: %s (%zu bytes)",
                         state->colName.c_str(),
                         state->cell.blobKind.empty() ? "" : " (",
                         state->cell.blobKind.empty() ? "" : (state->cell.blobKind + ")").c_str(),
                         sizeStr.c_str(), state->cell.blobBytes);
            }
        }
        else
        {
            snprintf(infoText, sizeof(infoText),
                     "Column: %s  |  Type: Text / Data  |  Length: %zu characters",
                     state->colName.c_str(), state->cell.text.size());
        }

        state->hwndInfoLabel = CreateWindowEx(
            0, "STATIC", infoText,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 100, 30, hwnd, NULL, DLLInstance, NULL);
        SendMessage(state->hwndInfoLabel, WM_SETFONT, (WPARAM)state->hFontNormal, TRUE);

        // Buttons for mode
        state->hwndBtnHex = CreateWindowEx(
            0, "BUTTON", "&Hex View",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
            0, 0, 80, 24, hwnd, (HMENU)1001, DLLInstance, NULL);
        SendMessage(state->hwndBtnHex, WM_SETFONT, (WPARAM)state->hFontNormal, TRUE);

        state->hwndBtnText = CreateWindowEx(
            0, "BUTTON", "&Text View",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
            0, 0, 80, 24, hwnd, (HMENU)1002, DLLInstance, NULL);
        SendMessage(state->hwndBtnText, WM_SETFONT, (WPARAM)state->hFontNormal, TRUE);

        if (state->pImageBitmap != nullptr)
        {
            state->hwndBtnImage = CreateWindowEx(
                0, "BUTTON", "&Image Preview",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
                0, 0, 100, 24, hwnd, (HMENU)1003, DLLInstance, NULL);
            SendMessage(state->hwndBtnImage, WM_SETFONT, (WPARAM)state->hFontNormal, TRUE);
        }

        // Multi-line edit view
        state->hwndEditView = CreateWindowEx(
            WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
            0, 0, 100, 100, hwnd, (HMENU)1004, DLLInstance, NULL);
        SendMessage(state->hwndEditView, WM_SETFONT, (WPARAM)state->hFontMono, TRUE);

        // Image Canvas
        WNDCLASSA wcCanvas = {0};
        wcCanvas.lpfnWndProc = ImageCanvasProc;
        wcCanvas.hInstance = DLLInstance;
        wcCanvas.lpszClassName = "SqliteBlobCanvas";
        wcCanvas.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassA(&wcCanvas);

        state->hwndImageCanvas = CreateWindowEx(
            WS_EX_CLIENTEDGE, "SqliteBlobCanvas", "",
            WS_CHILD | WS_TABSTOP,
            0, 0, 100, 100, hwnd, (HMENU)1005, DLLInstance, NULL);
        SetWindowLongPtr(state->hwndImageCanvas, GWLP_USERDATA, (LONG_PTR)state);

        // Bottom buttons
        state->hwndBtnSave = CreateWindowEx(
            0, "BUTTON", "&Save to File...",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            0, 0, 120, 26, hwnd, (HMENU)1006, DLLInstance, NULL);
        SendMessage(state->hwndBtnSave, WM_SETFONT, (WPARAM)state->hFontNormal, TRUE);

        state->hwndBtnCopy = CreateWindowEx(
            0, "BUTTON", "&Copy",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            0, 0, 90, 26, hwnd, (HMENU)1007, DLLInstance, NULL);
        SendMessage(state->hwndBtnCopy, WM_SETFONT, (WPARAM)state->hFontNormal, TRUE);

        state->hwndBtnClose = CreateWindowEx(
            0, "BUTTON", "&Close",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            0, 0, 90, 26, hwnd, (HMENU)IDCANCEL, DLLInstance, NULL);
        SendMessage(state->hwndBtnClose, WM_SETFONT, (WPARAM)state->hFontNormal, TRUE);

        // Dark mode
        SqliteDarkMode::ApplyWindowTheme(hwnd);
        SqliteDarkMode::ApplyEditTheme(state->hwndEditView);

        // Initial mode
        if (state->pImageBitmap != nullptr)
            state->mode = BlobViewMode::Image;
        else if (state->cell.isBlob && state->cell.blobKind != "Text" && state->cell.blobKind != "JSON")
            state->mode = BlobViewMode::Hex;
        else
            state->mode = BlobViewMode::Text;

        UpdateBlobViewMode(state);
        return 0;
    }

    case WM_SIZE:
        LayoutBlobDialog(state);
        return 0;

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        if (id == 1001) // Hex
        {
            state->mode = BlobViewMode::Hex;
            UpdateBlobViewMode(state);
            return 0;
        }
        else if (id == 1002) // Text
        {
            state->mode = BlobViewMode::Text;
            UpdateBlobViewMode(state);
            return 0;
        }
        else if (id == 1003) // Image
        {
            state->mode = BlobViewMode::Image;
            UpdateBlobViewMode(state);
            return 0;
        }
        else if (id == 1006) // Save
        {
            OnSaveBlobToFile(state);
            return 0;
        }
        else if (id == 1007) // Copy
        {
            OnCopyBlob(state);
            return 0;
        }
        else if (id == IDCANCEL || id == IDOK)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ShowBlobInspector(HWND hParent, const std::string& colName, const SqliteEngine::CellValue& cell)
{
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    BlobInspectorState state;
    state.colName = colName;
    state.cell = cell;

    // Prepare Hex & Plain Text
    if (cell.isBlob)
    {
        state.hexText = SqliteEngine::CSqliteEngine::FormatHexDump(cell.blobData.data(), cell.blobData.size());
        state.plainText.assign((const char*)cell.blobData.data(), cell.blobData.size());

        // Check if image can be decoded
        if (cell.blobData.size() > 0 && (cell.blobKind == "PNG" || cell.blobKind == "JPEG" ||
                                         cell.blobKind == "GIF" || cell.blobKind == "BMP" ||
                                         cell.blobKind == "WebP"))
        {
            IStream* pStream = SHCreateMemStream(cell.blobData.data(), (UINT)cell.blobData.size());
            if (pStream)
            {
                Gdiplus::Bitmap* pBmp = Gdiplus::Bitmap::FromStream(pStream);
                if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok)
                {
                    state.pImageBitmap = pBmp;
                    state.imgWidth = pBmp->GetWidth();
                    state.imgHeight = pBmp->GetHeight();
                }
                else if (pBmp)
                {
                    delete pBmp;
                }
                pStream->Release();
            }
        }
    }
    else
    {
        state.plainText = cell.text;
        state.hexText = SqliteEngine::CSqliteEngine::FormatHexDump((const uint8_t*)cell.text.data(), cell.text.size());
    }

    // Register Window Class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = BlobInspectorWndProc;
    wc.hInstance = DLLInstance;
    wc.lpszClassName = "SqliteBlobInspectorWnd";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassA(&wc);

    char title[256];
    snprintf(title, sizeof(title), "BLOB / Value Inspector - [%s]", colName.c_str());

    int w = 720;
    int h = 540;
    RECT rcParent;
    if (GetWindowRect(hParent, &rcParent))
    {
        int pw = rcParent.right - rcParent.left;
        int ph = rcParent.bottom - rcParent.top;
        int x = rcParent.left + (pw - w) / 2;
        int y = rcParent.top + (ph - h) / 2;
        if (x < 0) x = 100;
        if (y < 0) y = 100;

        HWND hwnd = CreateWindowExA(
            WS_EX_DLGMODALFRAME,
            "SqliteBlobInspectorWnd",
            title,
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE | WS_CLIPCHILDREN,
            x, y, w, h,
            hParent, NULL, DLLInstance, &state);

        if (hwnd)
        {
            EnableWindow(hParent, FALSE);
            SetForegroundWindow(hwnd);

            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0))
            {
                if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
                {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    continue;
                }

                if (!IsDialogMessage(hwnd, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            EnableWindow(hParent, TRUE);
            SetForegroundWindow(hParent);
        }
    }

    if (state.pImageBitmap)
    {
        delete state.pImageBitmap;
        state.pImageBitmap = nullptr;
    }
    if (state.hFontNormal) DeleteObject(state.hFontNormal);
    if (state.hFontMono) DeleteObject(state.hFontMono);

    Gdiplus::GdiplusShutdown(gdiplusToken);
}

} // namespace BlobDialog
