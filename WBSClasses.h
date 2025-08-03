#pragma once

/*
 * WBSClasses.h - Work Breakdown Structure（作業分解構造）管理システム
 * 
 * このヘッダーファイルは、プロジェクト管理に必要なWBS機能を提供します。
 * WBSItem（作業項目）とWBSProject（プロジェクト）クラスを定義し、
 * タスクの階層管理、進捗追跡、工数管理などの機能を実装しています。
 * 
 * 主な機能:
 * - 階層構造でのタスク管理
 * - タスクの進捗状況追跡
 * - 工数（見積もり・実績）管理
 * - プロジェクト全体の統計情報提供
 * - XML形式でのデータ永続化対応
 * 
 * 作成者: [開発者名]
 * 作成日: [日付]
 * 最終更新: [日付]
 * バージョン: 1.0
 */

#include <vector>
#include <string>
#include <memory>
#include <windows.h>
#include <commctrl.h>

// =============================================================================
// 列挙型定義
// =============================================================================

/**
 * @brief WBSタスクの進行状態を表す列挙型
 * 
 * プロジェクト内のタスクが現在どの段階にあるかを示します。
 * この状態は進捗管理や統計情報の計算に使用されます。
 */
enum class TaskStatus {
    NOT_STARTED,    ///< 未開始 - タスクがまだ開始されていない状態
    IN_PROGRESS,    ///< 進行中 - タスクが現在実行されている状態
    COMPLETED,      ///< 完了 - タスクが正常に完了した状態
    ON_HOLD,        ///< 保留 - 一時的に作業が停止されている状態
    CANCELLED       ///< キャンセル - タスクが中止された状態
};

/**
 * @brief WBSタスクの重要度・緊急度を表す列挙型
 * 
 * タスクの優先順位を決定するために使用されます。
 * リソース配分や作業順序の決定に活用されます。
 */
enum class TaskPriority {
    LOW,            ///< 低優先度 - 時間に余裕がある場合に実行
    MEDIUM,         ///< 中優先度 - 標準的な優先度レベル
    HIGH,           ///< 高優先度 - 重要で早めの完了が必要
    URGENT          ///< 緊急 - 最優先で即座に対応が必要
};

// =============================================================================
// クラス定義
// =============================================================================

/**
 * @brief WBS（Work Breakdown Structure）の個別作業項目を表すクラス
 * 
 * プロジェクトを構成する個々のタスクやサブタスクを表現します。
 * 階層構造を持ち、親子関係によってプロジェクトの作業分解を表現できます。
 * 
 * 主な機能:
 * - タスクの基本情報管理（名前、説明、担当者など）
 * - 進捗状況と工数の追跡
 * - 親子関係による階層構造の管理
 * - 統計情報の計算（進捗率、総工数など）
 * 
 * 使用例:
 * @code
 * auto task = std::make_shared<WBSItem>("システム設計");
 * task->SetDescription("システムの基本設計を行う");
 * task->SetEstimatedHours(40.0);
 * task->SetStatus(TaskStatus::IN_PROGRESS);
 * @endcode
 */
class WBSItem {
public:
    // =========================================================================
    // コンストラクタ・デストラクタ
    // =========================================================================
    
    /**
     * @brief デフォルトコンストラクタ
     * 
     * 新しいWBSアイテムを作成します。
     * デフォルト値で初期化され、後から各プロパティを設定できます。
     */
    WBSItem();
    
    /**
     * @brief 名前指定コンストラクタ
     * 
     * @param name タスク名
     * 
     * 指定された名前でWBSアイテムを作成します。
     * その他のプロパティはデフォルト値で初期化されます。
     */
    WBSItem(const std::wstring& name);
    
    /**
     * @brief デストラクタ
     * 
     * WBSアイテムのリソースを適切に解放します。
     */
    ~WBSItem();

    // =========================================================================
    // アクセサーメソッド群
    // =========================================================================
    
    /**
     * @brief タスク名を取得
     * @return タスク名の参照
     */
    const std::wstring& GetTaskName() const { return taskName; }
    
    /**
     * @brief タスク名を設定
     * @param name 新しいタスク名
     */
    void SetTaskName(const std::wstring& name) { taskName = name; }
    
    /**
     * @brief タスクの詳細説明を取得
     * @return 説明文の参照
     */
    const std::wstring& GetDescription() const { return description; }
    
    /**
     * @brief タスクの詳細説明を設定
     * @param desc 説明文
     */
    void SetDescription(const std::wstring& desc) { description = desc; }
    
    /**
     * @brief 担当者名を取得
     * @return 担当者名の参照
     */
    const std::wstring& GetAssignedTo() const { return assignedTo; }
    
    /**
     * @brief 担当者を設定
     * @param assigned 担当者名
     */
    void SetAssignedTo(const std::wstring& assigned) { assignedTo = assigned; }
    
    /**
     * @brief 現在のタスク状態を取得
     * @return TaskStatus値
     */
    TaskStatus GetStatus() const { return status; }
    
    /**
     * @brief タスク状態を設定
     * @param stat 新しい状態
     */
    void SetStatus(TaskStatus stat) { status = stat; }
    
    /**
     * @brief タスクの優先度を取得
     * @return TaskPriority値
     */
    TaskPriority GetPriority() const { return priority; }
    
    /**
     * @brief タスクの優先度を設定
     * @param prio 新しい優先度
     */
    void SetPriority(TaskPriority prio) { priority = prio; }
    
    /**
     * @brief 見積もり工数を取得（時間単位）
     * @return 見積もり工数
     */
    double GetEstimatedHours() const { return estimatedHours; }
    
    /**
     * @brief 見積もり工数を設定
     * @param hours 見積もり工数（時間）
     */
    void SetEstimatedHours(double hours) { estimatedHours = hours; }
    
    /**
     * @brief 実績工数を取得（時間単位）
     * @return 実績工数
     */
    double GetActualHours() const { return actualHours; }
    
    /**
     * @brief 実績工数を設定
     * @param hours 実績工数（時間）
     */
    void SetActualHours(double hours) { actualHours = hours; }
    
    /**
     * @brief タスク開始予定日を取得
     * @return 開始日のSYSTEMTIME構造体の参照
     */
    const SYSTEMTIME& GetStartDate() const { return startDate; }
    
    /**
     * @brief タスク開始予定日を設定
     * @param date 開始日
     */
    void SetStartDate(const SYSTEMTIME& date) { startDate = date; }
    
    /**
     * @brief タスク終了予定日を取得
     * @return 終了日のSYSTEMTIME構造体の参照
     */
    const SYSTEMTIME& GetEndDate() const { return endDate; }
    
    /**
     * @brief タスク終了予定日を設定
     * @param date 終了日
     */
    void SetEndDate(const SYSTEMTIME& date) { endDate = date; }
    
    /**
     * @brief タスクIDを取得
     * @return タスクID文字列の参照
     */
    const std::wstring& GetId() const { return id; }
    
    /**
     * @brief タスクIDを設定
     * @param newId 新しいタスクID
     */
    void SetId(const std::wstring& newId) { id = newId; }
    
    /**
     * @brief 階層レベルを取得
     * @return 階層レベル（0がルート）
     */
    int GetLevel() const { return level; }
    
    /**
     * @brief 階層レベルを設定
     * @param lev 新しい階層レベル
     */
    void SetLevel(int lev) { level = lev; }

    // =========================================================================
    // 階層管理メソッド
    // =========================================================================
    
    /**
     * @brief 子タスクを追加
     * 
     * @param child 追加する子タスク
     * 
     * 指定されたタスクを子タスクとして追加します。
     * 子タスクの親参照と階層レベルは自動的に設定されます。
     */
    void AddChild(std::shared_ptr<WBSItem> child);
    
    /**
     * @brief 子タスクを削除
     * 
     * @param child 削除する子タスク
     * 
     * 指定された子タスクをリストから削除します。
     */
    void RemoveChild(std::shared_ptr<WBSItem> child);
    
    /**
     * @brief 全ての子タスクを取得
     * @return 子タスクのベクターの参照
     */
    const std::vector<std::shared_ptr<WBSItem>>& GetChildren() const { return children; }
    
    /**
     * @brief 親タスクを取得
     * @return 親タスクのshared_ptr（ルートの場合はnullptr）
     */
    std::shared_ptr<WBSItem> GetParent() const { return parent.lock(); }
    
    /**
     * @brief 親タスクを設定
     * @param par 新しい親タスク
     */
    void SetParent(std::shared_ptr<WBSItem> par) { parent = par; }

    // =========================================================================
    // ユーティリティメソッド
    // =========================================================================
    
    /**
     * @brief 進捗率を計算
     * @return 進捗率（0.0?100.0）
     * 
     * 実績工数を見積もり工数で割って進捗率を計算します。
     * 見積もり工数が0の場合は0.0を返します。
     */
    double GetProgressPercentage() const;
    
    /**
     * @brief このタスクとその子タスクの総見積もり工数を計算
     * @return 総見積もり工数
     */
    double GetTotalEstimatedHours() const;
    
    /**
     * @brief このタスクとその子タスクの総実績工数を計算
     * @return 総実績工数
     */
    double GetTotalActualHours() const;
    
    /**
     * @brief ステータスを文字列形式で取得
     * @return ステータスの日本語表記
     */
    std::wstring GetStatusString() const;
    
    /**
     * @brief 優先度を文字列形式で取得
     * @return 優先度の日本語表記
     */
    std::wstring GetPriorityString() const;
    
    /**
     * @brief 自動的にIDを生成
     * 
     * 親タスクのIDと子タスクの順序を基にして、
     * 一意のIDを自動生成します。
     */
    void GenerateId();

private:
    // =========================================================================
    // プライベートメンバ変数
    // =========================================================================
    
    std::wstring id;                                        ///< タスクの一意識別子
    std::wstring taskName;                                  ///< タスク名
    std::wstring description;                               ///< タスクの詳細説明
    std::wstring assignedTo;                                ///< 担当者名
    TaskStatus status;                                      ///< 現在の進行状態
    TaskPriority priority;                                  ///< 優先度レベル
    double estimatedHours;                                  ///< 見積もり工数（時間）
    double actualHours;                                     ///< 実績工数（時間）
    SYSTEMTIME startDate;                                   ///< 開始予定日
    SYSTEMTIME endDate;                                     ///< 終了予定日
    int level;                                              ///< 階層レベル（0=ルート）
    
    std::vector<std::shared_ptr<WBSItem>> children;         ///< 子タスクのリスト
    std::weak_ptr<WBSItem> parent;                          ///< 親タスクへの弱参照
};

/**
 * @brief WBSプロジェクト全体を管理するクラス
 * 
 * プロジェクト全体の情報を管理し、ルートタスクを通じて
 * 全てのタスクにアクセスできます。プロジェクトレベルでの
 * 統計情報の計算や、ファイルへの保存・読み込み機能を提供します。
 * 
 * 主な機能:
 * - プロジェクト基本情報の管理
 * - ルートタスクを通じた全タスクへのアクセス
 * - プロジェクト全体の統計情報提供
 * - XMLファイルでの保存・読み込み
 * 
 * 使用例:
 * @code
 * auto project = std::make_unique<WBSProject>("新システム開発");
 * project->SetDescription("顧客管理システムの開発プロジェクト");
 * project->SaveToFile(L"project.xml");
 * @endcode
 */
class WBSProject {
public:
    // =========================================================================
    // コンストラクタ・デストラクタ
    // =========================================================================
    
    /**
     * @brief デフォルトコンストラクタ
     * 
     * 新しいWBSプロジェクトを作成します。
     * デフォルトのプロジェクト名でルートタスクが自動作成されます。
     */
    WBSProject();
    
    /**
     * @brief 名前指定コンストラクタ
     * 
     * @param name プロジェクト名
     * 
     * 指定された名前でWBSプロジェクトを作成します。
     * プロジェクト名と同じ名前でルートタスクが作成されます。
     */
    WBSProject(const std::wstring& name);
    
    /**
     * @brief デストラクタ
     * 
     * プロジェクトのリソースを適切に解放します。
     */
    ~WBSProject();

    // =========================================================================
    // アクセサーメソッド群
    // =========================================================================
    
    /**
     * @brief プロジェクト名を取得
     * @return プロジェクト名の参照
     */
    const std::wstring& GetProjectName() const { return projectName; }
    
    /**
     * @brief プロジェクト名を設定
     * @param name 新しいプロジェクト名
     */
    void SetProjectName(const std::wstring& name) { projectName = name; }
    
    /**
     * @brief プロジェクトの説明を取得
     * @return 説明文の参照
     */
    const std::wstring& GetDescription() const { return description; }
    
    /**
     * @brief プロジェクトの説明を設定
     * @param desc 説明文
     */
    void SetDescription(const std::wstring& desc) { description = desc; }
    
    /**
     * @brief ルートタスクを取得
     * @return ルートタスクのshared_ptr
     */
    std::shared_ptr<WBSItem> GetRootTask() const { return rootTask; }

    // =========================================================================
    // プロジェクト管理メソッド
    // =========================================================================
    
    /**
     * @brief ルートレベルにタスクを追加
     * 
     * @param task 追加するタスク
     * 
     * プロジェクトのルートレベルに新しいタスクを追加します。
     * 通常はプロジェクトの主要なフェーズやカテゴリに使用します。
     */
    void AddRootTask(std::shared_ptr<WBSItem> task);
    
    /**
     * @brief プロジェクトをファイルに保存
     * 
     * @param filename 保存先ファイル名
     * @return 保存に成功した場合true
     * 
     * プロジェクトの全データをXML形式でファイルに保存します。
     */
    bool SaveToFile(const std::wstring& filename);
    
    /**
     * @brief ファイルからプロジェクトを読み込み
     * 
     * @param filename 読み込み元ファイル名
     * @return 読み込みに成功した場合true
     * 
     * XML形式で保存されたプロジェクトファイルを読み込みます。
     */
    bool LoadFromFile(const std::wstring& filename);
    
    // =========================================================================
    // 統計情報メソッド
    // =========================================================================
    
    /**
     * @brief プロジェクト全体の総見積もり工数を計算
     * @return 総見積もり工数（時間）
     * 
     * プロジェクト内の全タスクの見積もり工数を合計します。
     */
    double GetTotalEstimatedHours() const;
    
    /**
     * @brief プロジェクト全体の総実績工数を計算
     * @return 総実績工数（時間）
     * 
     * プロジェクト内の全タスクの実績工数を合計します。
     */
    double GetTotalActualHours() const;
    
    /**
     * @brief プロジェクト全体の進捗率を計算
     * @return 全体進捗率（0.0?100.0）
     * 
     * 全タスクの実績工数と見積もり工数から
     * プロジェクト全体の進捗率を計算します。
     */
    double GetOverallProgress() const;

private:
    // =========================================================================
    // プライベートメンバ変数
    // =========================================================================
    
    std::wstring projectName;                               ///< プロジェクト名
    std::wstring description;                               ///< プロジェクトの説明
    std::shared_ptr<WBSItem> rootTask;                      ///< ルートタスク
    std::wstring filename;                                  ///< 保存ファイル名
};

// =============================================================================
// ユーティリティ関数群
// =============================================================================

/**
 * @brief TaskStatusを日本語文字列に変換
 * @param status 変換するステータス
 * @return 日本語のステータス文字列
 */
std::wstring TaskStatusToString(TaskStatus status);

/**
 * @brief 文字列をTaskStatusに変換
 * @param str 変換する文字列
 * @return 対応するTaskStatus値
 */
TaskStatus StringToTaskStatus(const std::wstring& str);

/**
 * @brief TaskPriorityを日本語文字列に変換
 * @param priority 変換する優先度
 * @return 日本語の優先度文字列
 */
std::wstring TaskPriorityToString(TaskPriority priority);

/**
 * @brief 文字列をTaskPriorityに変換
 * @param str 変換する文字列
 * @return 対応するTaskPriority値
 */
TaskPriority StringToTaskPriority(const std::wstring& str);

/**
 * @brief SYSTEMTIMEを文字列形式に変換
 * @param st 変換するSYSTEMTIME
 * @param str 結果を格納する文字列
 */
void SystemTimeToString(const SYSTEMTIME& st, std::wstring& str);

/**
 * @brief 文字列をSYSTEMTIMEに変換
 * @param str 変換する文字列
 * @param st 結果を格納するSYSTEMTIME
 * @return 変換に成功した場合true
 */
bool StringToSystemTime(const std::wstring& str, SYSTEMTIME& st);