/*
 * ============================================================================
 * Resource.h - WBSアプリケーション リソースID定義ヘッダー
 * ============================================================================
 * 
 * このファイルは、WBS（Work Breakdown Structure）管理アプリケーションで使用される
 * 全てのWindowsリソースの識別子（ID）を一元管理します。
 * Visual Studio のリソースエディタによって自動生成・更新される部分と、
 * 手動で追加されたWBS固有のリソースIDが含まれています。
 * 
 * 【主な役割】
 * - メニューコマンドIDの定義
 * - ダイアログボックスIDの定義  
 * - UIコントロールIDの定義
 * - アイコン・文字列リソースIDの定義
 * - リソースエディタとの連携
 * 
 * 【ID命名規則】
 * - IDM_xxx: メニューコマンド（Menu）
 * - IDD_xxx: ダイアログボックス（Dialog）
 * - IDC_xxx: コントロール（Control）
 * - IDS_xxx: 文字列リソース（String）
 * - IDI_xxx: アイコンリソース（Icon）
 * - IDR_xxx: 汎用リソース（Resource）
 * 
 * 【ID範囲割り当て】
 * - 100番台: システム標準・アプリケーション基本
 * - 110番台: ファイル操作メニュー
 * - 120番台: 編集操作メニュー  
 * - 130番台: 表示操作メニュー
 * - 140番台: 編集拡張機能（元に戻す等）
 * - 150番台: 印刷関連機能
 * - 160番台: ヘルプ関連機能
 * - 200番台: ダイアログボックス
 * - 1000番台: UIコントロール
 * 
 * 【保守上の注意】
 * - Visual Studio リソースエディタが自動更新する部分の手動編集は避ける
 * - 新しいIDは適切な範囲に追加し、重複を避ける
 * - 削除されたIDは再利用せず、新しい番号を使用する
 * - コメントで用途を明確に記述する
 * 
 * 作成者: Visual Studio + WBS開発チーム
 * 作成日: 2024年
 * 最終更新: 2024年
 * バージョン: 1.0
 * ============================================================================
 */

//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ で生成されたインクルード ファイルです。
// WBS_cpp_win32.rc で使用
//
#define IDC_MYICON                      2
#define IDD_WBS_CPP_WIN32_DIALOG        102
#define IDS_APP_TITLE                   103
#define IDD_ABOUTBOX                    103
#define IDM_ABOUT                       104
#define IDM_EXIT                        105
#define IDI_WBS_CPP_WIN32               107
#define IDI_SMALL                       108
#define IDC_WBS_CPP_WIN32               109
#define IDR_MAINFRAME                   128

// 汎用静的コントロールID（ラベル、グループボックス等）
#define IDC_STATIC                      -1

// WBSメインダイアログ
#define IDD_WBS_MAIN                    201

// メニューアイテム
#define IDM_FILE_NEW                    110
#define IDM_FILE_OPEN                   111
#define IDM_FILE_SAVE                   112

#define IDM_EDIT_ADD_TASK              120
#define IDM_EDIT_ADD_SUBTASK           121
#define IDM_EDIT_EDIT_TASK             122
#define IDM_EDIT_DELETE_TASK           123

#define IDM_VIEW_EXPAND_ALL            130
#define IDM_VIEW_COLLAPSE_ALL          131

// コントロールID（メインダイアログ）
#define IDC_TREE_WBS                    1001
#define IDC_LIST_DETAILS                1002
#define IDC_BUTTON_EXPAND_ALL           1003
#define IDC_BUTTON_COLLAPSE_ALL         1004
#define IDC_BUTTON_ADD_TASK             1005
#define IDC_BUTTON_ADD_SUBTASK          1006
#define IDC_BUTTON_EDIT_TASK            1007
#define IDC_BUTTON_DELETE_TASK          1008
#define IDC_BUTTON_NEW_PROJECT          1009
#define IDC_BUTTON_OPEN_PROJECT         1010
#define IDC_BUTTON_SAVE_PROJECT         1011
#define IDC_BUTTON_EXIT                 1012

// タスク編集ダイアログ
#define IDD_TASK_EDIT                   301
#define IDC_EDIT_TASK_NAME              1101
#define IDC_EDIT_DESCRIPTION            1102
#define IDC_EDIT_ASSIGNED_TO            1103
#define IDC_COMBO_STATUS                1104
#define IDC_COMBO_PRIORITY              1105
#define IDC_EDIT_ESTIMATED_HOURS        1106
#define IDC_EDIT_ACTUAL_HOURS           1107
#define IDC_DATETIME_START              1108
#define IDC_DATETIME_END                1109
#define IDC_PROGRESS_BAR                1110
#define IDC_STATIC_PROGRESS             1111
#define IDC_BUTTON_APPLY                1112

// アイコンリソース
#define IDI_WBSCPPWIN32                 107

// アクセラレータ
#define IDC_WBSCPPWIN32                 109

// 次のオブジェクトの既定値
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        129
#define _APS_NEXT_COMMAND_VALUE         32771
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           110
#endif
#endif

// ============================================================================
// ID管理ガイドライン
// ============================================================================

/*
 * 【新しいID追加時の注意事項】
 * 
 * 1. 適切な範囲に追加:
 *    - メニュー: 110-169番台（機能別）
 *    - ダイアログ: 200-299番台
 *    - コントロール: 1000-1999番台
 * 
 * 2. ID命名規則の遵守:
 *    - 機能を表す明確な名前を使用
 *    - 略語は一般的なもののみ使用
 *    - アンダースコアで単語を区切る
 * 
 * 3. コメントの追加:
 *    - 用途と動作を明確に記述
 *    - 将来実装予定の場合は明記
 *    - 関連する他のIDとの関係を記述
 * 
 * 4. 削除時の対応:
 *    - 削除されたIDは再利用禁止
 *    - コメントアウトして履歴を残す
 *    - 代替IDがある場合は併記
 * 
 * 【パフォーマンス考慮事項】
 * - IDの範囲を適切に分散させることで、switch文での分岐効率が向上
 * - 頻繁に使用されるIDは小さい値を割り当てる
 * - 連続した値を使用することでコンパイラ最適化が効果的
 */
