// SPDX-FileCopyrightText: 2026 Open Salamander Authors, Red Salamander Authors & Ondrej Kotas
// SPDX-FileContributor: Dark Mode host policy and theme integration based on fork by Ondrej Kotas (KRtkovo-eu-AI)
// SPDX-FileContributor: Ported to Open Salamander framework by fila73
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sqlitedarkmode.h"

namespace SqliteDarkMode
{

static COLORREF Blend(COLORREF a, COLORREF b, int percentB)
{
    int p = percentB, q = 100 - percentB;
    return RGB((GetRValue(a) * q + GetRValue(b) * p) / 100,
               (GetGValue(a) * q + GetGValue(b) * p) / 100,
               (GetBValue(a) * q + GetBValue(b) * p) / 100);
}

bool IsDarkMode()
{
    return DarkModeShouldUseDarkColors();
}

Palette GetPalette()
{
    const DarkModeColors& c = DarkModeGetColors();
    bool dark = IsDarkMode();

    // Host panel colours (SALCOL_ITEM_FG/BK_NORMAL) in dark mode, system colours otherwise.
    COLORREF bg = dark ? c.background : GetSysColor(COLOR_WINDOW);
    COLORREF fg = dark ? c.text : GetSysColor(COLOR_WINDOWTEXT);

    Palette p;
    p.bgMain = bg;
    p.textMain = fg;
    p.bgAlternate = Blend(bg, fg, 4);
    p.bgSelected = dark ? Blend(bg, GetSysColor(COLOR_HIGHLIGHT), 60) : GetSysColor(COLOR_HIGHLIGHT);
    p.textSelected = dark ? fg : GetSysColor(COLOR_HIGHLIGHTTEXT);
    p.textNull = DarkModeEnsureReadableForeground(Blend(fg, bg, 45), bg);
    p.textBlob = DarkModeEnsureReadableForeground(dark ? RGB(120, 180, 220) : RGB(0, 102, 180), bg);
    return p;
}

void ApplyListViewColors(HWND hwndList)
{
    if (!hwndList)
        return;
    Palette p = GetPalette();
    ListView_SetBkColor(hwndList, p.bgMain);
    ListView_SetTextBkColor(hwndList, p.bgMain);
    ListView_SetTextColor(hwndList, p.textMain);
}

LRESULT HandleListViewCustomDraw(HWND hwndList, LPNMLVCUSTOMDRAW lplvcd, bool isQueryMode)
{
    switch (lplvcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
    {
        Palette p = GetPalette();
        DWORD_PTR item = lplvcd->nmcd.dwItemSpec;
        bool selected = (ListView_GetItemState(hwndList, item, LVIS_SELECTED) & LVIS_SELECTED) != 0;

        if (selected)
        {
            lplvcd->clrTextBk = p.bgSelected;
            lplvcd->clrText = p.textSelected;
        }
        else
        {
            lplvcd->clrTextBk = (item % 2 == 1) ? p.bgAlternate : p.bgMain;
            lplvcd->clrText = p.textMain;
        }
        return lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ? CDRF_NOTIFYSUBITEMDRAW : CDRF_DODEFAULT;
    }

    default:
        return CDRF_DODEFAULT;
    }
}

} // namespace SqliteDarkMode
