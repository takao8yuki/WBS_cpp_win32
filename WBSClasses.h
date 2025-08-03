#pragma once

#include <vector>
#include <string>
#include <memory>
#include <windows.h>
#include <commctrl.h>

// WBS�^�X�N�̏��
enum class TaskStatus {
    NOT_STARTED,    // ���J�n
    IN_PROGRESS,    // �i�s��
    COMPLETED,      // ����
    ON_HOLD,        // �ۗ�
    CANCELLED       // �L�����Z��
};

// WBS�^�X�N�̗D��x
enum class TaskPriority {
    LOW,            // ��
    MEDIUM,         // ��
    HIGH,           // ��
    URGENT          // �ً}
};

// WBS�A�C�e���N���X
class WBSItem {
public:
    // �R���X�g���N�^
    WBSItem();
    WBSItem(const std::wstring& name);
    
    // �f�X�g���N�^
    ~WBSItem();

    // �A�N�Z�T�[
    const std::wstring& GetTaskName() const { return taskName; }
    void SetTaskName(const std::wstring& name) { taskName = name; }
    
    const std::wstring& GetDescription() const { return description; }
    void SetDescription(const std::wstring& desc) { description = desc; }
    
    const std::wstring& GetAssignedTo() const { return assignedTo; }
    void SetAssignedTo(const std::wstring& assigned) { assignedTo = assigned; }
    
    TaskStatus GetStatus() const { return status; }
    void SetStatus(TaskStatus stat) { status = stat; }
    
    TaskPriority GetPriority() const { return priority; }
    void SetPriority(TaskPriority prio) { priority = prio; }
    
    double GetEstimatedHours() const { return estimatedHours; }
    void SetEstimatedHours(double hours) { estimatedHours = hours; }
    
    double GetActualHours() const { return actualHours; }
    void SetActualHours(double hours) { actualHours = hours; }
    
    const SYSTEMTIME& GetStartDate() const { return startDate; }
    void SetStartDate(const SYSTEMTIME& date) { startDate = date; }
    
    const SYSTEMTIME& GetEndDate() const { return endDate; }
    void SetEndDate(const SYSTEMTIME& date) { endDate = date; }
    
    const std::wstring& GetId() const { return id; }
    void SetId(const std::wstring& newId) { id = newId; }
    
    int GetLevel() const { return level; }
    void SetLevel(int lev) { level = lev; }

    // �q�^�X�N�Ǘ�
    void AddChild(std::shared_ptr<WBSItem> child);
    void RemoveChild(std::shared_ptr<WBSItem> child);
    const std::vector<std::shared_ptr<WBSItem>>& GetChildren() const { return children; }
    
    // �e�^�X�N�Ǘ�
    std::shared_ptr<WBSItem> GetParent() const { return parent.lock(); }
    void SetParent(std::shared_ptr<WBSItem> par) { parent = par; }

    // ���[�e�B���e�B�֐�
    double GetProgressPercentage() const;
    double GetTotalEstimatedHours() const;
    double GetTotalActualHours() const;
    std::wstring GetStatusString() const;
    std::wstring GetPriorityString() const;
    void GenerateId();

private:
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
};

// WBS�v���W�F�N�g�N���X
class WBSProject {
public:
    WBSProject();
    WBSProject(const std::wstring& name);
    ~WBSProject();

    // �A�N�Z�T�[
    const std::wstring& GetProjectName() const { return projectName; }
    void SetProjectName(const std::wstring& name) { projectName = name; }
    
    const std::wstring& GetDescription() const { return description; }
    void SetDescription(const std::wstring& desc) { description = desc; }
    
    std::shared_ptr<WBSItem> GetRootTask() const { return rootTask; }

    // �v���W�F�N�g�Ǘ�
    void AddRootTask(std::shared_ptr<WBSItem> task);
    bool SaveToFile(const std::wstring& filename);
    bool LoadFromFile(const std::wstring& filename);
    
    // ���v���
    double GetTotalEstimatedHours() const;
    double GetTotalActualHours() const;
    double GetOverallProgress() const;

private:
    std::wstring projectName;
    std::wstring description;
    std::shared_ptr<WBSItem> rootTask;
    std::wstring filename;
};

// ���[�e�B���e�B�֐�
std::wstring TaskStatusToString(TaskStatus status);
TaskStatus StringToTaskStatus(const std::wstring& str);
std::wstring TaskPriorityToString(TaskPriority priority);
TaskPriority StringToTaskPriority(const std::wstring& str);
void SystemTimeToString(const SYSTEMTIME& st, std::wstring& str);
bool StringToSystemTime(const std::wstring& str, SYSTEMTIME& st);