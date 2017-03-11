#pragma once

#include "oleidl.h"
#include <windows.h>
#include "Shlobj.h"
#include "shellapi.h"
#include <vector>
#include <string>
#include <fstream>

#include "VideoWindow.h"
#include "ControlsWindow.h"
#include "PlayListWindow.h"

using namespace std;

namespace Star_VideoPlayer
{
	class vp_DragDrop : public IDropTarget
	{
	public:
		vp_DragDrop();
		~vp_DragDrop();
		void DragDropRevoke();

		HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;
		ULONG STDMETHODCALLTYPE AddRef(void) override;
		ULONG STDMETHODCALLTYPE Release(void) override;

		HRESULT DragDropRegister(CVideoWindow *vw);
		HRESULT DragDropRegister(CControlWindow *cw);
		HRESULT DragDropRegister(CPlayListWindow *pw);

		HRESULT STDMETHODCALLTYPE DragEnter(
			/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ __RPC__inout DWORD *pdwEffect) override;

		HRESULT STDMETHODCALLTYPE DragOver(
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ __RPC__inout DWORD *pdwEffect) override;

		HRESULT STDMETHODCALLTYPE DragLeave(void) override;

		HRESULT STDMETHODCALLTYPE Drop(
			/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ __RPC__inout DWORD *pdwEffect) override;

		static bool ReadAllowedExts(wstring strAppPath);
		CVideoWindow* pcVideo = nullptr;

	private:

		volatile LONG lRefCount;

		IDropTargetHelper* pDropHelper;
		IDragSourceHelper* pSourceHelper;
		IDragSourceHelper2* pSourceHelper2;
		HWND hWnd;

		typedef DWORD DROPEFFECT;

		bool GetGlobalData(LPDATAOBJECT pIDataObj, LPCTSTR lpszFormat, FORMATETC& FormatEtc, STGMEDIUM& StgMedium);

		static inline CLIPFORMAT RegisterFormat(LPCTSTR lpszFormat)
		{
			return static_cast<CLIPFORMAT>(::RegisterClipboardFormat(lpszFormat));
		}

		DWORD GetGlobalDataDWord(LPDATAOBJECT pIDataObj, LPCTSTR lpszFormat);

		int WINAPI ProcessData(IDataObject *pDataObj);

		static vector<wstring> allowedexts;

		int datatype;  // �������ͣ�0. �� 1. �ļ����ļ��� 2. �����б��ļ�
		int retOfProcessData = 0;

		vector<wstring> data_files;

		bool bDescriptionChanged = false;

		IDataObject* pSavedDataObj = NULL;

		int IsValidExt(const TCHAR* szFilename);

		bool ReadData();
		bool ReadDir(wstring sFile, UINT &filenum);
		static void __cdecl thread_ReadData(void*);
		// ����������ڱ��������߳��л�õ�svplwrapperָ�룬��ֹ���̶߳�д��ͻ
		shared_ptr<svplwrapper> m_multithread_svplwrapper;
	};
}