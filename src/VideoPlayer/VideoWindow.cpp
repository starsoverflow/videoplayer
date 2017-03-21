
#include "VideoWindow.h"
#include "ControlsWindow.h"
#include "msg.h"
#include "../mydebug.h"
#include "svplwriter.h"

#include "DragDrop.h"
#include "../EVRPresenter/EVRPresenter.h"

#include "../ScopeGuard.h"

#include <gdiplus.h>

#define MyClassName _T("Star Video Player_Video Window")
#define MyTitle _T("Star Video Player")

#define HOTKEY_PlayStop 4001
#define HOTKEY_Next 4002
#define HOTKEY_Prev 4003
#define HOTKEY_VolUp 4004
#define HOTKEY_VolDown 4005
#define HOTKEY_Mute 4007

namespace Star_VideoPlayer
{
	CVideoWindow::CVideoWindow(wstring appPath, wstring playlistPath)
		: m_strAppPath(appPath),
		  m_strPlaylistFile(playlistPath)
	{
		srand((unsigned int)time(nullptr));
	}

	CVideoWindow::~CVideoWindow()
	{
		ReleaseVideoWindow();
	}

	LRESULT CVideoWindow::CreateVideoWindow(HINSTANCE hInstance)
	{
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_DBLCLKS;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VIDEOPLAYER));
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
		wcex.lpszClassName = MyClassName;
		wcex.lpszMenuName = NULL;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_VIDEOPLAYER));
		RegisterClassEx(&wcex);

		bool bLoadSuccess = false, bShowErrorMsg = true;

		if (m_svplwrapper.get() == nullptr)
		{
			if (m_strPlaylistFile.empty()) {
				m_strPlaylistFile = m_strAppPath + L"lastSaved.svpl"; bShowErrorMsg = false; m_bSavePlaylistWhenExit = false;
			}
			m_svplwrapper.reset(new svplwrapper(m_strPlaylistFile));
		}

		if (m_svplwrapper->parserStatus == 0) bLoadSuccess = true;
		else if (m_svplwrapper->parserStatus == -1 && bShowErrorMsg) { SHOWMSG(MsgIcon::Exclamation, ERR_CANNOTOPENPLAYLIST, m_strPlaylistFile.c_str()); }
		else if (m_svplwrapper->parserStatus == -2 && bShowErrorMsg) { SHOWMSG(MsgIcon::Exclamation, ERR_CANNOTREADPLAYLIST, m_strPlaylistFile.c_str()); }

		auto rsvpl = m_svplwrapper->Get();

		int width = DEFAULT_WIDTH;
		int height = DEFAULT_HEIGHT;

		if (!bLoadSuccess)
		{
			m_svplwrapper->CreateDefaultSvpl();
		}

		try {
			m_nowplaylist = rsvpl->config.normalplaylist;
			m_nowplaying = rsvpl->playlists.at(m_nowplaylist).currentindex;
			m_keepwidth = rsvpl->playlists.at(m_nowplaylist).keepwidth;
			m_windowAspectRatio = rsvpl->config.windowAspectRatio;
		}
		catch (...) {
			_trace(L"This can't happen in videowindow.cpp line %d", __LINE__);
			ASSERT(FALSE);
		}
		
		if (m_keepwidth != 0)
		{
			if (m_keepwidth < MINIMUM_WIDTH || m_keepwidth > MAXIMUM_WIDTH) m_keepwidth = DEFAULT_WIDTH;
			width = m_keepwidth;
			height = int(m_keepwidth / DEFAULT_SCALE);
		}
		else
		{
			if (rsvpl->config.size.x < MINIMUM_WIDTH || rsvpl->config.size.x > MAXIMUM_WIDTH) rsvpl->config.size.x = DEFAULT_WIDTH;
			width = rsvpl->config.size.x;
			height = rsvpl->config.size.y;
		}

		if (m_windowAspectRatio.x && m_windowAspectRatio.y)
		{
			height = width * m_windowAspectRatio.y / m_windowAspectRatio.x;
			if (height < MINIMUM_HEIGHT || height > MAXIMUM_HEIGHT)
			{
				m_windowAspectRatio = { 16, 9 };
				height = width * m_windowAspectRatio.y / m_windowAspectRatio.x;
				SHOWMSG(MsgIcon::Exclamation, "Your request of Window's Aspect Ratio was rejected.\nCheck your playlist file.\nThe default setting 16:9 was applied.");
			}
		}

		if (width < MINIMUM_WIDTH || height < MINIMUM_HEIGHT)
		{
			width = DEFAULT_WIDTH;
			height = DEFAULT_HEIGHT;
		}

		if (rsvpl->config.alwaystop) m_bTopMost = true;
		else m_bTopMost = false;

		m_hwnd = CreateWindowEx((m_bTopMost ? WS_EX_TOPMOST : NULL), MyClassName, MyTitle, WS_POPUP | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CAPTION,
			rsvpl->config.location.x, rsvpl->config.location.y, width, height, NULL, NULL, hInstance, this);

		if (!m_hwnd)
		{
			return -1;
		}

		// 初始化rcVideo
		GetWindowRect(m_hwnd, &m_rcVideo);

		ShowWindow(m_hwnd, SW_NORMAL);
		UpdateWindow(m_hwnd);

		m_conMenu.reset(new ConMenu(L"Controls_Menu.xml"));

		if (!bLoadSuccess)
		{
			InitPlayerWindow();
		}
		else
		{
			m_mousehidetime = rsvpl->config.mousehidetime;
			m_iVolume = rsvpl->playlists.at(m_nowplaylist).volume;
		}

		SetTimer(m_hwnd, 1, 100, nullptr);
		SetTimer(m_hwnd, 2, 100, nullptr);  // 判断鼠标移入移出界面

		vp_DragDrop::ReadAllowedExts(m_strAppPath + L"DragAcceptExts.txt");

		myDragDrop = new vp_DragDrop();
		myDragDrop->DragDropRegister(this);

		RegisterHotkey();

		m_psCurrent = Stopped;
		
		OpenClip(m_nowplaying);
		
		return 0;
	}

	LRESULT CVideoWindow::ReleaseVideoWindow()
	{
		if (myDragDrop)
		{
			myDragDrop->DragDropRevoke();
			myDragDrop->Release();

			myDragDrop = nullptr;
		}
		UnRegisterHotkey();
		return 0;
	}

	LRESULT CVideoWindow::RegisterHotkey()
	{
		if (!RegisterHotKey(m_hwnd, HOTKEY_PlayStop, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, VK_SPACE))
			SHOWMSG_D(5000, MsgIcon::Information, ERR_REGHOTKEY, L"Ctrl-Alt-Space");
		if (!RegisterHotKey(m_hwnd, HOTKEY_Next, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, VK_RIGHT))
			SHOWMSG_D(5000, MsgIcon::Information, ERR_REGHOTKEY, L"Ctrl-Alt-Right");
		if (!RegisterHotKey(m_hwnd, HOTKEY_Prev, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, VK_LEFT))
			SHOWMSG_D(5000, MsgIcon::Information, ERR_REGHOTKEY, L"Ctrl-Alt-Left");
		if (!RegisterHotKey(m_hwnd, HOTKEY_VolUp, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, VK_UP))
			SHOWMSG_D(5000, MsgIcon::Information, ERR_REGHOTKEY, L"Ctrl-Alt-Up");
		if (!RegisterHotKey(m_hwnd, HOTKEY_VolDown, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, VK_DOWN))
			SHOWMSG_D(5000, MsgIcon::Information, ERR_REGHOTKEY, L"Ctrl-Alt-Down");
		if (!RegisterHotKey(m_hwnd, HOTKEY_Mute, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, 0x4D)) // Ctrl+Alt+M
			SHOWMSG_D(5000, MsgIcon::Information, ERR_REGHOTKEY, L"Ctrl-Alt+M");
		return 0;
	}

	LRESULT CVideoWindow::UnRegisterHotkey()
	{
		UnregisterHotKey(m_hwnd, HOTKEY_PlayStop);
		UnregisterHotKey(m_hwnd, HOTKEY_Next);
		UnregisterHotKey(m_hwnd, HOTKEY_Prev);
		UnregisterHotKey(m_hwnd, HOTKEY_VolUp);
		UnregisterHotKey(m_hwnd, HOTKEY_VolDown);
		UnregisterHotKey(m_hwnd, HOTKEY_Mute);
		return 0;
	}

	LRESULT CVideoWindow::EnableFullScreen()
	{
		if (m_bFullScreen) return -1;
		m_bFullScreen = true;
		m_originalTopMost = m_cCon->m_alwaystop_true->IsVisible();
		if (m_pDisplay)
			m_pDisplay->SetAspectRatioMode(NoStretch);
		SetAllWindowPos(HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL);
		if (m_cCon) m_cCon->m_alwaystop_false->SetVisible(false);
		if (m_cCon) m_cCon->m_alwaystop_true->SetVisible(false);
		if (m_cCon) m_cCon->m_btnMin->SetEnabled(false);
	
		return 0;
	}

	LRESULT CVideoWindow::DisableFullScreen()
	{
		if (!m_bFullScreen) return -1;
		m_bFullScreen = false;
		mousemove_first = true;
		if (m_pDisplay && m_keepwidth != 0)
			m_pDisplay->SetAspectRatioMode(Stretch);
		SetAllWindowPos(m_originalTopMost ? HWND_TOPMOST : HWND_NOTOPMOST, m_rcVideo.left, m_rcVideo.top, RectGetWidth(m_rcVideo), RectGetHeight(m_rcVideo), NULL);
		if (m_cCon && m_originalTopMost) m_cCon->m_alwaystop_true->SetVisible(true);
		else if (m_cCon && !m_originalTopMost) m_cCon->m_alwaystop_false->SetVisible(true);
		if (m_cCon) m_cCon->m_btnMin->SetEnabled(true);
		m_iPlaylistAttached = 3;
		return 0;
	}

	LRESULT CVideoWindow::BindControlWindow(bool Show)
	{
		m_cCon = new CControlWindow(_T("Controls.xml"), this);
		if (!m_cCon) return -1;

		m_hCon = m_cCon->Create(m_hwnd, _T("Controls Window"), WS_POPUP, NULL);
		if (m_hCon == 0) return -1;

		RECT rc;
		GetWindowRect(m_hwnd, &rc);
		SetWindowPos(m_hCon, NULL, rc.left, rc.top, RectGetWidth(rc), RectGetHeight(rc), SWP_NOZORDER);

		// Bind时更新ControlWindow
		if (m_psCurrent == Running)
		{
			m_cCon->m_btnPlay->SetVisible(false);
			m_cCon->m_btnPause->SetVisible(true);
		}
		else
		{
			m_cCon->m_btnPlay->SetVisible(true);
			m_cCon->m_btnPause->SetVisible(false);
		}

		if (m_bTopMost)
		{
			m_cCon->m_alwaystop_true->SetVisible(true);
			m_cCon->m_alwaystop_false->SetVisible(false);
		}
		else
		{
			m_cCon->m_alwaystop_true->SetVisible(false);
			m_cCon->m_alwaystop_false->SetVisible(true);
		}

		m_cCon->m_SliderVolume->SetValue(m_iVolume);
		UpdateVideoPos();
		UpdateMainTitle();

		if (Show)
		{
			ShowWindow(m_hCon, SW_NORMAL);
		}

		return 0;
	}

	LRESULT CVideoWindow::UnBindControlWindow()
	{
		if (!m_cCon) return -1;
		m_cCon->Close();
		m_cCon = nullptr;

		return 0;
	}

	HWND CVideoWindow::GetHWND()
	{
		return m_hwnd;
	}

	LRESULT CVideoWindow::ResizeWindow(int cx, int cy)
	{
		// NOTICE: 这里只考虑了 keepwidth!=0 和固定窗口大小时 无需移动播放列表窗口的情况。
		if (m_cCon)
		{
			HDWP hdwp = BeginDeferWindowPos(2);
			DeferWindowPos(hdwp, m_hwnd, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
			DeferWindowPos(hdwp, m_hCon, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
			EndDeferWindowPos(hdwp);
		}
		else
		{
			SetWindowPos(m_hwnd, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
		}
		return 0;
	}

	LRESULT CVideoWindow::SetAllWindowPos(HWND insert, int x, int y, int cx, int cy, UINT uFlags)
	{
		if (m_cCon)
		{
			HDWP hdwp = BeginDeferWindowPos(3);
			DeferWindowPos(hdwp, m_hwnd, insert, x, y, cx, cy, uFlags);

			if (m_iPlaylistAttached == 1)
			{
				RECT rc = { 0 }, vrc = { 0 };
				GetWindowRect(m_cCon->GetPlayListWindow()->GetHWND(), &rc);
				GetWindowRect(m_hwnd, &vrc);
				DeferWindowPos(hdwp, m_cCon->GetPlayListWindow()->GetHWND(), insert, x - RectGetWidth(rc), rc.top + y - vrc.top, 0, 0, uFlags | SWP_NOSIZE);
			}
			else if (m_iPlaylistAttached == 2)
			{
				RECT rc = { 0 }, vrc = { 0 };
				GetWindowRect(m_cCon->GetPlayListWindow()->GetHWND(), &rc);
				GetWindowRect(m_hwnd, &vrc);
				DeferWindowPos(hdwp, m_cCon->GetPlayListWindow()->GetHWND(), insert, x + RectGetWidth(vrc), rc.top + y - vrc.top, 0, 0, uFlags | SWP_NOSIZE);
			}
			else if (m_iPlaylistAttached == 0)
			{
				DeferWindowPos(hdwp, m_cCon->GetPlayListWindow()->GetHWND(), insert, 0, 0, 0, 0, uFlags | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);
			}

			DeferWindowPos(hdwp, m_hCon, m_cCon->GetPlayListWindow()->GetHWND(), x, y, cx, cy, uFlags);

			EndDeferWindowPos(hdwp);
		}
		else
		{
			SetWindowPos(m_hwnd, insert, x, y, cx, cy, uFlags);
		}
		return 0;
	}

	LRESULT CVideoWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CONTEXTMENU:
		{
			if (m_bCapture) break;
			m_bInMenu = true;
			MyShowCursor(true);
			POINT point;
			GetCursorPos(&point);
			m_conMenu->LoadPopupMenu(m_hwnd);
			m_conMenu->GetMenuContainer()->FindSubControl(L"icon_mutechecked")->SetVisible(m_bMuted);
			if (m_PlayMode == PlayMode::Queue)
			{
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_queue")->SetVisible(true);
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_loop")->SetVisible(false);
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_random")->SetVisible(false);
			}
			else if (m_PlayMode == PlayMode::Loop)
			{
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_queue")->SetVisible(false);
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_loop")->SetVisible(true);
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_random")->SetVisible(false);
			}
			else if ((m_PlayMode == PlayMode::Random))
			{
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_queue")->SetVisible(false);
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_loop")->SetVisible(false);
				m_conMenu->GetMenuContainer()->FindSubControl(L"icon_checked_random")->SetVisible(true);
			}

			LPCTSTR command = m_conMenu->TrackPopupMenu(point.x, point.y);
			m_bInMenu = false;
			if (_tcscmp(command, _T("playpause")) == 0)
			{
				PauseClip();
			}
			else if (_tcscmp(command, _T("stop")) == 0)
			{
				StopClip();
			}
			else if (_tcscmp(command, _T("playprev")) == 0)
			{
				PlayPrev();
			}
			else if (_tcscmp(command, _T("playnext")) == 0)
			{
				PlayNext();
			}
			else if (_tcscmp(command, _T("mute")) == 0)
			{
				Mute();
			}
			else if (_tcscmp(command, _T("playmode_queue")) == 0)
			{
				m_PlayMode = PlayMode::Queue;
			}
			else if (_tcscmp(command, _T("playmode_loop")) == 0)
			{
				m_PlayMode = PlayMode::Loop;
			}
			else if (_tcscmp(command, _T("playmode_random")) == 0)
			{
				m_PlayMode = PlayMode::Random;
			}
			else if (_tcscmp(command, _T("settings")) == 0)
			{
				SHOWMSG_D(4000, MsgIcon::Information, ERR_NOT_IMPL);
			}
			else if (_tcscmp(command, _T("help")) == 0)
			{
				SHOWMSG_D(4000, MsgIcon::Information, ERR_NOT_IMPL);
			}
			else if (_tcscmp(command, _T("about")) == 0)
			{
				SHOWMSG_D(4000, MsgIcon::Information, ERR_NOT_IMPL);
			}
			break;
		}

		case WM_CREATE:
		{
			return 0;
			break;
		}

		case WM_MOUSEWHEEL:
		{
			short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (zDelta < 0) VolDown(2);
			else if (zDelta > 0) VolUp(2);
			break;
		}

		case WM_TIMER:
		{
			UINT nIDEvent = (UINT)wParam;
			switch (nIDEvent)
			{
			case 1:
			{
				if (m_psCurrent == Running) UpdateVideoPos();
				break;
			}
			case 2:
			{
				//_trace(L"%d\n", m_hCon);
				static POINT LastPoint;     // 上一次的鼠标位置
				static bool IsInWindow;

				bool ismoved = false;

				POINT point;
				GetCursorPos(&point);
				if (LastPoint.x != point.x || LastPoint.y != point.y) ismoved = true;
				LastPoint = point;

				if (m_bInMenu || m_bCapture || (m_cCon && m_cCon->IsCaptured())) {
					if (m_cCon) m_cCon->Hide(false);
					m_ulTicktime = GetTickCount64();
					break;
				}

				if (!IsInWindow && !ismoved) break;

				if (IsCursorInWindow(m_hwnd, point) == 1)
				{
					if (IsInWindow) { // 已经在窗口里
						if (m_cCon)
						{
							if (ismoved || m_cCon->IsOnControl(point))
							{
								m_ulTicktime = GetTickCount64();
								m_cCon->Hide(false);
								MyShowCursor(true);
							}
							else {
								if (GetTickCount64() >= m_ulTicktime + m_mousehidetime * 1000) {
									m_cCon->Hide();
									MyShowCursor(false);
								}
							}
						}
					}
					else {
						m_ulTicktime = GetTickCount64();
						IsInWindow = true;
						if (m_cCon) m_cCon->Hide(false);
					}
				}
				else
				{
					if (m_cCon) m_cCon->Hide();
					IsInWindow = false;
					MyShowCursor(true);
				}
				break;
			}
			}
			break;
		}
	
		case WM_NCCALCSIZE:
			return 0;   // 这将删除标题栏和边框
			break;

		case WM_SYSCOMMAND:
		{
			if ((UINT)(0xFFF0 & wParam) == SC_MINIMIZE) {
				if (m_bFullScreen) break;
				KillTimer(m_hwnd, 2);

				ShowWindow(m_hwnd, SW_MINIMIZE);
			}
			else if ((UINT)(0xFFF0 & wParam) == SC_MAXIMIZE) {

			}
			else if ((UINT)(0xFFF0 & wParam) == SC_RESTORE) {
				if (m_bFullScreen) break;
				WINDOWPLACEMENT wndpm;
				wndpm.length = sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(m_hwnd, &wndpm);
				wndpm.rcNormalPosition = m_rcVideo;
				SetWindowPlacement(m_hwnd, &wndpm);

				ShowWindow(m_hwnd, SW_RESTORE);
				if (m_pDisplay && m_psCurrent == Paused) m_pDisplay->RepaintVideo();
				SetTimer(m_hwnd, 2, 100, NULL);

			}
			else return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
			break;
		}

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmm = (LPMINMAXINFO)lParam;
			if (lpmm)
			{
				lpmm->ptMinTrackSize.x = MINIMUM_WIDTH;
				lpmm->ptMinTrackSize.y = MINIMUM_HEIGHT;
			}
			break;
		}

		case WM_LBUTTONDBLCLK:
			if (!m_bFullScreen) EnableFullScreen();
			else DisableFullScreen();
			break;

		case WM_LBUTTONDOWN:
			SetCapture(m_hwnd);
			m_bCapture = true;
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
			m_bCapture = false;
			break;

		case WM_KILLFOCUS:
			ReleaseCapture();
			m_bCapture = false;
			break;

		case WM_MOUSEMOVE:
		{
			if (m_bFullScreen) break;
			static POINT lastPoint = { 0 };
			if (mousemove_first)
			{
				mousemove_first = false;
				lastPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			}
			POINT point;
			point.x = GET_X_LPARAM(lParam);
			point.y = GET_Y_LPARAM(lParam);
			//_trace(L"point: %d  %d \n", point.x, point.y);
			if (/*wParam == MK_LBUTTON && */m_bCapture)
			{
				if (point.x != lastPoint.x || point.y != lastPoint.y) {
					RECT rcWindow;
					GetWindowRect(m_hwnd, &rcWindow);
					SetAllWindowPos(NULL, rcWindow.left + point.x - lastPoint.x, rcWindow.top + point.y - lastPoint.y, 0, 0, SWP_NOSIZE);
					return 0;
				}
			}
			lastPoint = point;

		}
		break;

		case WM_GRAPHNOTIFY:
			HandleGraphEvent();
			break;

		case WM_ERASEBKGND:
			return 1;
			break;
		
		case WM_PAINT:
		{
			if (m_psCurrent == Stopped) PaintWelcomeImg(1);
			else if (m_psCurrent == Running && m_bAudioOnly == TRUE) PaintWelcomeImg(2);
			DefWindowProc(m_hwnd, uMsg, wParam, lParam);
			break;
		}

		case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED)
			{
				//delete: Already processed in ControlsWindow.cpp
				//if (m_cCon) m_cCon->Hide();
			}
			else
			{
				int cx = GET_X_LPARAM(lParam);
				int cy = GET_Y_LPARAM(lParam);
				if (!m_bAudioOnly) MoveVideoWindow(cx, cy);
				RECT l_rc;
				GetWindowRect(m_hwnd, &l_rc);
				SetAllWindowPos(NULL, l_rc.left, l_rc.top, RectGetWidth(l_rc), RectGetHeight(l_rc), SWP_NOACTIVATE | SWP_NOZORDER);
				if (!IsIconic(m_hwnd) && !m_bFullScreen) {
					if (m_keepwidth != 0) m_keepwidth = cx;
					// 保存视频位置信息
					m_rcVideo = l_rc;
				}
				//_trace(L"m_rcVideo: %d,%d,%d,%d\n", m_rcVideo.left, m_rcVideo.right, m_rcVideo.top, m_rcVideo.bottom);
				if (m_psCurrent == Stopped) InvalidateRect(m_hwnd, NULL, false);
				else if (m_psCurrent == Running && m_bAudioOnly == TRUE) InvalidateRect(m_hwnd, NULL, false);
				break;
			}
		}

		case WM_MOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);

			if (!IsIconic(m_hwnd) && !m_bFullScreen) {
				int width = RectGetWidth(m_rcVideo);
				int height = RectGetHeight(m_rcVideo);
				m_rcVideo.left = x;
				m_rcVideo.right = x + width;
				m_rcVideo.top = y;
				m_rcVideo.bottom = y + height;
				SetWindowPos(m_hCon, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
				//_trace(L"m_rcVideo: %d,%d,%d,%d\n", m_rcVideo.left, m_rcVideo.right, m_rcVideo.top, m_rcVideo.bottom);
			}
			break;
		}
		case WM_NCHITTEST:
		{
			if (m_bFullScreen) return HTCLIENT;
			POINT pt;
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			RECT rcClient;
			GetWindowRect(m_hwnd, &rcClient);

			int hitrange = 10;

			if (!IsZoomed(m_hwnd))
			{
				if (pt.x > rcClient.right - 75 && pt.y < rcClient.top + 25)    // 缩小关闭按钮处
				{
					return HTCLIENT;
				}
				if (pt.x < rcClient.left + hitrange && pt.y < rcClient.top + hitrange) //左上角
				{
					return HTTOPLEFT;
				}
				else if (pt.x > rcClient.right - hitrange && pt.y < rcClient.top + hitrange) //右上角
				{
					return HTTOPRIGHT;
				}
				else if (pt.x < rcClient.left + hitrange && pt.y > rcClient.bottom - hitrange) //左下角
				{
					return HTBOTTOMLEFT;
				}
				else if (pt.x > rcClient.right - hitrange && pt.y > rcClient.bottom - hitrange) //右下角
				{
					return HTBOTTOMRIGHT;
				}
				else if (pt.x < rcClient.left + 10)
				{
					return HTLEFT;
				}
				else if (pt.x > rcClient.right - 10)
				{
					return HTRIGHT;
				}
				else if (pt.y < rcClient.top + 10)
				{
					return HTTOP;
				}
				else if (pt.y > rcClient.bottom - 5)
				{
					return HTBOTTOM;
				}
				else
				{
					return HTCLIENT;
				}
			}
			break;
		}

		case WM_SIZING:
		{
			if (m_keepwidth || (m_windowAspectRatio.x && m_windowAspectRatio.y))
			{
				double scale = m_keepwidth ? m_dScale : (double)m_windowAspectRatio.x / m_windowAspectRatio.y;
				LPRECT pRect = (LPRECT)lParam;
				switch (wParam) {
				case WMSZ_BOTTOMLEFT:
				case WMSZ_BOTTOMRIGHT:
				case WMSZ_LEFT:
				case WMSZ_RIGHT:
					pRect->bottom = (long)(pRect->top + (pRect->right - pRect->left) / scale);
					break;
				case WMSZ_TOPLEFT:
				case WMSZ_TOPRIGHT:
					pRect->top = (long)(pRect->bottom - (pRect->right - pRect->left) / scale);
					break;
				case WMSZ_TOP:
				case WMSZ_BOTTOM:
					pRect->right = (long)((pRect->bottom - pRect->top) * scale + pRect->left);
					break;
				}
				return TRUE;
			}
			break;
		}

		case WM_CLOSE:
		{
			KillTimer(m_hwnd, 1);
			KillTimer(m_hwnd, 2);
		
			MyShowCursor(true);
			CloseInterfaces();
			DestroyWindow(m_hwnd);
			if (m_bSaveHistoryPlaylist) SavePlaylist(m_strAppPath + L"lastSaved.svpl");
			if (m_bSavePlaylistWhenExit && !m_strPlaylistFile.empty()) SavePlaylist(m_strPlaylistFile);
			break;
		}
		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_HOTKEY:
			// if (IsInMenu && conMenu) conMenu->ExitMenu(_T("global shortcut detected"));
			if ((int)wParam == HOTKEY_PlayStop) PauseClip();
			else if ((int)wParam == HOTKEY_Next) PlayNext();
			else if ((int)wParam == HOTKEY_Prev) PlayPrev();
			else if ((int)wParam == HOTKEY_VolUp) {
				VolUp();
			}
			else if ((int)wParam == HOTKEY_VolDown) {
				VolDown();
			}
			else if ((int)wParam == HOTKEY_Mute) {
				Mute();
			}
			break;

		case WM_DragDropDone:
		{
			if ((UINT)wParam >= 1)
			{
				SHOWMSG_D(3000, MsgIcon::Information, "已成功读取 %d 个媒体文件", (UINT)(wParam));
				if (m_cCon) m_cCon->GetPlayListWindow()->Refresh();
				if (m_psCurrent == Stopped)
				{
					// WM_DragDropDone 处理完毕之前用户无法改变添加至播放列表末尾的 wParam 个文件
					OpenClip(m_svplwrapper->GetItemNum(m_nowplaylist) - wParam);
				}
			}
			else
				SHOWMSG(MsgIcon::Information, "已完成遍历，没有可识别的媒体文件", (UINT)(wParam));
			break;
		}

		case WM_KEYDOWN:
		{
			if (wParam == VK_RIGHT)
			{
				double position = 0.0, duration = 0.0;
				if (!GetCurrentPosition(&position)) break;
				if (!GetDuration(&duration)) break;
				SetCurrentPosition(position + 0.01 * duration);
				UpdateVideoPos();
			}
			else if (wParam == VK_LEFT)
			{
				double position = 0.0, duration = 0.0;
				if (!GetCurrentPosition(&position)) break;
				if (!GetDuration(&duration)) break;
				SetCurrentPosition(position - 0.01 * duration);
				UpdateVideoPos();
			}
			else if (wParam == VK_UP)
			{
				VolUp();
			}
			else if (wParam == VK_DOWN)
			{
				VolDown();
			}
			else if (wParam == VK_SPACE)
			{
				PauseClip();
			}
			break;
		}

		case WM_SwitchClip:
		{
			int index = (int)wParam;
			if (index < 0 || index >= (int)m_svplwrapper->GetItemNum(m_nowplaylist)) return -1;
			OpenClip(index);
			break;
		}

		case WM_PlayNext:
		{
			PlayNext();
			break;
		}

		case WM_AddMediaFile:
		{
			if (wParam != 0xffff || lParam != 0xffff) break;
			HANDLE hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, 0, L"Local\\videopl{52DC5C06-AAA4-46C5-B753-3BC8E28099B1}");
			if (hMap == nullptr) return -1;
			ON_SCOPE_EXIT([hMap] { CloseHandle(hMap); });
			wchar_t* pbuf = (wchar_t*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 2048);
			if (pbuf == nullptr) return -1;
			ON_SCOPE_EXIT([pbuf] { UnmapViewOfFile(pbuf); });
			svpl_item newItem;
			newItem.path = pbuf;
			m_svplwrapper->AddItem(newItem, m_nowplaylist);
			if (m_cCon) m_cCon->GetPlayListWindow()->Refresh();
			break;
		}

		case WM_ReloadPlaylist:
		{
			if (lParam != 0xffff) break;
			HANDLE hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, 0, L"Local\\videopl{52DC5C06-AAA4-46C5-B753-3BC8E28099B1}");
			if (hMap == nullptr) return -1;
			ON_SCOPE_EXIT([hMap] { CloseHandle(hMap); });
			wchar_t* pbuf = (wchar_t*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 2048);
			if (pbuf == nullptr) return -1;
			ON_SCOPE_EXIT([pbuf] { UnmapViewOfFile(pbuf); });
			shared_ptr<svplwrapper> new_svplwrapper(new svplwrapper(pbuf));
			if (new_svplwrapper->parserStatus != 0) return new_svplwrapper->parserStatus;
			SetNewWrapper(new_svplwrapper);
			if (wParam == 1)
			{
				double pos = 0.0;
				GetCurrentPosition(&pos);
				OpenClip(m_nowplaying);
				SetCurrentPosition(pos);
			}
			else if (wParam == 2)
			{
				OpenClip(m_nowplaying);
			}
			else if (wParam == 3)
			{
				m_bBeginFromFirst = true;
				OpenClip(m_nowplaying);
			}
			break;
		}

		default:
			return ::DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		}

		// 如果前面没有返回
		return 0;
	}

	void CVideoWindow::VolUp(int delta)
	{
		m_iVolume += delta;
		if (m_iVolume > 99) m_iVolume = 99;
		if (m_iVolume < 0) m_iVolume = 0;
		int newVolume = int(m_iVolume * m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).volume / 100);
		ChangeVolume(newVolume);
		m_cCon->m_SliderVolume->SetValue(m_iVolume);
	}

	void CVideoWindow::VolDown(int delta)
	{
		m_iVolume -= delta;
		if (m_iVolume > 99) m_iVolume = 99;
		if (m_iVolume < 0) m_iVolume = 0;
		int newVolume = int(m_iVolume * m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).volume / 100);
		ChangeVolume(newVolume);
		m_cCon->m_SliderVolume->SetValue(m_iVolume);
	}


	LRESULT CVideoWindow::SavePlaylist(wstring path)
	{
		try {
			auto svplCurrent = m_svplwrapper->Get();
			if (svplCurrent->playlists.size() != 0) {
				svplCurrent->playlists.at(m_nowplaylist).volume = m_iVolume;
				svplCurrent->playlists.at(m_nowplaylist).currentindex = m_nowplaying;
				if (svplCurrent->playlists.at(m_nowplaylist).keepwidth != 0)
					svplCurrent->playlists.at(m_nowplaylist).keepwidth = m_keepwidth;
				if (m_bFullScreen)
				{
					svplCurrent->config.alwaystop = m_originalTopMost;
				}
				else
				{
					svplCurrent->config.alwaystop = m_cCon->m_alwaystop_true->IsVisible();
				}
				svplCurrent->config.size = { RectGetWidth(m_rcVideo), RectGetHeight(m_rcVideo) };
				svplCurrent->config.location = { m_rcVideo.left, m_rcVideo.top };
				svplwriter::WritePlayListToFile(path, svplCurrent.GetPtr());
			}
		}
		catch (...)
		{

		}
		return 0;
	}

	LRESULT CALLBACK CVideoWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CVideoWindow* pThis = NULL;
		if (uMsg == WM_NCCREATE) {
			LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			pThis = static_cast<CVideoWindow*>(lpcs->lpCreateParams);
			pThis->m_hwnd = hWnd;
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pThis));
		}
		else {
			pThis = reinterpret_cast<CVideoWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (uMsg == WM_NCDESTROY && pThis != NULL) {
				LRESULT lRes = ::SetWindowLongPtr(pThis->m_hwnd, GWLP_USERDATA, 0L);
				pThis->m_hwnd = NULL;
				return lRes;
			}
		}
		if (pThis != NULL) {
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		else {
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	
	LRESULT CVideoWindow::PaintWelcomeImg(int msgType)
	{
		const int canvascx = 2560; const int canvascy = 1440;

		static bool firstload = true;
		static HBITMAP bgbmp = nullptr;
		static HDC bgmemdc = nullptr;

		static Gdiplus::PrivateFontCollection fontCollection;

		RECT rc;
		GetWindowRect(m_hwnd, &rc);
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(m_hwnd, &ps);

		if (firstload)
		{
			firstload = false;
			bgmemdc = CreateCompatibleDC(nullptr);
			SetBkMode(bgmemdc, TRANSPARENT);

			BITMAPINFO bmpinfo;
			ZeroMemory(&bmpinfo, sizeof(bmpinfo));
			bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmpinfo.bmiHeader.biBitCount = 32;
			bmpinfo.bmiHeader.biWidth = canvascx;
			bmpinfo.bmiHeader.biHeight = canvascy;
			bmpinfo.bmiHeader.biCompression = BI_RGB;
			bmpinfo.bmiHeader.biPlanes = 1;

			void* pBits = nullptr;
			bgbmp = CreateDIBSection(bgmemdc, &bmpinfo, DIB_RGB_COLORS, &pBits, nullptr, 0);
			HGDIOBJ hOld = SelectObject(bgmemdc, bgbmp);

			Gdiplus::Graphics bggraphics(bgmemdc);
			Gdiplus::Image image((m_strAppPath + L"welcome.png").c_str());
			
			bggraphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			bggraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
			bggraphics.DrawImage(&image, 0, 0, canvascx, canvascy);
			
			SelectObject(bgmemdc, hOld);
			DeleteDC(bgmemdc);

			fontCollection.AddFontFile((m_strAppPath + L"Regular.otf").c_str());
		}

		static int lastMsgType = 0;

		bool useLasthbmp = lastMsgType == msgType;
		lastMsgType = msgType;

		wchar_t* strmsg = nullptr;
		switch (msgType)
		{
		case 1: strmsg = L"Drag a file here\nto start";	break;
		case 2:	strmsg = L"Enjoy the music"; break;
		}

		static HBITMAP hbmp = nullptr;

		if (!useLasthbmp || hbmp == nullptr)
		{
			// 来自 http://blog.sina.com.cn/s/blog_5f853eb10100unr6.html
			HDC srcdc;
			HDC dstdc;
			srcdc = CreateCompatibleDC(nullptr);
			dstdc = CreateCompatibleDC(nullptr);
			HDC screendc = GetDC(nullptr);
			if (hbmp) DeleteObject(hbmp);
			hbmp = CreateCompatibleBitmap(screendc, canvascx, canvascy);

			HGDIOBJ srcold = SelectObject(srcdc, bgbmp);
			HGDIOBJ dstold = SelectObject(dstdc, hbmp);
			BitBlt(dstdc, 0, 0, canvascx, canvascy, srcdc, 0, 0, SRCCOPY);

			SelectObject(srcdc, srcold);
			SelectObject(dstdc, dstold);
			DeleteObject(srcdc);
			DeleteObject(dstdc);
			ReleaseDC(nullptr, screendc);
		}

		HDC memdc = CreateCompatibleDC(hdc);
		HGDIOBJ hOld = SelectObject(memdc, hbmp);

		if (strmsg && !useLasthbmp)
		{
			Gdiplus::Graphics graphics(memdc);
			graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
			graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

			int numFound = 0;
			unique_ptr<Gdiplus::FontFamily> fontFamily(new Gdiplus::FontFamily);
			fontCollection.GetFamilies(1, fontFamily.get(), &numFound);
			if (numFound <= 0)
			{
				fontFamily.reset(new Gdiplus::FontFamily(L"Microsoft Yahei"));
			}

			Gdiplus::StringFormat strFormat;
			strFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
			Gdiplus::Font font(fontFamily.get(), 120, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
			Gdiplus::RectF dstRect(0, 500, 1500, 1000);

			graphics.DrawString(strmsg, -1, &font, dstRect, &strFormat, &Gdiplus::SolidBrush(Gdiplus::Color::AntiqueWhite));
		}

		SetStretchBltMode(hdc, HALFTONE);
		StretchBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memdc, 0, 0, canvascx, canvascy, SRCCOPY);

		SelectObject(memdc, hOld);
		EndPaint(m_hwnd, &ps);
		return 0;
	}

	HRESULT CVideoWindow::PlayMovieInWindow(LPCTSTR szFile)
	{

	#define JIF(x) if (FAILED(hr=(x))) goto ret

		HRESULT hr = S_OK;
		if (szFile == NULL) return E_POINTER;

		hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		IBaseFilter *pLavSplitterSource = nullptr;
		IFileSourceFilter *pFileSourceFilter = nullptr;
		IBaseFilter *pLavVideoDecoder = nullptr;
		IBaseFilter *pLavAudioDecoder = nullptr;
		IBaseFilter *pAudioRender = nullptr;
		IBaseFilter *pEVR = nullptr;

		IMFGetService *pGS = nullptr;

		IPin *pPinVideo = nullptr;
		IPin *pPinVideoInput = nullptr;
		IPin *pPinAudioInput = nullptr;
		IPin *pPinAudio = nullptr;
		IPin *pPinVideoOutput = nullptr;
		IPin *pPinAudioOutput = nullptr;

		IBaseFilter *pFFDSHOW = nullptr;

		IMFVideoRenderer* imfvr = nullptr;
		IMFVideoPresenter* evrcp = nullptr;

		JIF(CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGB));
		JIF(LoadExternalObject((m_strAppPath + L"filters\\LAVSplitter.ax").c_str(), CLSID_LavSplitter_Source, IID_IBaseFilter, (void**)&pLavSplitterSource));
		JIF(pGB->AddFilter(pLavSplitterSource, L"Lav Splitter Source"));

		JIF(pLavSplitterSource->QueryInterface(IID_IFileSourceFilter, (void **)&pFileSourceFilter));
		JIF(pFileSourceFilter->Load(szFile, NULL));

		// 音频
		JIF(pLavSplitterSource->FindPin(L"Audio", &pPinAudio));

		JIF(LoadExternalObject((m_strAppPath + L"filters\\LAVAudio.ax").c_str(), CLSID_LavAudioDecoder, IID_IBaseFilter, (void**)&pLavAudioDecoder));
		JIF(pGB->AddFilter(pLavAudioDecoder, L"Lav Audio Decoder"));
		JIF(CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pAudioRender));
		JIF(pGB->AddFilter(pAudioRender, L"DirectSound Audio render"));

		JIF(FindUnconnectedPin(pLavAudioDecoder, PINDIR_INPUT, &pPinAudioInput));
		JIF(pGB->Connect(pPinAudio, pPinAudioInput));

		JIF(ConnectFilters(pGB, pLavAudioDecoder, pAudioRender));

		// 视频
		hr = pLavSplitterSource->FindPin(L"Video", &pPinVideo);
		if (FAILED(hr)) {
			m_bAudioOnly = true;
			goto finish;
		}

		JIF(CoCreateInstance(CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pEVR));

		JIF(LoadExternalObject((m_strAppPath + L"filters\\LAVVideo.ax").c_str(), CLSID_LavVideoDecoder, IID_IBaseFilter, (void**)&pLavVideoDecoder));
		JIF(pGB->AddFilter(pLavVideoDecoder, L"Lav Video Decoder"));

		JIF(pGB->AddFilter(pEVR, L"EVR"));
	/*
	  original_EVR: 
		JIF(pEVR->QueryInterface(IID_PPV_ARGS(&pGS)));
		JIF(pGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_pDisplay)));

		if (!IsIconic(m_hwnd)) {
			JIF(m_pDisplay->SetVideoWindow(m_hwnd));
			JIF(m_pDisplay->SetAspectRatioMode(MFVideoARMode_NonLinearStretch | MFVideoARMode_PreservePixel | MFVideoARMode_PreservePicture));
		}
	*/
		JIF(pEVR->QueryInterface(IID_IMFVideoRenderer, (void**)&imfvr));

		JIF(EVRCustomPresenter::CreateInstance(IID_IMFVideoPresenter, (void**)&evrcp));

		JIF(imfvr->InitializeRenderer(nullptr, evrcp));

		JIF(evrcp->QueryInterface(&m_pDisplay));
		JIF(m_pDisplay->SetVideoWindow(m_hwnd));

		if (m_bFullScreen || m_keepwidth == 0) m_pDisplay->SetAspectRatioMode(NoStretch);
		else m_pDisplay->SetAspectRatioMode(Stretch);
		COLORREF border = m_svplwrapper->Get()->config.borderColor;
		if (m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).borderColor != 0)
			border = m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).borderColor;
		if (border != 0) m_pDisplay->SetBorderColor(border);

		JIF(FindUnconnectedPin(pLavVideoDecoder, PINDIR_INPUT, &pPinVideoInput));
		hr = pGB->ConnectDirect(pPinVideo, pPinVideoInput, NULL);

		if (FAILED(hr)) {
			SAFE_RELEASE(pPinVideoInput);
			JIF(pGB->RemoveFilter(pLavVideoDecoder));
			JIF(CoCreateInstance(CLSID_ffdshowVideoDecoder, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pFFDSHOW));
			JIF(pGB->AddFilter(pFFDSHOW, L"ffdshow video Decoder"));
			JIF(pFFDSHOW->FindPin(L"In", &pPinVideoInput));
			JIF(pGB->ConnectDirect(pPinVideo, pPinVideoInput, NULL));
			JIF(ConnectFilters(pGB, pFFDSHOW, pEVR));
			SAFE_RELEASE(pFFDSHOW);
		}
		else {
			IPin *pIn = nullptr;
			hr = FindUnconnectedPin(pEVR, PINDIR_INPUT, &pIn);
			IPin *pOut = nullptr;
			if (SUCCEEDED(hr)) hr = FindUnconnectedPin(pLavVideoDecoder, PINDIR_OUTPUT, &pOut);
			if (SUCCEEDED(hr)) hr = pGB->ConnectDirect(pOut, pIn, nullptr);
			SAFE_RELEASE(pIn);
			SAFE_RELEASE(pOut);
			if (FAILED(hr)) goto ret;
		}


	finish:

		SAFE_RELEASE(pPinVideo);
		SAFE_RELEASE(pPinVideoInput);
		SAFE_RELEASE(pPinAudioInput);
		SAFE_RELEASE(pPinAudio);
		SAFE_RELEASE(pPinVideoOutput);
		SAFE_RELEASE(pPinAudioOutput);
		SAFE_RELEASE(pGS);
		SAFE_RELEASE(pEVR);
		SAFE_RELEASE(pAudioRender);
		SAFE_RELEASE(pLavAudioDecoder);
		SAFE_RELEASE(pLavVideoDecoder);
		SAFE_RELEASE(pFileSourceFilter);
		SAFE_RELEASE(pLavSplitterSource);

		// Query DirectShow interfaces
		JIF(pGB->QueryInterface(IID_IMediaControl, (void **)&pMC));
		JIF(pGB->QueryInterface(IID_IMediaEventEx, (void **)&pME));
		JIF(pGB->QueryInterface(IID_IMediaSeeking, (void **)&pMS));
		JIF(pGB->QueryInterface(IID_IMediaPosition, (void **)&pMP));
		JIF(pGB->QueryInterface(IID_IBasicAudio, (void **)&pBA));
		
		{
			SIZE lSize = { -100, -100 };

			// Is this an audio-only file
			if (!m_bAudioOnly)
			{
				m_bAudioOnly = FALSE;
				if (m_pDisplay)
				{
					m_pDisplay->GetNativeVideoSize(&lSize, NULL); // won't fail
					_trace(L"Video Size: %d,%d\n", lSize.cx, lSize.cy);
					if ((lSize.cx <= 0) || (lSize.cy <= 0)) m_bAudioOnly = TRUE;
				}
				else
				{
					m_bAudioOnly = TRUE;
				}
			}

			// Have the graph signal event via window callbacks for performance

			JIF(pME->SetNotifyWindow((OAHWND)m_hwnd, WM_GRAPHNOTIFY, 0));

			if (!m_bAudioOnly)
			{
				JIF(InitVideoWindow(lSize));
			}
			else
			{
				JIF(InitPlayerWindow());
			}

		}

		// Complete initialization
		UpdateMainTitle();
		JIF(pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME));

		double StartPos = 0, EndPos = 0, Duration = 0;
		StartPos = m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).begintime;
		EndPos = m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).endtime;
		GetDuration(&Duration);
		if (StartPos <= 0) StartPos = 0;
		if (EndPos > Duration || EndPos <= StartPos) EndPos = Duration;
		SetStartStopPosition(StartPos, EndPos);

		JIF(pMC->Run());

		m_psCurrent = Running;

	ret:
		SAFE_RELEASE(pFFDSHOW);
		SAFE_RELEASE(pPinVideo);
		SAFE_RELEASE(pPinVideoInput);
		SAFE_RELEASE(pPinAudioInput);
		SAFE_RELEASE(pPinAudio);
		SAFE_RELEASE(pPinVideoOutput);
		SAFE_RELEASE(pPinAudioOutput);
		SAFE_RELEASE(pGS);
		SAFE_RELEASE(pEVR);
		SAFE_RELEASE(pAudioRender);
		SAFE_RELEASE(pLavAudioDecoder);
		SAFE_RELEASE(pLavVideoDecoder);
		SAFE_RELEASE(pFileSourceFilter);
		SAFE_RELEASE(pLavSplitterSource);

		SAFE_RELEASE(evrcp);
		SAFE_RELEASE(imfvr);
		return hr;
	#undef JIF
	}

	HRESULT CVideoWindow::InitVideoWindow(SIZE lSize)
	{
		long lHeight, lWidth;
		HRESULT hr = S_OK;
		if (!m_pDisplay) return S_OK;

		if (hr == E_NOINTERFACE) return S_OK;
		lHeight = lSize.cy;
		lWidth = lSize.cx;
		
		m_dScale = (double)lWidth / (double)lHeight;  // 存储视频横纵比信息
		if (m_keepwidth != 0) {
			lWidth = m_keepwidth;
			lHeight = (long)((double)lWidth / m_dScale);
		}
		else if (m_windowAspectRatio.x != 0 && m_windowAspectRatio.y != 0) {
			lWidth = RectGetWidth(m_rcVideo);
			lHeight = (long)((double)lWidth * m_windowAspectRatio.y / m_windowAspectRatio.x);
		}
		else {
			lWidth = RectGetWidth(m_rcVideo);
			lHeight = RectGetHeight(m_rcVideo);
		}
		if (!IsIconic(m_hwnd)) {
			if (!m_bFullScreen)
			{
				ResizeWindow(lWidth, lHeight);

				RECT rcDest;
				GetClientRect(m_hwnd, &rcDest);
				hr = m_pDisplay->SetVideoPosition(NULL, &rcDest);
				GetWindowRect(m_hwnd, &m_rcVideo);
			}
			else
			{
				RECT rcDest;
				GetClientRect(m_hwnd, &rcDest);
				hr = m_pDisplay->SetVideoPosition(NULL, &rcDest);

				m_rcVideo.right = m_rcVideo.left + lWidth;
				m_rcVideo.bottom = m_rcVideo.top + lHeight;
			}
		}
		else {
			m_rcVideo.right = m_rcVideo.left + lWidth;
			m_rcVideo.bottom = m_rcVideo.top + lHeight;
		}

		return hr;
	}

	HRESULT CVideoWindow::InitPlayerWindow()
	{
		long lWidth = RectGetWidth(m_rcVideo);
		long lHeight = RectGetHeight(m_rcVideo);

		if (m_keepwidth != 0) {
			lWidth = m_keepwidth;
			lHeight = (long)(lWidth / DEFAULT_SCALE);
		}
		m_rcVideo.right = m_rcVideo.left + lWidth;
		m_rcVideo.bottom = m_rcVideo.top + lHeight;

		if (!m_bFullScreen)
		{
			SetWindowPos(m_hwnd, NULL, 0, 0, lWidth, lHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		InvalidateRect(m_hwnd, NULL, true);
		return S_OK;
	}

	void CVideoWindow::MoveVideoWindow(int cx, int cy)
	{
		// Track the movement of the container window and resize as needed
		HRESULT hr;
		if (m_pDisplay)
		{
			RECT rcDest = { 0, 0, cx, cy };
			hr = m_pDisplay->SetVideoPosition(NULL, &rcDest);
		}
	}

	void CVideoWindow::PauseClip()
	{
		if (!pMC) return;
		if (m_psCurrent == Paused)
		{
			if (SUCCEEDED(pMC->Run())) {
				m_psCurrent = Running;
				if (m_cCon) m_cCon->m_btnPlay->SetVisible(false);
				if (m_cCon) m_cCon->m_btnPause->SetVisible(true);
			}

		}
		else if (m_psCurrent == Running)
		{
			if (SUCCEEDED(pMC->Pause())) {
				m_psCurrent = Paused;
				if (m_cCon) m_cCon->m_btnPlay->SetVisible(true);
				if (m_cCon) m_cCon->m_btnPause->SetVisible(false);
			}
		}
	}

	void CVideoWindow::StopClip()
	{
		CloseClip();
		m_psCurrent = Stopped;   // 通知WM_PAINT以start界面填充
		InvalidateRect(m_hwnd, NULL, true);

		if (m_cCon) m_cCon->m_btnPlay->SetVisible(true);
		if (m_cCon) m_cCon->m_btnPause->SetVisible(false);
		if (m_cCon) m_cCon->m_SliderPlayProcess->SetValue(0);
	}

	int CVideoWindow::OpenClip(int index)
	{
		if (m_bBeginFromFirst)
		{
			index = 0;
			m_bBeginFromFirst = false;
		}
		if (index < 0 || index >= (int)m_svplwrapper->GetItemNum(m_nowplaylist)) return -1;
		m_nowplaying = index;
		OpenClip(m_svplwrapper->GetItem(m_nowplaylist, index).path.c_str());
		return 0;
	}

	void CVideoWindow::OpenClip(LPCTSTR szFilename)
	{
		if (m_psCurrent != Stopped && m_psCurrent != Switching) CloseClip();

		HRESULT hr;
		m_strMediaFile = szFilename;

		// 统计自上次成功播放或者提示ERR_TOO_MANY_INCORRECT_CLIP后的所有错误次数
		static int total_err_times = 0;

		// Start playing the media file
		hr = PlayMovieInWindow(szFilename);
		if (FAILED(hr)) {
			total_err_times++;
			SHOWMSG(MsgIcon::Exclamation, ERR_CANNOT_OPEN_CLIP, GetFilename(szFilename).c_str(), szFilename, hr);
			if (total_err_times > 30)
			{
				StopClip();
				total_err_times = 0;
				SHOWMSG_R(MsgIcon::Exclamation, ERR_TOO_MANY_INCORRECT_CLIP);
			}
			else if (m_svplwrapper->GetItemNum(m_nowplaylist) < 50) // 数量小于50才遍历，不然由 total_err_times 保证
			{
				// Checkn errtime
				bool ExistAvailabeClip = false;
				auto rsvpl = m_svplwrapper->Get();
				if (rsvpl->playlists.at(m_nowplaylist).items.at(m_nowplaying).errtime < 10000)
					rsvpl->playlists.at(m_nowplaylist).items.at(m_nowplaying).errtime++;
				for (auto ritem : rsvpl->playlists.at(m_nowplaylist).items)
				{
					if (ritem.errtime <= 3) { ExistAvailabeClip = true; break; }
				}
				if (ExistAvailabeClip)
				{
					CloseClip();
					PostMessage(m_hwnd, WM_PlayNext, 0, 0);
				}
				else
				{
					StopClip();
					SHOWMSG_R(MsgIcon::Exclamation, ERR_NO_AVAILABLE_CLIP);
				}
			}
			else
			{
				CloseClip();
				PostMessage(m_hwnd, WM_PlayNext, 0, 0);
			}
			return;
		}

		if (m_cCon) m_cCon->m_btnPlay->SetVisible(false);
		if (m_cCon) m_cCon->m_btnPause->SetVisible(true);

		ChangeVolume((long)(m_iVolume * m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).volume / 100));

		total_err_times = 0;
	}

	void CVideoWindow::CloseClip()
	{
		// Free DirectShow interfaces
		CloseInterfaces();

		m_psCurrent = Switching;
		m_dScale = DEFAULT_SCALE;
		m_strMediaFile = L"";
		m_bAudioOnly = false;

		if (m_cCon) m_cCon->m_lblTime->SetText(L"");
		if (m_cCon) m_cCon->m_lblTitle->SetText(L"");
		if (m_cCon) m_cCon->m_SliderPlayProcess->SetValue(0);

		UpdateMainTitle();
	}

	void CVideoWindow::PlayPrev() {
		if (m_svplwrapper->GetItemNum(m_nowplaylist) == 0) return;
		if (m_nowplaying <= 0) {
			// 到了播放列表头
			m_nowplaying = m_svplwrapper->GetItemNum(m_nowplaylist) - 1;
		}
		else {
			m_nowplaying--;
		}
		OpenClip(m_nowplaying);
	}

	void CVideoWindow::PlayNext() {
		size_t num = m_svplwrapper->GetItemNum(m_nowplaylist);
		if (num == 0) return;
		switch (m_PlayMode)
		{
		case PlayMode::Queue:
			if (m_nowplaying >= (int)num - 1) {
				// 到了播放列表末尾
				m_nowplaying = 0;
			}
			else {
				m_nowplaying++;
			}
			break;
		case PlayMode::Loop:
			break;
		case PlayMode::Random:
			if (num == 1) break;
			do
				m_nowplaying = rand() % num;
			while (m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).path == m_strMediaFile);
			_trace(L"m_nowplaying: %d \n", m_nowplaying);
			break;
		}

		OpenClip(m_nowplaying);

	}

	void CVideoWindow::CloseInterfaces()
	{
		HRESULT hr;
		// Stop media playback
		if (pMC)
			hr = pMC->Stop();

		// Disable event callbacks
		if (pME)
			hr = pME->SetNotifyWindow((OAHWND)NULL, 0, 0);

		if (m_pDisplay) {
			//hr = m_pDisplay->SetVideoWindow((HWND)NULL);
			//hr = m_pDisplay->SetVideoPosition(NULL, NULL);
		}

		if (pGB) {
			// Enumerate the filters And remove them
			IEnumFilters *pEnum = NULL;
			hr = pGB->EnumFilters(&pEnum);
			if (SUCCEEDED(hr))
			{
				IBaseFilter *pFilter = NULL;
				while (S_OK == pEnum->Next(1, &pFilter, NULL))
				{
					// Remove the filter.
					pGB->RemoveFilter(pFilter);
					// Reset the enumerator.
					pEnum->Reset();
					SAFE_RELEASE(pFilter);
				}
				pEnum->Release();
			}
		}

		// Release and zero DirectShow interfaces
		SAFE_RELEASE(pME);
		SAFE_RELEASE(pMS);
		SAFE_RELEASE(pMP);
		SAFE_RELEASE(pMC);
		SAFE_RELEASE(pBA);
		SAFE_RELEASE(m_pDisplay);
		SAFE_RELEASE(pGB);
	}

	void CVideoWindow::UpdateMainTitle()
	{
		wstring newtitle;
		newtitle = GetFilename(m_strMediaFile);
		if (m_cCon) m_cCon->m_lblTitle->SetText(newtitle.c_str());

		if (newtitle == L"") newtitle = MyTitle;
		else newtitle = newtitle + _T(" - ") + MyTitle;
		SetWindowText(m_hwnd, newtitle.c_str());

	}

	HRESULT CVideoWindow::HandleGraphEvent()
	{
		LONG evCode;
		LONG_PTR evParam1, evParam2;
		HRESULT hr = S_OK;

		// Make sure that we don't access the media event interface
		// after it has already been released.
		if (!pME)
			return S_OK;

		// Process all queued events
		while (SUCCEEDED(pME->GetEvent(&evCode, &evParam1, &evParam2, 0)))
		{
			// Free memory associated with callback, since we're not using it
			hr = pME->FreeEventParams(evCode, evParam1, evParam2);

			// If this is the end of the clip, do something
			if (EC_COMPLETE == evCode)
			{
				// 播放结束
				if (m_cCon) m_cCon->ReleaseCapture();   // 释放鼠标，防止切换时正在拖动slider_playprocess而出问题
				PlayNext();
				break;
			}
		}
		return hr;
	}

	HRESULT CVideoWindow::ChangeVolume(long volume)
	{
		if (!pBA) return 0;
		if (m_bMuted) volume = 0;
		HRESULT hr = S_OK;
		if (volume < 0) volume = 0;
		if (volume >= 100) volume = 99;
		hr = pBA->put_Volume(volumes[volume]);
		return hr;
	}

	HRESULT CVideoWindow::Mute()
	{
		HRESULT hr = S_OK;
		if (m_bMuted)
		{
			m_bMuted = false;
			hr = ChangeVolume(long(m_iVolume * m_svplwrapper->GetItem(m_nowplaylist, m_nowplaying).volume / 100));
		}
		else
		{
			m_bMuted = true;
			hr = ChangeVolume(0);
		}
		return hr;
	}

	bool CVideoWindow::GetDuration(double * outDuration)
	{
		if (pMS)
		{
			__int64 length = 0;
			if (SUCCEEDED(pMS->GetDuration(&length)))
			{
				*outDuration = (double)(length / 10000000.0);
				return true;
			}
		}
		return false;
	}

	bool CVideoWindow::GetCurrentPosition(double *outPosition)
	{
		if (pMS)
		{
			__int64 position = 0;
			if (SUCCEEDED(pMS->GetCurrentPosition(&position)))
			{
				*outPosition = (double)(position / 10000000.0);
				return true;
			}
		}
		return false;
	}

	bool CVideoWindow::GetStopPosition(double *outPosition)
	{
		if (pMS)
		{
			__int64 position = 0;
			if (SUCCEEDED(pMS->GetStopPosition(&position)))
			{
				*outPosition = (double)(position / 10000000.0);
				return true;
			}
		}
		return false;
	}

	bool CVideoWindow::SetCurrentPosition(double Position)
	{
		if (pMS)
		{
			__int64 pos = __int64(10000000 * Position);//首先转换为正规时间格式  
			HRESULT hr = pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, AM_SEEKING_NoPositioning);
			if (SUCCEEDED(hr))
			{
				return true;
			}
		}
		return false;
	}

	bool CVideoWindow::SetStartStopPosition(double inStart, double inStop)
	{
		if (pMS)
		{
			__int64 one = 10000000;
			__int64 startPos = (__int64)(one * inStart);
			__int64 stopPos = (__int64)(one * inStop);
			HRESULT hr = pMS->SetPositions(&startPos, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame,
				&stopPos, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame);
			return SUCCEEDED(hr);
		}
		return false;
	}

	void CVideoWindow::ConvertTime(double time, TCHAR *strBuffer)
	{
		int hour = int(time / 3600);
		time = time - hour * 3600;
		int minute = int(time / 60);
		int second = int(time - minute * 60);
		if (hour != 0) swprintf(strBuffer, 100, L"%02d:%02d:%02d", hour, minute, second);
		else swprintf(strBuffer, 100, L"%02d:%02d", minute, second);
	}

	// 判断鼠标是否在窗口内
	// 返回值：窗口不可见返回-1 光标在窗口RECT外返回-2 光标在RECT内但被其他窗口遮挡返回0 光标在RECT内且未被其他窗口遮挡返回1
	BOOL CVideoWindow::IsCursorInWindow(HWND hWnd, POINT ptcursor)
	{
		RECT rect;
		GetWindowRect(hWnd, &rect);
		if (!IsWindowVisible(hWnd)) return -1;

		while (NULL != (hWnd = GetNextWindow(hWnd, GW_HWNDPREV)))
		{
			/*if (GetParent(hWnd) == m_hwnd || GetParent(hWnd) == m_hCon) continue;*/
			if (IsWindowVisible(hWnd) && hWnd != m_hCon)
			{
				RECT rcWindow;
				GetWindowRect(hWnd, &rcWindow);
				if (ptcursor.x > rcWindow.left && ptcursor.x < rcWindow.right && ptcursor.y > rcWindow.top && ptcursor.y < rcWindow.bottom)
				{
					return 0;
				}
			}
		}

		// 因为要判断最小化关闭按钮，必须先判断是否覆盖，将以下两句移到最后
		if (ptcursor.x > rect.right - 75 && ptcursor.x < rect.right && ptcursor.y < rect.top + 25 && ptcursor.y > rect.top) return 1;
		if (!(rect.bottom >= ptcursor.y + 5 && rect.top <= ptcursor.y - 10 && rect.left <= ptcursor.x - 10 && rect.right >= ptcursor.x + 10)) return -2;

		return 1;
	}

	// 更新Control窗口中的slider、label信息
	HRESULT CVideoWindow::UpdateVideoPos(bool updateSlider /* = true*/)
	{
		HRESULT hr = S_OK;
		if (m_psCurrent == Running || m_psCurrent == Paused) {
			double pos = 0, duration = 1;
			GetCurrentPosition(&pos);
			GetDuration(&duration);
			// _trace(L"newpos=%d pos=%f duration=%f \n", newpos, pos, duration);
			TCHAR msgProcess[100] = { 0 };
			TCHAR tempstr[100];
			ConvertTime(pos, tempstr);
			wcscat(msgProcess, tempstr);
			wcscat(msgProcess, L" / ");
			ConvertTime(duration, tempstr);
			wcscat(msgProcess, tempstr);
			if (m_cCon)
			{
				m_cCon->m_lblTime->SetText(msgProcess);
				if (updateSlider)
				{
					int newpos = int(pos * 1000 / duration);
					if (m_cCon->m_SliderPlayProcess->GetValue() != newpos)
					{
						m_cCon->m_SliderPlayProcess->SetValue(newpos);
					}
				}
			}
		}
		return hr;
	}

	void CVideoWindow::SetNewWrapper(shared_ptr<svplwrapper> new_svplwrapper)
	{
		m_svplwrapper = new_svplwrapper;
		m_strPlaylistFile = L"";
		auto rsvpl = m_svplwrapper->Get();
		m_nowplaylist = rsvpl->config.normalplaylist;
		m_keepwidth = rsvpl->playlists.at(m_nowplaylist).keepwidth;
		m_iVolume = rsvpl->playlists.at(m_nowplaylist).volume;
		m_windowAspectRatio = rsvpl->config.windowAspectRatio;
		if (m_cCon)
		{
			m_cCon->GetPlayListWindow()->Refresh();
			m_cCon->m_SliderVolume->SetValue(m_iVolume);
		}
		_trace(L"count: %d\n", new_svplwrapper.use_count());
	
	}

	bool CVideoWindow::IsMuted()
	{
		return m_bMuted;
	}

}
