
#include "DragDrop.h"
#include "VideoWindow.h"
#include "ControlsWindow.h"
#include "PlayListWindow.h"
#include "svplreader.h"
#include <InitGuid.h>
#include "..\mydebug.h"

#include <io.h>
#include <process.h>
#include <algorithm>
#include "msg.h"

namespace SVideoPlayer
{
	// {4657278A-411B-11D2-839A-00C04FD918D0}
	static const GUID CLSID_DragDropHelper = { 0x4657278A, 0x411B, 0x11D2, { 0x83, 0x9A, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xD0 } };

	vector<wstring> vp_DragDrop::allowedexts;

	vp_DragDrop::vp_DragDrop()
	{
		lRefCount = 1;
		CoInitialize(NULL);
		CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper, (void**)&pDropHelper);
	}

	vp_DragDrop::~vp_DragDrop()
	{
		if (pDropHelper)
		{
			pDropHelper->Release();
		}
		lRefCount = 0;
	}

	bool vp_DragDrop::ReadAllowedExts(wstring strfilename)
	{
		wifstream fin(strfilename);

		allowedexts.clear();

		wstring sline;

		if (!fin) {
			SHOWMSG(MsgIcon::Exclamation, ERR_NO_SUPPORT_FILE, L"DragAcceptExts.txt", L"�Ѽ���ȱʡ���ã�����ܵ��²��ܽ���ĳЩ��ק");
			goto loaddefault;
		}
		while (getline(fin, sline) && allowedexts.size() <= 50) {
			if (sline.substr(0, 1) != L"#") {
				allowedexts.push_back(sline);
			}
		}
		fin.close();
		return true;

	loaddefault:
		allowedexts.push_back(L"flv");
		allowedexts.push_back(L"mp4");
		allowedexts.push_back(L"aac");
		allowedexts.push_back(L"ape");
		allowedexts.push_back(L"flac");
		allowedexts.push_back(L"mp3");
		allowedexts.push_back(L"wav");
		return true;
	}

	HRESULT vp_DragDrop::QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if (riid == IID_IUnknown || riid == IID_IDropTarget)
		{
			*ppvObject = this;
		}
		else {
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
		static_cast<IUnknown *>(*ppvObject)->AddRef();
		return S_OK;
	}

	ULONG vp_DragDrop::AddRef()
	{
		return InterlockedIncrement(&lRefCount);
	}

	ULONG vp_DragDrop::Release()
	{
		if (InterlockedDecrement(&lRefCount) == 0) {
			delete this;
			return 0;
		}
		return lRefCount;
	}

	HRESULT vp_DragDrop::DragDropRegister(CVideoWindow* vw)
	{
		OleInitialize(NULL);
		RegisterDragDrop(vw->GetHWND(), this);
		hWnd = vw->GetHWND();
		pcVideo = vw;
		return S_OK;
	}

	HRESULT vp_DragDrop::DragDropRegister(CControlWindow* cw)
	{
		OleInitialize(NULL);
		RegisterDragDrop(cw->GetHWND(), this);
		hWnd = cw->GetHWND();
		pcVideo = cw->GetBindedVideoWindow();
		return S_OK;
	}
	HRESULT vp_DragDrop::DragDropRegister(CPlayListWindow *pw)
	{
		OleInitialize(NULL);
		RegisterDragDrop(pw->GetHWND(), this);
		hWnd = pw->GetHWND();
		pcVideo = pw->m_videownd;
		return S_OK;
	}

	void vp_DragDrop::DragDropRevoke()
	{
		RevokeDragDrop(hWnd);
		return;
	}
	HRESULT vp_DragDrop::DragOver(
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ __RPC__inout DWORD *pdwEffect)
	{
		*pdwEffect = DROPEFFECT_LINK;
		if (pDropHelper) {
			pDropHelper->DragOver((LPPOINT)&pt, *pdwEffect);
		}
		return S_OK;
	}

	HRESULT vp_DragDrop::DragEnter(
		/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
		/* [in] */ DWORD grfKeyState, /* keystate: ���1 �Ҽ�2 */
		/* [in] */ POINTL pt,
		/* [out][in] */ __RPC__inout DWORD *pdwEffect)
	{
		_trace(L"Drag Enter begintime:%d keystate: %d\n", GetTickCount(), grfKeyState);

		int type = ProcessData(pDataObj);
		wstring strDescription;
		DROPIMAGETYPE nImageType = DROPIMAGE_NONE;
		switch (type)
		{
		case 1:
			strDescription = L"��ӵ������б�";
			nImageType = DROPIMAGE_LINK;
			break;
		case 2:
			if (pcVideo->GetPlayState() == Stopped)
			{
				strDescription = L"���ش˲����б�";
			}
			else
			{
				strDescription = L"���ش˲����б���һ�β�����Ч��";
			}
			nImageType = DROPIMAGE_LINK;
			break;
		case 3:
			strDescription = L"�������ý���ļ��������б�";
			nImageType = DROPIMAGE_LINK;
			break;
		case 4:
			strDescription = L"û�п�ʶ���ý���ļ�";
			nImageType = DROPIMAGE_NONE;
			break;
		case 5:
			// can never reach here.
			break;
		case 6:
			strDescription = L"����ʶ����ַ���";
			nImageType = DROPIMAGE_NONE;
			break;
		case 7:
			strDescription = L"���ǿ�ʶ���ý���ļ�";
			nImageType = DROPIMAGE_NONE;
			break;
		case 8:
			strDescription = L"����ļ����е�ý���ļ��������б�";
			nImageType = DROPIMAGE_LINK;
			break;
		default:
			strDescription = L"�����ܴ�������";
			nImageType = DROPIMAGE_NONE;
			break;
		}

		bool ShowDescription = true;
		ShowDescription = (0 != GetGlobalDataDWord(pDataObj, _T("DragWindow")));
		if (ShowDescription)
			ShowDescription = (DSH_ALLOWDROPDESCRIPTIONTEXT == GetGlobalDataDWord(pDataObj, _T("DragSourceHelperFlags")));

		if (ShowDescription)
		{
			STGMEDIUM StgMedium;
			FORMATETC FormatEtc;
			if (GetGlobalData(pDataObj, _T("DropDescription"), FormatEtc, StgMedium))
			{
				DROPDESCRIPTION *pDropDescription = static_cast<DROPDESCRIPTION*>(::GlobalLock(StgMedium.hGlobal));
				wcsncpy_s(pDropDescription->szMessage, sizeof(pDropDescription->szMessage) / sizeof(pDropDescription->szMessage[0]), strDescription.c_str(), _TRUNCATE);
				pDropDescription->type = nImageType;
				::GlobalUnlock(StgMedium.hGlobal);

				if (!SUCCEEDED(pDataObj->SetData(&FormatEtc, &StgMedium, TRUE)))
				{
					::ReleaseStgMedium(&StgMedium);
				}
				else {
					bDescriptionChanged = true;
					pSavedDataObj = pDataObj;
				}
			}
		}

		*pdwEffect = nImageType;

		if (pDropHelper) {
			pDropHelper->DragEnter(hWnd, pDataObj, (LPPOINT)&pt, *pdwEffect);
		}

		_trace(L"Drag Enter endtime: %d\n", GetTickCount());
		return S_OK;
	}

	HRESULT vp_DragDrop::DragLeave()
	{
		// Release

		// reset description
		if (bDescriptionChanged)
		{
			STGMEDIUM StgMedium;
			FORMATETC FormatEtc;
			if (GetGlobalData(pSavedDataObj, _T("DropDescription"), FormatEtc, StgMedium))
			{
				DROPDESCRIPTION *pDropDescription = static_cast<DROPDESCRIPTION*>(::GlobalLock(StgMedium.hGlobal));
				wcsncpy_s(pDropDescription->szMessage, sizeof(pDropDescription->szMessage) / sizeof(pDropDescription->szMessage[0]), L"", _TRUNCATE);
				pDropDescription->type = DROPIMAGE_INVALID;
				::GlobalUnlock(StgMedium.hGlobal);
				if (!SUCCEEDED(pSavedDataObj->SetData(&FormatEtc, &StgMedium, TRUE)))
				{
					::ReleaseStgMedium(&StgMedium);
				}
			}
			bDescriptionChanged = false;
			pSavedDataObj = NULL;
		}

		if (pDropHelper) {
			pDropHelper->DragLeave();
		}

		data_files.clear();
		datatype = 0;

		return S_OK;
	}

	// fix 2016.10.7 : ȡ���� data_file
	//                 �򵥵���Ϊ�����б�����ڶ���ļ����ļ�����
	// fix 2016.10.7 : datatype ���������� ret ����
	//                 �� datatype

	// �������ݲ������������ͣ�
	// 1. ý���ļ�(4. ��ȡ���ļ���û��ƥ���(����ļ�) 7. ֻ��һ��������ƥ��)
	// 2. �����б��ļ� 3. �ļ�����Ч�ģ����ļ���  8. ���ļ���
	// 6. ��Ч�ַ���                                                            
	// 0. ��Ч����������
	// �����ã�5. ��Ч�ַ���

	int vp_DragDrop::ProcessData(IDataObject *pDataObj) {
		int ret = 0;
		datatype = 0;
		data_files.clear();

		STGMEDIUM stg;
		FORMATETC FormatEtc = { (CLIPFORMAT)CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		pDataObj->GetData(&FormatEtc, &stg);
		if (stg.tymed == TYMED_HGLOBAL) {
			TCHAR* pBuff = NULL;
			pBuff = (LPTSTR)GlobalLock(stg.hGlobal);
			GlobalUnlock(stg.hGlobal);
			_trace(L"%s\n", pBuff);

			DWORD attr = GetFileAttributes(pBuff);
			if (attr != -1)
			{
				if (attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE))
				{
					data_files.push_back(pBuff);
					ret = 8;
				}
				else
				{
					int isvalid = IsValidExt(pBuff);
					if (isvalid == 2) {
						data_files.push_back(pBuff);
						ret = 2;
					}
					else if (isvalid == 1)
					{
						data_files.push_back(pBuff);
						ret = 1;
					}
					else
					{
						ret = 6;
					}
				}
			}
			else { ret = 6; }
			goto done;
		}

		FormatEtc = { (CLIPFORMAT)CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		pDataObj->GetData(&FormatEtc, &stg);
		if (stg.tymed == TYMED_HGLOBAL) {
			TCHAR szFilename[MAX_PATH + 1] = { 0 };
			UINT filenum = DragQueryFile((HDROP)stg.hGlobal, 0xFFFFFFFF, NULL, 0);

			if (filenum == 0)
			{
				// a must.. due to UINT
			}
			// �����жϵ����ļ������
			else if (filenum == 1)
			{
				DragQueryFile((HDROP)stg.hGlobal, 0, szFilename, MAX_PATH);
				DWORD attr = GetFileAttributes(szFilename);
				if (attr != -1 && attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE))
				{
					data_files.push_back(szFilename);
					ret = 8;
				}
				else
				{
					int isvalid = IsValidExt(szFilename);
					if (isvalid == 2) {
						data_files.push_back(szFilename);
						ret = 2;
					}
					else if (isvalid == 1)
					{
						data_files.push_back(szFilename);
						ret = 1;
					}
					else
					{
						ret = 7;
					}
				}
			}
			else
			{
				bool b_hasdir = false;
				bool b_hasfile = false;

				for (UINT i = 0; i <= filenum - 1; i++) {
					DragQueryFile((HDROP)stg.hGlobal, i, szFilename, MAX_PATH);
					DWORD attr = GetFileAttributes(szFilename);
					if (attr != -1 && (attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE))) {
						data_files.push_back(szFilename);
						b_hasdir = true;
					}
					else {
						if (IsValidExt(szFilename) == 1)
						{
							data_files.push_back(szFilename);
							b_hasfile = true;
						}
					}
				}  // for

				if (data_files.size() == 0) ret = 4;   // nothing
				else
				{
					if (b_hasfile && !b_hasdir) ret = 1;
					else if (b_hasfile && b_hasdir) ret = 3;
					else if (!b_hasfile && b_hasdir) ret = 8;
				}
			}

			DragFinish((HDROP)stg.hGlobal);
		}

	done:
		ReleaseStgMedium(&stg);

		retOfProcessData = ret;
		// ����ret�ж�datatype
		switch (ret)
		{
		case 1:
		case 3:
		case 8:
			datatype = 1;
			break;
		case 2:
			datatype = 2;
			break;
		default:
			datatype = 0;
		}

		return ret;

	}

	HRESULT vp_DragDrop::Drop(
		/* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ __RPC__inout DWORD *pdwEffect)
	{
		_trace(L"Drag Drop begintime: %d\n", GetTickCount());
		if (pDropHelper) {
			pDropHelper->Drop(pDataObj, (LPPOINT)&pt, *pdwEffect);
		}

		switch (datatype)
		{
		case 1:
		{
			if (retOfProcessData == 3 || retOfProcessData == 8 || data_files.size() >= 50)
				SHOWMSG(MsgIcon::Information, "�Ѿ���ʼ��������ק�������ļ�\n�����Լ������Ĺ���\n��Ƶ���ֲ����ж�");
			m_multithread_svplwrapper = pcVideo->GetSvplWrapper();
			_beginthread(vp_DragDrop::thread_ReadData, NULL, this);
			// �߳���ɺ�ᷢ��֪ͨ
			break;
		}
		case 2:
		{
			shared_ptr<svplwrapper> new_svplwrapper(new svplwrapper(data_files.at(0)));
			if (new_svplwrapper->parserStatus != 0)
			{
				SHOWMSG(MsgIcon::Exclamation, "�޷���ȡ�����б� %s\n��ȷ��������Ч�Ĳ����б��ļ�", data_files.at(0).c_str());
				break;
			}

			shared_ptr<svplwrapper> old_svplwrapper = pcVideo->GetSvplWrapper();
			_trace(L"count: %d\n", new_svplwrapper.use_count());
			if (old_svplwrapper->GetItemNum(pcVideo->m_nowplaylist) != 0)
			{
				// �����������û�ȷ���Ƿ����
				auto lambda_continue = [this, new_svplwrapper](int ret) mutable
				{
					if (ret == 1)
					{
						pcVideo->SetNewWrapper(new_svplwrapper);
						if (pcVideo->GetPlayState() == Stopped)
						{
							pcVideo->m_bBeginFromFirst = true;
							pcVideo->OpenClip(-1);
						}
						else
						{
							SHOWMSG(MsgIcon::Information, "�����б��Ѽ������\n������һ�β���ʱ��Ч");
							pcVideo->m_bBeginFromFirst = true;
						}
					}
					new_svplwrapper.reset();
				};
				SHOWMSG_RB(MsgIcon::Question, lambda_continue, "Continue", "Cancel", "���������µĲ����б�\nԭ�еĲ����б��ᱻ���ǡ�\n�Ƿ������");
			}
			else
			{
				pcVideo->SetNewWrapper(new_svplwrapper);
				pcVideo->m_bBeginFromFirst = true;
				pcVideo->OpenClip(-1);
			}
			break;
		}
		}

		_trace(L"Drag Drop endtime: %d\n", GetTickCount());
		return S_OK;
	}

	bool vp_DragDrop::GetGlobalData(LPDATAOBJECT pIDataObj, LPCTSTR lpszFormat, FORMATETC& FormatEtc, STGMEDIUM& StgMedium)
	{
		bool bRet = false;
		FormatEtc.cfFormat = RegisterFormat(lpszFormat);
		FormatEtc.ptd = NULL;
		FormatEtc.dwAspect = DVASPECT_CONTENT;
		FormatEtc.lindex = -1;
		FormatEtc.tymed = TYMED_HGLOBAL;
		if (SUCCEEDED(pIDataObj->QueryGetData(&FormatEtc)))
		{
			if (SUCCEEDED(pIDataObj->GetData(&FormatEtc, &StgMedium)))
			{
				bRet = (TYMED_HGLOBAL == StgMedium.tymed);
				if (!bRet)
					::ReleaseStgMedium(&StgMedium);
			}
		}
		return bRet;
	}

	DWORD vp_DragDrop::GetGlobalDataDWord(LPDATAOBJECT pIDataObj, LPCTSTR lpszFormat)
	{
		DWORD dwData = 0;
		FORMATETC FormatEtc;
		STGMEDIUM StgMedium;
		if (GetGlobalData(pIDataObj, lpszFormat, FormatEtc, StgMedium))
		{
			if (::GlobalSize(StgMedium.hGlobal) >= sizeof(DWORD)) {
				dwData = *(static_cast<LPDWORD>(::GlobalLock(StgMedium.hGlobal)));
				::GlobalUnlock(StgMedium.hGlobal);
				::ReleaseStgMedium(&StgMedium);
			}
		}
		return dwData;
	}

	// �ж��Ƿ�����Ч���ļ���׺����1. ý���ļ� 2. �����б��ļ� 0. ��Ч�ļ�
	int vp_DragDrop::IsValidExt(const TCHAR* szFilename) {
		int lenf = wcsnlen_s(szFilename, MAX_PATH);
		wstring fileext;
		bool validext = false;
		if (lenf >= 1) {
			lenf--;
			for (; lenf >= 0; lenf--)
			{
				validext = false;
				if (szFilename[lenf] == '.') {
					validext = true; break;
				}
				fileext = szFilename[lenf] + fileext;
			}

			if (validext) {
				transform(fileext.begin(), fileext.end(), fileext.begin(), tolower);
				if (fileext == L"svpl") {
					return 2;
				}
				for (UINT t = 0; t <= allowedexts.size() - 1; t++) {
					if (allowedexts.at(t) == fileext) {
						return 1;
						break;
					}
				}
			}
		}
		return 0;
	}

	bool vp_DragDrop::ReadData()
	{
		if (data_files.size() == 0) return true;
		UINT filenum = 0;

		for (UINT ii = 0; ii <= data_files.size() - 1; ii++)
		{
			wstring sFile = data_files.at(ii);
			DWORD attr = GetFileAttributes(sFile.c_str());
			if (attr == -1) continue;
			if (attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE))
			{
				// ���������������һ����б��
				if (*(sFile.end() - 1) == L'\\') sFile = sFile.substr(0, sFile.length() - 1);
				ReadDir(sFile, filenum);
			}
			else
			{
				svpl_item newitem;
				newitem.path = sFile;
				m_multithread_svplwrapper->AddItem(newitem, pcVideo->m_nowplaylist);
				filenum++;
			}
		}
		m_multithread_svplwrapper.reset();
		PostMessage(pcVideo->GetHWND(), WM_DragDropDone, filenum, NULL);
		return true;
	}

	bool vp_DragDrop::ReadDir(wstring sFile, UINT &filenum)
	{
		long hFile = 0;
		_wfinddata_t fd;
		if ((hFile = _wfindfirst((sFile + L"\\*").c_str(), &fd)) != -1)
		{
			do
			{
				if (fd.attrib & _A_SUBDIR)
				{
					if (wcscmp(fd.name, L".") != 0 && wcscmp(fd.name, L"..") != 0)
						ReadDir(sFile + L"\\" + fd.name, filenum);
				}
				else
				{
					if (IsValidExt(fd.name))
					{
						svpl_item newitem;
						newitem.path = sFile + L"\\" + fd.name;
						m_multithread_svplwrapper->AddItem(newitem, pcVideo->m_nowplaylist);
						filenum++;
					}
				}
			} while (_wfindnext(hFile, &fd) == 0);
		}
		_findclose(hFile);
		return true;
	}

	void vp_DragDrop::thread_ReadData(void* arg)
	{
		_trace(L"thread_readdir begin: %d \n", GetTickCount());
		if (arg)
		{
			vp_DragDrop* myDragDrop = (vp_DragDrop*)arg;
			myDragDrop->ReadData();
		}

		_trace(L"thread_readdir end: %d \n", GetTickCount());
		_endthread();
	}
}