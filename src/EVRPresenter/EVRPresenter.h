//////////////////////////////////////////////////////////////////////////
//
// EVRPresenter.h : Internal header for building the DLL.
//
// Based on a sample from(c) Microsoft Corporation. 
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <intsafe.h>
#include <math.h>
#include <strsafe.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evcode.h> // EVR event codes (IMediaEventSink)
#include <d3d9.h>
#include <dxva2api.h>
#include <evr9.h>

#include <Dshow.h>
#include <Dvdmedia.h>

#include "linklist.h"
#include "critsec.h"
#include "MFClasses.h"

// define FILE_LOGGING to write a log-file

enum SVP_ASPECTRATIOMODE
{
	Stretch = 0,
	NoStretch = 1
};

using namespace Classes;

#ifndef CHECK_HR
#define CHECK_HR(hr) IF_FAILED_GOTO(hr, done)
#endif

#ifndef FAILED
#define FAILED(hr)  (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr)  (((HRESULT)(hr)) >= 0)
#endif

// IF_FAILED_GOTO macro.
// Jumps to 'label' on failure.
#ifndef IF_FAILED_GOTO
#define IF_FAILED_GOTO(hr, label) if (FAILED(hr)) { goto label; }
#endif

// CheckPointer macro.
// Returns 'hr' if pointer 'x' is NULL.
#ifndef CheckPointer
#define CheckPointer(x, hr) if (x == NULL) { return hr; }
#endif

// Releases a COM pointer if the pointer is not NULL, and sets the pointer.
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = NULL; } }
#endif

typedef ComPtrList<IMFSample>  VideoSampleList;

// Custom Attributes

// MFSamplePresenter_SampleCounter
// Data type: UINT32
//
// Version number for the video samples. When the presenter increments the version
// number, all samples with the previous version number are stale and should be
// discarded.
static const GUID MFSamplePresenter_SampleCounter =
{ 0xb0bb83cc, 0xf10f, 0x4e2e, { 0xaa, 0x2b, 0x29, 0xea, 0x5e, 0x92, 0xef, 0x85 } };

// MFSamplePresenter_SampleSwapChain
// Data type: IUNKNOWN
//
// Pointer to a Direct3D swap chain.
static const GUID MFSamplePresenter_SampleSwapChain =
{ 0xad885bd1, 0x7def, 0x414a, { 0xb5, 0xb0, 0xd3, 0xd2, 0x63, 0xd6, 0xe9, 0x6d } };

// Project headers.
#include "Helpers.h"
#include "SamplePool.h"
#include "Scheduler.h"
#include "PresentEngine.h"
#include "Presenter.h"

