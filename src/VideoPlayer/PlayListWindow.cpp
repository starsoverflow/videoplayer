
#include "PlayListWindow.h"
#include "VideoWindow.h"
#include "MyMenu.h"
#include "DragListUI.h"

namespace Star_VideoPlayer
{
	CPlayListWindow::CPlayListWindow(CVideoWindow* videoWindow)
		:
		m_videownd(videoWindow)
	{

	}

	CPlayListWindow::~CPlayListWindow()
	{

	}

	LPCTSTR CPlayListWindow::GetWindowClassName() const
	{
		return _T("Star Video Player_Playlist");
	}

	CDuiString CPlayListWindow::GetSkinFile()
	{
		return _T("PlayList.xml");
	}

	CDuiString CPlayListWindow::GetSkinFolder()
	{
		return _T("Skin");
	}

	void CPlayListWindow::InitWindow()
	{
		m_playlist = static_cast<CListUI*>(m_PaintManager.FindControl(_T("playlist")));
	}

	void CPlayListWindow::OnPrepare(TNotifyUI& msg)
	{
		Refresh();
	}

	void CPlayListWindow::Notify(TNotifyUI& msg)
	{
		CDuiString strControlName = msg.pSender->GetName();

		if (msg.sType == _T("windowinit"))
		{
			OnPrepare(msg);
		}
		else if (msg.sType == _T("itemclick"))
		{
			// 判断连续点击
			if (dbclickfirsttime == 0) dbclickfirsttime = GetTickCount64();
			else if (GetTickCount64() - dbclickfirsttime <= GetDoubleClickTime())
			{
				// 判断鼠标是否仍在原位置
				POINT ncursorPoint = { 0 };
				GetCursorPos(&ncursorPoint);
				if (ncursorPoint.x == cursorPoint.x && ncursorPoint.y == cursorPoint.y)
				{
					// 认为是双击
					m_videownd->OpenClip(this->m_playlist->GetCurSel());
					// 重置
					dbclickfirsttime = 0;
				}
				else
				{
					dbclickfirsttime = GetTickCount64();
				}
			}
			else
			{
				dbclickfirsttime = GetTickCount64();
			}
			GetCursorPos(&cursorPoint);
		}
		else if (msg.sType == _T("menu"))
		{
			if (msg.pSender->GetName() == _T("playlist"))
			{
				ConMenu* playlistMenu = new ConMenu(_T("Playlist_Menu.xml"));
				POINT point;
				GetCursorPos(&point);
				playlistMenu->LoadPopupMenu(m_hWnd);
				CDuiString command = playlistMenu->TrackPopupMenu(point.x, point.y);
				if (command == L"delete")
				{
					int index = this->m_playlist->GetCurSel();
					if (index != -1)
					{
						m_videownd->GetSvplWrapper()->DeleteItem(index, m_videownd->m_nowplaylist);
						this->m_playlist->RemoveAt(index);
						if (index <= m_videownd->m_nowplaying)
						{
							m_videownd->m_nowplaying--;
						}
					}
				}
				else if (command == L"whereismysong")
				{
					this->m_playlist->SelectItem(m_videownd->m_nowplaying);
				}
				else if (command == L"open_in_explorer")
				{
					if (this->m_playlist->GetCurSel() != -1)
					{
						SHELLEXECUTEINFOW sei;
						ZeroMemory(&sei, sizeof(sei));
						sei.cbSize = sizeof(SHELLEXECUTEINFOW);
						sei.fMask = SEE_MASK_FLAG_NO_UI;
						wstring commandline = L"/select,\"" + m_videownd->GetSvplWrapper()->GetItem(m_videownd->m_nowplaylist, this->m_playlist->GetCurSel()).path + L"\"";
						sei.lpParameters = commandline.c_str();
						sei.lpFile = L"explorer.exe";
						sei.nShow = SW_SHOWNORMAL;
						ShellExecuteExW(&sei);
					}
				}
			}
		}

		__super::Notify(msg);
	}

	CControlUI* CPlayListWindow::CreateControl(LPCTSTR pstrClassName)
	{
		return NULL;
	}

	void CPlayListWindow::Refresh()
	{
		m_playlist->RemoveAll();

		auto rsvpl = m_videownd->GetSvplWrapper()->Get();
		if (rsvpl->playlists.size() == 0) return;
		for (int i = 0; i <= (int)rsvpl->playlists.at(m_videownd->m_nowplaylist).items.size() - 1;i++) {
			DragListUI* pListElement = new DragListUI(this);
			wstring name = rsvpl->playlists.at(m_videownd->m_nowplaylist).items.at(i).title;
			if (name == L"") name = GetFilename(rsvpl->playlists.at(m_videownd->m_nowplaylist).items.at(i).path.c_str());
			pListElement->SetAttribute(_T("text"), name.c_str());
			pListElement->SetPadding({ 5,5,5,0 });
			m_playlist->Add(pListElement);
		}
	}

	LRESULT CPlayListWindow::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
		::ScreenToClient(*this, &pt);

		RECT rcClient;
		::GetClientRect(*this, &rcClient);

		if (!::IsZoomed(*this))
		{
			RECT rcSizeBox{ 10, 10, 4, 4 };
			if (pt.y < rcClient.top + rcSizeBox.top)
			{
				if (pt.x < rcClient.left + rcSizeBox.left) return HTTOPLEFT;
				if (pt.x > rcClient.right - rcSizeBox.right) return HTTOPRIGHT;
				return HTTOP;
			}
			else if (pt.y > rcClient.bottom - rcSizeBox.bottom)
			{
				if (pt.x < rcClient.left + rcSizeBox.left) return HTBOTTOMLEFT;
				if (pt.x > rcClient.right - rcSizeBox.right) return HTBOTTOMRIGHT;
				return HTBOTTOM;
			}

			if (pt.x < rcClient.left + rcSizeBox.left) return HTLEFT;
			if (pt.x > rcClient.right - rcSizeBox.right) return HTRIGHT;
		}

		RECT rcCaption = m_PaintManager.GetCaptionRect();
		if (pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right \
			&& pt.y >= rcCaption.top && pt.y < rcCaption.bottom) {
			CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(pt));
			if (pControl && _tcsicmp(pControl->GetClass(), _T("ButtonUI")) != 0 &&
				_tcsicmp(pControl->GetClass(), _T("OptionUI")) != 0 &&
				_tcsicmp(pControl->GetClass(), _T("SliderUI")) != 0 &&
				_tcsicmp(pControl->GetClass(), _T("EditUI")) != 0 &&
				_tcsicmp(pControl->GetClass(), _T("RichEditUI")) != 0)
				return HTCAPTION;
		}

		return HTCLIENT;
	}

	LRESULT CPlayListWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		case WM_KEYDOWN:		lRes = OnKeyDown(uMsg, wParam, lParam, bHandled); break;
		case WM_KILLFOCUS:		lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); break;
		case WM_SETFOCUS:		lRes = OnSetFocus(uMsg, wParam, lParam, bHandled); break;
		case WM_LBUTTONUP:		lRes = OnLButtonUp(uMsg, wParam, lParam, bHandled); break;
		case WM_LBUTTONDOWN:	lRes = OnLButtonDown(uMsg, wParam, lParam, bHandled); break;
		case WM_MOUSEMOVE:		lRes = OnMouseMove(uMsg, wParam, lParam, bHandled); break;
		case WM_MOUSEHOVER:	lRes = OnMouseHover(uMsg, wParam, lParam, bHandled); break;
		case WM_MOVING:
		{
			int AttachRange = 5;
			RECT rc = { 0 }, myrc = { 0 };
			GetWindowRect(m_videownd->GetHWND(), &rc);
			RECT* pChRC = (RECT*)lParam;
			myrc = *pChRC;
			if (myrc.left >= rc.right - AttachRange && myrc.left <= rc.right + AttachRange)
			{
				lRes = 1;
				pChRC->right += rc.right - myrc.left;
				pChRC->left = rc.right;
				m_videownd->m_iPlaylistAttached = 2;
			}
			else if (myrc.right <= rc.left + AttachRange && myrc.right >= rc.left - AttachRange)
			{
				lRes = 1;
				pChRC->left = rc.left - RectGetWidth(myrc);
				pChRC->right = rc.left;
				m_videownd->m_iPlaylistAttached = 1;
			}
			else
				m_videownd->m_iPlaylistAttached = 0;
			break;
		}
		default:				bHandled = FALSE; break;
		}
		if (bHandled) return lRes;

		lRes = HandleCustomMessage(uMsg, wParam, lParam, bHandled);
		if (bHandled) return lRes;

		if (m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes))
			return lRes;

		return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	}
}