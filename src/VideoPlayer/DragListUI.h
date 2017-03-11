#pragma once
#include "WindowUI.h"

using namespace DuiLib;
namespace Star_VideoPlayer
{
	class DragListUI;
	class CPlayListWindow;

	class DragImgUI : public WindowUI
	{
	public:
		explicit DragImgUI();
		~DragImgUI();

		CLabelUI label;
		void CreateControl();

		void Notify(TNotifyUI& msg);
		LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

		LPCTSTR GetWindowClassName() const;


		LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

		LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

		LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

		LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

		bool FirstClick = false;

		DragListUI* owner = nullptr;

		LRESULT WindowClose();

	};

	class DragListUI : public CListLabelElementUI
	{
	public:
		DragListUI(CPlayListWindow* playlistWindow)
		{
			m_plwnd = playlistWindow;
		}

		void DoEvent(TEventUI& event);
		void BeginDrag();

		void MoveToNewPos();
		void EndDrag();

		LPCTSTR DragListUI::GetClass() const;
		LPVOID DragListUI::GetInterface(LPCTSTR pstrName);

		CControlUI* dragline = nullptr;

		int index = 0;

		CPlayListWindow* m_plwnd = nullptr;

		DragImgUI* dragimg;

		static DragListUI* pScrollTimerData;

		void KillScrollTimer();
	private:
		static void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
	};
}