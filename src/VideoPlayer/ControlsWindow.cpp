
#include "ControlsWindow.h"
#include "VideoWindow.h"
#include "PlaylistWindow.h"
#include "DragDrop.h"

namespace Star_VideoPlayer
{
	CControlWindow::CControlWindow(LPCTSTR pszXMLPath, CVideoWindow* BindingVideoWindow)
		: m_strXMLPath(pszXMLPath), m_cVideo(BindingVideoWindow)
	{
		m_hVideo = BindingVideoWindow->GetHWND();
		m_cPlaylist = new CPlayListWindow(BindingVideoWindow);
		m_cPlaylist->Create(m_hVideo, _T("PlayList Window"), WS_POPUP, WS_EX_TOOLWINDOW);

		plwnd_DragDrop = new vp_DragDrop();
		plwnd_DragDrop->DragDropRegister(m_cPlaylist);
	}

	CControlWindow::~CControlWindow()
	{
		
	}

	LPCTSTR CControlWindow::GetWindowClassName() const
	{
		return _T("Star Video Player_Control Window");
	}

	CDuiString CControlWindow::GetSkinFile()
	{
		return m_strXMLPath;
	}

	CDuiString CControlWindow::GetSkinFolder()
	{
		return _T("Skin");
	}

	UILIB_RESOURCETYPE CControlWindow::GetResourceType() const
	{
	 	return UILIB_ZIPRESOURCE;
	}
	 
 	LPCTSTR CControlWindow::GetResourceID() const
 	{
 		return MAKEINTRESOURCE(IDR_ZIP_SKIN);
 	}

	void CControlWindow::Hide(bool hide /* = true */)
	{
		if (!hide) {
			if (!m_barControl->IsVisible()) {
				m_barControl->SetVisible(true);
				m_barTitle->SetVisible(true);
				if (IsWindowVisible(m_cPlaylist->GetHWND())) SetActiveWindow(m_cPlaylist->GetHWND());
			}
		}
		else {
			if (m_barControl->IsVisible()) {
				m_barControl->SetVisible(false);
				m_barTitle->SetVisible(false);
			}
		}
	}

	void CControlWindow::InitWindow()
	{
		m_SliderPlayProcess = static_cast<CSliderUI*>(m_PaintManager.FindControl(L"slider_play_process"));
		m_SliderVolume = static_cast<CSliderUI*>(m_PaintManager.FindControl(L"slider_volume"));
		m_btnPlay = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"button_play"));
		m_btnPause = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"button_pause"));
		m_barTitle = static_cast<CContainerUI*>(m_PaintManager.FindControl(L"bar_title"));
		m_barControl = static_cast<CContainerUI*>(m_PaintManager.FindControl(L"bar_control"));
		m_lblTime = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"label_time"));
		m_alwaystop_true = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"button_alwaystop_true"));
		m_alwaystop_false = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"button_alwaystop_false"));
		m_lblTitle = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"label_title"));
		m_btnMin = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"button_min"));
	}

	void CControlWindow::OnPrepare(TNotifyUI& msg)
	{

	}

	void CControlWindow::ReleaseCapture()
	{
		if ((m_SliderPlayProcess->m_uButtonState & UISTATE_CAPTURED) != 0) {
			m_PaintManager.ReleaseCapture();
			m_SliderPlayProcess->m_uButtonState &= ~UISTATE_CAPTURED;
		}
	}

	bool CControlWindow::IsCaptured()
	{
		return m_PaintManager.IsCaptured();
	}

	void CControlWindow::Notify(TNotifyUI& msg)
	{
		if (msg.sType == _T("windowinit"))
		{
			OnPrepare(msg);
		}
		else if (msg.sType == _T("click"))
		{
			if (msg.pSender->GetName() == _T("button_play") || msg.pSender->GetName() == _T("button_pause"))
			{
				m_cVideo->PauseClip();
			}
			else if (msg.pSender->GetName() == _T("button_min"))
			{
				// 强制重绘，以避免控件窗口留在图像缓存中。
				this->Hide();
				LRESULT lr;
				m_PaintManager.MessageHandler(WM_PAINT, 0, 0, lr);
				::PostMessage(m_hVideo, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}
			else if (msg.pSender->GetName() == _T("button_close"))
			{
				::PostMessage(m_hVideo, WM_CLOSE, 0, 0);
			}
			else if (msg.pSender->GetName() == _T("button_next"))
			{
				m_cVideo->PlayNext();
			}
			else if (msg.pSender->GetName() == _T("button_prev"))
			{
				m_cVideo->PlayPrev();
			}
			else if (msg.pSender->GetName() == _T("button_reload"))
			{
				//m_cVideo->ReloadPlaylist(file_playlist);
			}
			else if (msg.pSender->GetName() == _T("button_list"))
			{
				if (!IsWindowVisible(m_cPlaylist->GetHWND())) {
					if (m_cVideo->m_iPlaylistAttached != 0)
					{
						if (m_cVideo->m_iPlaylistAttached == 1 || m_cVideo->m_iPlaylistAttached == 3)
						{
							RECT rect, rectp;
							GetWindowRect(m_cVideo->GetHWND(), &rect);
							GetWindowRect(m_cPlaylist->GetHWND(), &rectp);
							int newx = rect.left - RectGetWidth(rectp);
							m_cVideo->m_iPlaylistAttached = 1;
							if (newx <= 0) {
								newx = rect.right;
								m_cVideo->m_iPlaylistAttached = 2;
								if (newx + RectGetWidth(rectp) >= GetSystemMetrics(SM_CXSCREEN))
								{
									newx = rect.left;
									m_cVideo->m_iPlaylistAttached = 0;
								}
							}
							SetWindowPos(m_cPlaylist->GetHWND(), nullptr, newx, rect.top, 0, 0, SWP_NOSIZE);
						}
						else
						{
							// 优先选择上一次粘附的位置
							RECT rect, rectp;
							GetWindowRect(m_cVideo->GetHWND(), &rect);
							GetWindowRect(m_cPlaylist->GetHWND(), &rectp);
							int newx = rect.right;
							m_cVideo->m_iPlaylistAttached = 2;
							if (newx + RectGetWidth(rectp) >= GetSystemMetrics(SM_CXSCREEN)) {
								newx = rect.left - RectGetWidth(rectp);
								m_cVideo->m_iPlaylistAttached = 1;
								if (newx <= 0)
								{
									newx = rect.left;
									m_cVideo->m_iPlaylistAttached = 0;
								}
							}
							SetWindowPos(m_cPlaylist->GetHWND(), nullptr, newx, rect.top, 0, 0, SWP_NOSIZE);
						}
					}
					m_cPlaylist->ShowWindow(true, false);
				}
				else m_cPlaylist->ShowWindow(false, false);

			}
			else if (msg.pSender->GetName() == _T("button_alwaystop_true")) {
				if (!m_cVideo->IsFullScreen())
				{
					m_alwaystop_true->SetVisible(false);
					m_alwaystop_false->SetVisible(true);
					SetWindowPos(m_hVideo, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				}
			}
			else if (msg.pSender->GetName() == _T("button_alwaystop_false")) {
				if (!m_cVideo->IsFullScreen())
				{
					m_alwaystop_false->SetVisible(false);
					m_alwaystop_true->SetVisible(true);
					SetWindowPos(m_hVideo, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				}
			}
			else if (msg.pSender->GetName() == _T("Settings")) {
				// NOT impl
			}
		}
		else if (msg.sType == _T("buttondownandposchanged") || msg.sType == _T("movevaluechanged"))
		{
			if (msg.pSender->GetName() == _T("slider_play_process"))
			{
				if (msg.sType == _T("movevaluechanged"))
				{
					if (msg.ptMouse.x == lastPointx) return;
					else lastPointx = msg.ptMouse.x;
				}
				if (m_cVideo->GetPlayState() == Running || m_cVideo->GetPlayState() == Paused)
				{
					double duration = 1;
					double pos = m_SliderPlayProcess->GetValue();
					m_cVideo->GetDuration(&duration);
					double newPos = duration*pos / 1000;
					m_cVideo->SetCurrentPosition(newPos);
					if (m_cVideo->GetPlayState() == Paused)
					{
						// Update the video time label.
						m_cVideo->UpdateVideoPos(false);
					}
				}
			}
			else if (msg.pSender->GetName() == _T("slider_volume")) {
				int m_pos = m_SliderVolume->GetValue();
				m_cVideo->m_iVolume = m_pos;
				if (!m_cVideo->IsMuted())
					m_cVideo->ChangeVolume(long(m_pos * m_cVideo->GetSvplWrapper()->GetItem(m_cVideo->m_nowplaylist, m_cVideo->m_nowplaying).volume / 100));
			}
		}

		__super::Notify(msg);
	}

	CControlUI* CControlWindow::CreateControl(LPCTSTR pstrClassName)
	{
		return NULL;
	}

	bool CControlWindow::IsOnControl(POINT pt) {
		::ScreenToClient(*this, &pt);
		CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(pt));
		if (pControl && _tcsicmp(pControl->GetClass(), _T("ButtonUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("OptionUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("SliderUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("EditUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("RichEditUI")) != 0) return false;
		else return true;
	}

	LRESULT CControlWindow::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		if (m_cPlaylist)
		{
			::SendMessage(m_cPlaylist->GetHWND(), WM_CLOSE, 0, 0);
			delete m_cPlaylist;
			m_cPlaylist = nullptr;
		}
		if (plwnd_DragDrop)
		{
			plwnd_DragDrop->DragDropRevoke();
			plwnd_DragDrop->Release();
			plwnd_DragDrop = nullptr;
		}
		return 0;
	}

	LRESULT CControlWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lRes = 0;
		BOOL bHandled = TRUE;

		switch (uMsg)
		{
		case WM_CREATE:			lRes = OnCreate(uMsg, wParam, lParam, bHandled); break;
		case WM_CLOSE:			lRes = OnClose(uMsg, wParam, lParam, bHandled); break;
		case WM_DESTROY:		lRes = OnDestroy(uMsg, wParam, lParam, bHandled); break;
#if defined(WIN32) && !defined(UNDER_CE)
		case WM_NCACTIVATE:		lRes = OnNcActivate(uMsg, wParam, lParam, bHandled); break;
		case WM_NCCALCSIZE:		lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled); break;
		case WM_NCPAINT:		lRes = OnNcPaint(uMsg, wParam, lParam, bHandled); break;
		case WM_NCHITTEST:		lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
		case WM_GETMINMAXINFO:	lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); break;
		case WM_MOUSEWHEEL:		lRes = OnMouseWheel(uMsg, wParam, lParam, bHandled); break;
#endif
		case WM_SIZE:           lRes = OnSize(uMsg, wParam, lParam, bHandled); break;
		case WM_CHAR:			lRes = OnChar(uMsg, wParam, lParam, bHandled); break;
		case WM_SYSCOMMAND:		lRes = OnSysCommand(uMsg, wParam, lParam, bHandled); break;
		case WM_KEYDOWN:
		{
			if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_SPACE)
				::SendMessage(m_hVideo, uMsg, wParam, 0);
			else
				bHandled = FALSE;
			break;
		}
		case WM_KILLFOCUS:		lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); break;
		case WM_SETFOCUS:		lRes = OnSetFocus(uMsg, wParam, lParam, bHandled); break;
		case WM_LBUTTONUP:		lRes = OnLButtonUp(uMsg, wParam, lParam, bHandled);	break;
		case WM_LBUTTONDOWN:	lRes = OnLButtonDown(uMsg, wParam, lParam, bHandled); break;
		case WM_MOUSEMOVE:		lRes = OnMouseMove(uMsg, wParam, lParam, bHandled); break;
		case WM_MOUSEHOVER:	lRes = OnMouseHover(uMsg, wParam, lParam, bHandled); break;
		case WM_RBUTTONUP: break;
		case WM_RBUTTONDOWN: break;

		default:				bHandled = FALSE; break;
		}
		if (bHandled) return lRes;

		lRes = HandleCustomMessage(uMsg, wParam, lParam, bHandled);
		if (bHandled) return lRes;

		if (m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes))
			return lRes;

		return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	}

	LRESULT CControlWindow::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
		::ScreenToClient(*this, &pt);

		CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(pt));
		if (pControl && _tcsicmp(pControl->GetClass(), _T("ButtonUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("OptionUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("SliderUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("EditUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("RichEditUI")) != 0)
			return HTTRANSPARENT;

		return HTCLIENT;
	}

	LRESULT CControlWindow::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		if (uMsg == WM_SYSKEYDOWN && wParam == VK_F4) bHandled = true;
		return 0;
	}
}