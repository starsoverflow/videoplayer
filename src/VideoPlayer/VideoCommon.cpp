#include "VideoCommon.h"
#include <map>
using namespace std;

namespace Star_VideoPlayer
{
	// from mpc-hc
	HRESULT LoadExternalObject(LPCTSTR path, REFCLSID clsid, REFIID iid, void** ppv)
	{
		static map<wstring, HINSTANCE> g_DLLInst;

		HRESULT hr = E_FAIL;

		HINSTANCE hInstDll = nullptr;
		bool fFound = false;

		map<wstring, HINSTANCE>::iterator l_hinst;

		l_hinst = g_DLLInst.find(path);

		if (l_hinst != g_DLLInst.end()) {
			hInstDll = l_hinst->second;
			fFound = true;
		}

		if (!hInstDll) {
			hInstDll = LoadLibraryEx(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
		}

		if (hInstDll) {
			typedef HRESULT(__stdcall * PDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
			PDllGetClassObject p = (PDllGetClassObject)GetProcAddress(hInstDll, "DllGetClassObject");

			if (p && FAILED(hr = p(clsid, iid, ppv))) {
				IClassFactory *pCF;
				if (SUCCEEDED(hr = p(clsid, IID_PPV_ARGS(&pCF)))) {
					hr = pCF->CreateInstance(nullptr, iid, ppv);
				}
			}
		}

		if (FAILED(hr) && hInstDll && !fFound) {
			FreeLibrary(hInstDll);
			return hr;
		}

		if (hInstDll && !fFound) {
			g_DLLInst.insert(pair<wstring, HINSTANCE>(path, hInstDll));
		}

		return hr;
	}
}
