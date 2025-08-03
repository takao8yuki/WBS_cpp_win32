/*
 * ============================================================================
 * WBS_cpp_win32.cpp - Work Breakdown Structure（作業分解構造）管理システム
 * ============================================================================
 * 
 * このファイルは、Windows Win32 APIを使用したWBSプロジェクト管理アプリケーション
 * のメイン実装ファイルです。TreeViewとListViewを使用した2ペイン構成のUIで、
 * 階層的なタスク管理機能を提供します。
 * 
 * 【主な機能】
 * - プロジェクトの階層的タスク管理
 * - XML形式でのプロジェクトデータ保存・読み込み
 * - タスクの詳細情報表示・編集
 * - ツリー表示の展開・折りたたみ操作
 * - 最後に開いたファイルの自動復元機能
 * 
 * 【アーキテクチャ概要】
 * - 言語: C++14準拠
 * - GUI: Win32 API + Common Controls 6.0
 * - データ永続化: XML形式
 * - 文字エンコーディング: Unicode (UTF-16LE)
 * - メモリ管理: RAII + std::shared_ptr/std::unique_ptr
 * 
 * 【UI構成】
 * - 左ペイン: TreeView（階層構造でタスクを表示）
 * - 右ペイン: ListView（選択されたタスクの詳細情報を表示）
 * - メニューバー: ファイル、編集、表示、ヘルプの各操作
 * 
 * 【設計パターン】
 * - MVC的構造（Model: WBSクラス, View: UI Controls, Controller: Event Handlers）
 * - Observer パターン（UI更新通知）
 * - Command パターン（メニュー操作）
 * 
 * 作成者: 開発チーム
 * 作成日: 2024年
 * 最終更新: 2024年
 * バージョン: 1.0
 * ============================================================================
 */

// WBS_cpp_win32.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "WBS_cpp_win32.h"
#include <vector>
#include <string>
#include <memory>
#include <commctrl.h>  // TreeView, ListView用
#include <commdlg.h>   // ファイルダイアログ用
#include <fstream>     // ファイルI/O用
#include <sstream>     // 文字列ストリーム用
#include <codecvt>     // 文字コード変換用
#include <locale>      // ロケール設定用
#include <Windows.h>   // Windows基本API
#include <shlobj.h>    // シェル操作API（フォルダパス取得）

#pragma comment(lib, "comctl32.lib")  // Common Controls
#pragma comment(lib, "shell32.lib")   // Shell API

// Windowsサブシステムを指定（コンソールウィンドウを表示しない）
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// ============================================================================
// 定数定義
// ============================================================================
#define MAX_LOADSTRING 100  // 文字列リソースの最大長

// ============================================================================
// 列挙型定義 - タスク管理で使用する状態と優先度
// ============================================================================

/**
 * @brief WBSタスクの進行状態を表す列挙型
 * 
 * プロジェクト管理において、各タスクの現在の状況を明確に分類するために使用します。
 * 進捗レポートの生成や、プロジェクト全体の状況把握に活用されます。
 * 
 * @note 状態遷移の推奨パターン:
 *       NOT_STARTED → IN_PROGRESS → COMPLETED
 *       例外的に ON_HOLD や CANCELLED への変更も可能
 */
enum class TaskStatus {
    NOT_STARTED = 0,    // 未開始 - タスクがまだ開始されていない状態
    IN_PROGRESS = 1,    // 進行中 - タスクが現在実行されている状態  
    COMPLETED = 2,      // 完了 - タスクが正常に完了した状態
    ON_HOLD = 3,        // 保留 - 一時的に作業が停止されている状態
    CANCELLED = 4       // キャンセル - タスクが中止された状態
};

/**
 * @brief WBSタスクの重要度・緊急度を表す列挙型
 * 
 * リソース配分や作業順序の決定、スケジュール調整において
 * タスクの優先順位を判断するために使用します。
 * 
 * @note 優先度の判断基準:
 *       URGENT: 即座に対応が必要（期限切れリスク高)
 *       HIGH: 重要で早めの完了が必要（プロジェクト成功に直結）
 *       MEDIUM: 標準的な優先度（通常の作業順序で処理)
 *       LOW: 時間に余裕がある場合に実行（先送り可能)
 */
enum class TaskPriority {
    LOW = 0,            // 低優先度 - 時間に余裕がある場合に実行
    MEDIUM = 1,         // 中優先度 - 標準的な優先度レベル
    HIGH = 2,           // 高優先度 - 重要で早めの完了が必要
    URGENT = 3          // 緊急 - 最優先で即座に対応が必要
};

// ============================================================================
// クラス定義 - WBSタスク管理の中核となるクラス群
// ============================================================================

/**
 * @brief WBS（Work Breakdown Structure）の個別作業項目を表すクラス
 * 
 * プロジェクトを構成する個々のタスクやサブタスクを表現し、階層構造を形成します。
 * shared_from_thisを継承することで、安全な自己参照機能を提供し、
 * 親子関係の管理における循環参照を防ぎます。
 * 
 * @details 設計思想:
 * - RAII原則: コンストラクタでの完全初期化、デストラクタでの自動クリーンアップ
 * - const-correctness: 読み取り専用操作を明確に区別
 * - 強い型安全性: 列挙型による状態・優先度の型安全な管理
 * - メモリ安全性: shared_ptr/weak_ptrによる自動メモリ管理
 * 
 * @warning 重要な注意事項:
 * - 必ずstd::make_sharedで作成すること（shared_from_thisのため)
 * - 親子関係の設定はAddChildメソッドを使用すること
 * - 循環参照を避けるため、親への強参照は禁止
 * 
 * @example 基本的な使用パターン:
 * @code
 * // タスクの作成
 * auto mainTask = std::make_shared<WBSItem>("システム開発");
 * mainTask->SetDescription("新システムの設計・開発・テスト");
 * mainTask->SetEstimatedHours(200.0);
 * mainTask->SetPriority(TaskPriority::HIGH);
 * 
 * // サブタスクの追加
 * auto designTask = std::make_shared<WBSItem>("設計フェーズ");
 * mainTask->AddChild(designTask);
 * 
 * // 進捗の更新
 * designTask->SetActualHours(25.0);
 * designTask->SetStatus(TaskStatus::IN_PROGRESS);
 * @endcode
 */
class WBSItem : public std::enable_shared_from_this<WBSItem> {
public:
    // メンバ変数（publicアクセス - 簡易的な実装のため）
    std::wstring id;                                        ///< タスクの一意識別子（階層的なID体系)
    std::wstring taskName;                                  ///< タスクの名称
    std::wstring description;                               ///< タスクの詳細説明
    std::wstring assignedTo;                                ///< 担当者名
    TaskStatus status;                                      ///< 現在の進行状態
    TaskPriority priority;                                  ///< 優先度レベル
    double estimatedHours;                                  ///< 見積もり工数（時間単位)
    double actualHours;                                     ///< 実績工数（時間単位)
    SYSTEMTIME startDate;                                   ///< 開始予定日
    SYSTEMTIME endDate;                                     ///< 終了予定日
    int level;                                              ///< 階層レベル（0=ルート)
    std::vector<std::shared_ptr<WBSItem>> children;         ///< 子タスクのコレクション
    std::weak_ptr<WBSItem> parent;                          ///< 親タスクへの弱参照（循環参照回避）

    /**
     * @brief デフォルトコンストラクタ
     * 
     * 新しいWBSアイテムを安全な初期状態で作成します。
     * 全てのメンバ変数が適切なデフォルト値で初期化され、
     * 即座に使用可能な状態になります。
     * 
     * @post 以下の状態で初期化される:
     *       - status: NOT_STARTED
     *       - priority: MEDIUM
     *       - estimatedHours/actualHours: 0.0
     *       - level: 0
     *       - startDate/endDate: 現在のシステム時刻
     *       - taskName: "新しいタスク"
     */
    WBSItem() : status(TaskStatus::NOT_STARTED), priority(TaskPriority::MEDIUM), 
                estimatedHours(0.0), actualHours(0.0), level(0) {
        GetSystemTime(&startDate);
        GetSystemTime(&endDate);
        taskName = L"新しいタスク";
    }

    /**
     * @brief 名前指定コンストラクタ
     * 
     * @param name タスク名
     * 
     * 指定された名前でWBSアイテムを作成します。
     * デフォルトコンストラクタを委譲呼び出しして基本初期化を行い、
     * その後でタスク名を設定します。
     * 
     * @param name 設定するタスク名（空文字列も許可）
     * 
     * @post デフォルト初期化 + taskName = nameの状態
     */
    WBSItem(const std::wstring& name) : WBSItem() {
        taskName = name;
    }

    /**
     * @brief 子タスクを階層構造に追加
     * 
     * @param child 追加する子タスク
     * 
     * 指定されたタスクを子タスクとして追加し、適切な階層関係を構築します。
     * 子タスクの親参照、階層レベル、IDが自動的に設定されます。
     * 
     * @pre child != nullptr
     * @pre child が既に他の親を持っていないこと
     * 
     * @post 以下の状態変更が行われる:
     *       - child->parent = this（弱参照）
     *       - child->level = this->level + 1
     *       - child->id = this->id + "." + (子タスク番号)
     *       - this->children に child が追加される
     * 
     * @note IDの自動生成例: 親ID"1.2" + 3番目の子 → "1.2.3"
     */
    void AddChild(std::shared_ptr<WBSItem> child) {
        child->parent = shared_from_this();
        child->level = this->level + 1;
        child->id = this->id + L"." + std::to_wstring(children.size() + 1);
        children.push_back(child);
    }

    /**
     * @brief タスクの状態を日本語文字列で取得
     * 
     * @return 状態の日本語表記
     * 
     * TaskStatus列挙値を、ユーザーフレンドリーな日本語文字列に変換します。
     * UI表示やレポート生成において使用されます。
     * 
     * @note 戻り値の対応表:
     *       NOT_STARTED → "未開始"
     *       IN_PROGRESS → "進行中"  
     *       COMPLETED → "完了"
     *       ON_HOLD → "保留"
     *       CANCELLED → "キャンセル"
     *       その他 → "未開始"（フォールバック）
     */
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

    /**
     * @brief タスクの優先度を日本語文字列で取得
     * 
     * @return 優先度の日本語表記
     * 
     * TaskPriority列挙値を、視覚的に分かりやすい日本語文字列に変換します。
     * タスク一覧の表示や、優先度によるソート表示で使用されます。
     */
    std::wstring GetPriorityString() const {
        switch (priority) {
            case TaskPriority::LOW: return L"低";
            case TaskPriority::MEDIUM: return L"中";
            case TaskPriority::HIGH: return L"高";
            case TaskPriority::URGENT: return L"緊急";
            default: return L"中";
        }
    }

    /**
     * @brief タスクの進捗率を計算
     * 
     * @return 進捗率（0.0〜100.0の範囲）
     * 
     * 実績工数を見積もり工数で除算して進捗率を算出します。
     * プロジェクト管理ダッシュボードやガントチャートでの
     * 視覚的な進捗表示に使用されます。
     * 
     * @note 計算仕様:
     *       - estimatedHours == 0.0 の場合: 0.0を返す（ゼロ除算回避）
     *       - 100.0を超える値も許可（工数超過の可視化）
     *       - 負の値は発生しない（actualHours >= 0.0 を前提）
     * 
     * @warning 進捗率 > 100% の場合は工数見積もりの見直しが必要
     */
    double GetProgressPercentage() const {
        if (estimatedHours == 0.0) return 0.0;
        return (actualHours / estimatedHours) * 100.0;
    }
};

/**
 * @brief WBSプロジェクト全体を統括管理するクラス
 * 
 * プロジェクトレベルでのメタデータを管理し、ルートタスクを通じて
 * 全ての階層タスクへの一元的なアクセスポイントを提供します。
 * XML形式でのデータ永続化機能も包含しています。
 * 
 * @details 責務:
 * - プロジェクト基本情報の管理（名前、説明、作成日など)
 * - ルートタスクを起点とした全タスクツリーの管理
 * - プロジェクト全体の統計情報計算
 * - ファイルI/Oを通じたデータ永続化
 * 
 * @example プロジェクトの基本的な操作:
 * @code
 * // プロジェクトの作成と初期設定
 * auto project = std::make_unique<WBSProject>("ECサイト構築");
 * project->description = "新規ECサイトの企画・設計・開発・運用開始まで";
 * 
 * // メインフェーズの追加
 * auto planningPhase = std::make_shared<WBSItem>("企画フェーズ");
 * project->rootTask->AddChild(planningPhase);
 * @endcode
 */
class WBSProject {
public:
    std::wstring projectName;                               ///< プロジェクトの正式名称
    std::wstring description;                               ///< プロジェクトの概要説明
    std::shared_ptr<WBSItem> rootTask;                      ///< 全タスクの最上位ノード

    /**
     * @brief デフォルトコンストラクタ
     * 
     * 標準的なプロジェクト名でWBSプロジェクトを初期化します。
     * ルートタスクが自動作成され、即座に使用可能な状態になります。
     * 
     * @post 以下の状態で初期化:
     *       - projectName: "新規WBSプロジェクト"
     *       - rootTask: 自動作成（ID="1", level=0）
     *       - description: 空文字列
     */
    WBSProject() {
        projectName = L"新規WBSプロジェクト";
        rootTask = std::make_shared<WBSItem>(projectName);
        rootTask->id = L"1";
        rootTask->level = 0;
    }

    /**
     * @brief 名前指定コンストラクタ
     * 
     * @param name プロジェクト名
     * 
     * 指定された名前でWBSプロジェクトを作成します。
     * デフォルトコンストラクタを委譲して基本初期化を行い、
     * その後でプロジェクト名とルートタスク名を同期設定します。
     */
    WBSProject(const std::wstring& name) : WBSProject() {
        projectName = name;
        rootTask->taskName = name;
    }
};

// ============================================================================
// グローバル変数定義 - アプリケーション全体で共有される状態
// ============================================================================

// Windows APIアプリケーション標準のグローバル変数
HINSTANCE hInst;                                // 現在のアプリケーションインスタンス
WCHAR szTitle[MAX_LOADSTRING];                  // ウィンドウタイトルバーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メインウィンドウクラス名

// WBS アプリケーション固有のグローバル変数
std::unique_ptr<WBSProject> g_currentProject;   // 現在開いているプロジェクト
HWND g_hTreeWBS = nullptr;                      // 左ペインのTreeViewコントロール
HWND g_hListDetails = nullptr;                  // 右ペインのListViewコントロール  
HTREEITEM g_selectedItem = nullptr;             // TreeViewで現在選択されているアイテム

// ============================================================================
// リソースID定義 - メニューとコントロールの識別子
// ============================================================================

// ファイル操作メニューのコマンドID（110番台）
#define IDM_FILE_NEW        110                 // 新規プロジェクト作成
#define IDM_FILE_OPEN       111                 // プロジェクトファイルを開く
#define IDM_FILE_SAVE       112                 // プロジェクトファイルを保存

// 編集操作メニューのコマンドID（120番台）
#define IDM_EDIT_ADD_TASK   120                 // ルートレベルにタスクを追加
#define IDM_EDIT_ADD_SUBTASK 121                // 選択タスクにサブタスクを追加
#define IDM_EDIT_EDIT_TASK  122                 // 選択タスクの詳細編集
#define IDM_EDIT_DELETE_TASK 123                // 選択タスクの削除

// 表示操作メニューのコマンドID（130番台）
#define IDM_VIEW_EXPAND_ALL 130                 // 全てのツリーノードを展開
#define IDM_VIEW_COLLAPSE_ALL 131               // 全てのツリーノードを折りたたみ

// UIコントロールのID（1000番台）
#define IDC_TREE_WBS        1000                // 左ペインのTreeViewコントロール
#define IDC_LIST_DETAILS    1001                // 右ペインのListViewコントロール

// ============================================================================
// 関数の前方宣言 - Windows標準とWBS固有の関数群
// ============================================================================

// Windows APIアプリケーション標準の関数
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// WBS アプリケーション固有の関数群

// === 初期化・UI作成関連 ===
void InitializeWBS(HWND hWnd);                              // WBSシステム全体の初期化
void CreateWBSControls(HWND hWnd);                          // TreeView/ListView の作成と設定

// === UI更新・表示関連 ===  
void RefreshTreeView();                                     // TreeView の内容を完全再構築
void RefreshListView();                                     // ListView の詳細表示を更新
HTREEITEM AddTreeItem(HTREEITEM hParent, std::shared_ptr<WBSItem> item);  // TreeView に単一アイテム追加
void AddTreeItemRecursive(HTREEITEM hParent, std::shared_ptr<WBSItem> item); // TreeView に階層アイテム追加

// === データ操作・変換関連 ===
std::shared_ptr<WBSItem> GetItemFromTreeItem(HTREEITEM hItem);  // TreeViewアイテムからWBSItem取得

// === イベントハンドラ関連 ===
void OnTreeSelectionChanged();                             // TreeView選択変更時の処理
void OnAddTask();                                          // タスク追加コマンドの処理
void OnAddSubTask();                                       // サブタスク追加コマンドの処理
void OnEditTask();                                         // タスク編集コマンドの処理
void OnDeleteTask();                                       // タスク削除コマンドの処理

// === ツリー操作関連 ===
void ExpandAllTreeItems(HTREEITEM hItem);                  // 指定ノード以下を全展開
void CollapseAllTreeItems(HTREEITEM hItem);                // 指定ノード以下を全折りたたみ

// === 設定ファイル関連 ===
void SaveLastOpenedFile(const std::wstring& filePath);     // 最後に開いたファイルパスを保存
std::wstring GetLastOpenedFile();                          // 最後に開いたファイルパスを取得

// === XML操作関連（実装は WBS_XML_Functions.cpp にあり）===
extern std::wstring XmlEscape(const std::wstring& text);
extern std::wstring XmlUnescape(const std::wstring& text);
extern std::wstring SystemTimeToString(const SYSTEMTIME& st);
extern SYSTEMTIME StringToSystemTime(const std::wstring& str);
extern std::wstring WBSItemToXml(std::shared_ptr<WBSItem> item, int indent);
extern std::wstring ProjectToXml();
extern std::shared_ptr<WBSItem> ParseTaskFromXml(const std::wstring& xml, size_t& pos);
extern std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tag, size_t startPos);

// === ファイル操作関連（実装は WBS_XML_Functions.cpp にあり）===
extern void OnSaveProject();
extern void OnOpenProject();
extern bool LoadProjectFromFile(const std::wstring& filePath);

// ============================================================================
// アプリケーションエントリポイント
// ============================================================================

/**
 * @brief Windowsアプリケーションのメインエントリポイント
 * 
 * @param hInstance 現在のアプリケーションインスタンス
 * @param hPrevInstance 前のインスタンス（Win32では常にNULL）
 * @param lpCmdLine コマンドライン引数
 * @param nCmdShow ウィンドウ表示方法の指定
 * @return アプリケーション終了コード
 * 
 * Windowsアプリケーションの標準的な初期化シーケンスを実行し、
 * メッセージループを開始します。WBS固有の初期化も含まれます。
 * 
 * @details 実行フロー:
 * 1. コマンドライン引数の処理（現在は使用しない）
 * 2. Common Controls の初期化（TreeView/ListView使用のため）
 * 3. ウィンドウクラスの登録
 * 4. メインウィンドウの作成と表示
 * 5. アクセラレータテーブルの読み込み
 * 6. メッセージループの開始
 */
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Common Controls の初期化（TreeView/ListView 使用のため必須）
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // リソースからグローバル文字列を読み込み
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WBSCPPWIN32, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WBSCPPWIN32));

    MSG msg;

    // メインメッセージループ: ウィンドウが閉じられるまで継続
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

// 設定ファイル操作用のヘルパー関数
std::wstring GetConfigFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath) == S_OK) {
        std::wstring configDir = std::wstring(appDataPath) + L"\\WBS_cpp_win32";
        CreateDirectory(configDir.c_str(), nullptr);
        return configDir + L"\\config.txt";
    }
    return L"config.txt"; // フォールバック
}

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

// WBS_XML_Functions.cppをインクルードしてリンクエラーを解決
#include "..\WBS_XML_Functions.cpp"
