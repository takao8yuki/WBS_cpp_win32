// WBS XML Processing Module - 修正版
// XML処理専門モジュール（重複定義を削除）

#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <memory>
#include <vector>
#include <string>

// 外部変数・関数の宣言（WBS_cpp_win32.cppで定義）
extern std::unique_ptr<WBSProject> g_currentProject;
extern void RefreshTreeView();
extern void RefreshListView();
extern void SaveLastOpenedFile(const std::wstring& filePath);

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

// XMLから値を抽出するヘルパー関数
std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tag, size_t startPos) {
    std::wstring startTag = L"<" + tag + L">";
    std::wstring endTag = L"</" + tag + L">";
    
    size_t start = xml.find(startTag, startPos);
    if (start == std::wstring::npos) return L"";
    
    start += startTag.length();
    size_t end = xml.find(endTag, start);
    if (end == std::wstring::npos) return L"";
    
    return XmlUnescape(xml.substr(start, end - start));
}

// WBSアイテムをXMLに変換
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
    item->id = ExtractXmlValue(taskXml, L"ID", 0);
    item->taskName = ExtractXmlValue(taskXml, L"Name", 0);
    item->description = ExtractXmlValue(taskXml, L"Description", 0);
    item->assignedTo = ExtractXmlValue(taskXml, L"AssignedTo", 0);
    
    std::wstring statusStr = ExtractXmlValue(taskXml, L"Status", 0);
    if (!statusStr.empty()) {
        item->status = (TaskStatus)std::stoi(statusStr);
    }
    
    std::wstring priorityStr = ExtractXmlValue(taskXml, L"Priority", 0);
    if (!priorityStr.empty()) {
        item->priority = (TaskPriority)std::stoi(priorityStr);
    }
    
    std::wstring estimatedStr = ExtractXmlValue(taskXml, L"EstimatedHours", 0);
    if (!estimatedStr.empty()) {
        item->estimatedHours = std::stod(estimatedStr);
    }
    
    std::wstring actualStr = ExtractXmlValue(taskXml, L"ActualHours", 0);
    if (!actualStr.empty()) {
        item->actualHours = std::stod(actualStr);
    }
    
    std::wstring startDateStr = ExtractXmlValue(taskXml, L"StartDate", 0);
    if (!startDateStr.empty()) {
        item->startDate = StringToSystemTime(startDateStr);
    }
    
    std::wstring endDateStr = ExtractXmlValue(taskXml, L"EndDate", 0);
    if (!endDateStr.empty()) {
        item->endDate = StringToSystemTime(endDateStr);
    }
    
    std::wstring levelStr = ExtractXmlValue(taskXml, L"Level", 0);
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
        g_currentProject = std::make_unique<WBSProject>(projectName);
        g_currentProject->description = description;
        
        // ルートタスクを解析
        size_t rootTaskStart = xmlContent.find(L"<RootTask>");
        if (rootTaskStart != std::wstring::npos) {
            size_t rootTaskEnd = xmlContent.find(L"</RootTask>");
            if (rootTaskEnd != std::wstring::npos) {
                std::wstring rootTaskXml = xmlContent.substr(rootTaskStart + 10, rootTaskEnd - rootTaskStart - 10);
                size_t pos = 0;
                auto loadedRootTask = ParseTaskFromXml(rootTaskXml, pos);
                
                if (loadedRootTask) {
                    // ルートタスクの情報を設定
                    loadedRootTask->taskName = projectName;
                    g_currentProject->rootTask = loadedRootTask;
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