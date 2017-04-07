#pragma once

#include "MultiLineLabelUI.h"
#include "../DuiLib/UIlib.h"
#include <functional>
#include "../resource.h"

#define ERR_REGHOTKEY                  "全局热键 %s 已被占用，无法生效\n请检查是否打开了多个播放器实例\n此提示可以被安全忽略"
#define ERR_CANNOT_OPEN_CLIP           "无法播放视频文件 %s\n文件位于 %s\n错误代码：0x%X\n文件不存在或者文件格式不受支持"
#define ERR_NO_AVAILABLE_CLIP          "没有可播放的视频文件，播放已停止\n请检查播放列表设置和媒体介质"
#define ERR_TOO_MANY_INCORRECT_CLIP    "无法播放的视频文件个数过多\n为保证程序响应，播放已停止\n请检查播放列表设置和媒体介质"
#define ERR_NO_SUPPORT_FILE            "没有找到或未能打开 %s\n%s"
#define ERR_CANNOTOPENPLAYLIST         "无法打开播放列表\n%s"
#define ERR_CANNOTREADPLAYLIST         "无法读取播放列表\n%s\n必要的值可能未定义，请参考 svplsample.txt"

#define ERR_NOT_IMPL                   "此功能尚未完成~~"

#define ERR_CANNOTLOADFONTS            "无法加载字体 %s %d"
#define ERR_MULTIINSTANCES             "%s 已经在运行中"
#define ERR_CREATEMUTEX                "创建互斥量错误"

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
		int             m_DropToY;         // 掉落到的坐标，有消息窗口被关闭时调度
		ULONGLONG       beginTime = 0;     // 第一次收到DropTimer消息的时间

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

		bool            m_ValidReset = true;   // 标志位
	};
}