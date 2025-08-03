// WBS_cpp_win32.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "WBS_cpp_win32.h"
#include <vector>
#include <string>
#include <memory>
#include <commctrl.h>
#include <commdlg.h>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <Windows.h> // For Registry functions

#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

// WBSタスクの状態
enum class TaskStatus {
    NOT_STARTED = 0,    // 未開始
    IN_PROGRESS = 1,    // 進行中
    COMPLETED = 2,      // 完了
    ON_HOLD = 3,        // 保留
    CANCELLED = 4       // キャンセル
};

// WBSタスクの優先度
enum class TaskPriority {
    LOW = 0,            // 低
    MEDIUM = 1,         // 中
    HIGH = 2,           // 高
    URGENT = 3          // 緊急
};

// 前方宣言
class WBSItem;
class WBSProject;

// XMLエスケープ用のヘルパー関数
std::wstring XmlEscape(const std::wstring& text) {
    std::wstring result;
    for (wchar_t c : text) {
        switch (c) {
            case L'&': result += L"&amp;"; break;
            case L'<': result += L"&lt;"; break;
            case L'>': result += L"&gt;"; break;
            case L'"': result += L"&quot;"; break;
            case L'\'': result += L"&apos;"; break;
            default: result += c; break;
        }
    }
    return result;
}

// XMLアンエスケープ用のヘルパー関数
std::wstring XmlUnescape(const std::wstring& text) {
    std::wstring result = text;
    size_t pos = 0;
    while ((pos = result.find(L"&amp;", pos)) != std::wstring::npos) {
        result.replace(pos, 5, L"&");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find(L"&lt;", pos)) != std::wstring::npos) {
        result.replace(pos, 4, L"<");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find(L"&gt;", pos)) != std::wstring::npos) {
        result.replace(pos, 4, L">");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find(L"&quot;", pos)) != std::wstring::npos) {
        result.replace(pos, 6, L"\"");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find(L"&apos;", pos)) != std::wstring::npos) {
        result.replace(pos, 6, L"'");
        pos += 1;
    }
    return result;
}

// SYSTEMTIME to wstring変換
std::wstring SystemTimeToString(const SYSTEMTIME& st) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"%04d-%02d-%02dT%02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buffer);
}

// wstring to SYSTEMTIME変換
SYSTEMTIME StringToSystemTime(const std::wstring& str) {
    SYSTEMTIME st = {};
    if (str.length() >= 19) {
        swscanf_s(str.c_str(), L"%04hu-%02hu-%02huT%02hu:%02hu:%02hu",
            &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
    } else {
        GetSystemTime(&st);
    }
    return st;
}

// WBSアイテムクラス
class WBSItem : public std::enable_shared_from_this<WBSItem> {
public:
    std::wstring id;
    std::wstring taskName;
    std::wstring description;
    std::wstring assignedTo;
    TaskStatus status;
    TaskPriority priority;
    double estimatedHours;
    double actualHours;
    SYSTEMTIME startDate;
    SYSTEMTIME endDate;
    int level;
    std::vector<std::shared_ptr<WBSItem>> children;
    std::weak_ptr<WBSItem> parent;

    WBSItem() : status(TaskStatus::NOT_STARTED), priority(TaskPriority::MEDIUM), 
                estimatedHours(0.0), actualHours(0.0), level(0) {
        GetSystemTime(&startDate);
        GetSystemTime(&endDate);
        taskName = L"新しいタスク";
    }

    WBSItem(const std::wstring& name) : WBSItem() {
        taskName = name;
    }

    void AddChild(std::shared_ptr<WBSItem> child) {
        child->parent = shared_from_this();
        child->level = this->level + 1;
        child->id = this->id + L"." + std::to_wstring(children.size() + 1);
        children.push_back(child);
    }

    std::wstring GetStatusString() const {
        switch (status) {
            case TaskStatus::NOT_STARTED: return L"未開始";
            case TaskStatus::IN_PROGRESS: return L"進行中";
            case TaskStatus::COMPLETED: return L"完了";
            case TaskStatus::ON_HOLD: return L"保留";
            case TaskStatus::CANCELLED: return L"キャンセル";
            default: return L"未開始";
        }
    }

    std::wstring GetPriorityString() const {
        switch (priority) {
            case TaskPriority::LOW: return L"低";
            case TaskPriority::MEDIUM: return L"中";
            case TaskPriority::HIGH: return L"高";
            case TaskPriority::URGENT: return L"緊急";
            default: return L"中";
        }
    }

    double GetProgressPercentage() const {
        if (estimatedHours == 0.0) return 0.0;
        return (actualHours / estimatedHours) * 100.0;
    }
};

// WBSプロジェクトクラス
class WBSProject {
public:
    std::wstring projectName;
    std::wstring description;
    std::shared_ptr<WBSItem> rootTask;

    WBSProject() {
        projectName = L"新規WBSプロジェクト";
        rootTask = std::make_shared<WBSItem>(projectName);
        rootTask->id = L"1";
        rootTask->level = 0;
    }

    WBSProject(const std::wstring& name) : WBSProject() {
        projectName = name;
        rootTask->taskName = name;
    }
};

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

// WBS関連のグローバル変数
std::unique_ptr<WBSProject> g_currentProject;
HWND g_hTreeWBS = nullptr;
HWND g_hListDetails = nullptr;
HTREEITEM g_selectedItem = nullptr;

// WBS関連のメニューID
#define IDM_FILE_NEW        110
#define IDM_FILE_OPEN       111
#define IDM_FILE_SAVE       112
#define IDM_EDIT_ADD_TASK   120
#define IDM_EDIT_ADD_SUBTASK 121
#define IDM_EDIT_EDIT_TASK  122
#define IDM_EDIT_DELETE_TASK 123
#define IDM_VIEW_EXPAND_ALL 130
#define IDM_VIEW_COLLAPSE_ALL 131

// コントロールID
#define IDC_TREE_WBS        1000
#define IDC_LIST_DETAILS    1001

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// WBS関連の関数宣言
void InitializeWBS(HWND hWnd);
void CreateWBSControls(HWND hWnd);
void RefreshTreeView();
void RefreshListView();
HTREEITEM AddTreeItem(HTREEITEM hParent, std::shared_ptr<WBSItem> item);
void AddTreeItemRecursive(HTREEITEM hParent, std::shared_ptr<WBSItem> item);
std::shared_ptr<WBSItem> GetItemFromTreeItem(HTREEITEM hItem);
void OnTreeSelectionChanged();
void OnAddTask();
void OnAddSubTask();
void OnEditTask();
void OnDeleteTask();
void OnSaveProject();
void OnOpenProject();
void ExpandAllTreeItems(HTREEITEM hItem);
void CollapseAllTreeItems(HTREEITEM hItem);
bool LoadProjectFromFile(const std::wstring& filePath);
void SaveLastOpenedFile(const std::wstring& filePath);
std::wstring GetLastOpenedFile();

// XML操作関数の宣言
std::wstring XmlEscape(const std::wstring& text);
std::wstring XmlUnescape(const std::wstring& text);
std::wstring SystemTimeToString(const SYSTEMTIME& st);
SYSTEMTIME StringToSystemTime(const std::wstring& str);
std::wstring WBSItemToXml(std::shared_ptr<WBSItem> item, int indent);
std::wstring ProjectToXml();
std::shared_ptr<WBSItem> ParseTaskFromXml(const std::wstring& xml, size_t& pos);

// XMLから値を抽出するヘルパー関数
std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tag, size_t startPos = 0) {
    std::wstring startTag = L"<" + tag + L">";
    std::wstring endTag = L"</" + tag + L">";
    
    size_t start = xml.find(startTag, startPos);
    if (start == std::wstring::npos) return L"";
    
    start += startTag.length();
    size_t end = xml.find(endTag, start);
    if (end == std::wstring::npos) return L"";
    
    return XmlUnescape(xml.substr(start, end - start));
}

// レジストリ操作用のヘルパー関数
void SaveLastOpenedFile(const std::wstring& filePath) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\WBS_cpp_win32", 0, NULL, 
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, L"LastOpenedFile", 0, REG_SZ, 
                      (BYTE*)filePath.c_str(), (filePath.length() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
}

std::wstring GetLastOpenedFile() {
    HKEY hKey;
    std::wstring result;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\WBS_cpp_win32", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);
        DWORD type;
        
        if (RegQueryValueEx(hKey, L"LastOpenedFile", NULL, &type, 
                            (BYTE*)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
            result = buffer;
        }
        RegCloseKey(hKey);
    }
    
    return result;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Common Controlsを初期化
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WBSCPPWIN32, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WBSCPPWIN32));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WBSCPPWIN32));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WBSCPPWIN32);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // WBSを初期化
   InitializeWBS(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージ を処理します。
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateWBSControls(hWnd);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_FILE_NEW:
                g_currentProject = std::make_unique<WBSProject>();
                RefreshTreeView();
                RefreshListView();
                break;
            case IDM_FILE_SAVE:
                OnSaveProject();
                break;
            case IDM_FILE_OPEN:
                OnOpenProject();
                break;
            case IDM_EDIT_ADD_TASK:
                OnAddTask();
                break;
            case IDM_EDIT_ADD_SUBTASK:
                OnAddSubTask();
                break;
            case IDM_EDIT_EDIT_TASK:
                OnEditTask();
                break;
            case IDM_EDIT_DELETE_TASK:
                OnDeleteTask();
                break;
            case IDM_VIEW_EXPAND_ALL:
                if (g_hTreeWBS) {
                    ExpandAllTreeItems(TreeView_GetRoot(g_hTreeWBS));
                }
                break;
            case IDM_VIEW_COLLAPSE_ALL:
                if (g_hTreeWBS) {
                    CollapseAllTreeItems(TreeView_GetRoot(g_hTreeWBS));
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->hwndFrom == g_hTreeWBS && pnmh->code == TVN_SELCHANGED) {
                OnTreeSelectionChanged();
            }
        }
        break;
    case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            
            if (g_hTreeWBS) {
                SetWindowPos(g_hTreeWBS, nullptr, 0, 0, width / 2, height, SWP_NOZORDER);
            }
            if (g_hListDetails) {
                SetWindowPos(g_hListDetails, nullptr, width / 2, 0, width / 2, height, SWP_NOZORDER);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// WBS関連の実装

void InitializeWBS(HWND hWnd) {
    // 最後に開いたファイルがあるかチェック
    std::wstring lastFile = GetLastOpenedFile();
    
    if (!lastFile.empty()) {
        // ファイルが存在するかチェック
        std::wifstream checkFile(lastFile);
        if (checkFile.good()) {
            checkFile.close();
            // 最後のファイルを読み込む
            if (LoadProjectFromFile(lastFile)) {
                return; // 読み込み成功
            }
        }
    }
    
    // 最後のファイルが無い、または読み込み失敗の場合はサンプルプロジェクトを作成
    g_currentProject = std::make_unique<WBSProject>(L"サンプルWBSプロジェクト");
    
    // サンプルタスクを追加
    auto task1 = std::make_shared<WBSItem>(L"要件定義");
    task1->id = L"1.1";
    task1->level = 1;
    task1->description = L"システム要件の定義と分析";
    task1->estimatedHours = 40.0;
    task1->status = TaskStatus::COMPLETED;
    task1->assignedTo = L"田中";
    g_currentProject->rootTask->AddChild(task1);

    auto task2 = std::make_shared<WBSItem>(L"設計");
    task2->id = L"1.2";
    task2->level = 1;
    task2->description = L"システム設計書の作成";
    task2->estimatedHours = 60.0;
    task2->status = TaskStatus::IN_PROGRESS;
    task2->assignedTo = L"佐藤";
    g_currentProject->rootTask->AddChild(task2);

    // サブタスクを追加
    auto subTask1 = std::make_shared<WBSItem>(L"基本設計");
    subTask1->description = L"基本設計書の作成";
    subTask1->estimatedHours = 30.0;
    subTask1->status = TaskStatus::COMPLETED;
    subTask1->assignedTo = L"佐藤";
    task2->AddChild(subTask1);

    auto subTask2 = std::make_shared<WBSItem>(L"詳細設計");
    subTask2->description = L"詳細設計書の作成";
    subTask2->estimatedHours = 30.0;
    subTask2->status = TaskStatus::IN_PROGRESS;
    subTask2->assignedTo = L"佐藤";
    task2->AddChild(subTask2);

    auto task3 = std::make_shared<WBSItem>(L"実装");
    task3->id = L"1.3";
    task3->level = 1;
    task3->description = L"システムの実装";
    task3->estimatedHours = 120.0;
    task3->status = TaskStatus::NOT_STARTED;
    task3->assignedTo = L"鈴木";
    g_currentProject->rootTask->AddChild(task3);
}

void CreateWBSControls(HWND hWnd) {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // TreeViewを作成
    g_hTreeWBS = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        0, 0, width / 2, height,
        hWnd,
        (HMENU)IDC_TREE_WBS,
        hInst,
        nullptr
    );

    // ListViewを作成
    g_hListDetails = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        width / 2, 0, width / 2, height,
        hWnd,
        (HMENU)IDC_LIST_DETAILS,
        hInst,
        nullptr
    );

    // ListViewの拡張スタイルを設定
    if (g_hListDetails) {
        SendMessage(g_hListDetails, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        
        LVCOLUMN lvc = {};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;
        
        lvc.pszText = (LPWSTR)L"項目";
        lvc.cx = 120;
        ListView_InsertColumn(g_hListDetails, 0, &lvc);
        
        lvc.pszText = (LPWSTR)L"値";
        lvc.cx = 200;
        ListView_InsertColumn(g_hListDetails, 1, &lvc);
    }

    RefreshTreeView();
}

void RefreshTreeView() {
    if (!g_hTreeWBS || !g_currentProject) return;

    TreeView_DeleteAllItems(g_hTreeWBS);
    
    HTREEITEM hRoot = AddTreeItem(TVI_ROOT, g_currentProject->rootTask);
    AddTreeItemRecursive(hRoot, g_currentProject->rootTask);
    
    TreeView_Expand(g_hTreeWBS, hRoot, TVE_EXPAND);
}

HTREEITEM AddTreeItem(HTREEITEM hParent, std::shared_ptr<WBSItem> item) {
    if (!g_hTreeWBS || !item) return nullptr;

    TVINSERTSTRUCT tvis = {};
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
    
    std::wstring displayText = item->id + L" - " + item->taskName + L" (" + item->GetStatusString() + L")";
    tvis.item.pszText = const_cast<LPWSTR>(displayText.c_str());
    tvis.item.lParam = reinterpret_cast<LPARAM>(item.get());

    return TreeView_InsertItem(g_hTreeWBS, &tvis);
}

void AddTreeItemRecursive(HTREEITEM hParent, std::shared_ptr<WBSItem> item) {
    if (!item) return;

    for (auto& child : item->children) {
        HTREEITEM hChild = AddTreeItem(hParent, child);
        if (hChild) {
            AddTreeItemRecursive(hChild, child);
        }
    }
}

std::shared_ptr<WBSItem> GetItemFromTreeItem(HTREEITEM hItem) {
    if (!g_hTreeWBS || !hItem) return nullptr;

    TVITEM tvi = {};
    tvi.mask = TVIF_PARAM;
    tvi.hItem = hItem;
    
    if (TreeView_GetItem(g_hTreeWBS, &tvi)) {
        WBSItem* rawPtr = reinterpret_cast<WBSItem*>(tvi.lParam);
        if (rawPtr) {
            return rawPtr->shared_from_this();
        }
    }
    
    return nullptr;
}

void OnTreeSelectionChanged() {
    HTREEITEM hSelected = TreeView_GetSelection(g_hTreeWBS);
    if (hSelected) {
        g_selectedItem = hSelected;
        RefreshListView();
    }
}

void RefreshListView() {
    if (!g_hListDetails) return;

    ListView_DeleteAllItems(g_hListDetails);

    if (!g_selectedItem) return;

    std::shared_ptr<WBSItem> item = GetItemFromTreeItem(g_selectedItem);
    if (!item) return;

    // アイテムの詳細をListViewに表示
    LVITEM lvi = {};
    lvi.mask = LVIF_TEXT;
    
    // タスク名
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = (LPWSTR)L"タスク名";
    ListView_InsertItem(g_hListDetails, &lvi);
    ListView_SetItemText(g_hListDetails, 0, 1, const_cast<LPWSTR>(item->taskName.c_str()));

    // ID
    lvi.iItem = 1;
    lvi.pszText = (LPWSTR)L"ID";
    ListView_InsertItem(g_hListDetails, &lvi);
    ListView_SetItemText(g_hListDetails, 1, 1, const_cast<LPWSTR>(item->id.c_str()));

    // 説明
    lvi.iItem = 2;
    lvi.pszText = (LPWSTR)L"説明";
    ListView_InsertItem(g_hListDetails, &lvi);
    ListView_SetItemText(g_hListDetails, 2, 1, const_cast<LPWSTR>(item->description.c_str()));

    // ステータス
    lvi.iItem = 3;
    lvi.pszText = (LPWSTR)L"ステータス";
    ListView_InsertItem(g_hListDetails, &lvi);
    std::wstring statusStr = item->GetStatusString();
    ListView_SetItemText(g_hListDetails, 3, 1, const_cast<LPWSTR>(statusStr.c_str()));

    // 優先度
    lvi.iItem = 4;
    lvi.pszText = (LPWSTR)L"優先度";
    ListView_InsertItem(g_hListDetails, &lvi);
    std::wstring priorityStr = item->GetPriorityString();
    ListView_SetItemText(g_hListDetails, 4, 1, const_cast<LPWSTR>(priorityStr.c_str()));

    // 予定時間
    lvi.iItem = 5;
    lvi.pszText = (LPWSTR)L"予定時間";
    ListView_InsertItem(g_hListDetails, &lvi);
    std::wstring estimatedStr = std::to_wstring(item->estimatedHours) + L"時間";
    ListView_SetItemText(g_hListDetails, 5, 1, const_cast<LPWSTR>(estimatedStr.c_str()));

    // 実績時間
    lvi.iItem = 6;
    lvi.pszText = (LPWSTR)L"実績時間";
    ListView_InsertItem(g_hListDetails, &lvi);
    std::wstring actualStr = std::to_wstring(item->actualHours) + L"時間";
    ListView_SetItemText(g_hListDetails, 6, 1, const_cast<LPWSTR>(actualStr.c_str()));

    // 進捗率
    lvi.iItem = 7;
    lvi.pszText = (LPWSTR)L"進捗率";
    ListView_InsertItem(g_hListDetails, &lvi);
    std::wstring progressStr = std::to_wstring((int)item->GetProgressPercentage()) + L"%";
    ListView_SetItemText(g_hListDetails, 7, 1, const_cast<LPWSTR>(progressStr.c_str()));

    // 担当者
    lvi.iItem = 8;
    lvi.pszText = (LPWSTR)L"担当者";
    ListView_InsertItem(g_hListDetails, &lvi);
    ListView_SetItemText(g_hListDetails, 8, 1, const_cast<LPWSTR>(item->assignedTo.c_str()));
}

void OnAddTask() {
    if (!g_currentProject) return;

    auto newTask = std::make_shared<WBSItem>(L"新しいタスク");
    g_currentProject->rootTask->AddChild(newTask);
    
    RefreshTreeView();
    MessageBox(nullptr, L"新しいタスクを追加しました。", L"情報", MB_OK | MB_ICONINFORMATION);
}

void OnAddSubTask() {
    if (!g_selectedItem) {
        MessageBox(nullptr, L"親タスクを選択してください。", L"エラー", MB_OK | MB_ICONWARNING);
        return;
    }

    std::shared_ptr<WBSItem> parentItem = GetItemFromTreeItem(g_selectedItem);
    if (!parentItem) return;

    auto newSubTask = std::make_shared<WBSItem>(L"新しいサブタスク");
    parentItem->AddChild(newSubTask);
    
    RefreshTreeView();
    MessageBox(nullptr, L"新しいサブタスクを追加しました。", L"情報", MB_OK | MB_ICONINFORMATION);
}

void OnEditTask() {
    MessageBox(nullptr, L"タスク編集機能は今後実装予定です。", L"情報", MB_OK | MB_ICONINFORMATION);
}

void OnDeleteTask() {
    if (!g_selectedItem) {
        MessageBox(nullptr, L"削除するタスクを選択してください。", L"エラー", MB_OK | MB_ICONWARNING);
        return;
    }

    if (MessageBox(nullptr, L"選択したタスクを削除しますか？", L"確認", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        // TODO: 実際の削除処理を実装
        MessageBox(nullptr, L"タスク削除機能は今後実装予定です。", L"情報", MB_OK | MB_ICONINFORMATION);
    }
}

void OnSaveProject() {
    if (!g_currentProject) {
        MessageBox(nullptr, L"保存するプロジェクトがありません。", L"エラー", MB_OK | MB_ICONWARNING);
        return;
    }

    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L"";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XMLファイル\0*.xml\0すべてのファイル\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"xml";

    if (GetSaveFileName(&ofn) == TRUE) {
        try {
            std::wofstream file(szFile);
            if (file.is_open()) {
                file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
                std::wstring xmlContent = ProjectToXml();
                file << xmlContent;
                file.close();
                
                // 最後に保存したファイルパスを記録
                SaveLastOpenedFile(szFile);
                
                MessageBox(nullptr, L"プロジェクトをXMLファイルに保存しました。", L"情報", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBox(nullptr, L"ファイルを開けませんでした。", L"エラー", MB_OK | MB_ICONERROR);
            }
        } catch (...) {
            MessageBox(nullptr, L"プロジェクトの保存中にエラーが発生しました。", L"エラー", MB_OK | MB_ICONERROR);
        }
    }
}

void OnOpenProject() {
    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L"";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XMLファイル\0*.xml\0すべてのファイル\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        LoadProjectFromFile(szFile);
    }
}

// ファイルからプロジェクトを読み込む関数
bool LoadProjectFromFile(const std::wstring& filePath) {
    try {
        std::wifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
        std::wstring xmlContent((std::istreambuf_iterator<wchar_t>(file)),
                              std::istreambuf_iterator<wchar_t>());
        file.close();
        
        // プロジェクト名と説明を取得
        std::wstring projectName = ExtractXmlValue(xmlContent, L"ProjectName", 0);
        std::wstring description = ExtractXmlValue(xmlContent, L"Description", 0);
        
        if (projectName.empty()) {
            projectName = L"読み込まれたプロジェクト";
        }
        
        // 新しいプロジェクトを作成
        g_currentProject = std::make_unique<WBSProject>();
        
        // プロジェクト情報を解析
        g_currentProject->projectName = ExtractXmlValue(xmlContent, L"ProjectName", 0);
        g_currentProject->description = ExtractXmlValue(xmlContent, L"Description", 0);
        
        // ルートタスクを解析（修正版）
        size_t rootTaskStart = xmlContent.find(L"<RootTask>");
        if (rootTaskStart != std::wstring::npos) {
            size_t rootTaskEnd = xmlContent.find(L"</RootTask>");
            if (rootTaskEnd != std::wstring::npos) {
                std::wstring rootTaskXml = xmlContent.substr(rootTaskStart + 10, rootTaskEnd - rootTaskStart - 10);
                size_t pos = 0;
                auto loadedRootTask = ParseTaskFromXml(rootTaskXml, pos);
                
                if (loadedRootTask) {
                    // ルートタスクの情報を完全に置き換え
                    g_currentProject->rootTask = loadedRootTask;
                    // プロジェクト名をルートタスクにも設定
                    if (!g_currentProject->projectName.empty()) {
                        g_currentProject->rootTask->taskName = g_currentProject->projectName;
                    }
                }
            }
        }
        
        RefreshTreeView();
        RefreshListView();
        
        // 最後に開いたファイルパスを保存
        SaveLastOpenedFile(filePath);
        
        MessageBox(nullptr, L"プロジェクトをXMLファイルから読み込みました。", L"情報", MB_OK | MB_ICONINFORMATION);
        
        return true;
    } catch (...) {
        MessageBox(nullptr, L"プロジェクトの読み込み中にエラーが発生しました。", L"エラー", MB_OK | MB_ICONERROR);
        return false;
    }
}

std::wstring WBSItemToXml(std::shared_ptr<WBSItem> item, int indent) {
    if (!item) return L"";
    
    std::wstring indentStr(indent * 2, L' ');
    std::wstring xml;
    
    xml += indentStr + L"<Task>\n";
    xml += indentStr + L"  <ID>" + XmlEscape(item->id) + L"</ID>\n";
    xml += indentStr + L"  <Name>" + XmlEscape(item->taskName) + L"</Name>\n";
    xml += indentStr + L"  <Description>" + XmlEscape(item->description) + L"</Description>\n";
    xml += indentStr + L"  <AssignedTo>" + XmlEscape(item->assignedTo) + L"</AssignedTo>\n";
    xml += indentStr + L"  <Status>" + std::to_wstring((int)item->status) + L"</Status>\n";
    xml += indentStr + L"  <Priority>" + std::to_wstring((int)item->priority) + L"</Priority>\n";
    xml += indentStr + L"  <EstimatedHours>" + std::to_wstring(item->estimatedHours) + L"</EstimatedHours>\n";
    xml += indentStr + L"  <ActualHours>" + std::to_wstring(item->actualHours) + L"</ActualHours>\n";
    xml += indentStr + L"  <StartDate>" + SystemTimeToString(item->startDate) + L"</StartDate>\n";
    xml += indentStr + L"  <EndDate>" + SystemTimeToString(item->endDate) + L"</EndDate>\n";
    xml += indentStr + L"  <Level>" + std::to_wstring(item->level) + L"</Level>\n";
    
    if (!item->children.empty()) {
        xml += indentStr + L"  <Children>\n";
        for (const auto& child : item->children) {
            xml += WBSItemToXml(child, indent + 2);
        }
        xml += indentStr + L"  </Children>\n";
    }
    
    xml += indentStr + L"</Task>\n";
    return xml;
}

// プロジェクト全体をXMLに変換
std::wstring ProjectToXml() {
    if (!g_currentProject) return L"";
    
    std::wstring xml;
    xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml += L"<WBSProject>\n";
    xml += L"  <ProjectName>" + XmlEscape(g_currentProject->projectName) + L"</ProjectName>\n";
    xml += L"  <Description>" + XmlEscape(g_currentProject->description) + L"</Description>\n";
    xml += L"  <RootTask>\n";
    xml += WBSItemToXml(g_currentProject->rootTask, 2);
    xml += L"  </RootTask>\n";
    xml += L"</WBSProject>\n";
    return xml;
}

// XMLからWBSアイテムを解析
std::shared_ptr<WBSItem> ParseTaskFromXml(const std::wstring& xml, size_t& pos) {
    size_t taskStart = xml.find(L"<Task>", pos);
    if (taskStart == std::wstring::npos) return nullptr;
    
    size_t taskEnd = xml.find(L"</Task>", taskStart);
    if (taskEnd == std::wstring::npos) return nullptr;
    
    std::wstring taskXml = xml.substr(taskStart, taskEnd - taskStart + 7);
    pos = taskEnd + 7;
    
    auto item = std::make_shared<WBSItem>();
    item->id = ExtractXmlValue(taskXml, L"ID");
    item->taskName = ExtractXmlValue(taskXml, L"Name");
    item->description = ExtractXmlValue(taskXml, L"Description");
    item->assignedTo = ExtractXmlValue(taskXml, L"AssignedTo");
    
    std::wstring statusStr = ExtractXmlValue(taskXml, L"Status");
    if (!statusStr.empty()) {
        item->status = (TaskStatus)std::stoi(statusStr);
    }
    
    std::wstring priorityStr = ExtractXmlValue(taskXml, L"Priority");
    if (!priorityStr.empty()) {
        item->priority = (TaskPriority)std::stoi(priorityStr);
    }
    
    std::wstring estimatedStr = ExtractXmlValue(taskXml, L"EstimatedHours");
    if (!estimatedStr.empty()) {
        item->estimatedHours = std::stod(estimatedStr);
    }
    
    std::wstring actualStr = ExtractXmlValue(taskXml, L"ActualHours");
    if (!actualStr.empty()) {
        item->actualHours = std::stod(actualStr);
    }
    
    std::wstring startDateStr = ExtractXmlValue(taskXml, L"StartDate");
    if (!startDateStr.empty()) {
        item->startDate = StringToSystemTime(startDateStr);
    }
    
    std::wstring endDateStr = ExtractXmlValue(taskXml, L"EndDate");
    if (!endDateStr.empty()) {
        item->endDate = StringToSystemTime(endDateStr);
    }
    
    std::wstring levelStr = ExtractXmlValue(taskXml, L"Level");
    if (!levelStr.empty()) {
        item->level = std::stoi(levelStr);
    }
    
    // 子タスクを解析
    size_t childrenStart = taskXml.find(L"<Children>");
    if (childrenStart != std::wstring::npos) {
        size_t childrenEnd = taskXml.find(L"</Children>");
        if (childrenEnd != std::wstring::npos) {
            std::wstring childrenXml = taskXml.substr(childrenStart + 10, childrenEnd - childrenStart - 10);
            size_t childPos = 0;
            while (true) {
                auto child = ParseTaskFromXml(childrenXml, childPos);
                if (!child) break;
                child->parent = item;
                item->children.push_back(child);
            }
        }
    }
    
    return item;
}

// ExpandAllTreeItems関数とCollapseAllTreeItems関数の実装を追加

void ExpandAllTreeItems(HTREEITEM hItem) {
    if (!hItem || !g_hTreeWBS) return;
    
    TreeView_Expand(g_hTreeWBS, hItem, TVE_EXPAND);
    
    HTREEITEM hChild = TreeView_GetChild(g_hTreeWBS, hItem);
    while (hChild) {
        ExpandAllTreeItems(hChild);
        hChild = TreeView_GetNextSibling(g_hTreeWBS, hChild);
    }
}

void CollapseAllTreeItems(HTREEITEM hItem) {
    if (!hItem || !g_hTreeWBS) return;
    
    HTREEITEM hChild = TreeView_GetChild(g_hTreeWBS, hItem);
    while (hChild) {
        CollapseAllTreeItems(hChild);
        hChild = TreeView_GetNextSibling(g_hTreeWBS, hChild);
    }
    
    TreeView_Expand(g_hTreeWBS, hItem, TVE_COLLAPSE);
}
