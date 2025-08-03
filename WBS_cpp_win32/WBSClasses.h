/*
 * ============================================================================
 * WBSClasses.h - WBS�A�v���P�[�V���� �N���X��`�w�b�_�[
 * ============================================================================
 * 
 * ���̃t�@�C���ɂ́AWBS�iWork Breakdown Structure�j�Ǘ��A�v���P�[�V������
 * �g�p������v�ȃN���X�Ɨ񋓌^�̒�`���܂܂�Ă��܂��B
 * 
 * �y�܂܂��N���X�z
 * - TaskStatus: �^�X�N�̐i�s��Ԃ�\���񋓌^
 * - TaskPriority: �^�X�N�̗D��x��\���񋓌^  
 * - WBSItem: �ʂ̃^�X�N�E�T�u�^�X�N��\���N���X
 * - WBSProject: �v���W�F�N�g�S�̂��Ǘ�����N���X
 * 
 * �y�݌v�����z
 * - RAII: �I�u�W�F�N�g�̎����I�ȃ��\�[�X�Ǘ�
 * - �^���S��: �񋓃N���X�ɂ�鋭���^�t��
 * - ���������S��: �X�}�[�g�|�C���^�̊��p
 * - �g����: �����̋@�\�ǉ��ɑΉ������݌v
 * 
 * �쐬��: WBS�J���`�[��
 * �쐬��: 2024�N
 * �o�[�W����: 1.0
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
// Common Controls �}�N����`�⊮
// ============================================================================

// ComboBox�}�N������`����Ă��Ȃ��ꍇ�̕⊮��`
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

// ComboBox���b�Z�[�W�萔�̒�`
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
// �񋓌^��`
// ============================================================================

/**
 * @brief WBS�^�X�N�̐i�s��Ԃ�\���񋓌^
 * 
 * �v���W�F�N�g�Ǘ��ɂ����āA�e�^�X�N�̌��݂̏󋵂𖾊m�ɕ��ނ��邽�߂Ɏg�p���܂��B
 * �i�����|�[�g�̐�����A�v���W�F�N�g�S�̂̏󋵔c���Ɋ��p����܂��B
 * 
 * @note ��ԑJ�ڂ̐����p�^�[��:
 *       NOT_STARTED �� IN_PROGRESS �� COMPLETED
 *       ��O�I�� ON_HOLD �� CANCELLED �ւ̕ύX���\
 */
enum class TaskStatus {
    NOT_STARTED = 0,    // ���J�n - �^�X�N���܂��J�n����Ă��Ȃ����
    IN_PROGRESS = 1,    // �i�s�� - �^�X�N�����ݎ��s����Ă�����  
    COMPLETED = 2,      // ���� - �^�X�N������Ɋ����������
    ON_HOLD = 3,        // �ۗ� - �ꎞ�I�ɍ�Ƃ���~����Ă�����
    CANCELLED = 4       // �L�����Z�� - �^�X�N�����~���ꂽ���
};

/**
 * @brief WBS�^�X�N�̏d�v�x�E�ً}�x��\���񋓌^
 * 
 * ���\�[�X�z�����Ə����̌���A�X�P�W���[�������ɂ�����
 * �^�X�N�̗D�揇�ʂ𔻒f���邽�߂Ɏg�p���܂��B
 * 
 * @note �D��x�̔��f�:
 *       URGENT: �����ɑΉ����K�v�i�����؂ꃊ�X�N��)
 *       HIGH: �d�v�ő��߂̊������K�v�i�v���W�F�N�g�����ɒ����j
 *       MEDIUM: �W���I�ȗD��x�i�ʏ�̍�Ə����ŏ���)
 *       LOW: ���Ԃɗ]�T������ꍇ�Ɏ��s�i�摗��\)
 */
enum class TaskPriority {
    LOW = 0,            // ��D��x - ���Ԃɗ]�T������ꍇ�Ɏ��s
    MEDIUM = 1,         // ���D��x - �W���I�ȗD��x���x��
    HIGH = 2,           // ���D��x - �d�v�ő��߂̊������K�v
    URGENT = 3          // �ً} - �ŗD��ő����ɑΉ����K�v
};

// ============================================================================
// �N���X��`
// ============================================================================

/**
 * @brief WBS�iWork Breakdown Structure�j�̌ʍ�ƍ��ڂ�\���N���X
 * 
 * �v���W�F�N�g���\������X�̃^�X�N��T�u�^�X�N��\�����A�K�w�\�����`�����܂��B
 * shared_from_this���p�����邱�ƂŁA���S�Ȏ��ȎQ�Ƌ@�\��񋟂��A
 * �e�q�֌W�̊Ǘ��ɂ�����z�Q�Ƃ�h���܂��B
 */
class WBSItem : public std::enable_shared_from_this<WBSItem> {
public:
    // �����o�ϐ��ipublic�A�N�Z�X - �ȈՓI�Ȏ����̂��߁j
    std::wstring id;                                        ///< �^�X�N�̈�ӎ��ʎq�i�K�w�I��ID�̌n)
    std::wstring taskName;                                  ///< �^�X�N�̖���
    std::wstring description;                               ///< �^�X�N�̏ڍא���
    std::wstring assignedTo;                                ///< �S���Җ�
    TaskStatus status;                                      ///< ���݂̐i�s���
    TaskPriority priority;                                  ///< �D��x���x��
    double estimatedHours;                                  ///< ���ς���H���i���ԒP��)
    double actualHours;                                     ///< ���эH���i���ԒP��)
    SYSTEMTIME startDate;                                   ///< �J�n�\���
    SYSTEMTIME endDate;                                     ///< �I���\���
    int level;                                              ///< �K�w���x���i0=���[�g)
    std::vector<std::shared_ptr<WBSItem>> children;         ///< �q�^�X�N�̃R���N�V����
    std::weak_ptr<WBSItem> parent;                          ///< �e�^�X�N�ւ̎�Q�Ɓi�z�Q�Ɖ���j

    /**
     * @brief �f�t�H���g�R���X�g���N�^
     */
    WBSItem() : status(TaskStatus::NOT_STARTED), priority(TaskPriority::MEDIUM), 
                estimatedHours(0.0), actualHours(0.0), level(0) {
        GetSystemTime(&startDate);
        GetSystemTime(&endDate);
        taskName = L"�V�����^�X�N";
    }

    /**
     * @brief ���O�w��R���X�g���N�^
     */
    WBSItem(const std::wstring& name) : WBSItem() {
        taskName = name;
    }

    /**
     * @brief �q�^�X�N���K�w�\���ɒǉ�
     */
    void AddChild(std::shared_ptr<WBSItem> child) {
        child->parent = shared_from_this();
        child->level = this->level + 1;
        child->id = this->id + L"." + std::to_wstring(children.size() + 1);
        children.push_back(child);
    }

    /**
     * @brief �^�X�N�̏�Ԃ���{�ꕶ����Ŏ擾
     */
    std::wstring GetStatusString() const {
        switch (status) {
            case TaskStatus::NOT_STARTED: return L"���J�n";
            case TaskStatus::IN_PROGRESS: return L"�i�s��";
            case TaskStatus::COMPLETED: return L"����";
            case TaskStatus::ON_HOLD: return L"�ۗ�";
            case TaskStatus::CANCELLED: return L"�L�����Z��";
            default: return L"���J�n";
        }
    }

    /**
     * @brief �^�X�N�̗D��x����{�ꕶ����Ŏ擾
     */
    std::wstring GetPriorityString() const {
        switch (priority) {
            case TaskPriority::LOW: return L"��";
            case TaskPriority::MEDIUM: return L"��";
            case TaskPriority::HIGH: return L"��";
            case TaskPriority::URGENT: return L"�ً}";
            default: return L"��";
        }
    }

    /**
     * @brief �^�X�N�̐i�������v�Z
     */
    double GetProgressPercentage() const {
        if (estimatedHours == 0.0) return 0.0;
        return (actualHours / estimatedHours) * 100.0;
    }
};

/**
 * @brief WBS�v���W�F�N�g�S�̂𓝊��Ǘ�����N���X
 * 
 * �v���W�F�N�g���x���ł̃��^�f�[�^���Ǘ����A���[�g�^�X�N��ʂ���
 * �S�Ă̊K�w�^�X�N�ւ̈ꌳ�I�ȃA�N�Z�X�|�C���g��񋟂��܂��B
 */
class WBSProject {
public:
    std::wstring projectName;                               ///< �v���W�F�N�g�̐�������
    std::wstring description;                               ///< �v���W�F�N�g�̊T�v����
    std::shared_ptr<WBSItem> rootTask;                      ///< �S�^�X�N�̍ŏ�ʃm�[�h

    /**
     * @brief �f�t�H���g�R���X�g���N�^
     */
    WBSProject() {
        projectName = L"�V�KWBS�v���W�F�N�g";
        rootTask = std::make_shared<WBSItem>(projectName);
        rootTask->id = L"1";
        rootTask->level = 0;
    }

    /**
     * @brief ���O�w��R���X�g���N�^
     */
    WBSProject(const std::wstring& name) : WBSProject() {
        projectName = name;
        rootTask->taskName = name;
    }
};