/*
 * ============================================================================
 * WBS_cpp_win32.cpp - Work Breakdown Structure（作業分解構造）管理システム
 * ============================================================================
 * 
 * フォームデザインベースのWBSプロジェクト管理アプリケーション
 * Visual Studioのリソースエディタで設計されたダイアログを使用
 * 
 * 【変更点】
 * - CreateWindowからDialogBoxに変更
 * - フォームデザイナーで設計されたレイアウトを使用
 * - ボタンクリックイベントをダイアログプロシージャで処理
 * 
 * 【UI構成】
 * - IDD_WBS_MAIN: メインフォーム（800x600ピクセル）
 * - IDD_TASK_EDIT: タスク編集ダイアログ
 * - Visual Studioリソースエディタで視覚的に設計可能
 * ============================================================================
 */

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
#include <Windows.h>
#include <shlobj.h>
#include <windowsx.h>  // ComboBoxマクロの定義を確実にインクルード

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// ComboBoxマクロが定義されていない場合の代替定義
#ifndef ComboBox_AddString
#define ComboBox_AddString(hwndCtl, lpsz) ((int)(DWORD)SendMessage((hwndCtl), CB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpsz)))
#endif

#ifndef ComboBox_SetCurSel
#define ComboBox_SetCurSel(hwndCtl, index) ((int)(DWORD)SendMessage((hwndCtl), CB_SETCURSEL, (WPARAM)(int)(index), 0L))
#endif

#ifndef ComboBox_GetCurSel
#define ComboBox_GetCurSel(hwndCtl) ((int)(DWORD)SendMessage((hwndCtl), CB_GETCURSEL, 0L, 0L))
#endif

#define MAX_LOADSTRING 100

// ============================================================================
// 列挙型定義
// ============================================================================

enum class TaskStatus {
    NOT_STARTED = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2,
    ON_HOLD = 3,
    CANCELLED = 4
};

enum class TaskPriority {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    URGENT = 3
};

// ============================================================================
// クラス定義（既存のWBSItemとWBSProjectクラス）
// ============================================================================

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

// ============================================================================
// グローバル変数
// ============================================================================

HINSTANCE hInst;
std::unique_ptr<WBSProject> g_currentProject;
HWND g_hMainDialog = nullptr;           // メインダイアログハンドル
HWND g_hTreeWBS = nullptr;
HWND g_hListDetails = nullptr;
HTREEITEM g_selectedItem = nullptr;

// ============================================================================
// 関数の前方宣言
// ============================================================================

INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TaskEditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void InitializeWBSDialog(HWND hDlg);
void RefreshTreeView();
void RefreshListView();
void OnTreeSelectionChanged();
HTREEITEM AddTreeItem(HTREEITEM hParent, std::shared_ptr<WBSItem> item);
void AddTreeItemRecursive(HTREEITEM hParent, std::shared_ptr<WBSItem> item);
std::shared_ptr<WBSItem> GetItemFromTreeItem(HTREEITEM hItem);

// 外部XML関数
extern void OnSaveProject();
extern void OnOpenProject();
extern bool LoadProjectFromFile(const std::wstring& filePath);

// ============================================================================
// メインエントリポイント - ダイアログベースアプリケーション
// ============================================================================

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    hInst = hInstance;

    // Common Controls の初期化
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // メインダイアログを表示
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_WBS_MAIN), nullptr, MainDlgProc);

    return 0;
}

// ============================================================================
// メインダイアログプロシージャ
// ============================================================================

INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            g_hMainDialog = hDlg;
            
            // コントロールハンドルを取得
            g_hTreeWBS = GetDlgItem(hDlg, IDC_TREE_WBS);
            g_hListDetails = GetDlgItem(hDlg, IDC_LIST_DETAILS);
            
            // WBSシステムを初期化
            InitializeWBSDialog(hDlg);
            
            return (INT_PTR)TRUE;
        }

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDC_BUTTON_NEW_PROJECT:
            case IDM_FILE_NEW:
                g_currentProject = std::make_unique<WBSProject>();
                RefreshTreeView();
                RefreshListView();
                break;
                
            case IDC_BUTTON_OPEN_PROJECT:
            case IDM_FILE_OPEN:
                OnOpenProject();
                break;
                
            case IDC_BUTTON_SAVE_PROJECT:
            case IDM_FILE_SAVE:
                OnSaveProject();
                break;
                
            case IDC_BUTTON_ADD_TASK:
            case IDM_EDIT_ADD_TASK:
                {
                    if (!g_currentProject) return FALSE;
                    auto newTask = std::make_shared<WBSItem>(L"新しいタスク");
                    g_currentProject->rootTask->AddChild(newTask);
                    RefreshTreeView();
                    MessageBox(hDlg, L"新しいタスクを追加しました。", L"情報", MB_OK | MB_ICONINFORMATION);
                }
                break;
                
            case IDC_BUTTON_ADD_SUBTASK:
            case IDM_EDIT_ADD_SUBTASK:
                {
                    if (!g_selectedItem) {
                        MessageBox(hDlg, L"親タスクを選択してください。", L"エラー", MB_OK | MB_ICONWARNING);
                        return FALSE;
                    }
                    std::shared_ptr<WBSItem> parentItem = GetItemFromTreeItem(g_selectedItem);
                    if (!parentItem) return FALSE;
                    auto newSubTask = std::make_shared<WBSItem>(L"新しいサブタスク");
                    parentItem->AddChild(newSubTask);
                    RefreshTreeView();
                    MessageBox(hDlg, L"新しいサブタスクを追加しました。", L"情報", MB_OK | MB_ICONINFORMATION);
                }
                break;
                
            case IDC_BUTTON_EDIT_TASK:
            case IDM_EDIT_EDIT_TASK:
                {
                    if (!g_selectedItem) {
                        MessageBox(hDlg, L"編集するタスクを選択してください。", L"エラー", MB_OK | MB_ICONWARNING);
                        return FALSE;
                    }
                    // タスク編集ダイアログを表示
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_TASK_EDIT), hDlg, TaskEditDlgProc);
                }
                break;
                
            case IDC_BUTTON_DELETE_TASK:
            case IDM_EDIT_DELETE_TASK:
                {
                    if (!g_selectedItem) {
                        MessageBox(hDlg, L"削除するタスクを選択してください。", L"エラー", MB_OK | MB_ICONWARNING);
                        return FALSE;
                    }
                    if (MessageBox(hDlg, L"選択したタスクを削除しますか？", L"確認", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        MessageBox(hDlg, L"タスク削除機能は今後実装予定です。", L"情報", MB_OK | MB_ICONINFORMATION);
                    }
                }
                break;
                
            case IDC_BUTTON_EXPAND_ALL:
            case IDM_VIEW_EXPAND_ALL:
                if (g_hTreeWBS) {
                    HTREEITEM hRoot = TreeView_GetRoot(g_hTreeWBS);
                    if (hRoot) {
                        // 展開ロジックをここに実装
                    }
                }
                break;
                
            case IDC_BUTTON_COLLAPSE_ALL:
            case IDM_VIEW_COLLAPSE_ALL:
                if (g_hTreeWBS) {
                    HTREEITEM hRoot = TreeView_GetRoot(g_hTreeWBS);
                    if (hRoot) {
                        // 折り畳みロジックをここに実装
                    }
                }
                break;
                
            case IDC_BUTTON_EXIT:
            case IDM_EXIT:
                EndDialog(hDlg, IDOK);
                break;
                
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
                break;
                
            default:
                return (INT_PTR)FALSE;
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

    case WM_CLOSE:
        EndDialog(hDlg, IDOK);
        break;

    default:
        return (INT_PTR)FALSE;
    }
    return (INT_PTR)TRUE;
}

// ============================================================================
// タスク編集ダイアログプロシージャ
// ============================================================================

INT_PTR CALLBACK TaskEditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // 選択されたタスクの情報をダイアログに設定
            if (g_selectedItem) {
                std::shared_ptr<WBSItem> item = GetItemFromTreeItem(g_selectedItem);
                if (item) {
                    SetDlgItemText(hDlg, IDC_EDIT_TASK_NAME, item->taskName.c_str());
                    SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, item->description.c_str());
                    SetDlgItemText(hDlg, IDC_EDIT_ASSIGNED_TO, item->assignedTo.c_str());
                    
                    // コンボボックスの初期化 - マクロを使用
                    HWND hComboStatus = GetDlgItem(hDlg, IDC_COMBO_STATUS);
                    ComboBox_AddString(hComboStatus, L"未開始");
                    ComboBox_AddString(hComboStatus, L"進行中");
                    ComboBox_AddString(hComboStatus, L"完了");
                    ComboBox_AddString(hComboStatus, L"保留");
                    ComboBox_AddString(hComboStatus, L"キャンセル");
                    ComboBox_SetCurSel(hComboStatus, (int)item->status);
                    
                    HWND hComboPriority = GetDlgItem(hDlg, IDC_COMBO_PRIORITY);
                    ComboBox_AddString(hComboPriority, L"低");
                    ComboBox_AddString(hComboPriority, L"中");
                    ComboBox_AddString(hComboPriority, L"高");
                    ComboBox_AddString(hComboPriority, L"緊急");
                    ComboBox_SetCurSel(hComboPriority, (int)item->priority);
                    
                    // 工数の設定
                    SetDlgItemText(hDlg, IDC_EDIT_ESTIMATED_HOURS, std::to_wstring((int)item->estimatedHours).c_str());
                    SetDlgItemText(hDlg, IDC_EDIT_ACTUAL_HOURS, std::to_wstring((int)item->actualHours).c_str());
                    
                    // 進捗バーの設定
                    HWND hProgress = GetDlgItem(hDlg, IDC_PROGRESS_BAR);
                    SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                    SendMessage(hProgress, PBM_SETPOS, (int)item->GetProgressPercentage(), 0);
                    
                    std::wstring progressText = std::to_wstring((int)item->GetProgressPercentage()) + L"%";
                    SetDlgItemText(hDlg, IDC_STATIC_PROGRESS, progressText.c_str());
                }
            }
            return (INT_PTR)TRUE;
        }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDC_BUTTON_APPLY)
        {
            // フォームからデータを取得してタスクを更新
            if (g_selectedItem) {
                std::shared_ptr<WBSItem> item = GetItemFromTreeItem(g_selectedItem);
                if (item) {
                    // タスク名の更新
                    wchar_t buffer[256];
                    GetDlgItemText(hDlg, IDC_EDIT_TASK_NAME, buffer, 256);
                    item->taskName = buffer;
                    
                    GetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, buffer, 256);
                    item->description = buffer;
                    
                    GetDlgItemText(hDlg, IDC_EDIT_ASSIGNED_TO, buffer, 256);
                    item->assignedTo = buffer;
                    
                    // ステータスと優先度の更新 - マクロを使用
                    HWND hComboStatus = GetDlgItem(hDlg, IDC_COMBO_STATUS);
                    item->status = (TaskStatus)ComboBox_GetCurSel(hComboStatus);
                    
                    HWND hComboPriority = GetDlgItem(hDlg, IDC_COMBO_PRIORITY);
                    item->priority = (TaskPriority)ComboBox_GetCurSel(hComboPriority);
                    
                    // 工数の更新
                    GetDlgItemText(hDlg, IDC_EDIT_ESTIMATED_HOURS, buffer, 256);
                    item->estimatedHours = _wtof(buffer);
                    
                    GetDlgItemText(hDlg, IDC_EDIT_ACTUAL_HOURS, buffer, 256);
                    item->actualHours = _wtof(buffer);
                    
                    // UIを更新
                    RefreshTreeView();
                    RefreshListView();
                }
            }
            
            if (LOWORD(wParam) == IDOK) {
                EndDialog(hDlg, IDOK);
            }
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ============================================================================
// 設定ファイル操作用のヘルパー関数
// ============================================================================

/**
 * @brief アプリケーション設定ファイルのパスを取得
 * 
 * @return 設定ファイルの完全パス
 * 
 * ユーザーのAppDataフォルダ内にWBSアプリケーション専用のフォルダを作成し、
 * 設定ファイルのパスを返します。フォルダが存在しない場合は自動作成されます。
 * 
 * @note フォールバック処理:
 * AppDataフォルダの取得に失敗した場合は、現在のディレクトリに
 * config.txtファイルを作成します。
 * 
 * @details パス構成:
 * %APPDATA%\WBS_cpp_win32\config.txt
 * 例: C:\Users\username\AppData\Roaming\WBS_cpp_win32\config.txt
 */
std::wstring GetConfigFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath) == S_OK) {
        std::wstring configDir = std::wstring(appDataPath) + L"\\WBS_cpp_win32";
        CreateDirectory(configDir.c_str(), nullptr);
        return configDir + L"\\config.txt";
    }
    return L"config.txt"; // フォールバック
}

/**
 * @brief 最後に開いたファイルのパスを保存
 * 
 * @param filePath 保存するファイルパス
 * 
 * ユーザーが最後に開いたプロジェクトファイルのパスを設定ファイルに保存します。
 * アプリケーション起動時の自動復元機能で使用されます。
 * 
 * @details 保存形式:
 * LastOpenedFile=C:\path\to\project.xml
 * 
 * @note エラーハンドリング:
 * ファイル保存に失敗した場合でも、アプリケーションの動作には影響しません。
 * 例外はキャッチして無視されます。
 */
void SaveLastOpenedFile(const std::wstring& filePath) {
    try {
        std::wstring configFile = GetConfigFilePath();
        std::wofstream file(configFile);
        if (file.is_open()) {
            file << L"LastOpenedFile=" << filePath << std::endl;
            file.close();
        }
    } catch (...) {
        // エラーは無視（設定保存に失敗しても動作に支障なし）
    }
}

/**
 * @brief 最後に開いたファイルのパスを取得
 * 
 * @return 最後に開いたファイルパス、見つからない場合は空文字列
 * 
 * 設定ファイルから最後に開いたプロジェクトファイルのパスを読み込みます。
 * アプリケーション起動時の自動復元機能で使用されます。
 * 
 * @note エラーハンドリング:
 * - 設定ファイルが存在しない場合: 空文字列を返す
 * - ファイル読み込みエラーの場合: 空文字列を返す
 * - 形式が正しくない場合: 空文字列を返す
 * 
 * @details 読み込み処理:
 * "LastOpenedFile="で始まる行を検索し、その後の部分をパスとして返します。
 */
std::wstring GetLastOpenedFile() {
    try {
        std::wstring configFile = GetConfigFilePath();
        std::wifstream file(configFile);
        if (file.is_open()) {
            std::wstring line;
            while (std::getline(file, line)) {
                if (line.find(L"LastOpenedFile=") == 0) {
                    return line.substr(15); // "LastOpenedFile="の文字数分をスキップ
                }
            }
            file.close();
        }
    } catch (...) {
        // エラーは無視
    }
    return L"";
}

// ============================================================================
// その他の関数（既存の実装を使用）
// ============================================================================

void InitializeWBSDialog(HWND hDlg) {
    // ListViewの初期化
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

    RefreshTreeView();
}

// 既存のRefreshTreeView, RefreshListView等の関数を使用
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

// WBS_XML_Functions.cppをインクルード
#include "..\WBS_XML_Functions.cpp"
