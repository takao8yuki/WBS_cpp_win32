/*
 * ============================================================================
 * WBS_XML_Functions.cpp - XML������僂�W���[��
 * ============================================================================
 * 
 * ���̃��W���[���́AWBS�v���W�F�N�g�f�[�^��XML�`���ł̉i�����@�\��񋟂��܂��B
 * ���C���A�v���P�[�V�����iWBS_cpp_win32.cpp�j����Ɨ������݌v�ɂ��A
 * �����ė��p���ƕێ琫���������Ă��܂��B
 * 
 * �y��ȋ@�\�z
 * - WBS�v���W�F�N�g��XML�V���A���C�[�[�V����
 * - XML��WBS�v���W�F�N�g�f�V���A���C�[�[�V����  
 * - �K�w�\���̊��S�ȕۑ��E����
 * - XML�G�X�P�[�v�ɂ����S�ȃe�L�X�g����
 * - UTF-8�G���R�[�f�B���O�Ή�
 * - �t�@�C��I/O��O����
 * 
 * �yXML�\���݌v�z
 * <?xml version="1.0" encoding="UTF-8"?>
 * <WBSProject>
 *   <ProjectName>�v���W�F�N�g��</ProjectName>
 *   <Description>�v���W�F�N�g����</Description>
 *   <RootTask>
 *     <Task>
 *       <ID>1</ID>
 *       <Name>���[�g�^�X�N</Name>
 *       <Description>����</Description>
 *       <AssignedTo>�S����</AssignedTo>
 *       <Status>0</Status>
 *       <Priority>1</Priority>
 *       <EstimatedHours>100.0</EstimatedHours>
 *       <ActualHours>25.0</ActualHours>
 *       <StartDate>2024-01-01T09:00:00</StartDate>
 *       <EndDate>2024-12-31T18:00:00</EndDate>
 *       <Level>0</Level>
 *       <Children>
 *         <!-- �q�^�X�N�̍ċA�I�\�� -->
 *         <Task>...</Task>
 *       </Children>
 *     </Task>
 *   </RootTask>
 * </WBSProject>
 * 
 * �y�݌v�����z
 * - �^���S��: �����^�`�F�b�N�ƓK�؂ȃL���X�g
 * - ��O���S��: ���S�ȗ�O�����ƃ��\�[�X�Ǘ�
 * - �g����: �V�����t�B�[���h�ǉ��ɑΉ��\�Ȑ݌v
 * - �݊���: �قȂ�o�[�W�����Ԃł̃f�[�^�݊���
 * 
 * �y�Z�p�d�l�z
 * - �����G���R�[�f�B���O: UTF-8 (�t�@�C��), UTF-16LE (��������)
 * - XML�W��: W3C XML 1.0����
 * - �����`��: ISO 8601���� (YYYY-MM-DDTHH:MM:SS)
 * - ���l�`��: IEEE 754�����{���x���������_
 * 
 * �쐬��: �J���`�[��
 * �쐬��: 2024�N
 * �ŏI�X�V: 2024�N
 * �o�[�W����: 1.0
 * ============================================================================
 */

// WBS XML Processing Module - �C����
// XML������僂�W���[���i�d����`���폜�j

#include <fstream>     // �t�@�C���X�g���[���i���o�͑���j
#include <sstream>     // ������X�g���[���i�����񑀍�j
#include <codecvt>     // �����R�[�h�ϊ��iUTF-8 �� UTF-16�j
#include <locale>      // ���P�[���ݒ�i���ۉ��Ή��j
#include <windows.h>   // Windows��{API
#include <commdlg.h>   // �R�����_�C�A���OAPI�i�t�@�C���I���j
#include <shlobj.h>    // �V�F���I�u�W�F�N�gAPI�i�t�H���_����j
#include <memory>      // �X�}�[�g�|�C���^�i���������S���j
#include <vector>      // ���I�z��i�K�w�f�[�^�Ǘ��j
#include <string>      // ������N���X�i�e�L�X�g�����j

// ============================================================================
// �O���ˑ��֌W - ���C���A�v���P�[�V�����Ƃ̘A�g
// ============================================================================

// �O���ϐ��F���C���A�v���P�[�V�����Œ�`����Ă���O���[�o�����
extern std::unique_ptr<WBSProject> g_currentProject;   // ���݂̃v���W�F�N�g�C���X�^���X
extern void RefreshTreeView();                         // UI�X�V�F�c���[�r���[�̍ĕ`��
extern void RefreshListView();                         // UI�X�V�F�ڍ׃r���[�̍ĕ`��
extern void SaveLastOpenedFile(const std::wstring& filePath);  // �ݒ�ۑ��F�Ō�ɊJ�����t�@�C��

// ============================================================================
// XML�������[�e�B���e�B�֐��Q
// ============================================================================

/**
 * @brief XML���ꕶ�����G�X�P�[�v����
 * 
 * @param text �G�X�P�[�v�Ώۂ̕�����
 * @return �G�X�P�[�v�ςݕ�����
 * 
 * XML�h�L�������g���œ��ʂȈӖ������������A�Ή�����XML�G���e�B�e�B�ɕϊ����܂��B
 * ����ɂ��A�C�ӂ̃��[�U�[�e�L�X�g�����S��XML�����ɖ��ߍ��ނ��Ƃ��ł��܂��B
 * 
 * @details �ϊ��d�l:
 * - & (�A���p�T���h) �� &amp;  : XML�G���e�B�e�B�̊J�n����
 * - < (���Ȃ�) �� &lt;        : XML�^�O�̊J�n��h��
 * - > (��Ȃ�) �� &gt;        : XML�^�O�̏I����h��  
 * - " (�_�u���N�H�[�g) �� &quot; : XML�����l�̋�؂蕶����h��
 * - ' (�V���O���N�H�[�g) �� &apos; : XML�����l�̋�؂蕶����h��
 * 
 * @note ���̏����́A���[�U�[�����͂����^�X�N�����������
 *       XML�\����j�󂷂镶�����܂܂�Ă��Ă����S�ɕۑ��ł���悤�ɂ��܂�
 * 
 * @example 
 * XmlEscape(L"<�d�v> A&B �v���W�F�N�g \"�ً}\"") 
 * �� L"&lt;�d�v&gt; A&amp;B �v���W�F�N�g &quot;�ً}&quot;"
 */
std::wstring XmlEscape(const std::wstring& text) {
    std::wstring result;
    result.reserve(text.length() * 1.2); // �p�t�H�[�}���X�œK���F�\�z�T�C�Y�ŗ\��
    
    for (wchar_t c : text) {
        switch (c) {
            case L'&': result += L"&amp;"; break;   // �ŏ��ɏ����i���̃G���e�B�e�B�Ƃ̋�������j
            case L'<': result += L"&lt;"; break;    // XML�^�O�J�n����
            case L'>': result += L"&gt;"; break;    // XML�^�O�I������
            case L'"': result += L"&quot;"; break;  // XML�����l�̋�؂蕶��
            case L'\'': result += L"&apos;"; break; // XML�����l�̋�؂蕶���i��ցj
            default: result += c; break;            // �ʏ핶���͂��̂܂�
        }
    }
    return result;
}

/**
 * @brief XML�G���e�B�e�B���A���G�X�P�[�v����
 * 
 * @param text �A���G�X�P�[�v�Ώۂ̕�����
 * @return �A���G�X�P�[�v�ςݕ�����
 * 
 * XML��������ǂݍ��񂾃e�L�X�g�Ɋ܂܂��XML�G���e�B�e�B���A
 * ���̕����ɕ������܂��BXmlEscape()�̋t�ϊ������ł��B
 * 
 * @details �ϊ��d�l�i�G�X�P�[�v�̋t���ŏ����j:
 * - &amp; �� &   : �Ō�ɏ����i���̃G���e�B�e�B�Ƃ̋�������j
 * - &lt; �� <    : XML�^�O�J�n�����̕���
 * - &gt; �� >    : XML�^�O�I�������̕���
 * - &quot; �� "  : �_�u���N�H�[�g�̕���
 * - &apos; �� '  : �V���O���N�H�[�g�̕���
 * 
 * @warning �����������d�v�F&amp;���Ō�ɏ������Ȃ��ƁA
 *          ���̃G���e�B�e�B��&����������ĕϊ������
 * 
 * @note �p�t�H�[�}���X�l���F�e�G���e�B�e�B���Ƃɕ�����S�̂𑖍�
 *       �p�ɂȃt�@�C���ǂݍ��݂łȂ�����A���p����Ȃ�
 */
std::wstring XmlUnescape(const std::wstring& text) {
    std::wstring result = text;
    size_t pos = 0;
    
    // &lt; �� < �̕ϊ�
    while ((pos = result.find(L"&lt;", pos)) != std::wstring::npos) {
        result.replace(pos, 4, L"<");
        pos += 1;
    }
    pos = 0;
    
    // &gt; �� > �̕ϊ�
    while ((pos = result.find(L"&gt;", pos)) != std::wstring::npos) {
        result.replace(pos, 4, L">");
        pos += 1;
    }
    pos = 0;
    
    // &quot; �� " �̕ϊ�
    while ((pos = result.find(L"&quot;", pos)) != std::wstring::npos) {
        result.replace(pos, 6, L"\"");
        pos += 1;
    }
    pos = 0;
    
    // &apos; �� ' �̕ϊ�
    while ((pos = result.find(L"&apos;", pos)) != std::wstring::npos) {
        result.replace(pos, 6, L"'");
        pos += 1;
    }
    pos = 0;
    
    // &amp; �� & �̕ϊ��i�Ō�Ɏ��s�j
    while ((pos = result.find(L"&amp;", pos)) != std::wstring::npos) {
        result.replace(pos, 5, L"&");
        pos += 1;
    }
    
    return result;
}

// ============================================================================
// �����ϊ����[�e�B���e�B�֐��Q
// ============================================================================

/**
 * @brief SYSTEMTIME�\���̂�ISO 8601������ɕϊ�
 * 
 * @param st �ϊ��Ώۂ�SYSTEMTIME�\����
 * @return ISO 8601�`���̓��������� (YYYY-MM-DDTHH:MM:SS)
 * 
 * Windows API��SYSTEMTIME�\���̂��A���ەW���̓���������`���ɕϊ����܂��B
 * XML�ۑ����ɁA�^�X�N�̊J�n���E�I������W���`���ŋL�^���邽�߂Ɏg�p���܂��B
 * 
 * @details �o�͌`��:
 * - �N: 4���i��F2024�j
 * - ��: 2���i��F01, 12�j
 * - ��: 2���i��F01, 31�j  
 * - ��؂蕶��: T�iISO 8601�����j
 * - ��: 2���i��F00, 23�j
 * - ��: 2���i��F00, 59�j
 * - �b: 2���i��F00, 59�j
 * - �~���b: �ȗ��i�v���W�F�N�g�Ǘ��ł͕s�v�j
 * 
 * @note �^�C���]�[�����͊܂܂�܂���i���[�J�������Ƃ��Ĉ����j
 * 
 * @example SystemTimeToString({2024, 12, 3, 25, 14, 30, 45, 0})
 *          �� L"2024-12-25T14:30:45"
 */
std::wstring SystemTimeToString(const SYSTEMTIME& st) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"%04d-%02d-%02dT%02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buffer);
}

/**
 * @brief ISO 8601�������SYSTEMTIME�\���̂ɕϊ�
 * 
 * @param str �ϊ��Ώۂ�ISO 8601�`��������
 * @return �ϊ����ꂽSYSTEMTIME�\����
 * 
 * XML��������ǂݍ��񂾓�����������AWindows API�Ŏg�p�\��
 * SYSTEMTIME�\���̂ɕϊ����܂��BSystemTimeToString()�̋t�ϊ������ł��B
 * 
 * @details ���͌`���v��:
 * - �ŏ���: 19���� ("YYYY-MM-DDTHH:MM:SS")
 * - ��؂蕶��: �n�C�t���AT�A�R�������������ʒu�ɂ��邱��
 * - ���l�͈�: �e�t�B�[���h���L���Ȕ͈͓��ł��邱��
 * 
 * @note �G���[����:
 * - �����񂪒Z������ꍇ: ���݂̃V�X�e��������Ԃ�
 * - �p�[�X�G���[�̏ꍇ: ���݂̃V�X�e��������Ԃ��i�t�H�[���o�b�N�j
 * - �����ȓ��t�̏ꍇ: ���̂܂ܐݒ�i�Ăяo�����Ō��؁j
 * 
 * @warning wDayOfWeek�t�B�[���h�͐ݒ肳��܂���i�V�X�e���������v�Z�j
 */
SYSTEMTIME StringToSystemTime(const std::wstring& str) {
    SYSTEMTIME st = {};
    
    if (str.length() >= 19) {
        // ISO 8601�`���̃p�[�X: "YYYY-MM-DDTHH:MM:SS"
        swscanf_s(str.c_str(), L"%04hu-%02hu-%02huT%02hu:%02hu:%02hu",
            &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
        
        // wDayOfWeek�͐ݒ肵�Ȃ��iWindows�������v�Z�j
        st.wMilliseconds = 0; // �~���b�͏��0
    } else {
        // �p�[�X�G���[���̃t�H�[���o�b�N�F���ݎ�����Ԃ�
        GetSystemTime(&st);
    }
    
    return st;
}

// ============================================================================
// XML��̓��[�e�B���e�B�֐��Q  
// ============================================================================

/**
 * @brief XML��������w��^�O�̒l�𒊏o
 * 
 * @param xml �����Ώۂ�XML������
 * @param tag ���o�Ώۂ̃^�O���i�J�n�E�I���^�O�̖��O�����̂݁j
 * @param startPos �����J�n�ʒu�i�f�t�H���g: 0�j
 * @return ���o���ꂽ�l�i�G�X�P�[�v�����ς݁j�A������Ȃ��ꍇ�͋󕶎���
 * 
 * XML��������P��̗v�f�l�𒊏o����ėp�֐��ł��B
 * �K�w�I��XML�p�[�X�̊�{�I�ȍ\���v�f�Ƃ��Ďg�p����܂��B
 * 
 * @details �����t���[:
 * 1. �J�n�^�O "<tag>" ������
 * 2. �Ή�����I���^�O "</tag>" ������  
 * 3. �^�O�Ԃ̃e�L�X�g�𒊏o
 * 4. XmlUnescape() �ŃG���e�B�e�B�𕜌�
 * 5. ���ʕ������Ԃ�
 * 
 * @note �l�X�g�����^�O�͍l�����܂���i�ŏ��Ɍ��������I���^�O���g�p�j
 *       ���G�ȃl�X�g�\���̏ꍇ�́A��p�̃p�[�T�[�֐����K�v
 * 
 * @warning �^�O���ɋ󔒕�������ꕶ�����܂܂�Ă��Ă͂����܂���
 * 
 * @example 
 * ExtractXmlValue(L"<Name>�v���W�F�N�g</Name>", L"Name") �� L"�v���W�F�N�g"
 * ExtractXmlValue(L"<Data><Name>�l</Name></Data>", L"Name") �� L"�l"
 */
std::wstring ExtractXmlValue(const std::wstring& xml, const std::wstring& tag, size_t startPos) {
    // �J�n�^�O�ƏI���^�O���\�z
    std::wstring startTag = L"<" + tag + L">";
    std::wstring endTag = L"</" + tag + L">";
    
    // �J�n�^�O�̈ʒu������
    size_t start = xml.find(startTag, startPos);
    if (start == std::wstring::npos) {
        return L""; // �J�n�^�O��������Ȃ�
    }
    
    // �e�L�X�g�����̊J�n�ʒu���v�Z
    start += startTag.length();
    
    // �I���^�O�̈ʒu������
    size_t end = xml.find(endTag, start);
    if (end == std::wstring::npos) {
        return L""; // �I���^�O��������Ȃ��i�\���G���[�j
    }
    
    // �^�O�Ԃ̃e�L�X�g�𒊏o���ăG�X�P�[�v����
    return XmlUnescape(xml.substr(start, end - start));
}

// ============================================================================
// WBS�f�[�^�V���A���C�[�[�V�����֐��Q
// ============================================================================

/**
 * @brief WBS�A�C�e����XML�`���ɕϊ��i�ċA�I�j
 * 
 * @param item �ϊ��Ώۂ�WBS�A�C�e��
 * @param indent �C���f���g���x���i���`�p�A�f�t�H���g: 0�j
 * @return XML�`���̕�����
 * 
 * �P���WBS�A�C�e���Ƃ��̑S�Ă̎q�v�f���A�\�������ꂽXML�`���ɕϊ����܂��B
 * �K�w�\�����ێ����邽�߁A�ċA�I�Ɏq�v�f����������܂��B
 * 
 * @details �o��XML�\��:
 * <Task>
 *   <ID>�^�X�NID</ID>
 *   <Name>�^�X�N��</Name>
 *   <Description>����</Description>
 *   <AssignedTo>�S����</AssignedTo>
 *   <Status>��Ԓl</Status>
 *   <Priority>�D��x�l</Priority>
 *   <EstimatedHours>���ς���H��</EstimatedHours>
 *   <ActualHours>���эH��</ActualHours>
 *   <StartDate>�J�n��</StartDate>
 *   <EndDate>�I����</EndDate>
 *   <Level>�K�w���x��</Level>
 *   <Children>
 *     <!-- �q�^�X�N���ċA�I�ɓW�J����� -->
 *   </Children>
 * </Task>
 * 
 * @note �p�t�H�[�}���X�l��:
 * - ������A���̍œK���istd::wstring �� += ���Z�q�j
 * - �C���f���g�p�󔒂̎��O�v�Z
 * - �q�v�f�̗L���`�F�b�N�ɂ��s�v�ȃ^�O���
 * 
 * @param indent �C���f���g���x���i2���� �~ ���x�����̋󔒂�}���j
 *               �ǐ��̍���XML�t�@�C���𐶐����邽��
 */
std::wstring WBSItemToXml(std::shared_ptr<WBSItem> item, int indent) {
    if (!item) {
        return L""; // nullptr�`�F�b�N�F���S���m��
    }
    
    // �C���f���g������𐶐��i�ǐ�����j
    std::wstring indentStr(indent * 2, L' ');
    std::wstring xml;
    
    // �^�X�N�J�n�^�O
    xml += indentStr + L"<Task>\n";
    
    // ��{�t�B�[���h�̃V���A���C�[�[�V����
    xml += indentStr + L"  <ID>" + XmlEscape(item->id) + L"</ID>\n";
    xml += indentStr + L"  <Name>" + XmlEscape(item->taskName) + L"</Name>\n";
    xml += indentStr + L"  <Description>" + XmlEscape(item->description) + L"</Description>\n";
    xml += indentStr + L"  <AssignedTo>" + XmlEscape(item->assignedTo) + L"</AssignedTo>\n";
    
    // �񋓌^�̐��l�ϊ��i�^���S���ƍ��ۉ��Ή��j
    xml += indentStr + L"  <Status>" + std::to_wstring((int)item->status) + L"</Status>\n";
    xml += indentStr + L"  <Priority>" + std::to_wstring((int)item->priority) + L"</Priority>\n";
    
    // ���������_���l�̕ϊ�
    xml += indentStr + L"  <EstimatedHours>" + std::to_wstring(item->estimatedHours) + L"</EstimatedHours>\n";
    xml += indentStr + L"  <ActualHours>" + std::to_wstring(item->actualHours) + L"</ActualHours>\n";
    
    // �����f�[�^�̕ϊ�
    xml += indentStr + L"  <StartDate>" + SystemTimeToString(item->startDate) + L"</StartDate>\n";
    xml += indentStr + L"  <EndDate>" + SystemTimeToString(item->endDate) + L"</EndDate>\n";
    
    // �K�w���
    xml += indentStr + L"  <Level>" + std::to_wstring(item->level) + L"</Level>\n";
    
    // �q�v�f�̏����i�ċA�I�j
    if (!item->children.empty()) {
        xml += indentStr + L"  <Children>\n";
        
        // �S�Ă̎q�v�f���ċA�I�ɃV���A���C�Y
        for (const auto& child : item->children) {
            xml += WBSItemToXml(child, indent + 2); // �C���f���g���x���𑝉�
        }
        
        xml += indentStr + L"  </Children>\n";
    }
    
    // �^�X�N�I���^�O
    xml += indentStr + L"</Task>\n";
    
    return xml;
}

/**
 * @brief �v���W�F�N�g�S�̂�XML�����ɕϊ�
 * 
 * @return ���S��XML����������
 * 
 * ���݂�WBS�v���W�F�N�g�S�̂��AXML�錾���܂ފ��S��XML�����ɕϊ����܂��B
 * �t�@�C���ۑ����Ɏg�p�����ŏ�ʃ��x���̃V���A���C�[�[�V�����֐��ł��B
 * 
 * @details �o��XML�\��:
 * <?xml version="1.0" encoding="UTF-8"?>
 * <WBSProject>
 *   <ProjectName>�v���W�F�N�g��</ProjectName>
 *   <Description>�v���W�F�N�g����</Description>
 *   <RootTask>
 *     <!-- WBSItemToXml()�̏o�� -->
 *   </RootTask>
 * </WBSProject>
 * 
 * @note �G���[����:
 * - g_currentProject �� nullptr �̏ꍇ: �󕶎����Ԃ�
 * - �v���W�F�N�g�f�[�^���s���S�ȏꍇ: ���p�\�ȕ����݂̂��o��
 * 
 * @warning ���̊֐��͊O���ˑ��֌W�ig_currentProject�j�Ɉˑ����܂�
 *          �P�̃e�X�g���s���ꍇ�́A�K�؂ȃ��b�N���K�v
 */
std::wstring ProjectToXml() {
    if (!g_currentProject) {
        return L""; // �v���W�F�N�g�����݂��Ȃ��ꍇ�̈��S�ȏ���
    }
    
    std::wstring xml;
    
    // XML�錾�F�����G���R�[�f�B���O�̖���
    xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    
    // ���[�g�v�f�J�n
    xml += L"<WBSProject>\n";
    
    // �v���W�F�N�g���^�f�[�^
    xml += L"  <ProjectName>" + XmlEscape(g_currentProject->projectName) + L"</ProjectName>\n";
    xml += L"  <Description>" + XmlEscape(g_currentProject->description) + L"</Description>\n";
    
    // ���[�g�^�X�N�Ƃ��̊K�w�\��
    xml += L"  <RootTask>\n";
    xml += WBSItemToXml(g_currentProject->rootTask, 2); // �C���f���g���x��2����J�n
    xml += L"  </RootTask>\n";
    
    // ���[�g�v�f�I��
    xml += L"</WBSProject>\n";
    
    return xml;
}

// ============================================================================
// XML�f�V���A���C�[�[�V�����֐��Q
// ============================================================================

/**
 * @brief XML�����񂩂�WBS�A�C�e������́i�ċA�I�j
 * 
 * @param xml ��͑Ώۂ�XML������
 * @param pos ��͊J�n�ʒu�i�Q�Ɠn���F��͌�̈ʒu���ݒ肳���j
 * @return ��͂��ꂽWBS�A�C�e���A�G���[�̏ꍇ��nullptr
 * 
 * XML�����񂩂�P���<Task>�v�f����͂��A�Ή�����WBSItem�I�u�W�F�N�g���\�z���܂��B
 * �q�v�f�����݂���ꍇ�́A�ċA�I�ɏ������Ċ��S�ȊK�w�\���𕜌����܂��B
 * 
 * @details ��̓v���Z�X:
 * 1. <Task>�J�n�^�O�̌���
 * 2. </Task>�I���^�O�̌���
 * 3. �^�X�N�v�f�̒��o
 * 4. �e�t�B�[���h�̃p�[�X�ƃf�[�^�^�ϊ�
 * 5. �q�v�f�̍ċA�I���
 * 6. �e�q�֌W�̐ݒ�
 * 
 * @note �f�[�^�^�ϊ�:
 * - ������t�B�[���h: ExtractXmlValue() + XmlUnescape()
 * - �񋓌^: std::stoi() + �L���X�g
 * - ���������_: std::stod()
 * - ����: StringToSystemTime()
 * - ����: std::stoi()
 * 
 * @warning �p�t�H�[�}���X�l������:
 * - �傫�ȃv���W�F�N�g�t�@�C���̏ꍇ�A�ċA�̐[���ɒ���
 * - �������g�p�ʂ͊K�w�̐[���ƃ^�X�N���ɔ��
 * 
 * @param pos ��͈ʒu�i���o�̓p�����[�^�j
 *            ����: ��͊J�n�ʒu
 *            �o��: ���̗v�f�̉�͊J�n�ʒu
 */
std::shared_ptr<WBSItem> ParseTaskFromXml(const std::wstring& xml, size_t& pos) {
    // <Task>�J�n�^�O�̌���
    size_t taskStart = xml.find(L"<Task>", pos);
    if (taskStart == std::wstring::npos) {
        return nullptr; // �^�X�N�v�f��������Ȃ�
    }
    
    // </Task>�I���^�O�̌���
    size_t taskEnd = xml.find(L"</Task>", taskStart);
    if (taskEnd == std::wstring::npos) {
        return nullptr; // �\���G���[�F�I���^�O���Ȃ�
    }
    
    // �^�X�N�v�f�S�̂𒊏o
    std::wstring taskXml = xml.substr(taskStart, taskEnd - taskStart + 7);
    pos = taskEnd + 7; // ���̉�͈ʒu���X�V
    
    // �V����WBSItem�C���X�^���X���쐬
    auto item = std::make_shared<WBSItem>();
    
    // ��{�t�B�[���h�̉��
    item->id = ExtractXmlValue(taskXml, L"ID", 0);
    item->taskName = ExtractXmlValue(taskXml, L"Name", 0);
    item->description = ExtractXmlValue(taskXml, L"Description", 0);
    item->assignedTo = ExtractXmlValue(taskXml, L"AssignedTo", 0);
    
    // �񋓌^�t�B�[���h�̉�́i�G���[�n���h�����O�t���j
    std::wstring statusStr = ExtractXmlValue(taskXml, L"Status", 0);
    if (!statusStr.empty()) {
        item->status = (TaskStatus)std::stoi(statusStr);
    }
    
    std::wstring priorityStr = ExtractXmlValue(taskXml, L"Priority", 0);
    if (!priorityStr.empty()) {
        item->priority = (TaskPriority)std::stoi(priorityStr);
    }
    
    // ���l�t�B�[���h�̉�́i�G���[�n���h�����O�t���j
    std::wstring estimatedStr = ExtractXmlValue(taskXml, L"EstimatedHours", 0);
    if (!estimatedStr.empty()) {
        item->estimatedHours = std::stod(estimatedStr);
    }
    
    std::wstring actualStr = ExtractXmlValue(taskXml, L"ActualHours", 0);
    if (!actualStr.empty()) {
        item->actualHours = std::stod(actualStr);
    }
    
    // �����t�B�[���h�̉��
    std::wstring startDateStr = ExtractXmlValue(taskXml, L"StartDate", 0);
    if (!startDateStr.empty()) {
        item->startDate = StringToSystemTime(startDateStr);
    }
    
    std::wstring endDateStr = ExtractXmlValue(taskXml, L"EndDate", 0);
    if (!endDateStr.empty()) {
        item->endDate = StringToSystemTime(endDateStr);
    }
    
    // �K�w���x���̉��
    std::wstring levelStr = ExtractXmlValue(taskXml, L"Level", 0);
    if (!levelStr.empty()) {
        item->level = std::stoi(levelStr);
    }
    
    // �q�v�f�̍ċA�I���
    size_t childrenStart = taskXml.find(L"<Children>");
    if (childrenStart != std::wstring::npos) {
        size_t childrenEnd = taskXml.find(L"</Children>");
        if (childrenEnd != std::wstring::npos) {
            // �q�v�f�͈͂�XML�𒊏o
            std::wstring childrenXml = taskXml.substr(childrenStart + 10, childrenEnd - childrenStart - 10);
            
            // �S�Ă̎q�^�X�N���ċA�I�ɉ��
            size_t childPos = 0;
            while (true) {
                auto child = ParseTaskFromXml(childrenXml, childPos);
                if (!child) break; // �q�v�f�Ȃ��A�܂��͉�͊���
                
                // �e�q�֌W�̐ݒ�
                child->parent = item;
                item->children.push_back(child);
            }
        }
    }
    
    return item;
}

// ============================================================================
// �t�@�C��I/O����֐��Q
// ============================================================================

/**
 * @brief XML�t�@�C������v���W�F�N�g��ǂݍ���
 * 
 * @param filePath �ǂݍ��ݑΏۂ̃t�@�C���p�X
 * @return �ǂݍ��ݐ�����true�A���s��false
 * 
 * �w�肳�ꂽXML�t�@�C������WBS�v���W�F�N�g�f�[�^��ǂݍ��݁A
 * �A�v���P�[�V�����̌��݂̃v���W�F�N�g��Ԃ��X�V���܂��B
 * 
 * @details �����t���[:
 * 1. �t�@�C���I�[�v���ƃG���R�[�f�B���O�ݒ�
 * 2. XML���e�̑S�ǂݍ���
 * 3. �v���W�F�N�g���^�f�[�^�̒��o
 * 4. ���[�g�^�X�N�̍ċA�I���
 * 5. �O���[�o����Ԃ̍X�V
 * 6. UI�ĕ`��̎��s
 * 7. �ݒ�t�@�C���̍X�V
 * 8. ���[�U�[�ʒm
 * 
 * @note �G���R�[�f�B���O����:
 * - �t�@�C��: UTF-8�iXML�̕W���j
 * - ��������: UTF-16LE�iWindows�̕W���j
 * - std::codecvt_utf8 �ɂ�鎩���ϊ�
 * 
 * @warning �G���[�n���h�����O:
 * - �t�@�C���A�N�Z�X�G���[
 * - XML�\���G���[
 * - �f�[�^�^�ϊ��G���[
 * - �������s��
 * �S�Ă̗�O���L���b�`����false��Ԃ�
 */
bool LoadProjectFromFile(const std::wstring& filePath) {
    try {
        // �t�@�C���X�g���[�����J���iUnicode�Ή��j
        std::wifstream file(filePath);
        if (!file.is_open()) {
            return false; // �t�@�C���I�[�v���G���[
        }
        
        // UTF-8�G���R�[�f�B���O�̐ݒ�
        file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
        
        // �t�@�C���S�̂𕶎���Ƃ��ēǂݍ���
        std::wstring xmlContent((std::istreambuf_iterator<wchar_t>(file)),
                              std::istreambuf_iterator<wchar_t>());
        file.close();
        
        // �v���W�F�N�g���^�f�[�^�̒��o
        std::wstring projectName = ExtractXmlValue(xmlContent, L"ProjectName", 0);
        std::wstring description = ExtractXmlValue(xmlContent, L"Description", 0);
        
        // �v���W�F�N�g���̃o���f�[�V����
        if (projectName.empty()) {
            projectName = L"�ǂݍ��܂ꂽ�v���W�F�N�g"; // �t�H�[���o�b�N��
        }
        
        // �V�����v���W�F�N�g�C���X�^���X�̍쐬
        g_currentProject = std::make_unique<WBSProject>(projectName);
        g_currentProject->description = description;
        
        // ���[�g�^�X�N�̉��
        size_t rootTaskStart = xmlContent.find(L"<RootTask>");
        if (rootTaskStart != std::wstring::npos) {
            size_t rootTaskEnd = xmlContent.find(L"</RootTask>");
            if (rootTaskEnd != std::wstring::npos) {
                // ���[�g�^�X�N�͈͂�XML�𒊏o
                std::wstring rootTaskXml = xmlContent.substr(rootTaskStart + 10, rootTaskEnd - rootTaskStart - 10);
                
                // ���[�g�^�X�N�̍ċA�I���
                size_t pos = 0;
                auto loadedRootTask = ParseTaskFromXml(rootTaskXml, pos);
                
                if (loadedRootTask) {
                    // ���[�g�^�X�N�̖��O���v���W�F�N�g���Ɠ���
                    loadedRootTask->taskName = projectName;
                    g_currentProject->rootTask = loadedRootTask;
                }
            }
        }
        
        // UI��Ԃ̍X�V
        RefreshTreeView();  // �c���[�r���[�̍ĕ`��
        RefreshListView();  // �ڍ׃r���[�̍ĕ`��
        
        // �A�v���P�[�V�����ݒ�̍X�V
        SaveLastOpenedFile(filePath);
        
        // ���[�U�[�ւ̐����ʒm
        MessageBox(nullptr, L"�v���W�F�N�g��XML�t�@�C������ǂݍ��݂܂����B", L"���", MB_OK | MB_ICONINFORMATION);
        
        return true;
        
    } catch (...) {
        // �S�Ă̗�O���L���b�`�i�t�@�C��I/O�AXML��́A�������s�����j
        MessageBox(nullptr, L"�v���W�F�N�g�̓ǂݍ��ݒ��ɃG���[���������܂����B", L"�G���[", MB_OK | MB_ICONERROR);
        return false;
    }
}

/**
 * @brief �v���W�F�N�g��XML�t�@�C���ɕۑ�
 * 
 * �t�@�C���I���_�C�A���O��\�����A���[�U�[���I�������ꏊ��
 * ���݂̃v���W�F�N�g��XML�`���ŕۑ����܂��B
 * 
 * @details �����t���[:
 * 1. �v���W�F�N�g���݃`�F�b�N
 * 2. �t�@�C���ۑ��_�C�A���O�̕\��
 * 3. XML�ϊ�����
 * 4. �t�@�C���������݁iUTF-8�G���R�[�f�B���O�j
 * 5. �ݒ�t�@�C���̍X�V
 * 6. ���[�U�[�ʒm
 * 
 * @note �_�C�A���O�ݒ�:
 * - �t�@�C���t�B���^: "*.xml" ����� "*.*"
 * - �f�t�H���g�g���q: ".xml"
 * - �㏑���m�F: �L��
 * - �p�X����: �L��
 */
void OnSaveProject() {
    // �v���W�F�N�g���݃`�F�b�N
    if (!g_currentProject) {
        MessageBox(nullptr, L"�ۑ�����v���W�F�N�g������܂���B", L"�G���[", MB_OK | MB_ICONWARNING);
        return;
    }

    // �t�@�C���ۑ��_�C�A���O�̐ݒ�
    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L""; // �t�@�C���p�X�o�b�t�@
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XML�t�@�C��\0*.xml\0���ׂẴt�@�C��\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // �p�X���؂Ə㏑���m�F
    ofn.lpstrDefExt = L"xml"; // �f�t�H���g�g���q

    // �t�@�C���ۑ��_�C�A���O�̕\��
    if (GetSaveFileName(&ofn) == TRUE) {
        try {
            // �t�@�C���X�g���[�����J��
            std::wofstream file(szFile);
            if (file.is_open()) {
                // UTF-8�G���R�[�f�B���O�̐ݒ�
                file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
                
                // �v���W�F�N�g��XML�ɕϊ�
                std::wstring xmlContent = ProjectToXml();
                
                // �t�@�C���ɏ�������
                file << xmlContent;
                file.close();
                
                // �A�v���P�[�V�����ݒ�̍X�V
                SaveLastOpenedFile(szFile);
                
                // ���[�U�[�ւ̐����ʒm
                MessageBox(nullptr, L"�v���W�F�N�g��XML�t�@�C���ɕۑ����܂����B", L"���", MB_OK | MB_ICONINFORMATION);
            } else {
                // �t�@�C���I�[�v���G���[
                MessageBox(nullptr, L"�t�@�C�����J���܂���ł����B", L"�G���[", MB_OK | MB_ICONERROR);
            }
        } catch (...) {
            // �������݃G���[�i�f�B�X�N�e�ʕs���A�����G���[���j
            MessageBox(nullptr, L"�v���W�F�N�g�̕ۑ����ɃG���[���������܂����B", L"�G���[", MB_OK | MB_ICONERROR);
        }
    }
}

/**
 * @brief �v���W�F�N�g�t�@�C�����J��
 * 
 * �t�@�C���I���_�C�A���O��\�����A���[�U�[���I������XML�t�@�C������
 * �v���W�F�N�g��ǂݍ��݂܂��B
 * 
 * @details �����t���[:
 * 1. �t�@�C���I���_�C�A���O�̕\��
 * 2. LoadProjectFromFile() �̌Ăяo��
 * 3. �G���[�n���h�����O��LoadProjectFromFile()�ɈϏ�
 * 
 * @note �_�C�A���O�ݒ�:
 * - �t�@�C���t�B���^: "*.xml" ����� "*.*"
 * - �t�@�C�����݃`�F�b�N: �L��
 * - �p�X����: �L��
 */
void OnOpenProject() {
    // �t�@�C���I���_�C�A���O�̐ݒ�
    OPENFILENAME ofn = {};
    WCHAR szFile[260] = L""; // �t�@�C���p�X�o�b�t�@
    
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"WBS XML�t�@�C��\0*.xml\0���ׂẴt�@�C��\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // �p�X�ƃt�@�C���̑��݃`�F�b�N

    // �t�@�C���I���_�C�A���O�̕\��
    if (GetOpenFileName(&ofn) == TRUE) {
        // �I�����ꂽ�t�@�C������v���W�F�N�g��ǂݍ���
        // �G���[�n���h�����O��LoadProjectFromFile()���Ŏ��s�����
        LoadProjectFromFile(szFile);
    }
}