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

		int m_nowplaying = 0;                // ��ǰ���ڲ��ŵ���Ƶ����
		int m_nowplaylist = 0;               // ��ǰ�����б������

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

		int m_iVolume = 50;                  // ������С

		int m_iPlaylistAttached = 3;        // ���������б����Ƿ��ŵ������ڣ����ֵ�� PlayList Window �޸�
											// һ�� PlayList Window ����ֵ����Ϊ 1/2 (��/��) ����ζ�� ������
											// �����޸� PlayList Window ��λ��
											// PS: ���Ĭ��ֵ������Ϊ3 ��ζ�ŵ�һ�ο���ʱ���Զ����ݴ���λ��
											// �����Ǹ��ŵ������

		PlayMode m_PlayMode = PlayMode::Queue;

		bool m_bBeginFromFirst = false;     // �Ƿ�ӵ�һ����ʼ����

		bool m_bSavePlaylistWhenExit = true;
		bool m_bSaveHistoryPlaylist = true;

	private:

		shared_ptr<svplwrapper> m_svplwrapper;

		HWND m_hwnd;
		HWND m_hCon;
		BOOL m_bAudioOnly = FALSE;
		RECT m_rcVideo = { 0 };              // ��Ƶ�����С
		double m_dScale = DEFAULT_SCALE;     // ��Ƶ�����
		bool m_bMuted = false;               // ����

		ULONGLONG m_ulTicktime = 0;          // ��һ���ж����λ��ʱ��ϵͳʱ��

		bool m_bCapture = false;             // �Ƿ�����Ƶ���ڻ���˽���
		bool m_bInMenu = false;              // �Ƿ����Ҽ��˵���

		PLAYSTATE m_psCurrent = Loading;

		IGraphBuilder *pGB = nullptr;
		IMediaControl *pMC = nullptr;
		IMediaEventEx *pME = nullptr;
		IBasicAudio *pBA = nullptr;
		IMediaSeeking *pMS = nullptr;
		IMediaPosition *pMP = nullptr;

		IMFVideoDisplayControl *m_pDisplay = nullptr;

		int m_keepwidth = 0;                 // 
		POINT m_windowAspectRatio = { 0 };   // ����ָ���Ĵ��ں��ݱ�

		wstring m_strPlaylistFile;           // ��ǰ���صĲ����б��ļ�
		wstring m_strMediaFile;              // ��ǰ���ڲ��ŵ���Ƶ·��
		wstring m_strAppPath;                // Ӧ�ó���·��

		int m_mousehidetime = 10;

		CControlWindow* m_cCon = nullptr;    // �󶨵Ŀؼ�����
		shared_ptr<ConMenu> m_conMenu = nullptr;        // �Ҽ��˵�����

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
		bool m_originalTopMost;             // ȫ��ǰ�Ƿ��ö�����
		bool m_bFullScreen = false;         // �Ƿ�ȫ��

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