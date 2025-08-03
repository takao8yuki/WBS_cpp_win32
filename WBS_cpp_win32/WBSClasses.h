/*
 * ============================================================================
 * WBSClasses.h - WBSアプリケーション クラス定義ヘッダー
 * ============================================================================
 * 
 * このファイルには、WBS（Work Breakdown Structure）管理アプリケーションで
 * 使用される主要なクラスと列挙型の定義が含まれています。
 * 
 * 【含まれるクラス】
 * - TaskStatus: タスクの進行状態を表す列挙型
 * - TaskPriority: タスクの優先度を表す列挙型  
 * - WBSItem: 個別のタスク・サブタスクを表すクラス
 * - WBSProject: プロジェクト全体を管理するクラス
 * 
 * 【設計原則】
 * - RAII: オブジェクトの自動的なリソース管理
 * - 型安全性: 列挙クラスによる強い型付け
 * - メモリ安全性: スマートポインタの活用
 * - 拡張性: 将来の機能追加に対応した設計
 * 
 * 作成者: WBS開発チーム
 * 作成日: 2024年
 * バージョン: 1.0
 * ============================================================================
 */

#pragma once

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <memory>

// ============================================================================
// Common Controls マクロ定義補完
// ============================================================================

// ComboBoxマクロが定義されていない場合の補完定義
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

// ComboBoxメッセージ定数の定義
#ifndef CB_ADDSTRING
#define CB_ADDSTRING            0x0143
#endif

#ifndef CB_SETCURSEL
#define CB_SETCURSEL            0x014E
#endif

#ifndef CB_GETCURSEL
#define CB_GETCURSEL            0x0147
#endif

// ============================================================================
// 列挙型定義
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
// クラス定義
// ============================================================================

/**
 * @brief WBS（Work Breakdown Structure）の個別作業項目を表すクラス
 * 
 * プロジェクトを構成する個々のタスクやサブタスクを表現し、階層構造を形成します。
 * shared_from_thisを継承することで、安全な自己参照機能を提供し、
 * 親子関係の管理における循環参照を防ぎます。
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
     */
    WBSItem() : status(TaskStatus::NOT_STARTED), priority(TaskPriority::MEDIUM), 
                estimatedHours(0.0), actualHours(0.0), level(0) {
        GetSystemTime(&startDate);
        GetSystemTime(&endDate);
        taskName = L"新しいタスク";
    }

    /**
     * @brief 名前指定コンストラクタ
     */
    WBSItem(const std::wstring& name) : WBSItem() {
        taskName = name;
    }

    /**
     * @brief 子タスクを階層構造に追加
     */
    void AddChild(std::shared_ptr<WBSItem> child) {
        child->parent = shared_from_this();
        child->level = this->level + 1;
        child->id = this->id + L"." + std::to_wstring(children.size() + 1);
        children.push_back(child);
    }

    /**
     * @brief タスクの状態を日本語文字列で取得
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
 */
class WBSProject {
public:
    std::wstring projectName;                               ///< プロジェクトの正式名称
    std::wstring description;                               ///< プロジェクトの概要説明
    std::shared_ptr<WBSItem> rootTask;                      ///< 全タスクの最上位ノード

    /**
     * @brief デフォルトコンストラクタ
     */
    WBSProject() {
        projectName = L"新規WBSプロジェクト";
        rootTask = std::make_shared<WBSItem>(projectName);
        rootTask->id = L"1";
        rootTask->level = 0;
    }

    /**
     * @brief 名前指定コンストラクタ
     */
    WBSProject(const std::wstring& name) : WBSProject() {
        projectName = name;
        rootTask->taskName = name;
    }
};