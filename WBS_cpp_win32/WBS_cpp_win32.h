#pragma once

#include "resource.h"

// ============================================================================
// Common Controls マクロ定義
// ============================================================================

// ComboBoxマクロの定義（windowsx.hからの抜粋）
#ifndef ComboBox_AddString
#define ComboBox_AddString(hwndCtl, lpsz) \
    ((int)(DWORD)SendMessage((hwndCtl), CB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpsz)))
#endif

#ifndef ComboBox_SetCurSel
#define ComboBox_SetCurSel(hwndCtl, index) \
    ((int)(DWORD)SendMessage((hwndCtl), CB_SETCURSEL, (WPARAM)(int)(index), 0L))
#endif

#ifndef ComboBox_GetCurSel
#define ComboBox_GetCurSel(hwndCtl) \
    ((int)(DWORD)SendMessage((hwndCtl), CB_GETCURSEL, 0L, 0L))
#endif

// ============================================================================
// ProgressBarマクロの定義
// ============================================================================

#ifndef ProgressBar_SetRange
#define ProgressBar_SetRange(hwndCtl, min, max) \
    ((DWORD)SendMessage((hwndCtl), PBM_SETRANGE, 0L, MAKELPARAM((min), (max))))
#endif

#ifndef ProgressBar_SetPos
#define ProgressBar_SetPos(hwndCtl, pos) \
    ((int)SendMessage((hwndCtl), PBM_SETPOS, (WPARAM)(int)(pos), 0L))
#endif

// ============================================================================
// ListViewマクロの定義
// ============================================================================

#ifndef ListView_InsertColumn
#define ListView_InsertColumn(hwnd, iCol, pcol) \
    (int)SNDMSG((hwnd), LVM_INSERTCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(const LV_COLUMN *)(pcol))
#endif

#ifndef ListView_InsertItem
#define ListView_InsertItem(hwnd, pitem) \
    (int)SNDMSG((hwnd), LVM_INSERTITEM, 0, (LPARAM)(const LV_ITEM *)(pitem))
#endif

#ifndef ListView_SetItemText
#define ListView_SetItemText(hwndLV, i, iSubItem_, pszText_) \
{ LV_ITEM _ms_lvi; \
  _ms_lvi.iSubItem = iSubItem_; \
  _ms_lvi.pszText = pszText_; \
  SNDMSG((hwndLV), LVM_SETITEMTEXT, (WPARAM)i, (LPARAM)(LV_ITEM *)&_ms_lvi); }
#endif

#ifndef ListView_DeleteAllItems
#define ListView_DeleteAllItems(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_DELETEALLITEMS, 0, 0L)
#endif

// ============================================================================
// TreeViewマクロの定義
// ============================================================================

#ifndef TreeView_GetRoot
#define TreeView_GetRoot(hwnd) \
    TreeView_GetNextItem(hwnd, NULL, TVGN_ROOT)
#endif

#ifndef TreeView_GetSelection
#define TreeView_GetSelection(hwnd) \
    TreeView_GetNextItem(hwnd, NULL, TVGN_CARET)
#endif

#ifndef TreeView_InsertItem
#define TreeView_InsertItem(hwnd, lpis) \
    (HTREEITEM)SNDMSG((hwnd), TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT)(lpis))
#endif

#ifndef TreeView_DeleteAllItems
#define TreeView_DeleteAllItems(hwnd) \
    (BOOL)SNDMSG((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)
#endif

#ifndef TreeView_GetItem
#define TreeView_GetItem(hwnd, pitem) \
    (BOOL)SNDMSG((hwnd), TVM_GETITEM, 0, (LPARAM)(TV_ITEM *)(pitem))
#endif

#ifndef TreeView_Expand
#define TreeView_Expand(hwnd, hitem, code) \
    (BOOL)SNDMSG((hwnd), TVM_EXPAND, (WPARAM)(code), (LPARAM)(HTREEITEM)(hitem))
#endif

#ifndef TreeView_GetNextItem
#define TreeView_GetNextItem(hwnd, hitem, code) \
    (HTREEITEM)SNDMSG((hwnd), TVM_GETNEXTITEM, (WPARAM)(code), (LPARAM)(HTREEITEM)(hitem))
#endif

#ifndef TreeView_GetChild
#define TreeView_GetChild(hwnd, hitem) \
    TreeView_GetNextItem(hwnd, hitem, TVGN_CHILD)
#endif

#ifndef TreeView_GetNextSibling
#define TreeView_GetNextSibling(hwnd, hitem) \
    TreeView_GetNextItem(hwnd, hitem, TVGN_NEXT)
#endif

// ============================================================================
// 共通コントロールメッセージ定数
// ============================================================================

// ComboBox メッセージ
#ifndef CB_ADDSTRING
#define CB_ADDSTRING            0x0143
#endif

#ifndef CB_SETCURSEL
#define CB_SETCURSEL            0x014E
#endif

#ifndef CB_GETCURSEL
#define CB_GETCURSEL            0x0147
#endif

// ProgressBar メッセージ
#ifndef PBM_SETRANGE
#define PBM_SETRANGE            (WM_USER+1)
#endif

#ifndef PBM_SETPOS
#define PBM_SETPOS              (WM_USER+2)
#endif

// TreeView 定数
#ifndef TVGN_ROOT
#define TVGN_ROOT               0x0000
#endif

#ifndef TVGN_NEXT
#define TVGN_NEXT               0x0001
#endif

#ifndef TVGN_CHILD
#define TVGN_CHILD              0x0004
#endif

#ifndef TVGN_CARET
#define TVGN_CARET              0x0009
#endif

#ifndef TVE_EXPAND
#define TVE_EXPAND              0x0002
#endif

#ifndef TVE_COLLAPSE
#define TVE_COLLAPSE            0x0001
#endif
