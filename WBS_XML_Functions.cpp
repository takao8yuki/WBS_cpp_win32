// WBS XML �t�@�C������@�\
// WBS_cpp_win32.cpp �ɒǉ�����@�\

#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <windows.h>
#include <commdlg.h>

// �O���錾
class WBSItem;
class WBSProject;
extern std::unique_ptr<WBSProject> g_currentProject;
extern void RefreshTreeView();
extern void RefreshListView();

// TaskStatus �� TaskPriority �̒�`
enum class TaskStatus {
    NOT_STARTED = 0,    // ���J�n
    IN_PROGRESS = 1,    // �i�s��
    COMPLETED = 2,      // ����
    ON_HOLD = 3,        // �ۗ�
    CANCELLED = 4       // �L�����Z��
};

enum class TaskPriority {
    LOW = 0,            // ��
    MEDIUM = 1,         // ��
    HIGH = 2,           // ��
    URGENT = 3          // �ً}
};

// XML�G�X�P�[�v�p�̃w���p�[�֐�
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

// XML�A���G�X�P�[�v�p�̃w���p�[�֐�
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

// SYSTEMTIME to wstring�ϊ�
std::wstring SystemTimeToString(const SYSTEMTIME& st) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"%04d-%02d-%02dT%02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buffer);
}

// wstring to SYSTEMTIME�ϊ�
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

// XML����l�𒊏o����w���p�[�֐��i�I�[�o�[���[�h�Łj
std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tag) {
    return ExtractXmlValue(xml, tag, 0);
}

// XML����l�𒊏o����w���p�[�֐��i���S�Łj
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

// WBS�A�C�e����XML�ɕϊ�
std::wstring WBSItemToXml(std::shared_ptr<WBSItem> item, int indent = 0) {
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

// �v���W�F�N�g�S�̂�XML�ɕϊ�
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

// XML����WBS�A�C�e�������
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
    
    // �q�^�X�N�����
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

void OnSaveProject() {
    if (!g_currentProject) {
        MessageBox(nullptr, L"�ۑ�����v���W�F�N�g������܂���B", L"�G���[", MB_OK | MB_ICONWARNING);
        return;
    }

    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L"";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XML�t�@�C��\0*.xml\0���ׂẴt�@�C��\0*.*\0";
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
                std::wstring xmlContent = ProjectToXml();
                file << xmlContent;
                file.close();
                MessageBox(nullptr, L"�v���W�F�N�g��XML�t�@�C���ɕۑ����܂����B", L"���", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBox(nullptr, L"�t�@�C�����J���܂���ł����B", L"�G���[", MB_OK | MB_ICONERROR);
            }
        } catch (...) {
            MessageBox(nullptr, L"�v���W�F�N�g�̕ۑ����ɃG���[���������܂����B", L"�G���[", MB_OK | MB_ICONERROR);
        }
    }
}

void OnOpenProject() {
    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L"";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XML�t�@�C��\0*.xml\0���ׂẴt�@�C��\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        try {
            std::wifstream file(szFile);
            if (file.is_open()) {
                std::wstring xmlContent((std::istreambuf_iterator<wchar_t>(file)),
                                      std::istreambuf_iterator<wchar_t>());
                file.close();
                
                // �V�����v���W�F�N�g���쐬
                g_currentProject = std::make_unique<WBSProject>();
                
                // �v���W�F�N�g�������
                g_currentProject->projectName = ExtractXmlValue(xmlContent, L"ProjectName");
                g_currentProject->description = ExtractXmlValue(xmlContent, L"Description");
                
                // ���[�g�^�X�N�����
                size_t rootTaskStart = xmlContent.find(L"<RootTask>");
                if (rootTaskStart != std::wstring::npos) {
                    size_t rootTaskEnd = xmlContent.find(L"</RootTask>");
                    if (rootTaskEnd != std::wstring::npos) {
                        std::wstring rootTaskXml = xmlContent.substr(rootTaskStart + 10, rootTaskEnd - rootTaskStart - 10);
                        size_t pos = 0;
                        g_currentProject->rootTask = ParseTaskFromXml(rootTaskXml, pos);
                    }
                }
                
                RefreshTreeView();
                RefreshListView();
                MessageBox(nullptr, L"�v���W�F�N�g��XML�t�@�C������ǂݍ��݂܂����B", L"���", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBox(nullptr, L"�t�@�C�����J���܂���ł����B", L"�G���[", MB_OK | MB_ICONERROR);
            }
        } catch (...) {
            MessageBox(nullptr, L"�v���W�F�N�g�̓ǂݍ��ݒ��ɃG���[���������܂����B", L"�G���[", MB_OK | MB_ICONERROR);
        }
    }
}