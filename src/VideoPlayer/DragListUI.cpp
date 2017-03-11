
#include "DragListUI.h"
#include "../mydebug.h"
#include "svplreader.h"

#include "PlayListWindow.h"
#include "VideoWindow.h"

namespace Star_VideoPlayer
{

#define ScrollTimerID 2421
#define ScrollStep 3
	DragListUI* DragListUI::pScrollTimerData = nullptr;

	LPCTSTR DragListUI::GetClass() const
	{
		return _T("DragListUI");
	}

	LPVOID DragListUI::GetInterface(LPCTSTR pstrName)
	{
		if (_tcscmp(pstrName, _T("DragList")) == 0) return static_cast<DragListUI*>(this);
		return CListLabelElementUI::GetInterface(pstrName);
	}

	void DragListUI::DoEvent(TEventUI & event)
	{
		static bool isbuttondown = false;

		if (event.Type == UIEVENT_BUTTONDOWN && IsEnabled())
		{
			//POINT mousept = event.ptMouse;
			//ClientToScreen(hPlaylist, &mousept);
			isbuttondown = true;
			_trace(L"UIEVENT_BUTTONDOWN\n");
		}

		if (event.Type == UIEVENT_MOUSEMOVE)
		{
			if (isbuttondown)
			{
				_trace(L"button down mousemove %d\n", GetTickCount());
				isbuttondown = false;
				SendMessage(m_plwnd->GetHWND(), WM_LBUTTONUP, 0, 0);
				BeginDrag();
			}
		}

		if (event.Type == UIEVENT_BUTTONUP)
		{
			//	if (isbuttondown)
			//	{
			isbuttondown = false;
			_trace(L"UIEVENT_BUTTONUP\n");
			//	}
		}
		CListLabelElementUI::DoEvent(event);

	}

	void DragListUI::BeginDrag()
	{
		// 创建一个新的窗口来存储图像
		RECT rect = this->GetPos();

		POINT pt = { rect.left,rect.top };

		ClientToScreen(m_plwnd->GetHWND(), &pt);

		dragimg = new DragImgUI;

		dragimg->FirstClick = true;
		dragimg->owner = this;

		index = this->m_pOwner->GetCurSel();

		dragimg->label.SetText(this->GetText());

		dragimg->label.SetTextColor(this->m_pOwner->GetListInfo()->dwSelectedTextColor);


		int windowwidth = this->EstimateSize({ 0 }).cx;

		if (windowwidth < this->m_pOwner->GetWidth())
		{
			windowwidth = this->m_pOwner->GetWidth();
			windowwidth -= this->GetPadding().left;
			if (this->m_pOwner->GetVerticalScrollBar()->IsVisible()) windowwidth -= this->m_pOwner->GetVerticalScrollBar()->GetWidth();
			if (this->m_pOwner->GetHorizontalScrollBar()->IsVisible()) windowwidth += this->m_pOwner->GetHorizontalScrollBar()->GetScrollPos();
		}

		dragimg->Create(m_plwnd->GetHWND(), L"DragWindow", WS_POPUP, NULL, pt.x, pt.y, windowwidth, rect.bottom - rect.top);

		dragimg->pRoot->SetBkColor(this->m_pOwner->GetListInfo()->dwSelectedBkColor);

		dragimg->CreateControl();
		dragimg->ShowWindow();

		::SetCapture(dragimg->GetHWND());

		_trace(L"%d %d\n", rect.top, rect.left);

	}

	void DragListUI::TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
	{
		if (pScrollTimerData != nullptr)
		{
			pScrollTimerData->MoveToNewPos();
		}
	}

	void DragListUI::KillScrollTimer()
	{
		pScrollTimerData = nullptr;
		KillTimer(nullptr, ScrollTimerID);
	}

	void DragListUI::MoveToNewPos()
	{
		if (!dragimg) return;
		if (!this->m_pOwner) return;
		RECT playlistrc;
		GetWindowRect(dragimg->GetHWND(), &playlistrc);
		POINT pt = { playlistrc.left, playlistrc.top };

		ScreenToClient(m_plwnd->GetHWND(), &pt);

		_trace(L"x:%d, y:%d", pt.x, pt.y);

		RECT rct = this->m_pOwner->GetPos();

		int singleheight = this->GetHeight();
		singleheight += this->GetPadding().bottom + this->GetPadding().top;

		// we only need pt.y

		int curindex = this->m_pOwner->GetCurSel();

		int maxindex = this->m_pOwner->GetCount() - 1;

		int height = pt.y - rct.top;

		if (height < -10)
		{
			this->m_pOwner->Scroll(0, -ScrollStep);
			bool TimerAlreadySet = pScrollTimerData != nullptr;
			pScrollTimerData = this;

			if (!TimerAlreadySet) SetTimer(nullptr, ScrollTimerID, 100, TimerProc);
		}
		else if (pt.y > rct.bottom)
		{
			this->m_pOwner->Scroll(0, ScrollStep);
			bool TimerAlreadySet = pScrollTimerData != nullptr;
			pScrollTimerData = this;
			if (!TimerAlreadySet) SetTimer(nullptr, ScrollTimerID, 100, TimerProc);
		}
		else
		{
			pScrollTimerData = nullptr;
			KillTimer(nullptr, ScrollTimerID);
		}
		// calculate the scroll bar

		SIZE scrollpos = this->m_pOwner->GetScrollPos();
		int hiddennum = scrollpos.cy / singleheight;
		int hiddenheight = scrollpos.cy - hiddennum * singleheight;
		height += hiddenheight;
		curindex -= hiddennum;

		if (height < 0) index = 0;
		else {
			index = height / singleheight + 1;
		}

		if (index > maxindex + 1) index = maxindex + 1;

		if (!dragline) dragline = m_pManager->FindControl(_T("dragline"));

		bool isvalidindex = true;

		if (dragline)
		{
			if (index != curindex && index != curindex + 1)
			{
				// paint a line
				int lineheight = index * singleheight + rct.top - hiddenheight;
				RECT rc = { 10/*5+5 5是padding*/, lineheight + 3, rct.right - rct.left /*-5+5=0 5是padding*/ , lineheight + 5 };

				// get the horizontal scroll bar height and judge whether to show the dragline
				if (lineheight<rct.top || lineheight>rct.bottom - ((this->m_pOwner->GetHorizontalScrollBar()->IsVisible()) ? this->m_pOwner->GetHorizontalScrollBar()->GetHeight() : 0))
				{
					dragline->SetVisible(false);
					isvalidindex = false;
				}
				else {
					dragline->SetPos(rc);
					dragline->SetVisible(true);
				}
			}
			else
			{
				dragline->SetVisible(false);
			}
		}

		if (isvalidindex) index += hiddennum;
		else index = -1;

		_trace(L"height:%d index:%d\n", height, index);
	}

	void DragListUI::EndDrag()
	{
		if (index < 0) return;

		int curindex = this->m_pOwner->GetCurSel();
		int maxindex = this->m_pOwner->GetCount() - 1;
		if (curindex < 0 || maxindex < 0) return;
		if (curindex != index)
		{

			SIZE scrollpos = this->m_pOwner->GetScrollPos();

			DragListUI* ta = new DragListUI(m_plwnd), *ttoclone = static_cast<DragListUI*>(this->m_pOwner->GetItemAt(curindex));
			if (ttoclone == nullptr) return;
			*ta = *ttoclone;
			
			// 类封装后添加 2016.10.5
			auto svplWrapper = m_plwnd->m_videownd->GetSvplWrapper();
			int nowplaylist = m_plwnd->m_videownd->m_nowplaylist;
			int nowplaying = m_plwnd->m_videownd->m_nowplaying;

			svpl_item tai;
			if (!svplWrapper->GetItem(curindex, &tai, nowplaylist)) return;

			this->m_pOwner->AddAt(ta, index);
			svplWrapper->AddItem(tai, index, nowplaylist);

			if (index < curindex) {
				this->m_pOwner->RemoveAt(curindex + 1);
				svplWrapper->DeleteItem(curindex + 1, nowplaylist);
				this->m_pOwner->SelectItem(index, true);
			}
			else {
				this->m_pOwner->RemoveAt(curindex);
				svplWrapper->DeleteItem(curindex, nowplaylist);
				this->m_pOwner->SelectItem(index - 1, true);
			}

			if (nowplaying == curindex)
			{
				nowplaying = index;
				if (index > curindex) {
					nowplaying--;
				}
			}
			else if (index <= nowplaying && curindex > nowplaying) {
				nowplaying++;
			}
			else if (index > nowplaying && curindex < nowplaying) {
				nowplaying--;
			}

			m_plwnd->m_videownd->m_nowplaying = nowplaying;

			this->m_pOwner->SetScrollPos(scrollpos);
			_trace(L"nowplaying: %d\n", nowplaying);
		}
	}

	DragImgUI::DragImgUI()
	{

	}

	DragImgUI::~DragImgUI()
	{
		_trace(L"released\n");
	}

	LPCTSTR DragImgUI::GetWindowClassName() const
	{
		return _T("Star Video Player_Drag Window");
	}

	LRESULT DragImgUI::WindowClose()
	{
		{
			owner->KillScrollTimer();
			ReleaseCapture();
			if (owner->dragline)
			{
				owner->dragline->SetVisible(false);
			}
			DestroyWindow(this->GetHWND());
		}
		return 0;
	}

	LRESULT DragImgUI::OnKillFocus(UINT, WPARAM, LPARAM, BOOL & bHandled)
	{
		bHandled = TRUE;

		_trace(L"OnKillFocus up \n");
		WindowClose();
		return LRESULT();
	}

	LRESULT DragImgUI::OnLButtonUp(UINT, WPARAM, LPARAM, BOOL & bHandled)
	{
		bHandled = TRUE;
		_trace(L"button up \n");

		WindowClose();
		owner->EndDrag();

		return LRESULT();
	}

	LRESULT DragImgUI::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
	{
		bHandled = TRUE;
		WindowClose();
		return LRESULT();
	}

	LRESULT DragImgUI::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled = TRUE;
		//_trace(L"mousemoved %d %d\n", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		static POINT lastPoint = { 0 };
		POINT point;
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);

		if (!FirstClick) {
			if (wParam == MK_LBUTTON)
			{
				::SetCapture(GetHWND());
				if (point.x != lastPoint.x || point.y != lastPoint.y) {
					RECT rcWindow;
					GetWindowRect(this->GetHWND(), &rcWindow);
					SetWindowPos(this->GetHWND(), NULL, rcWindow.left + point.x - lastPoint.x, rcWindow.top + point.y - lastPoint.y, 0, 0, SWP_NOSIZE);

					// 判断位置
					owner->MoveToNewPos();

					return 0;
				}
			}
		}
		else FirstClick = false;
		lastPoint = point;

		return LRESULT();
	}

	void DragImgUI::CreateControl()
	{
		// 这里的字体须和 List 中定义的相同
		m_PaintManager.SetDefaultFont(L"微软雅黑", 14, false, false, false);
		m_PaintManager.SetTransparent(100);
		pRoot->Add(&label);

	}

	void DragImgUI::Notify(TNotifyUI & msg)
	{

		if (msg.sType == _T("windowinit"))
		{

		}
		else if (msg.sType == _T("click"))
		{

		}
		else if (msg.sType == _T("menu"))
		{

			int x = GET_X_LPARAM(msg.lParam);
			int y = GET_Y_LPARAM(msg.lParam);

		}
	}

	LRESULT DragImgUI::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
	{
		return HTCLIENT;
	}
}