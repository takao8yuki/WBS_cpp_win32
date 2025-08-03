#pragma once

/*
 * WBSClasses.h - Work Breakdown Structure�i��ƕ����\���j�Ǘ��V�X�e��
 * 
 * ���̃w�b�_�[�t�@�C���́A�v���W�F�N�g�Ǘ��ɕK�v��WBS�@�\��񋟂��܂��B
 * WBSItem�i��ƍ��ځj��WBSProject�i�v���W�F�N�g�j�N���X���`���A
 * �^�X�N�̊K�w�Ǘ��A�i���ǐՁA�H���Ǘ��Ȃǂ̋@�\���������Ă��܂��B
 * 
 * ��ȋ@�\:
 * - �K�w�\���ł̃^�X�N�Ǘ�
 * - �^�X�N�̐i���󋵒ǐ�
 * - �H���i���ς���E���сj�Ǘ�
 * - �v���W�F�N�g�S�̂̓��v����
 * - XML�`���ł̃f�[�^�i�����Ή�
 * 
 * �쐬��: [�J���Җ�]
 * �쐬��: [���t]
 * �ŏI�X�V: [���t]
 * �o�[�W����: 1.0
 */

#include <vector>
#include <string>
#include <memory>
#include <windows.h>
#include <commctrl.h>

// =============================================================================
// �񋓌^��`
// =============================================================================

/**
 * @brief WBS�^�X�N�̐i�s��Ԃ�\���񋓌^
 * 
 * �v���W�F�N�g���̃^�X�N�����݂ǂ̒i�K�ɂ��邩�������܂��B
 * ���̏�Ԃ͐i���Ǘ��ⓝ�v���̌v�Z�Ɏg�p����܂��B
 */
enum class TaskStatus {
    NOT_STARTED,    ///< ���J�n - �^�X�N���܂��J�n����Ă��Ȃ����
    IN_PROGRESS,    ///< �i�s�� - �^�X�N�����ݎ��s����Ă�����
    COMPLETED,      ///< ���� - �^�X�N������Ɋ����������
    ON_HOLD,        ///< �ۗ� - �ꎞ�I�ɍ�Ƃ���~����Ă�����
    CANCELLED       ///< �L�����Z�� - �^�X�N�����~���ꂽ���
};

/**
 * @brief WBS�^�X�N�̏d�v�x�E�ً}�x��\���񋓌^
 * 
 * �^�X�N�̗D�揇�ʂ����肷�邽�߂Ɏg�p����܂��B
 * ���\�[�X�z�����Ə����̌���Ɋ��p����܂��B
 */
enum class TaskPriority {
    LOW,            ///< ��D��x - ���Ԃɗ]�T������ꍇ�Ɏ��s
    MEDIUM,         ///< ���D��x - �W���I�ȗD��x���x��
    HIGH,           ///< ���D��x - �d�v�ő��߂̊������K�v
    URGENT          ///< �ً} - �ŗD��ő����ɑΉ����K�v
};

// =============================================================================
// �N���X��`
// =============================================================================

/**
 * @brief WBS�iWork Breakdown Structure�j�̌ʍ�ƍ��ڂ�\���N���X
 * 
 * �v���W�F�N�g���\������X�̃^�X�N��T�u�^�X�N��\�����܂��B
 * �K�w�\���������A�e�q�֌W�ɂ���ăv���W�F�N�g�̍�ƕ�����\���ł��܂��B
 * 
 * ��ȋ@�\:
 * - �^�X�N�̊�{���Ǘ��i���O�A�����A�S���҂Ȃǁj
 * - �i���󋵂ƍH���̒ǐ�
 * - �e�q�֌W�ɂ��K�w�\���̊Ǘ�
 * - ���v���̌v�Z�i�i�����A���H���Ȃǁj
 * 
 * �g�p��:
 * @code
 * auto task = std::make_shared<WBSItem>("�V�X�e���݌v");
 * task->SetDescription("�V�X�e���̊�{�݌v���s��");
 * task->SetEstimatedHours(40.0);
 * task->SetStatus(TaskStatus::IN_PROGRESS);
 * @endcode
 */
class WBSItem {
public:
    // =========================================================================
    // �R���X�g���N�^�E�f�X�g���N�^
    // =========================================================================
    
    /**
     * @brief �f�t�H���g�R���X�g���N�^
     * 
     * �V����WBS�A�C�e�����쐬���܂��B
     * �f�t�H���g�l�ŏ���������A�ォ��e�v���p�e�B��ݒ�ł��܂��B
     */
    WBSItem();
    
    /**
     * @brief ���O�w��R���X�g���N�^
     * 
     * @param name �^�X�N��
     * 
     * �w�肳�ꂽ���O��WBS�A�C�e�����쐬���܂��B
     * ���̑��̃v���p�e�B�̓f�t�H���g�l�ŏ���������܂��B
     */
    WBSItem(const std::wstring& name);
    
    /**
     * @brief �f�X�g���N�^
     * 
     * WBS�A�C�e���̃��\�[�X��K�؂ɉ�����܂��B
     */
    ~WBSItem();

    // =========================================================================
    // �A�N�Z�T�[���\�b�h�Q
    // =========================================================================
    
    /**
     * @brief �^�X�N�����擾
     * @return �^�X�N���̎Q��
     */
    const std::wstring& GetTaskName() const { return taskName; }
    
    /**
     * @brief �^�X�N����ݒ�
     * @param name �V�����^�X�N��
     */
    void SetTaskName(const std::wstring& name) { taskName = name; }
    
    /**
     * @brief �^�X�N�̏ڍא������擾
     * @return �������̎Q��
     */
    const std::wstring& GetDescription() const { return description; }
    
    /**
     * @brief �^�X�N�̏ڍא�����ݒ�
     * @param desc ������
     */
    void SetDescription(const std::wstring& desc) { description = desc; }
    
    /**
     * @brief �S���Җ����擾
     * @return �S���Җ��̎Q��
     */
    const std::wstring& GetAssignedTo() const { return assignedTo; }
    
    /**
     * @brief �S���҂�ݒ�
     * @param assigned �S���Җ�
     */
    void SetAssignedTo(const std::wstring& assigned) { assignedTo = assigned; }
    
    /**
     * @brief ���݂̃^�X�N��Ԃ��擾
     * @return TaskStatus�l
     */
    TaskStatus GetStatus() const { return status; }
    
    /**
     * @brief �^�X�N��Ԃ�ݒ�
     * @param stat �V�������
     */
    void SetStatus(TaskStatus stat) { status = stat; }
    
    /**
     * @brief �^�X�N�̗D��x���擾
     * @return TaskPriority�l
     */
    TaskPriority GetPriority() const { return priority; }
    
    /**
     * @brief �^�X�N�̗D��x��ݒ�
     * @param prio �V�����D��x
     */
    void SetPriority(TaskPriority prio) { priority = prio; }
    
    /**
     * @brief ���ς���H�����擾�i���ԒP�ʁj
     * @return ���ς���H��
     */
    double GetEstimatedHours() const { return estimatedHours; }
    
    /**
     * @brief ���ς���H����ݒ�
     * @param hours ���ς���H���i���ԁj
     */
    void SetEstimatedHours(double hours) { estimatedHours = hours; }
    
    /**
     * @brief ���эH�����擾�i���ԒP�ʁj
     * @return ���эH��
     */
    double GetActualHours() const { return actualHours; }
    
    /**
     * @brief ���эH����ݒ�
     * @param hours ���эH���i���ԁj
     */
    void SetActualHours(double hours) { actualHours = hours; }
    
    /**
     * @brief �^�X�N�J�n�\������擾
     * @return �J�n����SYSTEMTIME�\���̂̎Q��
     */
    const SYSTEMTIME& GetStartDate() const { return startDate; }
    
    /**
     * @brief �^�X�N�J�n�\�����ݒ�
     * @param date �J�n��
     */
    void SetStartDate(const SYSTEMTIME& date) { startDate = date; }
    
    /**
     * @brief �^�X�N�I���\������擾
     * @return �I������SYSTEMTIME�\���̂̎Q��
     */
    const SYSTEMTIME& GetEndDate() const { return endDate; }
    
    /**
     * @brief �^�X�N�I���\�����ݒ�
     * @param date �I����
     */
    void SetEndDate(const SYSTEMTIME& date) { endDate = date; }
    
    /**
     * @brief �^�X�NID���擾
     * @return �^�X�NID������̎Q��
     */
    const std::wstring& GetId() const { return id; }
    
    /**
     * @brief �^�X�NID��ݒ�
     * @param newId �V�����^�X�NID
     */
    void SetId(const std::wstring& newId) { id = newId; }
    
    /**
     * @brief �K�w���x�����擾
     * @return �K�w���x���i0�����[�g�j
     */
    int GetLevel() const { return level; }
    
    /**
     * @brief �K�w���x����ݒ�
     * @param lev �V�����K�w���x��
     */
    void SetLevel(int lev) { level = lev; }

    // =========================================================================
    // �K�w�Ǘ����\�b�h
    // =========================================================================
    
    /**
     * @brief �q�^�X�N��ǉ�
     * 
     * @param child �ǉ�����q�^�X�N
     * 
     * �w�肳�ꂽ�^�X�N���q�^�X�N�Ƃ��Ēǉ����܂��B
     * �q�^�X�N�̐e�Q�ƂƊK�w���x���͎����I�ɐݒ肳��܂��B
     */
    void AddChild(std::shared_ptr<WBSItem> child);
    
    /**
     * @brief �q�^�X�N���폜
     * 
     * @param child �폜����q�^�X�N
     * 
     * �w�肳�ꂽ�q�^�X�N�����X�g����폜���܂��B
     */
    void RemoveChild(std::shared_ptr<WBSItem> child);
    
    /**
     * @brief �S�Ă̎q�^�X�N���擾
     * @return �q�^�X�N�̃x�N�^�[�̎Q��
     */
    const std::vector<std::shared_ptr<WBSItem>>& GetChildren() const { return children; }
    
    /**
     * @brief �e�^�X�N���擾
     * @return �e�^�X�N��shared_ptr�i���[�g�̏ꍇ��nullptr�j
     */
    std::shared_ptr<WBSItem> GetParent() const { return parent.lock(); }
    
    /**
     * @brief �e�^�X�N��ݒ�
     * @param par �V�����e�^�X�N
     */
    void SetParent(std::shared_ptr<WBSItem> par) { parent = par; }

    // =========================================================================
    // ���[�e�B���e�B���\�b�h
    // =========================================================================
    
    /**
     * @brief �i�������v�Z
     * @return �i�����i0.0?100.0�j
     * 
     * ���эH�������ς���H���Ŋ����Đi�������v�Z���܂��B
     * ���ς���H����0�̏ꍇ��0.0��Ԃ��܂��B
     */
    double GetProgressPercentage() const;
    
    /**
     * @brief ���̃^�X�N�Ƃ��̎q�^�X�N�̑����ς���H�����v�Z
     * @return �����ς���H��
     */
    double GetTotalEstimatedHours() const;
    
    /**
     * @brief ���̃^�X�N�Ƃ��̎q�^�X�N�̑����эH�����v�Z
     * @return �����эH��
     */
    double GetTotalActualHours() const;
    
    /**
     * @brief �X�e�[�^�X�𕶎���`���Ŏ擾
     * @return �X�e�[�^�X�̓��{��\�L
     */
    std::wstring GetStatusString() const;
    
    /**
     * @brief �D��x�𕶎���`���Ŏ擾
     * @return �D��x�̓��{��\�L
     */
    std::wstring GetPriorityString() const;
    
    /**
     * @brief �����I��ID�𐶐�
     * 
     * �e�^�X�N��ID�Ǝq�^�X�N�̏�������ɂ��āA
     * ��ӂ�ID�������������܂��B
     */
    void GenerateId();

private:
    // =========================================================================
    // �v���C�x�[�g�����o�ϐ�
    // =========================================================================
    
    std::wstring id;                                        ///< �^�X�N�̈�ӎ��ʎq
    std::wstring taskName;                                  ///< �^�X�N��
    std::wstring description;                               ///< �^�X�N�̏ڍא���
    std::wstring assignedTo;                                ///< �S���Җ�
    TaskStatus status;                                      ///< ���݂̐i�s���
    TaskPriority priority;                                  ///< �D��x���x��
    double estimatedHours;                                  ///< ���ς���H���i���ԁj
    double actualHours;                                     ///< ���эH���i���ԁj
    SYSTEMTIME startDate;                                   ///< �J�n�\���
    SYSTEMTIME endDate;                                     ///< �I���\���
    int level;                                              ///< �K�w���x���i0=���[�g�j
    
    std::vector<std::shared_ptr<WBSItem>> children;         ///< �q�^�X�N�̃��X�g
    std::weak_ptr<WBSItem> parent;                          ///< �e�^�X�N�ւ̎�Q��
};

/**
 * @brief WBS�v���W�F�N�g�S�̂��Ǘ�����N���X
 * 
 * �v���W�F�N�g�S�̂̏����Ǘ����A���[�g�^�X�N��ʂ���
 * �S�Ẵ^�X�N�ɃA�N�Z�X�ł��܂��B�v���W�F�N�g���x���ł�
 * ���v���̌v�Z��A�t�@�C���ւ̕ۑ��E�ǂݍ��݋@�\��񋟂��܂��B
 * 
 * ��ȋ@�\:
 * - �v���W�F�N�g��{���̊Ǘ�
 * - ���[�g�^�X�N��ʂ����S�^�X�N�ւ̃A�N�Z�X
 * - �v���W�F�N�g�S�̂̓��v����
 * - XML�t�@�C���ł̕ۑ��E�ǂݍ���
 * 
 * �g�p��:
 * @code
 * auto project = std::make_unique<WBSProject>("�V�V�X�e���J��");
 * project->SetDescription("�ڋq�Ǘ��V�X�e���̊J���v���W�F�N�g");
 * project->SaveToFile(L"project.xml");
 * @endcode
 */
class WBSProject {
public:
    // =========================================================================
    // �R���X�g���N�^�E�f�X�g���N�^
    // =========================================================================
    
    /**
     * @brief �f�t�H���g�R���X�g���N�^
     * 
     * �V����WBS�v���W�F�N�g���쐬���܂��B
     * �f�t�H���g�̃v���W�F�N�g���Ń��[�g�^�X�N�������쐬����܂��B
     */
    WBSProject();
    
    /**
     * @brief ���O�w��R���X�g���N�^
     * 
     * @param name �v���W�F�N�g��
     * 
     * �w�肳�ꂽ���O��WBS�v���W�F�N�g���쐬���܂��B
     * �v���W�F�N�g���Ɠ������O�Ń��[�g�^�X�N���쐬����܂��B
     */
    WBSProject(const std::wstring& name);
    
    /**
     * @brief �f�X�g���N�^
     * 
     * �v���W�F�N�g�̃��\�[�X��K�؂ɉ�����܂��B
     */
    ~WBSProject();

    // =========================================================================
    // �A�N�Z�T�[���\�b�h�Q
    // =========================================================================
    
    /**
     * @brief �v���W�F�N�g�����擾
     * @return �v���W�F�N�g���̎Q��
     */
    const std::wstring& GetProjectName() const { return projectName; }
    
    /**
     * @brief �v���W�F�N�g����ݒ�
     * @param name �V�����v���W�F�N�g��
     */
    void SetProjectName(const std::wstring& name) { projectName = name; }
    
    /**
     * @brief �v���W�F�N�g�̐������擾
     * @return �������̎Q��
     */
    const std::wstring& GetDescription() const { return description; }
    
    /**
     * @brief �v���W�F�N�g�̐�����ݒ�
     * @param desc ������
     */
    void SetDescription(const std::wstring& desc) { description = desc; }
    
    /**
     * @brief ���[�g�^�X�N���擾
     * @return ���[�g�^�X�N��shared_ptr
     */
    std::shared_ptr<WBSItem> GetRootTask() const { return rootTask; }

    // =========================================================================
    // �v���W�F�N�g�Ǘ����\�b�h
    // =========================================================================
    
    /**
     * @brief ���[�g���x���Ƀ^�X�N��ǉ�
     * 
     * @param task �ǉ�����^�X�N
     * 
     * �v���W�F�N�g�̃��[�g���x���ɐV�����^�X�N��ǉ����܂��B
     * �ʏ�̓v���W�F�N�g�̎�v�ȃt�F�[�Y��J�e�S���Ɏg�p���܂��B
     */
    void AddRootTask(std::shared_ptr<WBSItem> task);
    
    /**
     * @brief �v���W�F�N�g���t�@�C���ɕۑ�
     * 
     * @param filename �ۑ���t�@�C����
     * @return �ۑ��ɐ��������ꍇtrue
     * 
     * �v���W�F�N�g�̑S�f�[�^��XML�`���Ńt�@�C���ɕۑ����܂��B
     */
    bool SaveToFile(const std::wstring& filename);
    
    /**
     * @brief �t�@�C������v���W�F�N�g��ǂݍ���
     * 
     * @param filename �ǂݍ��݌��t�@�C����
     * @return �ǂݍ��݂ɐ��������ꍇtrue
     * 
     * XML�`���ŕۑ����ꂽ�v���W�F�N�g�t�@�C����ǂݍ��݂܂��B
     */
    bool LoadFromFile(const std::wstring& filename);
    
    // =========================================================================
    // ���v��񃁃\�b�h
    // =========================================================================
    
    /**
     * @brief �v���W�F�N�g�S�̂̑����ς���H�����v�Z
     * @return �����ς���H���i���ԁj
     * 
     * �v���W�F�N�g���̑S�^�X�N�̌��ς���H�������v���܂��B
     */
    double GetTotalEstimatedHours() const;
    
    /**
     * @brief �v���W�F�N�g�S�̂̑����эH�����v�Z
     * @return �����эH���i���ԁj
     * 
     * �v���W�F�N�g���̑S�^�X�N�̎��эH�������v���܂��B
     */
    double GetTotalActualHours() const;
    
    /**
     * @brief �v���W�F�N�g�S�̂̐i�������v�Z
     * @return �S�̐i�����i0.0?100.0�j
     * 
     * �S�^�X�N�̎��эH���ƌ��ς���H������
     * �v���W�F�N�g�S�̂̐i�������v�Z���܂��B
     */
    double GetOverallProgress() const;

private:
    // =========================================================================
    // �v���C�x�[�g�����o�ϐ�
    // =========================================================================
    
    std::wstring projectName;                               ///< �v���W�F�N�g��
    std::wstring description;                               ///< �v���W�F�N�g�̐���
    std::shared_ptr<WBSItem> rootTask;                      ///< ���[�g�^�X�N
    std::wstring filename;                                  ///< �ۑ��t�@�C����
};

// =============================================================================
// ���[�e�B���e�B�֐��Q
// =============================================================================

/**
 * @brief TaskStatus����{�ꕶ����ɕϊ�
 * @param status �ϊ�����X�e�[�^�X
 * @return ���{��̃X�e�[�^�X������
 */
std::wstring TaskStatusToString(TaskStatus status);

/**
 * @brief �������TaskStatus�ɕϊ�
 * @param str �ϊ����镶����
 * @return �Ή�����TaskStatus�l
 */
TaskStatus StringToTaskStatus(const std::wstring& str);

/**
 * @brief TaskPriority����{�ꕶ����ɕϊ�
 * @param priority �ϊ�����D��x
 * @return ���{��̗D��x������
 */
std::wstring TaskPriorityToString(TaskPriority priority);

/**
 * @brief �������TaskPriority�ɕϊ�
 * @param str �ϊ����镶����
 * @return �Ή�����TaskPriority�l
 */
TaskPriority StringToTaskPriority(const std::wstring& str);

/**
 * @brief SYSTEMTIME�𕶎���`���ɕϊ�
 * @param st �ϊ�����SYSTEMTIME
 * @param str ���ʂ��i�[���镶����
 */
void SystemTimeToString(const SYSTEMTIME& st, std::wstring& str);

/**
 * @brief �������SYSTEMTIME�ɕϊ�
 * @param str �ϊ����镶����
 * @param st ���ʂ��i�[����SYSTEMTIME
 * @return �ϊ��ɐ��������ꍇtrue
 */
bool StringToSystemTime(const std::wstring& str, SYSTEMTIME& st);