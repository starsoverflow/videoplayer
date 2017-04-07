#pragma once

#include "MultiLineLabelUI.h"
#include "../DuiLib/UIlib.h"
#include <functional>
#include "../resource.h"

#define ERR_REGHOTKEY                  "ȫ���ȼ� %s �ѱ�ռ�ã��޷���Ч\n�����Ƿ���˶��������ʵ��\n����ʾ���Ա���ȫ����"
#define ERR_CANNOT_OPEN_CLIP           "�޷�������Ƶ�ļ� %s\n�ļ�λ�� %s\n������룺0x%X\n�ļ������ڻ����ļ���ʽ����֧��"
#define ERR_NO_AVAILABLE_CLIP          "û�пɲ��ŵ���Ƶ�ļ���������ֹͣ\n���鲥���б����ú�ý�����"
#define ERR_TOO_MANY_INCORRECT_CLIP    "�޷����ŵ���Ƶ�ļ���������\nΪ��֤������Ӧ��������ֹͣ\n���鲥���б����ú�ý�����"
#define ERR_NO_SUPPORT_FILE            "û���ҵ���δ�ܴ� %s\n%s"
#define ERR_CANNOTOPENPLAYLIST         "�޷��򿪲����б�\n%s"
#define ERR_CANNOTREADPLAYLIST         "�޷���ȡ�����б�\n%s\n��Ҫ��ֵ����δ���壬��ο� svplsample.txt"

#define ERR_NOT_IMPL                   "�˹�����δ���~~"

#define ERR_CANNOTLOADFONTS            "�޷��������� %s %d"
#define ERR_MULTIINSTANCES             "%s �Ѿ���������"
#define ERR_CREATEMUTEX                "��������������"

#define DEFAULT_BEFORE_FADEOUT_DURATION 15000

#define SHOWMSG(icon, x, ...) \
        CMessageWindow::ShowMessage(DEFAULT_BEFORE_FADEOUT_DURATION, icon, nullptr, nullptr, nullptr, _T(x), __VA_ARGS__);
// User defined duration
#define SHOWMSG_D(duration, icon, x, ...) \
        CMessageWindow::ShowMessage(duration, icon, nullptr, nullptr, nullptr, _T(x), __VA_ARGS__);
// Resident Window
#define SHOWMSG_R(icon, x, ...) \
        CMessageWindow::ShowMessage(0, icon, nullptr, nullptr, nullptr, _T(x), __VA_ARGS__);
// Window with button
#define SHOWMSG_B(icon, callback, Button1, Button2, x, ...) \
        CMessageWindow::ShowMessage(DEFAULT_BEFORE_FADEOUT_DURATION, callback, _T(Button1), _T(Button2), _T(x), __VA_ARGS__);
// Resident window with button
#define SHOWMSG_RB(icon, callback, Button1, Button2, x, ...) \
        CMessageWindow::ShowMessage(0, icon, callback, _T(Button1), _T(Button2), _T(x), __VA_ARGS__);
// User-defined-duration window with button
#define SHOWMSG_DB(duration, icon, callback, Button1, Button2, x, ...) \
        CMessageWindow::ShowMessage(duration, icon, callback, _T(Button1), _T(Button2), _T(x), __VA_ARGS__);

#define MAX_MESSAGE_LENGTH 1024

namespace SVideoPlayer
{
	using namespace DuiLib;

	enum class MsgIcon
	{
		None = 0,
		Information = 1,
		Exclamation = 2,
		Question = 3
	};

	class CMessageWindow : public WindowImplBase
	{
	public:
		static void ShowMessage(ULONGLONG Duration, MsgIcon Icon, std::function<void(int)> Callback, LPCTSTR ButtonText1, LPCTSTR ButtonText2, LPCTSTR szFormat, ...);

		static CMessageWindow* pBegin;
		static CMessageWindow* pEnd;

	private:
		CMessageWindow* pPrevious = nullptr;
		CMessageWindow* pNext = nullptr;

		explicit CMessageWindow(LPCTSTR pszXMLPath);
		~CMessageWindow();

		void InitWindow();
		void Notify(TNotifyUI& msg);
		CControlUI* CreateControl(LPCTSTR pstrClassName);
		LRESULT CloseMessageWindow(int retValue);
		LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
		void OnFinalMessage(HWND hWnd);

	protected:
		LPCTSTR GetWindowClassName() const;
		CDuiString GetSkinFile();
		CDuiString GetSkinFolder();

		UILIB_RESOURCETYPE GetResourceType() const
		{
			return UILIB_ZIPRESOURCE;
		}

		LPCTSTR GetResourceID() const
		{
			return MAKEINTRESOURCE(IDR_ZIP_SKIN);
		}

	private:
		TCHAR           m_Message[MAX_MESSAGE_LENGTH];
		TCHAR           m_Title[MAX_MESSAGE_LENGTH];
		MsgIcon         m_Icon;

		bool            m_bResidentWindow = false;
		TCHAR           m_ButtonText1[MAX_MESSAGE_LENGTH] = { 0 };
		TCHAR           m_ButtonText2[MAX_MESSAGE_LENGTH] = { 0 };

		int             m_wndHeight;
		int             m_DropToY;         // ���䵽�����꣬����Ϣ���ڱ��ر�ʱ����
		ULONGLONG       beginTime = 0;     // ��һ���յ�DropTimer��Ϣ��ʱ��

		CDuiString		m_strXMLPath;
		ULONGLONG       m_FadeOutBeginTime;
		ULONGLONG       m_BeforeFadeOutDuration;

		CLabelUI*       m_content;
		CControlUI*     m_img_info;
		CControlUI*     m_img_exclamation;
		CControlUI*     m_img_question;

		CHorizontalLayoutUI* m_title_bar;
		CHorizontalLayoutUI* m_contentContainer;
		CHorizontalLayoutUI* m_buttonContainer;

		std::function<void(int)>  m_callback;

		bool            m_ValidReset = true;   // ��־λ
	};
}