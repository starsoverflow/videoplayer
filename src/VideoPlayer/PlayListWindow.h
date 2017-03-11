#pragma once

#include "../DuiLib/UIlib.h"
#include "../resource.h"

using namespace DuiLib;

namespace Star_VideoPlayer
{
	class CVideoWindow;
	class CPlayListWindow : public WindowImplBase
	{
	public:
		explicit CPlayListWindow(CVideoWindow* videoWindow);
		~CPlayListWindow();

		void InitWindow();
		void OnPrepare(TNotifyUI& msg);
		void Notify(TNotifyUI& msg);
		CControlUI* CreateControl(LPCTSTR pstrClassName);
		LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
		void Refresh();
		CListUI* m_playlist;
		CVideoWindow*   m_videownd;

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

		LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	private:

	};
}
