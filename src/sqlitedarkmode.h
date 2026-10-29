// SPDX-FileCopyrightText: 2026 Open Salamander Authors, Red Salamander Authors & Ondrej Kotas
// SPDX-FileContributor: Dark Mode host policy and theme integration based on fork by Ondrej Kotas (KRtkovo-eu-AI)
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Viewer palette and ListView custom-draw helper only. Dialog/window/control theming
// is done by the host dark mode (darkmodelib) through winliblt: WinLibApplyDarkMode().
// All colours are derived from DarkModeGetColors() / DarkModeShouldUseDarkColors().

#include <windows.h>
#include <commctrl.h>

namespace SqliteDarkMode
{

struct Palette
{
    COLORREF bgMain;
    COLORREF bgAlternate;
    COLORREF bgSelected;
    COLORREF textMain;
    COLORREF textSelected;
    COLORREF textNull;
    COLORREF textBlob;
};

bool IsDarkMode();
Palette GetPalette();

// Sets ListView background/text colours from the current palette.
void ApplyListViewColors(HWND hwndList);

LRESULT HandleListViewCustomDraw(HWND hwndList, LPNMLVCUSTOMDRAW lplvcd, bool isQueryMode = false);

} // namespace SqliteDarkMode
