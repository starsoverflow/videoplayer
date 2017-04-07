#pragma once
#include "../DuiLib/UIlib.h"
#include "../mydebug.h"
using namespace DuiLib;

namespace SVideoPlayer
{
	class CVideoWindow;
	class CPlayListWindow;
	class vp_DragDrop;

	class CControlWindow : public WindowImplBase
	{
	public:
		explicit CControlWindow(LPCTSTR pszXMLPath, CVideoWindow* BindingVideoWindow);
		~CControlWindow();

		CSliderUI *m_SliderPlayProcess = nullptr;
		CButtonUI *m_btnPrev = nullptr;
		CButtonUI *m_btnPlay = nullptr;
		CButtonUI *m_btnPause = nullptr;
		CButtonUI *m_btnNext = nullptr;
		CButtonUI *m_btnClose = nullptr;
		CSliderUI *m_SliderVolume = nullptr;
		CContainerUI *m_barTitle = nullptr;
		CContainerUI *m_barPlayStop = nullptr;
		CContainerUI *m_barSlider = nullptr;
		CContainerUI *m_barControl = nullptr;
		CLabelUI *m_lblTime = nullptr;
		CButtonUI *m_alwaystop_true = nullptr;
		CButtonUI *m_alwaystop_false = nullptr;
		CLabelUI *m_lblTitle = nullptr;
		CButtonUI *m_btnMin = nullptr;

		void Hide(bool hide = true);
		bool IsOnControl(POINT pt);
		void ReleaseCapture();

		LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

		CVideoWindow* GetBindedVideoWindow()
		{
			return m_cVideo;
		}

		CPlayListWindow* GetPlayListWindow()
		{
			return m_cPlaylist;
		}

		bool IsCaptured();

	protected:
		LPCTSTR GetWindowClassName() const;
		CDuiString GetSkinFile();
		CDuiString GetSkinFolder();
		UILIB_RESOURCETYPE GetResourceType() const;
		LPCTSTR GetResourceID() const;

		LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;

	private:

		void InitWindow();
		void OnPrepare(TNotifyUI& msg);
		void Notify(TNotifyUI& msg);
		CControlUI* CreateControl(LPCTSTR pstrClassName);
		LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
		LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) override;

		CDuiString m_strXMLPath;
		CVideoWindow* m_cVideo = nullptr;
		CPlayListWindow* m_cPlaylist = nullptr;
		HWND m_hVideo;

		vp_DragDrop* plwnd_DragDrop = nullptr;
		LONG lastPointx = 0;    // 存储上一次slider的横坐标值
	};
}