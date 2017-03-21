#pragma once
#include <tchar.h>

#include "VideoCommon.h"
#include "svplreader.h"
#include "MyMenu.h"

#include "../mydebug.h"

#include <memory>

#define WM_GRAPHNOTIFY    WM_USER+13
#define WM_DragDropDone   WM_USER+17
#define WM_SwitchClip     WM_USER+18
#define WM_PlayNext       WM_USER+19
#define WM_AddMediaFile   WM_USER+20
#define WM_ReloadPlaylist WM_USER+21

#define DEFAULT_WIDTH     800
#define DEFAULT_HEIGHT    450
#define MINIMUM_WIDTH     480
#define MINIMUM_HEIGHT    270
#define MAXIMUM_WIDTH     4096
#define MAXIMUM_HEIGHT    2160
#define DEFAULT_SCALE     double(DEFAULT_WIDTH.0/DEFAULT_HEIGHT.0)

namespace Star_VideoPlayer
{
	using std::shared_ptr;

	class CControlWindow;
	class vp_DragDrop;

	enum PLAYSTATE { Stopped, Paused, Running, Switching, Loading };

	enum class PlayMode
	{
		Queue, Loop, Random
	};

	class CVideoWindow
	{
	public:
		CVideoWindow(wstring appPath, wstring playlistPath);
		~CVideoWindow();

		LRESULT CreateVideoWindow(HINSTANCE hInstance);
		LRESULT ReleaseVideoWindow();

		LRESULT RegisterHotkey();
		LRESULT UnRegisterHotkey();

		LRESULT EnableFullScreen();
		LRESULT DisableFullScreen();

		LRESULT BindControlWindow(bool Show = true);
		LRESULT UnBindControlWindow();
		HWND GetHWND();

		void PauseClip();
		void StopClip();
		int  OpenClip(int index);
		void OpenClip(LPCTSTR szFilename);

		void PlayPrev();
		void PlayNext();

		HRESULT ChangeVolume(long volume);
		HRESULT Mute();

		bool GetDuration(double * outDuration);
		bool GetCurrentPosition(double * outPosition);
		bool GetStopPosition(double * outPosition);
		bool SetCurrentPosition(double Position);
		bool SetStartStopPosition(double inStart, double inStop);

		HRESULT UpdateVideoPos(bool updateSlider = true);

		PLAYSTATE GetPlayState()
		{
			return m_psCurrent;
		}

		shared_ptr<svplwrapper> GetSvplWrapper()
		{
			return m_svplwrapper;
		}

		void SetNewWrapper(shared_ptr<svplwrapper> new_svplwrapper);

		int m_nowplaying = 0;                // 当前正在播放的视频索引
		int m_nowplaylist = 0;               // 当前播放列表的索引

		bool IsMuted();

		CControlWindow* GetBindedControlWindow()
		{
			return m_cCon;
		}

		bool IsFullScreen()
		{
			return m_bFullScreen;
		}

		LRESULT SavePlaylist(wstring);

		int m_iVolume = 50;                  // 声音大小

		int m_iPlaylistAttached = 3;        // 决定播放列表窗口是否附着到主窗口，这个值由 PlayList Window 修改
											// 一旦 PlayList Window 将此值设置为 1/2 (左/右) 即意味着 主窗口
											// 可以修改 PlayList Window 的位置
											// PS: 这个默认值被设置为3 意味着第一次开启时将自动根据窗口位置
											// 决定是附着到左或右

		PlayMode m_PlayMode = PlayMode::Queue;

		bool m_bBeginFromFirst = false;     // 是否从第一个开始播放

		bool m_bSavePlaylistWhenExit = true;
		bool m_bSaveHistoryPlaylist = true;

	private:

		shared_ptr<svplwrapper> m_svplwrapper;

		HWND m_hwnd;
		HWND m_hCon;
		BOOL m_bAudioOnly = FALSE;
		RECT m_rcVideo = { 0 };              // 视频窗体大小
		double m_dScale = DEFAULT_SCALE;     // 视频长宽比
		bool m_bMuted = false;               // 静音

		ULONGLONG m_ulTicktime = 0;          // 上一次判断鼠标位置时的系统时间

		bool m_bCapture = false;             // 是否是视频窗口获得了焦点
		bool m_bInMenu = false;              // 是否在右键菜单中

		PLAYSTATE m_psCurrent = Loading;

		IGraphBuilder *pGB = nullptr;
		IMediaControl *pMC = nullptr;
		IMediaEventEx *pME = nullptr;
		IBasicAudio *pBA = nullptr;
		IMediaSeeking *pMS = nullptr;
		IMediaPosition *pMP = nullptr;

		IMFVideoDisplayControl *m_pDisplay = nullptr;

		int m_keepwidth = 0;                 // 
		POINT m_windowAspectRatio = { 0 };   // 保持指定的窗口横纵比

		wstring m_strPlaylistFile;           // 当前加载的播放列表文件
		wstring m_strMediaFile;              // 当前正在播放的视频路径
		wstring m_strAppPath;                // 应用程序路径

		int m_mousehidetime = 10;

		CControlWindow* m_cCon = nullptr;    // 绑定的控件窗口
		shared_ptr<ConMenu> m_conMenu = nullptr;        // 右键菜单窗口

		bool m_bTopMost = false;

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT __inline PaintWelcomeImg(int msgType);

		HRESULT PlayMovieInWindow(LPCTSTR szFile);
		HRESULT InitVideoWindow(SIZE);
		HRESULT InitPlayerWindow();
		void MoveVideoWindow(int cx, int cy);

		void CloseClip();
		void CloseInterfaces();
		void UpdateMainTitle();
		HRESULT HandleGraphEvent();

		void ConvertTime(double time, TCHAR * strBuffer);
		BOOL IsCursorInWindow(HWND hWnd, POINT ptcursor);

		LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

		LRESULT ResizeWindow(int cx, int cy);
		LRESULT SetAllWindowPos(HWND, int x, int y, int cx, int cy, UINT uFlags);

		void VolUp(int delta = 5);
		void VolDown(int delta = 5);

		vp_DragDrop* myDragDrop = nullptr;

		bool mousemove_first = true;
		bool m_originalTopMost;             // 全屏前是否置顶窗口
		bool m_bFullScreen = false;         // 是否全屏

		void __forceinline MyShowCursor(bool Show = true)
		{
			static bool AlreadyShow = true;
			static bool AlreadyHide = false;
			if (Show)
			{
				if (AlreadyShow) return;
				int i = ShowCursor(true);
				_trace(L"ShowCursor Reference Count: %d \n", i);
				while (i < 0) i = ShowCursor(true);
				AlreadyShow = true;
				AlreadyHide = false;
			}
			else
			{
				if (AlreadyHide) return;
				int i = ShowCursor(false);
				_trace(L"ShowCursor Reference Count %d \n", i);
				while (i >= 0) i = ShowCursor(false);
				AlreadyHide = true;
				AlreadyShow = false;
			}
		}
	};

}