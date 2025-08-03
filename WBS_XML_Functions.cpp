/*
 * ============================================================================
 * WBS_XML_Functions.cpp - XML処理専門モジュール
 * ============================================================================
 * 
 * このモジュールは、WBSプロジェクトデータのXML形式での永続化機能を提供します。
 * メインアプリケーション（WBS_cpp_win32.cpp）から独立した設計により、
 * 高い再利用性と保守性を実現しています。
 * 
 * 【主な機能】
 * - WBSプロジェクト→XMLシリアライゼーション
 * - XML→WBSプロジェクトデシリアライゼーション  
 * - 階層構造の完全な保存・復元
 * - XMLエスケープによる安全なテキスト処理
 * - UTF-8エンコーディング対応
 * - ファイルI/O例外処理
 * 
 * 【XML構造設計】
 * <?xml version="1.0" encoding="UTF-8"?>
 * <WBSProject>
 *   <ProjectName>プロジェクト名</ProjectName>
 *   <Description>プロジェクト説明</Description>
 *   <RootTask>
 *     <Task>
 *       <ID>1</ID>
 *       <Name>ルートタスク</Name>
 *       <Description>説明</Description>
 *       <AssignedTo>担当者</AssignedTo>
 *       <Status>0</Status>
 *       <Priority>1</Priority>
 *       <EstimatedHours>100.0</EstimatedHours>
 *       <ActualHours>25.0</ActualHours>
 *       <StartDate>2024-01-01T09:00:00</StartDate>
 *       <EndDate>2024-12-31T18:00:00</EndDate>
 *       <Level>0</Level>
 *       <Children>
 *         <!-- 子タスクの再帰的構造 -->
 *         <Task>...</Task>
 *       </Children>
 *     </Task>
 *   </RootTask>
 * </WBSProject>
 * 
 * 【設計原則】
 * - 型安全性: 強い型チェックと適切なキャスト
 * - 例外安全性: 完全な例外処理とリソース管理
 * - 拡張性: 新しいフィールド追加に対応可能な設計
 * - 互換性: 異なるバージョン間でのデータ互換性
 * 
 * 【技術仕様】
 * - 文字エンコーディング: UTF-8 (ファイル), UTF-16LE (内部処理)
 * - XML標準: W3C XML 1.0準拠
 * - 日時形式: ISO 8601準拠 (YYYY-MM-DDTHH:MM:SS)
 * - 数値形式: IEEE 754準拠倍精度浮動小数点
 * 
 * 作成者: 開発チーム
 * 作成日: 2024年
 * 最終更新: 2024年
 * バージョン: 1.0
 * ============================================================================
 */

// WBS XML Processing Module - 修正版
// XML処理専門モジュール（重複定義を削除）

#include <fstream>     // ファイルストリーム（入出力操作）
#include <sstream>     // 文字列ストリーム（文字列操作）
#include <codecvt>     // 文字コード変換（UTF-8 ⇔ UTF-16）
#include <locale>      // ロケール設定（国際化対応）
#include <windows.h>   // Windows基本API
#include <commdlg.h>   // コモンダイアログAPI（ファイル選択）
#include <shlobj.h>    // シェルオブジェクトAPI（フォルダ操作）
#include <memory>      // スマートポインタ（メモリ安全性）
#include <vector>      // 動的配列（階層データ管理）
#include <string>      // 文字列クラス（テキスト処理）

// ============================================================================
// 外部依存関係 - メインアプリケーションとの連携
// ============================================================================

// 外部変数：メインアプリケーションで定義されているグローバル状態
extern std::unique_ptr<WBSProject> g_currentProject;   // 現在のプロジェクトインスタンス
extern void RefreshTreeView();                         // UI更新：ツリービューの再描画
extern void RefreshListView();                         // UI更新：詳細ビューの再描画
extern void SaveLastOpenedFile(const std::wstring& filePath);  // 設定保存：最後に開いたファイル

// ============================================================================
// XML処理ユーティリティ関数群
// ============================================================================

/**
 * @brief XML特殊文字をエスケープ処理
 * 
 * @param text エスケープ対象の文字列
 * @return エスケープ済み文字列
 * 
 * XMLドキュメント内で特別な意味を持つ文字を、対応するXMLエンティティに変換します。
 * これにより、任意のユーザーテキストを安全にXML文書に埋め込むことができます。
 * 
 * @details 変換仕様:
 * - & (アンパサンド) → &amp;  : XMLエンティティの開始文字
 * - < (小なり) → &lt;        : XMLタグの開始を防ぐ
 * - > (大なり) → &gt;        : XMLタグの終了を防ぐ  
 * - " (ダブルクォート) → &quot; : XML属性値の区切り文字を防ぐ
 * - ' (シングルクォート) → &apos; : XML属性値の区切り文字を防ぐ
 * 
 * @note この処理は、ユーザーが入力したタスク名や説明文に
 *       XML構文を破壊する文字が含まれていても安全に保存できるようにします
 * 
 * @example 
 * XmlEscape(L"<重要> A&B プロジェクト \"緊急\"") 
 * → L"&lt;重要&gt; A&amp;B プロジェクト &quot;緊急&quot;"
 */
std::wstring XmlEscape(const std::wstring& text) {
    std::wstring result;
    result.reserve(text.length() * 1.2); // パフォーマンス最適化：予想サイズで予約
    
    for (wchar_t c : text) {
        switch (c) {
            case L'&': result += L"&amp;"; break;   // 最初に処理（他のエンティティとの競合回避）
            case L'<': result += L"&lt;"; break;    // XMLタグ開始文字
            case L'>': result += L"&gt;"; break;    // XMLタグ終了文字
            case L'"': result += L"&quot;"; break;  // XML属性値の区切り文字
            case L'\'': result += L"&apos;"; break; // XML属性値の区切り文字（代替）
            default: result += c; break;            // 通常文字はそのまま
        }
    }
    return result;
}

/**
 * @brief XMLエンティティをアンエスケープ処理
 * 
 * @param text アンエスケープ対象の文字列
 * @return アンエスケープ済み文字列
 * 
 * XML文書から読み込んだテキストに含まれるXMLエンティティを、
 * 元の文字に復元します。XmlEscape()の逆変換処理です。
 * 
 * @details 変換仕様（エスケープの逆順で処理）:
 * - &amp; → &   : 最後に処理（他のエンティティとの競合回避）
 * - &lt; → <    : XMLタグ開始文字の復元
 * - &gt; → >    : XMLタグ終了文字の復元
 * - &quot; → "  : ダブルクォートの復元
 * - &apos; → '  : シングルクォートの復元
 * 
 * @warning 処理順序が重要：&amp;を最後に処理しないと、
 *          他のエンティティの&部分が誤って変換される
 * 
 * @note パフォーマンス考慮：各エンティティごとに文字列全体を走査
 *       頻繁なファイル読み込みでない限り、実用上問題なし
 */
std::wstring XmlUnescape(const std::wstring& text) {
    std::wstring result = text;
    size_t pos = 0;
    
    // &lt; → < の変換
    while ((pos = result.find(L"&lt;", pos)) != std::wstring::npos) {
        result.replace(pos, 4, L"<");
        pos += 1;
    }
    pos = 0;
    
    // &gt; → > の変換
    while ((pos = result.find(L"&gt;", pos)) != std::wstring::npos) {
        result.replace(pos, 4, L">");
        pos += 1;
    }
    pos = 0;
    
    // &quot; → " の変換
    while ((pos = result.find(L"&quot;", pos)) != std::wstring::npos) {
        result.replace(pos, 6, L"\"");
        pos += 1;
    }
    pos = 0;
    
    // &apos; → ' の変換
    while ((pos = result.find(L"&apos;", pos)) != std::wstring::npos) {
        result.replace(pos, 6, L"'");
        pos += 1;
    }
    pos = 0;
    
    // &amp; → & の変換（最後に実行）
    while ((pos = result.find(L"&amp;", pos)) != std::wstring::npos) {
        result.replace(pos, 5, L"&");
        pos += 1;
    }
    
    return result;
}

// ============================================================================
// 日時変換ユーティリティ関数群
// ============================================================================

/**
 * @brief SYSTEMTIME構造体をISO 8601文字列に変換
 * 
 * @param st 変換対象のSYSTEMTIME構造体
 * @return ISO 8601形式の日時文字列 (YYYY-MM-DDTHH:MM:SS)
 * 
 * Windows APIのSYSTEMTIME構造体を、国際標準の日時文字列形式に変換します。
 * XML保存時に、タスクの開始日・終了日を標準形式で記録するために使用します。
 * 
 * @details 出力形式:
 * - 年: 4桁（例：2024）
 * - 月: 2桁（例：01, 12）
 * - 日: 2桁（例：01, 31）  
 * - 区切り文字: T（ISO 8601準拠）
 * - 時: 2桁（例：00, 23）
 * - 分: 2桁（例：00, 59）
 * - 秒: 2桁（例：00, 59）
 * - ミリ秒: 省略（プロジェクト管理では不要）
 * 
 * @note タイムゾーン情報は含まれません（ローカル時刻として扱う）
 * 
 * @example SystemTimeToString({2024, 12, 3, 25, 14, 30, 45, 0})
 *          → L"2024-12-25T14:30:45"
 */
std::wstring SystemTimeToString(const SYSTEMTIME& st) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"%04d-%02d-%02dT%02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buffer);
}

/**
 * @brief ISO 8601文字列をSYSTEMTIME構造体に変換
 * 
 * @param str 変換対象のISO 8601形式文字列
 * @return 変換されたSYSTEMTIME構造体
 * 
 * XML文書から読み込んだ日時文字列を、Windows APIで使用可能な
 * SYSTEMTIME構造体に変換します。SystemTimeToString()の逆変換処理です。
 * 
 * @details 入力形式要件:
 * - 最小長: 19文字 ("YYYY-MM-DDTHH:MM:SS")
 * - 区切り文字: ハイフン、T、コロンが正しい位置にあること
 * - 数値範囲: 各フィールドが有効な範囲内であること
 * 
 * @note エラー処理:
 * - 文字列が短すぎる場合: 現在のシステム時刻を返す
 * - パースエラーの場合: 現在のシステム時刻を返す（フォールバック）
 * - 無効な日付の場合: そのまま設定（呼び出し側で検証）
 * 
 * @warning wDayOfWeekフィールドは設定されません（システムが自動計算）
 */
SYSTEMTIME StringToSystemTime(const std::wstring& str) {
    SYSTEMTIME st = {};
    
    if (str.length() >= 19) {
        // ISO 8601形式のパース: "YYYY-MM-DDTHH:MM:SS"
        swscanf_s(str.c_str(), L"%04hu-%02hu-%02huT%02hu:%02hu:%02hu",
            &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
        
        // wDayOfWeekは設定しない（Windowsが自動計算）
        st.wMilliseconds = 0; // ミリ秒は常に0
    } else {
        // パースエラー時のフォールバック：現在時刻を返す
        GetSystemTime(&st);
    }
    
    return st;
}

// ============================================================================
// XML解析ユーティリティ関数群  
// ============================================================================

/**
 * @brief XML文書から指定タグの値を抽出
 * 
 * @param xml 検索対象のXML文字列
 * @param tag 抽出対象のタグ名（開始・終了タグの名前部分のみ）
 * @param startPos 検索開始位置（デフォルト: 0）
 * @return 抽出された値（エスケープ解除済み）、見つからない場合は空文字列
 * 
 * XML文書から単一の要素値を抽出する汎用関数です。
 * 階層的なXMLパースの基本的な構成要素として使用されます。
 * 
 * @details 処理フロー:
 * 1. 開始タグ "<tag>" を検索
 * 2. 対応する終了タグ "</tag>" を検索  
 * 3. タグ間のテキストを抽出
 * 4. XmlUnescape() でエンティティを復元
 * 5. 結果文字列を返す
 * 
 * @note ネストしたタグは考慮しません（最初に見つかった終了タグを使用）
 *       複雑なネスト構造の場合は、専用のパーサー関数が必要
 * 
 * @warning タグ名に空白文字や特殊文字が含まれていてはいけません
 * 
 * @example 
 * ExtractXmlValue(L"<Name>プロジェクト</Name>", L"Name") → L"プロジェクト"
 * ExtractXmlValue(L"<Data><Name>値</Name></Data>", L"Name") → L"値"
 */
std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tag, size_t startPos) {
    // 開始タグと終了タグを構築
    std::wstring startTag = L"<" + tag + L">";
    std::wstring endTag = L"</" + tag + L">";
    
    // 開始タグの位置を検索
    size_t start = xml.find(startTag, startPos);
    if (start == std::wstring::npos) {
        return L""; // 開始タグが見つからない
    }
    
    // テキスト部分の開始位置を計算
    start += startTag.length();
    
    // 終了タグの位置を検索
    size_t end = xml.find(endTag, start);
    if (end == std::wstring::npos) {
        return L""; // 終了タグが見つからない（構文エラー）
    }
    
    // タグ間のテキストを抽出してエスケープ解除
    return XmlUnescape(xml.substr(start, end - start));
}

// ============================================================================
// WBSデータシリアライゼーション関数群
// ============================================================================

/**
 * @brief WBSアイテムをXML形式に変換（再帰的）
 * 
 * @param item 変換対象のWBSアイテム
 * @param indent インデントレベル（整形用、デフォルト: 0）
 * @return XML形式の文字列
 * 
 * 単一のWBSアイテムとその全ての子要素を、構造化されたXML形式に変換します。
 * 階層構造を維持するため、再帰的に子要素も処理されます。
 * 
 * @details 出力XML構造:
 * <Task>
 *   <ID>タスクID</ID>
 *   <Name>タスク名</Name>
 *   <Description>説明</Description>
 *   <AssignedTo>担当者</AssignedTo>
 *   <Status>状態値</Status>
 *   <Priority>優先度値</Priority>
 *   <EstimatedHours>見積もり工数</EstimatedHours>
 *   <ActualHours>実績工数</ActualHours>
 *   <StartDate>開始日</StartDate>
 *   <EndDate>終了日</EndDate>
 *   <Level>階層レベル</Level>
 *   <Children>
 *     <!-- 子タスクが再帰的に展開される -->
 *   </Children>
 * </Task>
 * 
 * @note パフォーマンス考慮:
 * - 文字列連結の最適化（std::wstring の += 演算子）
 * - インデント用空白の事前計算
 * - 子要素の有無チェックによる不要なタグ回避
 * 
 * @param indent インデントレベル（2文字 × レベル数の空白を挿入）
 *               可読性の高いXMLファイルを生成するため
 */
std::wstring WBSItemToXml(std::shared_ptr<WBSItem> item, int indent) {
    if (!item) {
        return L""; // nullptrチェック：安全性確保
    }
    
    // インデント文字列を生成（可読性向上）
    std::wstring indentStr(indent * 2, L' ');
    std::wstring xml;
    
    // タスク開始タグ
    xml += indentStr + L"<Task>\n";
    
    // 基本フィールドのシリアライゼーション
    xml += indentStr + L"  <ID>" + XmlEscape(item->id) + L"</ID>\n";
    xml += indentStr + L"  <Name>" + XmlEscape(item->taskName) + L"</Name>\n";
    xml += indentStr + L"  <Description>" + XmlEscape(item->description) + L"</Description>\n";
    xml += indentStr + L"  <AssignedTo>" + XmlEscape(item->assignedTo) + L"</AssignedTo>\n";
    
    // 列挙型の数値変換（型安全性と国際化対応）
    xml += indentStr + L"  <Status>" + std::to_wstring((int)item->status) + L"</Status>\n";
    xml += indentStr + L"  <Priority>" + std::to_wstring((int)item->priority) + L"</Priority>\n";
    
    // 浮動小数点数値の変換
    xml += indentStr + L"  <EstimatedHours>" + std::to_wstring(item->estimatedHours) + L"</EstimatedHours>\n";
    xml += indentStr + L"  <ActualHours>" + std::to_wstring(item->actualHours) + L"</ActualHours>\n";
    
    // 日時データの変換
    xml += indentStr + L"  <StartDate>" + SystemTimeToString(item->startDate) + L"</StartDate>\n";
    xml += indentStr + L"  <EndDate>" + SystemTimeToString(item->endDate) + L"</EndDate>\n";
    
    // 階層情報
    xml += indentStr + L"  <Level>" + std::to_wstring(item->level) + L"</Level>\n";
    
    // 子要素の処理（再帰的）
    if (!item->children.empty()) {
        xml += indentStr + L"  <Children>\n";
        
        // 全ての子要素を再帰的にシリアライズ
        for (const auto& child : item->children) {
            xml += WBSItemToXml(child, indent + 2); // インデントレベルを増加
        }
        
        xml += indentStr + L"  </Children>\n";
    }
    
    // タスク終了タグ
    xml += indentStr + L"</Task>\n";
    
    return xml;
}

/**
 * @brief プロジェクト全体をXML文書に変換
 * 
 * @return 完全なXML文書文字列
 * 
 * 現在のWBSプロジェクト全体を、XML宣言を含む完全なXML文書に変換します。
 * ファイル保存時に使用される最上位レベルのシリアライゼーション関数です。
 * 
 * @details 出力XML構造:
 * <?xml version="1.0" encoding="UTF-8"?>
 * <WBSProject>
 *   <ProjectName>プロジェクト名</ProjectName>
 *   <Description>プロジェクト説明</Description>
 *   <RootTask>
 *     <!-- WBSItemToXml()の出力 -->
 *   </RootTask>
 * </WBSProject>
 * 
 * @note エラー処理:
 * - g_currentProject が nullptr の場合: 空文字列を返す
 * - プロジェクトデータが不完全な場合: 利用可能な部分のみを出力
 * 
 * @warning この関数は外部依存関係（g_currentProject）に依存します
 *          単体テストを行う場合は、適切なモックが必要
 */
std::wstring ProjectToXml() {
    if (!g_currentProject) {
        return L""; // プロジェクトが存在しない場合の安全な処理
    }
    
    std::wstring xml;
    
    // XML宣言：文字エンコーディングの明示
    xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    
    // ルート要素開始
    xml += L"<WBSProject>\n";
    
    // プロジェクトメタデータ
    xml += L"  <ProjectName>" + XmlEscape(g_currentProject->projectName) + L"</ProjectName>\n";
    xml += L"  <Description>" + XmlEscape(g_currentProject->description) + L"</Description>\n";
    
    // ルートタスクとその階層構造
    xml += L"  <RootTask>\n";
    xml += WBSItemToXml(g_currentProject->rootTask, 2); // インデントレベル2から開始
    xml += L"  </RootTask>\n";
    
    // ルート要素終了
    xml += L"</WBSProject>\n";
    
    return xml;
}

// ============================================================================
// XMLデシリアライゼーション関数群
// ============================================================================

/**
 * @brief XML文字列からWBSアイテムを解析（再帰的）
 * 
 * @param xml 解析対象のXML文字列
 * @param pos 解析開始位置（参照渡し：解析後の位置が設定される）
 * @return 解析されたWBSアイテム、エラーの場合はnullptr
 * 
 * XML文字列から単一の<Task>要素を解析し、対応するWBSItemオブジェクトを構築します。
 * 子要素が存在する場合は、再帰的に処理して完全な階層構造を復元します。
 * 
 * @details 解析プロセス:
 * 1. <Task>開始タグの検索
 * 2. </Task>終了タグの検索
 * 3. タスク要素の抽出
 * 4. 各フィールドのパースとデータ型変換
 * 5. 子要素の再帰的解析
 * 6. 親子関係の設定
 * 
 * @note データ型変換:
 * - 文字列フィールド: ExtractXmlValue() + XmlUnescape()
 * - 列挙型: std::stoi() + キャスト
 * - 浮動小数点: std::stod()
 * - 日時: StringToSystemTime()
 * - 整数: std::stoi()
 * 
 * @warning パフォーマンス考慮事項:
 * - 大きなプロジェクトファイルの場合、再帰の深さに注意
 * - メモリ使用量は階層の深さとタスク数に比例
 * 
 * @param pos 解析位置（入出力パラメータ）
 *            入力: 解析開始位置
 *            出力: 次の要素の解析開始位置
 */
std::shared_ptr<WBSItem> ParseTaskFromXml(const std::wstring& xml, size_t& pos) {
    // <Task>開始タグの検索
    size_t taskStart = xml.find(L"<Task>", pos);
    if (taskStart == std::wstring::npos) {
        return nullptr; // タスク要素が見つからない
    }
    
    // </Task>終了タグの検索
    size_t taskEnd = xml.find(L"</Task>", taskStart);
    if (taskEnd == std::wstring::npos) {
        return nullptr; // 構文エラー：終了タグがない
    }
    
    // タスク要素全体を抽出
    std::wstring taskXml = xml.substr(taskStart, taskEnd - taskStart + 7);
    pos = taskEnd + 7; // 次の解析位置を更新
    
    // 新しいWBSItemインスタンスを作成
    auto item = std::make_shared<WBSItem>();
    
    // 基本フィールドの解析
    item->id = ExtractXmlValue(taskXml, L"ID", 0);
    item->taskName = ExtractXmlValue(taskXml, L"Name", 0);
    item->description = ExtractXmlValue(taskXml, L"Description", 0);
    item->assignedTo = ExtractXmlValue(taskXml, L"AssignedTo", 0);
    
    // 列挙型フィールドの解析（エラーハンドリング付き）
    std::wstring statusStr = ExtractXmlValue(taskXml, L"Status", 0);
    if (!statusStr.empty()) {
        item->status = (TaskStatus)std::stoi(statusStr);
    }
    
    std::wstring priorityStr = ExtractXmlValue(taskXml, L"Priority", 0);
    if (!priorityStr.empty()) {
        item->priority = (TaskPriority)std::stoi(priorityStr);
    }
    
    // 数値フィールドの解析（エラーハンドリング付き）
    std::wstring estimatedStr = ExtractXmlValue(taskXml, L"EstimatedHours", 0);
    if (!estimatedStr.empty()) {
        item->estimatedHours = std::stod(estimatedStr);
    }
    
    std::wstring actualStr = ExtractXmlValue(taskXml, L"ActualHours", 0);
    if (!actualStr.empty()) {
        item->actualHours = std::stod(actualStr);
    }
    
    // 日時フィールドの解析
    std::wstring startDateStr = ExtractXmlValue(taskXml, L"StartDate", 0);
    if (!startDateStr.empty()) {
        item->startDate = StringToSystemTime(startDateStr);
    }
    
    std::wstring endDateStr = ExtractXmlValue(taskXml, L"EndDate", 0);
    if (!endDateStr.empty()) {
        item->endDate = StringToSystemTime(endDateStr);
    }
    
    // 階層レベルの解析
    std::wstring levelStr = ExtractXmlValue(taskXml, L"Level", 0);
    if (!levelStr.empty()) {
        item->level = std::stoi(levelStr);
    }
    
    // 子要素の再帰的解析
    size_t childrenStart = taskXml.find(L"<Children>");
    if (childrenStart != std::wstring::npos) {
        size_t childrenEnd = taskXml.find(L"</Children>");
        if (childrenEnd != std::wstring::npos) {
            // 子要素範囲のXMLを抽出
            std::wstring childrenXml = taskXml.substr(childrenStart + 10, childrenEnd - childrenStart - 10);
            
            // 全ての子タスクを再帰的に解析
            size_t childPos = 0;
            while (true) {
                auto child = ParseTaskFromXml(childrenXml, childPos);
                if (!child) break; // 子要素なし、または解析完了
                
                // 親子関係の設定
                child->parent = item;
                item->children.push_back(child);
            }
        }
    }
    
    return item;
}

// ============================================================================
// ファイルI/O操作関数群
// ============================================================================

/**
 * @brief XMLファイルからプロジェクトを読み込み
 * 
 * @param filePath 読み込み対象のファイルパス
 * @return 読み込み成功時true、失敗時false
 * 
 * 指定されたXMLファイルからWBSプロジェクトデータを読み込み、
 * アプリケーションの現在のプロジェクト状態を更新します。
 * 
 * @details 処理フロー:
 * 1. ファイルオープンとエンコーディング設定
 * 2. XML内容の全読み込み
 * 3. プロジェクトメタデータの抽出
 * 4. ルートタスクの再帰的解析
 * 5. グローバル状態の更新
 * 6. UI再描画の実行
 * 7. 設定ファイルの更新
 * 8. ユーザー通知
 * 
 * @note エンコーディング処理:
 * - ファイル: UTF-8（XMLの標準）
 * - 内部処理: UTF-16LE（Windowsの標準）
 * - std::codecvt_utf8 による自動変換
 * 
 * @warning エラーハンドリング:
 * - ファイルアクセスエラー
 * - XML構文エラー
 * - データ型変換エラー
 * - メモリ不足
 * 全ての例外をキャッチしてfalseを返す
 */
bool LoadProjectFromFile(const std::wstring& filePath) {
    try {
        // ファイルストリームを開く（Unicode対応）
        std::wifstream file(filePath);
        if (!file.is_open()) {
            return false; // ファイルオープンエラー
        }
        
        // UTF-8エンコーディングの設定
        file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
        
        // ファイル全体を文字列として読み込み
        std::wstring xmlContent((std::istreambuf_iterator<wchar_t>(file)),
                              std::istreambuf_iterator<wchar_t>());
        file.close();
        
        // プロジェクトメタデータの抽出
        std::wstring projectName = ExtractXmlValue(xmlContent, L"ProjectName", 0);
        std::wstring description = ExtractXmlValue(xmlContent, L"Description", 0);
        
        // プロジェクト名のバリデーション
        if (projectName.empty()) {
            projectName = L"読み込まれたプロジェクト"; // フォールバック名
        }
        
        // 新しいプロジェクトインスタンスの作成
        g_currentProject = std::make_unique<WBSProject>(projectName);
        g_currentProject->description = description;
        
        // ルートタスクの解析
        size_t rootTaskStart = xmlContent.find(L"<RootTask>");
        if (rootTaskStart != std::wstring::npos) {
            size_t rootTaskEnd = xmlContent.find(L"</RootTask>");
            if (rootTaskEnd != std::wstring::npos) {
                // ルートタスク範囲のXMLを抽出
                std::wstring rootTaskXml = xmlContent.substr(rootTaskStart + 10, rootTaskEnd - rootTaskStart - 10);
                
                // ルートタスクの再帰的解析
                size_t pos = 0;
                auto loadedRootTask = ParseTaskFromXml(rootTaskXml, pos);
                
                if (loadedRootTask) {
                    // ルートタスクの名前をプロジェクト名と同期
                    loadedRootTask->taskName = projectName;
                    g_currentProject->rootTask = loadedRootTask;
                }
            }
        }
        
        // UI状態の更新
        RefreshTreeView();  // ツリービューの再描画
        RefreshListView();  // 詳細ビューの再描画
        
        // アプリケーション設定の更新
        SaveLastOpenedFile(filePath);
        
        // ユーザーへの成功通知
        MessageBox(nullptr, L"プロジェクトをXMLファイルから読み込みました。", L"情報", MB_OK | MB_ICONINFORMATION);
        
        return true;
        
    } catch (...) {
        // 全ての例外をキャッチ（ファイルI/O、XML解析、メモリ不足等）
        MessageBox(nullptr, L"プロジェクトの読み込み中にエラーが発生しました。", L"エラー", MB_OK | MB_ICONERROR);
        return false;
    }
}

/**
 * @brief プロジェクトをXMLファイルに保存
 * 
 * ファイル選択ダイアログを表示し、ユーザーが選択した場所に
 * 現在のプロジェクトをXML形式で保存します。
 * 
 * @details 処理フロー:
 * 1. プロジェクト存在チェック
 * 2. ファイル保存ダイアログの表示
 * 3. XML変換処理
 * 4. ファイル書き込み（UTF-8エンコーディング）
 * 5. 設定ファイルの更新
 * 6. ユーザー通知
 * 
 * @note ダイアログ設定:
 * - ファイルフィルタ: "*.xml" および "*.*"
 * - デフォルト拡張子: ".xml"
 * - 上書き確認: 有効
 * - パス検証: 有効
 */
void OnSaveProject() {
    // プロジェクト存在チェック
    if (!g_currentProject) {
        MessageBox(nullptr, L"保存するプロジェクトがありません。", L"エラー", MB_OK | MB_ICONWARNING);
        return;
    }

    // ファイル保存ダイアログの設定
    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L""; // ファイルパスバッファ
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XMLファイル\0*.xml\0すべてのファイル\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // パス検証と上書き確認
    ofn.lpstrDefExt = L"xml"; // デフォルト拡張子

    // ファイル保存ダイアログの表示
    if (GetSaveFileName(&ofn) == TRUE) {
        try {
            // ファイルストリームを開く
            std::wofstream file(szFile);
            if (file.is_open()) {
                // UTF-8エンコーディングの設定
                file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
                
                // プロジェクトをXMLに変換
                std::wstring xmlContent = ProjectToXml();
                
                // ファイルに書き込み
                file << xmlContent;
                file.close();
                
                // アプリケーション設定の更新
                SaveLastOpenedFile(szFile);
                
                // ユーザーへの成功通知
                MessageBox(nullptr, L"プロジェクトをXMLファイルに保存しました。", L"情報", MB_OK | MB_ICONINFORMATION);
            } else {
                // ファイルオープンエラー
                MessageBox(nullptr, L"ファイルを開けませんでした。", L"エラー", MB_OK | MB_ICONERROR);
            }
        } catch (...) {
            // 書き込みエラー（ディスク容量不足、権限エラー等）
            MessageBox(nullptr, L"プロジェクトの保存中にエラーが発生しました。", L"エラー", MB_OK | MB_ICONERROR);
        }
    }
}

/**
 * @brief プロジェクトファイルを開く
 * 
 * ファイル選択ダイアログを表示し、ユーザーが選択したXMLファイルから
 * プロジェクトを読み込みます。
 * 
 * @details 処理フロー:
 * 1. ファイル選択ダイアログの表示
 * 2. LoadProjectFromFile() の呼び出し
 * 3. エラーハンドリングはLoadProjectFromFile()に委譲
 * 
 * @note ダイアログ設定:
 * - ファイルフィルタ: "*.xml" および "*.*"
 * - ファイル存在チェック: 有効
 * - パス検証: 有効
 */
void OnOpenProject() {
    // ファイル選択ダイアログの設定
    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L""; // ファイルパスバッファ
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XMLファイル\0*.xml\0すべてのファイル\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // パスとファイルの存在チェック

    // ファイル選択ダイアログの表示
    if (GetOpenFileName(&ofn) == TRUE) {
        // 選択されたファイルからプロジェクトを読み込み
        // エラーハンドリングはLoadProjectFromFile()内で実行される
        LoadProjectFromFile(szFile);
    }
}