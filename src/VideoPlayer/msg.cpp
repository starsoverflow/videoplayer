
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "msg.h"
#include "..\mydebug.h"
#include <string>
#include <memory>

using std::shared_ptr;

#define FadeOutTimerID      3101
#define FadeOutTime         4000
#define FadeOutTimerElapse  50               // Elapse 值越小，动画越流畅

#define WindowPaddingRight  5
#define WindowPaddingDown   5

#define DropTimerID         3102
#define DropTimerElapse     15
#define DropAcceleration    0.08

namespace SVideoPlayer
{
	CMessageWindow* CMessageWindow::pBegin = nullptr;
	CMessageWindow* CMessageWindow::pEnd = nullptr;

	__inline HRESULT myStringCchVPrintfW(
		_Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPWSTR pszDest,
		_In_ size_t cchDest,
		_In_ _Printf_format_string_ STRSAFE_LPCWSTR pszFormat,
		...)
	{
		HRESULT _Result;
		va_list _ArgList;
		__crt_va_start(_ArgList, pszFormat);
		_Result = StringCchVPrintfW(pszDest, cchDest, pszFormat, _ArgList);
		__crt_va_end(_ArgList);
		return _Result;
	}

	CMessageWindow::CMessageWindow(LPCTSTR pszXMLPath)
		: m_strXMLPath(pszXMLPath)
	{

	}

	CMessageWindow::~CMessageWindow()
	{

	}

	LPCTSTR CMessageWindow::GetWindowClassName() const
	{
		return _T("Star Video Player_Message Box");
	}

	CDuiString CMessageWindow::GetSkinFile()
	{
		return m_strXMLPath;
	}

	CDuiString CMessageWindow::GetSkinFolder()
	{
		return _T("Skin");
	}

	void CMessageWindow::InitWindow()
	{
		m_contentContainer = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(L"contentContainer"));
		m_img_info = static_cast<CControlUI*>(m_PaintManager.FindControl(L"img_info"));
		m_img_exclamation = static_cast<CControlUI*>(m_PaintManager.FindControl(L"img_exclamation"));
		m_img_question = static_cast<CControlUI*>(m_PaintManager.FindControl(L"img_question"));
		m_title_bar = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(L"title_bar"));
		m_buttonContainer = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(L"buttonContainer"));

		CLabelUI* label_title = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"title"));
		label_title->SetText(m_Title);

		if (m_ButtonText2[0] != L'\0')
		{
			CButtonUI* button2 = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"button2"));
			button2->SetText(m_ButtonText2);
			m_buttonContainer->SetVisible(true);

			CButtonUI* button1 = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"button1"));
			if (m_ButtonText1[0] != L'\0') button1->SetText(m_ButtonText1);
			else button1->SetVisible(false);
		}

		CDuiRect rc;
		GetWindowRect(m_hWnd, &rc);
		m_content = new CMultiLineLabelUI();
		m_content->SetFixedWidth(rc.GetWidth() - m_contentContainer->GetPadding().left - m_contentContainer->GetPadding().right);
		m_content->SetTextColor(0xffbfbfbf);
		m_contentContainer->Add(m_content);

		MONITORINFO oMonitor = {};
		oMonitor.cbSize = sizeof(oMonitor);
		::GetMonitorInfo(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &oMonitor);
		RECT rcArea = oMonitor.rcWork;

		m_content->SetText(m_Message);
		int windowHeight = m_content->EstimateSize({ m_contentContainer->GetPadding().left
													 + m_contentContainer->GetPadding().right, 0 }).cy
			+ m_content->GetPadding().top + m_content->GetPadding().bottom
			+ m_title_bar->GetPadding().top + m_title_bar->GetPadding().bottom + m_title_bar->GetFixedHeight()
			+ m_contentContainer->GetPadding().top + m_contentContainer->GetPadding().bottom;
		if (m_buttonContainer->IsVisible()) windowHeight +=
			+m_buttonContainer->GetPadding().top + m_buttonContainer->GetPadding().bottom + m_buttonContainer->GetFixedHeight();

		m_wndHeight = windowHeight;

		switch (m_Icon)
		{
		case MsgIcon::Information:
			m_img_info->SetVisible(true);
			break;
		case MsgIcon::Exclamation:
			m_img_exclamation->SetVisible(true);
			break;
		case MsgIcon::Question:
			m_img_question->SetVisible(true);
			break;
		}

		int pos_y = rcArea.bottom - windowHeight - WindowPaddingDown;

		auto wnd_iterator = pBegin;
		while (wnd_iterator)
		{
			pos_y -= wnd_iterator->m_wndHeight + WindowPaddingDown;
			wnd_iterator = wnd_iterator->pNext;
		}

		SetWindowPos(m_hWnd, HWND_TOPMOST, rcArea.right - rc.GetWidth() - WindowPaddingRight,
			pos_y, rc.GetWidth(), windowHeight, SWP_SHOWWINDOW | SWP_NOACTIVATE);

		if (!m_bResidentWindow)
		{
			this->m_PaintManager.SetTransparent(255);
			m_FadeOutBeginTime = GetTickCount64();
			SetTimer(m_hWnd, FadeOutTimerID, FadeOutTimerElapse, nullptr);
		}

		// Add to linked list
		if (pBegin == nullptr) pBegin = pEnd = this;
		else
		{
			this->pPrevious = pEnd;
			pEnd->pNext = this;
			pEnd = this;
		}

		m_DropToY = pos_y;
	}

	void CMessageWindow::Notify(TNotifyUI& msg)
	{
		if (msg.sType == _T("click"))
		{
			if (msg.pSender->GetName() == _T("button1"))
			{
				CloseMessageWindow(1);
			}
			else if (msg.pSender->GetName() == _T("button2"))
			{
				CloseMessageWindow(2);
			}
			else if (msg.pSender->GetName() == _T("button_close"))
			{
				CloseMessageWindow(0);
			}
		}
		else
		{
			__super::Notify(msg);
		}
	}

	CControlUI* CMessageWindow::CreateControl(LPCTSTR pstrClassName)
	{
		return NULL;
	}

	LRESULT CMessageWindow::CloseMessageWindow(int retValue)
	{
		auto wnd_iterator_r = pEnd;
		while (wnd_iterator_r != this)
		{
			wnd_iterator_r->m_DropToY += this->m_wndHeight + WindowPaddingDown;
			SetTimer(wnd_iterator_r->GetHWND(), DropTimerID, DropTimerElapse, nullptr);
			// _trace(L"DropToY of window: %d\n", wnd_iterator_r->m_DropToY);
			wnd_iterator_r = wnd_iterator_r->pPrevious;
		}

		// Delete this in linked list.
		if (pBegin == this) pBegin = this->pNext;
		if (pEnd == this) pEnd = this->pPrevious;
		if (pPrevious) pPrevious->pNext = this->pNext;
		if (pNext) pNext->pPrevious = this->pPrevious;
		
		if (m_callback) m_callback(retValue);
		Close();

		return 0;
	}

	LRESULT CMessageWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled = FALSE;
		if (uMsg == WM_TIMER && (UINT)wParam == FadeOutTimerID)
		{
			bHandled = TRUE;
			if (GetTickCount64() > m_FadeOutBeginTime && GetTickCount64() - m_FadeOutBeginTime > m_BeforeFadeOutDuration)
			{
				int opacity = this->m_PaintManager.GetTransparent();
				if (opacity <= 0)
				{
					KillTimer(m_hWnd, FadeOutTimerID);
					KillTimer(m_hWnd, DropTimerID);
					CloseMessageWindow(0);
				}
				else
				{
					opacity -= int((double)FadeOutTimerElapse / (double)FadeOutTime * 256.0);
					// Speed up.
					if (opacity < 100) opacity -= int((double)FadeOutTimerElapse / (double)FadeOutTime * 256.0 / 2);
					if (opacity < 0) opacity = 0;
					// _trace(L"Opacity: %d\n", opacity);
					this->m_PaintManager.SetTransparent(opacity);
				}
			}
		}
		else if (uMsg == WM_TIMER && (UINT)wParam == DropTimerID)
		{
			bHandled = TRUE;
			RECT rc;
			GetWindowRect(m_hWnd, &rc);
			if (m_DropToY > rc.top)
			{
				if (beginTime == 0)
				{
					beginTime = GetTickCount64();
				}
				else
				{
					// int v0 = int((lastTimerTime - beginTime) * DropAcceleration * 1.0);
					// _trace(L"v0: %d\n", v0);
					int v1 = int((GetTickCount64() - beginTime) * DropAcceleration * 1.0);
					// _trace(L"v1: %d\n", v1);
					// int y_to_move = int((v1*v1 - v0*v0) / DropAcceleration / 124.0);
					int y_to_move = int(v1 * v1 / DropAcceleration / 124.0);
					if (y_to_move <= 0) y_to_move = 1;
					// _trace(L"y_to_move: %d\n", y_to_move);
					if (rc.top + y_to_move > m_DropToY)
					{
						SetWindowPos(m_hWnd, nullptr, rc.left, m_DropToY, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER);
					}
					else
					{
						SetWindowPos(m_hWnd, nullptr, rc.left, rc.top + y_to_move, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER);
					}
				}
			}
			else
			{
				KillTimer(m_hWnd, DropTimerID);
				beginTime = 0;
				// lastTimerTime = 0;
			}
		}
		else if (!m_bResidentWindow && uMsg == WM_MOUSEHOVER)
		{
			m_FadeOutBeginTime = MAXULONGLONG;
			this->m_PaintManager.SetTransparent(255);
			m_ValidReset = true;
		}
		else if (!m_bResidentWindow && uMsg == WM_MOUSEMOVE)
		{
			if (m_ValidReset)
			{
				m_FadeOutBeginTime = MAXULONGLONG;
				this->m_PaintManager.SetTransparent(255);
			}
			m_ValidReset = true;
		}
		else if (!m_bResidentWindow && uMsg == WM_MOUSELEAVE)
		{
			m_ValidReset = false;
			m_FadeOutBeginTime = GetTickCount64();
		}
		return 0;
	}

	void CMessageWindow::ShowMessage(ULONGLONG Duration, MsgIcon Icon, std::function<void(int)> Callback, LPCTSTR ButtonText1, LPCTSTR ButtonText2, LPCTSTR szFormat, ...)
	{
		CMessageWindow* messageWindow = new CMessageWindow(_T("Message.xml"));

		va_list pArgs;
		va_start(pArgs, szFormat);
		StringCchVPrintf(messageWindow->m_Message, MAX_MESSAGE_LENGTH, szFormat, pArgs);
		va_end(pArgs);

		SYSTEMTIME timenow;
		GetLocalTime(&timenow);

		const wchar_t* titlecontent = L"";
		switch (Icon)
		{
		case MsgIcon::Information:
			titlecontent = L"Video Player 提示";
			break;
		case MsgIcon::Exclamation:
			titlecontent = L"Video Player 警告";
			break;
		case MsgIcon::Question:
			titlecontent = L"Video Player 询问";
			break;
		default:
			titlecontent = L"Video Player";
		}

		myStringCchVPrintfW(messageWindow->m_Title, MAX_MESSAGE_LENGTH, _T("[%02u:%02u:%02u] %s"), timenow.wHour, timenow.wMinute, timenow.wSecond, titlecontent);
		messageWindow->m_Icon = Icon;

		if (ButtonText1) StringCchCopy(messageWindow->m_ButtonText1, MAX_MESSAGE_LENGTH, ButtonText1);
		if (ButtonText2) StringCchCopy(messageWindow->m_ButtonText2, MAX_MESSAGE_LENGTH, ButtonText2);

		messageWindow->m_bResidentWindow = Duration == 0;
		messageWindow->m_callback = Callback;
		messageWindow->m_BeforeFadeOutDuration = Duration;
		messageWindow->Create(nullptr, _T("Video Player Message Window"), WS_POPUP, WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST);

	}

	void CMessageWindow::OnFinalMessage(HWND hWnd)
	{
		delete this;
	}
}