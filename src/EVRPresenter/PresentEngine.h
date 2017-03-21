//////////////////////////////////////////////////////////////////////////
//
// PresentEngine.h: Defines the D3DPresentEngine object.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//
//////////////////////////////////////////////////////////////////////////

#pragma once

//-----------------------------------------------------------------------------
// D3DPresentEngine class
//
// This class creates the Direct3D device, allocates Direct3D surfaces for
// rendering, and presents the surfaces. This class also owns the Direct3D
// device manager and provides the IDirect3DDeviceManager9 interface via
// GetService.
//
// The goal of this class is to isolate the EVRCustomPresenter class from
// the details of Direct3D as much as possible.
//-----------------------------------------------------------------------------

class D3DPresentEngine : public SchedulerCallback
{
public:

	HRESULT GetDisplaySize(SIZE *size)
	{
		size->cx = m_DisplayMode.Width;
		size->cy = m_DisplayMode.Height;
		return S_OK;
	}
	HRESULT GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp);
	// State of the Direct3D device.
	enum DeviceState
	{
		DeviceOK,
		DeviceReset,    // The device was reset OR re-created.
		DeviceRemoved,  // The device was removed.
	};

	D3DPresentEngine(HRESULT& hr);
	virtual ~D3DPresentEngine();

	// GetService: Returns the IDirect3DDeviceManager9 interface.
	// (The signature is identical to IMFGetService::GetService but
	// this object does not derive from IUnknown.)
	virtual HRESULT GetService(REFGUID guidService, REFIID riid, void** ppv);
	virtual HRESULT CheckFormat(D3DFORMAT format);

	// Video window / destination rectangle:
	// This object implements a sub-set of the functions defined by the
	// IMFVideoDisplayControl interface. However, some of the method signatures
	// are different. The presenter's implementation of IMFVideoDisplayControl
	// calls these methods.
	HRESULT SetVideoWindow(HWND hwnd);
	HWND    GetVideoWindow() const { return m_hwnd; }
	HRESULT SetDestinationRect(const RECT& rcDest);
	RECT    GetDestinationRect() const { return m_rcDestRect; };

	HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue);
	void    ReleaseResources();

	HRESULT CheckDeviceState(DeviceState *pState);
	HRESULT PresentSample(IMFSample* pSample, LONGLONG llTarget);

	UINT    RefreshRate() const { return m_DisplayMode.RefreshRate; }

	HRESULT SetBorderColor(COLORREF Clr);
	HRESULT GetBorderColor(COLORREF* Clr);

	void ShowBorder(bool Show)
	{
		EnterCriticalSection(&m_ObjectLock);
		m_bShowBorder = Show;
		LeaveCriticalSection(&m_ObjectLock);
	}

	bool IsShowBorder() { return m_bShowBorder; }

protected:
	HRESULT InitializeD3D();
	HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP);
	HRESULT CreateD3DDevice();
	HRESULT UpdateDestRect();

	virtual void    PaintFrameWithGDI();

protected:
	UINT                        m_DeviceResetToken;     // Reset token for the D3D device manager.

	HWND                        m_hwnd;                 // Application-provided destination window.
	RECT                        m_rcDestRect;           // Destination rectangle.
	D3DDISPLAYMODE              m_DisplayMode;          // Adapter's display mode.

	CRITICAL_SECTION            m_ObjectLock;           // Thread lock for the D3D device.

														// COM interfaces
	IDirect3D9Ex                *m_pD3D9;
	IDirect3DDevice9Ex          *m_pDevice;
	IDirect3DDeviceManager9     *m_pDeviceManager;        // Direct3D device manager.
	IDirect3DSurface9           *m_pSurfaceRepaint;       // Surface for repaint requests.

	COLORREF                    m_BorderColor;          // The color of the border.
	bool                        m_bShowBorder;          // Determine whether to keep scale and show the border.

	D3DPRESENT_PARAMETERS       m_d3dpp;                //

	IDirect3DSwapChain9*        m_pSwapChain;           //
	
	RECT                        m_rcBackScreen = { 0 }; // 存储了视频大小

};