/*
 * ============================================================================
 * WBS_cpp_win32_main.cpp - Work Breakdown Structure�i��ƕ����\���j�Ǘ��V�X�e��
 * ============================================================================
 * 
 * ���X�|���V�u�Ή���WBS�v���W�F�N�g�Ǘ��A�v���P�[�V����
 * 
 * �y��ȋ@�\�z
 * - �_�C�A���O�x�[�X�̃��[�U�[�C���^�[�t�F�[�X
 * - ���X�|���V�u���C�A�E�g�i�T�C�Y�ύX�Ή��j
 * - TreeView�ɂ��K�w�I�^�X�N�\��
 * - ListView�ɂ��ڍ׏��\��
 * - XML�`���ł̃v���W�F�N�g�ۑ�/�ǂݍ���
 * 
 * �y���X�|���V�u�@�\�z
 * - WM_SIZE���b�Z�[�W�Ή�
 * - �ŏ��T�C�Y����
 * - �R���g���[���̎������T�C�Y
 * ============================================================================
 */

// �v���W�F�N�g��p�w�b�_�[
#include "framework.h"
#include "Resource.h"
#include "WBSClasses.h"

// �ǉ���Windows API
#include <commdlg.h>
#include <shlobj.h>

// C++�W�����C�u����
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>

#pragma comment(lib, "shell32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#define MAX_LOADSTRING 100

// ============================================================================
// �O���[�o���ϐ�
// ============================================================================

HINSTANCE hInst;
std::unique_ptr<WBSProject> g_currentProject;
HWND g_hMainDialog = nullptr;
HWND g_hTreeWBS = nullptr;
HWND g_hListDetails = nullptr;
HTREEITEM g_selectedItem = nullptr;

// ============================================================================
// �֐��̑O���錾
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

// �ݒ�t�@�C���֐�
std::wstring GetConfigFilePath();
void SaveLastOpenedFile(const std::wstring& filePath);
std::wstring GetLastOpenedFile();

// �O��XML�֐��i�X�^�u�����j
void OnSaveProject() { 
    MessageBox(g_hMainDialog, L"�ۑ��@�\�͎������ł��B", L"���", MB_OK | MB_ICONINFORMATION);
}

void OnOpenProject() { 
    MessageBox(g_hMainDialog, L"�ǂݍ��݋@�\�͎������ł��B", L"���", MB_OK | MB_ICONINFORMATION);
}

bool LoadProjectFromFile(const std::wstring& filePath) { 
    return false; 
}

// ============================================================================
// �ݒ�t�@�C������֐�
// ============================================================================

std::wstring GetConfigFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath) == S_OK) {
        std::wstring configDir = std::wstring(appDataPath) + L"\\WBS_cpp_win32";
        CreateDirectory(configDir.c_str(), nullptr);
        return configDir + L"\\config.txt";
    }
    return L"config.txt";
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
        // �G���[�͖���
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
                    return line.substr(15);
                }
            }
            file.close();
        }
    } catch (...) {
        // �G���[�͖���
    }
    return L"";
}

// ============================================================================
// ���C���G���g���|�C���g
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

    // Common Controls �̏�����
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // ���C���_�C�A���O��\��
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_WBS_MAIN), nullptr, MainDlgProc);

    return 0;
}

// ============================================================================
// ���X�|���V�u�Ή����C���_�C�A���O�v���V�[�W��
// ============================================================================

INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            g_hMainDialog = hDlg;
            
            // �R���g���[���n���h�����擾
            g_hTreeWBS = GetDlgItem(hDlg, IDC_TREE_WBS);
            g_hListDetails = GetDlgItem(hDlg, IDC_LIST_DETAILS);
            
            // WBS�V�X�e����������
            InitializeWBSDialog(hDlg);
            
            return (INT_PTR)TRUE;
        }

    case WM_SIZE:
        {
            // ���X�|���V�u�Ή�: �_�C�A���O�̃T�C�Y�ύX���̏���
            if (wParam != SIZE_MINIMIZED && g_hTreeWBS && g_hListDetails) {
                RECT dlgRect;
                GetClientRect(hDlg, &dlgRect);
                int dialogWidth = dlgRect.right - dlgRect.left;
                int dialogHeight = dlgRect.bottom - dlgRect.top;
                
                // ���y�C���iTreeView�j�̃��T�C�Y - ���͌Œ�A�����͊g��
                int newTreeHeight = dialogHeight - 80;
                if (newTreeHeight > 0) {
                    SetWindowPos(g_hTreeWBS, nullptr, 
                               14, 20, 366, newTreeHeight,
                               SWP_NOZORDER | SWP_NOACTIVATE);
                }
                
                // �E�y�C���iListView�j�̃��T�C�Y - ���ƍ������g��
                int newListWidth = dialogWidth - 422;
                int newListHeight = dialogHeight - 220;
                if (newListWidth > 0 && newListHeight > 0) {
                    SetWindowPos(g_hListDetails, nullptr,
                               402, 20, newListWidth, newListHeight,
                               SWP_NOZORDER | SWP_NOACTIVATE);
                }
                
                // �{�^���̈ʒu����
                struct ButtonInfo {
                    int id;
                    int x;
                    int y;
                };
                
                ButtonInfo buttons[] = {
                    {IDC_BUTTON_EXPAND_ALL, 14, newTreeHeight + 25},
                    {IDC_BUTTON_COLLAPSE_ALL, 80, newTreeHeight + 25},
                    {IDC_BUTTON_ADD_TASK, dialogWidth - 395, dialogHeight - 180},
                    {IDC_BUTTON_ADD_SUBTASK, dialogWidth - 285, dialogHeight - 180},
                    {IDC_BUTTON_EDIT_TASK, dialogWidth - 175, dialogHeight - 180},
                    {IDC_BUTTON_DELETE_TASK, dialogWidth - 75, dialogHeight - 180},
                    {IDC_BUTTON_NEW_PROJECT, dialogWidth - 395, dialogHeight - 75},
                    {IDC_BUTTON_OPEN_PROJECT, dialogWidth - 315, dialogHeight - 75},
                    {IDC_BUTTON_SAVE_PROJECT, dialogWidth - 235, dialogHeight - 75},
                    {IDC_BUTTON_EXIT, dialogWidth - 75, dialogHeight - 75},
                };
                
                for (const auto& btn : buttons) {
                    HWND hButton = GetDlgItem(hDlg, btn.id);
                    if (hButton) {
                        SetWindowPos(hButton, nullptr, btn.x, btn.y, 0, 0, 
                                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                }
                
                InvalidateRect(hDlg, nullptr, TRUE);
            }
            return (INT_PTR)TRUE;
        }

    case WM_GETMINMAXINFO:
        {
            // �ŏ��T�C�Y�̐�����ݒ�
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMinTrackSize.x = 600;
            lpMMI->ptMinTrackSize.y = 400;
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
                    auto newTask = std::make_shared<WBSItem>(L"�V�����^�X�N");
                    g_currentProject->rootTask->AddChild(newTask);
                    RefreshTreeView();
                    MessageBox(hDlg, L"�V�����^�X�N��ǉ����܂����B", L"���", MB_OK | MB_ICONINFORMATION);
                }
                break;
                
            case IDC_BUTTON_ADD_SUBTASK:
            case IDM_EDIT_ADD_SUBTASK:
                {
                    if (!g_selectedItem) {
                        MessageBox(hDlg, L"�e�^�X�N��I�����Ă��������B", L"�G���[", MB_OK | MB_ICONWARNING);
                        return FALSE;
                    }
                    std::shared_ptr<WBSItem> parentItem = GetItemFromTreeItem(g_selectedItem);
                    if (!parentItem) return FALSE;
                    auto newSubTask = std::make_shared<WBSItem>(L"�V�����T�u�^�X�N");
                    parentItem->AddChild(newSubTask);
                    RefreshTreeView();
                    MessageBox(hDlg, L"�V�����T�u�^�X�N��ǉ����܂����B", L"���", MB_OK | MB_ICONINFORMATION);
                }
                break;
                
            case IDC_BUTTON_EDIT_TASK:
            case IDM_EDIT_EDIT_TASK:
                {
                    if (!g_selectedItem) {
                        MessageBox(hDlg, L"�ҏW����^�X�N��I�����Ă��������B", L"�G���[", MB_OK | MB_ICONWARNING);
                        return FALSE;
                    }
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_TASK_EDIT), hDlg, TaskEditDlgProc);
                }
                break;
                
            case IDC_BUTTON_DELETE_TASK:
            case IDM_EDIT_DELETE_TASK:
                {
                    if (!g_selectedItem) {
                        MessageBox(hDlg, L"�폜����^�X�N��I�����Ă��������B", L"�G���[", MB_OK | MB_ICONWARNING);
                        return FALSE;
                    }
                    if (MessageBox(hDlg, L"�I�������^�X�N���폜���܂����H", L"�m�F", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        MessageBox(hDlg, L"�^�X�N�폜�@�\�͍�������\��ł��B", L"���", MB_OK | MB_ICONINFORMATION);
                    }
                }
                break;
                
            case IDC_BUTTON_EXPAND_ALL:
            case IDM_VIEW_EXPAND_ALL:
                if (g_hTreeWBS) {
                    HTREEITEM hItem = TreeView_GetRoot(g_hTreeWBS);
                    while (hItem) {
                        TreeView_Expand(g_hTreeWBS, hItem, TVE_EXPAND);
                        hItem = TreeView_GetNextSibling(g_hTreeWBS, hItem);
                    }
                }
                break;
                
            case IDC_BUTTON_COLLAPSE_ALL:
            case IDM_VIEW_COLLAPSE_ALL:
                if (g_hTreeWBS) {
                    HTREEITEM hItem = TreeView_GetRoot(g_hTreeWBS);
                    while (hItem) {
                        TreeView_Expand(g_hTreeWBS, hItem, TVE_COLLAPSE);
                        hItem = TreeView_GetNextSibling(g_hTreeWBS, hItem);
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
// �^�X�N�ҏW�_�C�A���O�v���V�[�W��
// ============================================================================

INT_PTR CALLBACK TaskEditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            if (g_selectedItem) {
                std::shared_ptr<WBSItem> item = GetItemFromTreeItem(g_selectedItem);
                if (item) {
                    SetDlgItemText(hDlg, IDC_EDIT_TASK_NAME, item->taskName.c_str());
                    SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, item->description.c_str());
                    SetDlgItemText(hDlg, IDC_EDIT_ASSIGNED_TO, item->assignedTo.c_str());
                    
                    HWND hComboStatus = GetDlgItem(hDlg, IDC_COMBO_STATUS);
                    ComboBox_AddString(hComboStatus, L"���J�n");
                    ComboBox_AddString(hComboStatus, L"�i�s��");
                    ComboBox_AddString(hComboStatus, L"����");
                    ComboBox_AddString(hComboStatus, L"�ۗ�");
                    ComboBox_AddString(hComboStatus, L"�L�����Z��");
                    ComboBox_SetCurSel(hComboStatus, (int)item->status);
                    
                    HWND hComboPriority = GetDlgItem(hDlg, IDC_COMBO_PRIORITY);
                    ComboBox_AddString(hComboPriority, L"��");
                    ComboBox_AddString(hComboPriority, L"��");
                    ComboBox_AddString(hComboPriority, L"��");
                    ComboBox_AddString(hComboPriority, L"�ً}");
                    ComboBox_SetCurSel(hComboPriority, (int)item->priority);
                    
                    SetDlgItemText(hDlg, IDC_EDIT_ESTIMATED_HOURS, std::to_wstring((int)item->estimatedHours).c_str());
                    SetDlgItemText(hDlg, IDC_EDIT_ACTUAL_HOURS, std::to_wstring((int)item->actualHours).c_str());
                    
                    HWND hProgress = GetDlgItem(hDlg, IDC_PROGRESS_BAR);
                    if (hProgress) {
                        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                        SendMessage(hProgress, PBM_SETPOS, (int)item->GetProgressPercentage(), 0);
                    }
                    
                    std::wstring progressText = std::to_wstring((int)item->GetProgressPercentage()) + L"%";
                    SetDlgItemText(hDlg, IDC_STATIC_PROGRESS, progressText.c_str());
                }
            }
            return (INT_PTR)TRUE;
        }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDC_BUTTON_APPLY)
        {
            if (g_selectedItem) {
                std::shared_ptr<WBSItem> item = GetItemFromTreeItem(g_selectedItem);
                if (item) {
                    wchar_t buffer[256];
                    GetDlgItemText(hDlg, IDC_EDIT_TASK_NAME, buffer, 256);
                    item->taskName = buffer;
                    
                    GetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, buffer, 256);
                    item->description = buffer;
                    
                    GetDlgItemText(hDlg, IDC_EDIT_ASSIGNED_TO, buffer, 256);
                    item->assignedTo = buffer;
                    
                    HWND hComboStatus = GetDlgItem(hDlg, IDC_COMBO_STATUS);
                    item->status = (TaskStatus)ComboBox_GetCurSel(hComboStatus);
                    
                    HWND hComboPriority = GetDlgItem(hDlg, IDC_COMBO_PRIORITY);
                    item->priority = (TaskPriority)ComboBox_GetCurSel(hComboPriority);
                    
                    GetDlgItemText(hDlg, IDC_EDIT_ESTIMATED_HOURS, buffer, 256);
                    item->estimatedHours = _wtof(buffer);
                    
                    GetDlgItemText(hDlg, IDC_EDIT_ACTUAL_HOURS, buffer, 256);
                    item->actualHours = _wtof(buffer);
                    
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
// ���̑��̊֐�����
// ============================================================================

void InitializeWBSDialog(HWND hDlg) {
    if (g_hListDetails) {
        SendMessage(g_hListDetails, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        
        LVCOLUMN lvc = {};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;
        
        lvc.pszText = (LPWSTR)L"����";
        lvc.cx = 120;
        ListView_InsertColumn(g_hListDetails, 0, &lvc);
        
        lvc.pszText = (LPWSTR)L"�l";
        lvc.cx = 200;
        ListView_InsertColumn(g_hListDetails, 1, &lvc);
    }
    
    g_currentProject = std::make_unique<WBSProject>(L"�T���v��WBS�v���W�F�N�g");
    
    auto task1 = std::make_shared<WBSItem>(L"�v����`");
    task1->id = L"1.1";
    task1->level = 1;
    task1->description = L"�V�X�e���v���̒�`�ƕ���";
    task1->estimatedHours = 40.0;
    task1->status = TaskStatus::COMPLETED;
    task1->assignedTo = L"�c��";
    g_currentProject->rootTask->AddChild(task1);

    auto task2 = std::make_shared<WBSItem>(L"�݌v");
    task2->id = L"1.2";
    task2->level = 1;
    task2->description = L"�V�X�e���݌v���̍쐬";
    task2->estimatedHours = 60.0;
    task2->status = TaskStatus::IN_PROGRESS;
    task2->assignedTo = L"����";
    g_currentProject->rootTask->AddChild(task2);

    auto subTask1 = std::make_shared<WBSItem>(L"��{�݌v");
    subTask1->description = L"��{�݌v���̍쐬";
    subTask1->estimatedHours = 30.0;
    subTask1->status = TaskStatus::COMPLETED;
    subTask1->assignedTo = L"����";
    task2->AddChild(subTask1);

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

    LVITEM lvi = {};
    lvi.mask = LVIF_TEXT;
    
    const std::pair<std::wstring, std::wstring> details[] = {
        {L"�^�X�N��", item->taskName},
        {L"ID", item->id},
        {L"����", item->description},
        {L"�X�e�[�^�X", item->GetStatusString()},
        {L"�D��x", item->GetPriorityString()},
        {L"�\�莞��", std::to_wstring(item->estimatedHours) + L"����"},
        {L"���ю���", std::to_wstring(item->actualHours) + L"����"},
        {L"�i����", std::to_wstring((int)item->GetProgressPercentage()) + L"%"},
        {L"�S����", item->assignedTo}
    };
    
    for (int i = 0; i < sizeof(details) / sizeof(details[0]); ++i) {
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(details[i].first.c_str());
        ListView_InsertItem(g_hListDetails, &lvi);
        ListView_SetItemText(g_hListDetails, i, 1, const_cast<LPWSTR>(details[i].second.c_str()));
    }
}

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