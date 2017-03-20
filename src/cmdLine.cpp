#include "cmdLine.h"

#include "VideoPlayer/VideoWindow.h"
#include "mydebug.h"

class VPInterfaces : public IRunnable
{
public:
	int     Parse(wchar_t*);
	int     Execute();
	wstring GetLastError() { return parserLastError; }
	int     ParserTest();

	VPInterfaces() = default;
	VPInterfaces(const VPInterfaces &) = delete;
	VPInterfaces& operator= (const VPInterfaces &) = delete;

protected:
	int CreateVideoPlayerInstance();
	int AddMediaFileToCurrentInstance();
	static int FindInstance(HWND dest[], int maxCount);

private:
	wstring playlistFile;
	wstring mediaFile;
	bool    savePlaylistWhenExit = true;
	bool    saveHistoryPlaylist = true;
	bool    addToCurrentInstance = false;
	bool    alwaysOnTop = false;
	bool    keepWindowAspectRatio = false;
	int     keepWidth = 0;
	POINT   wndSize = { 0 };
	wstring parserLastError;

	const wchar_t* VPCommands[8] = { L"nooverwrite", L"nosavehistory", L"add", L"mediafile", L"playlist", L"keepwidth",
		L"size", L"alwaysontop" };

	static int __stdcall EnumWindowsCallback(HWND hwnd, LPARAM lParam);
};

unique_ptr<IRunnable> GetInterface()
{
	return unique_ptr<IRunnable>(new VPInterfaces());
}

int VPInterfaces::Parse(wchar_t* cmdLine)
{
	parserLastError = L"Success";
	
	wstring strCmdLine(cmdLine);
	int offset = 0, state = 0, token = 0, strvaluetoken = 0, valuetoken = 0;
	const wchar_t* acceptedCommand = nullptr;
	while (true)
	{
		wchar_t ch = cmdLine[offset];
		switch (state)
		{
		case 0:
			if (ch == L'/' || ch == L'-') { state = 1; token = offset + 1; }
			else if (ch != L' ' && ch != L'\0') { parserLastError = L"����Ԥ�ϵ� "; parserLastError.append(1, ch); return -1; }
			break;
		case 1:
			// if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z')) { } else 
			if (ch == L' ' || ch == L'/' || ch == L'-' || ch == L'\0' || ch == L'=')
			{
				acceptedCommand = nullptr;
				for (auto command : VPCommands)
				{
					bool accepted = true;
					int offset1 = token, offset2 = 0;
					if ((offset - offset1) != wcslen(command)) { accepted = false; }
					while (accepted && offset1 < offset && command[offset2] != L'\0')
					{
						if (cmdLine[offset1] != command[offset2]) accepted = false;
						offset1++; offset2++;
					}
					if (accepted) {
						state = 0; acceptedCommand = command;
						if (command == VPCommands[0]) savePlaylistWhenExit = false;
						else if (command == VPCommands[1]) saveHistoryPlaylist = false;
						else if (command == VPCommands[2]) addToCurrentInstance = true;
						else if (command == VPCommands[7]) alwaysOnTop = true;
						else { state = 2; }
						break;
					}
				}
				if (!acceptedCommand) { parserLastError = L"��Ч��������ѡ�� " + strCmdLine.substr(token - 1, offset - token + 1); return -1; }
				offset--;
			}
			break;
		case 2:
			if (ch == L'=') {
				if (acceptedCommand == VPCommands[3] || acceptedCommand == VPCommands[4]) {
					state = 3;
				}
				else if (acceptedCommand == VPCommands[5]) {
					valuetoken = offset + 1;
					state = 5;
				}
				else if (acceptedCommand == VPCommands[6]) {
					valuetoken = offset + 1;
					state = 6;
				}
			}
			else {
				parserLastError = wstring(acceptedCommand) + L" ��ֵ����Ϊ��";
				return -1;
			}
			break;
		case 3:
			if (ch == L'"') { state = 4; strvaluetoken = offset + 1; }
			else { parserLastError = L"��ʹ������"; return -1; }
			break;
		case 4:
			if (ch == L'"') {
				state = 0;
				if (strvaluetoken >= offset) { parserLastError = wstring(acceptedCommand) + L" ��ֵ����Ϊ��"; return -1; }
				if (acceptedCommand == VPCommands[3]) mediaFile = strCmdLine.substr(strvaluetoken, offset - strvaluetoken);
				else if (acceptedCommand == VPCommands[4]) { playlistFile = strCmdLine.substr(strvaluetoken, offset - strvaluetoken); }

			}
			else if (ch == L'\0')
			{
				parserLastError = L"����δ�պ�";
				return -1;
			}
			break;
		case 5:
			if (ch == L'/' || ch == L'-') { state = 0; }
			else if (ch == L' ' || ch == L'\0') { state = 0; }
			else if (ch < L'0' || ch > L'9') { parserLastError = wstring(acceptedCommand) + L" ��ֵֻ��Ϊ����"; return -1; }
			if (state == 0) {
				if (valuetoken >= offset) { parserLastError = wstring(acceptedCommand) + L" ��ֵ����Ϊ��"; return -1; }
				keepWidth = _wtoi(strCmdLine.substr(valuetoken, offset - valuetoken).c_str());
				if (keepWidth > 2560) { parserLastError = L"keepwidth ��ֵ̫��"; return -1; }
			}
			if (ch == L'/' || ch == L'-') { offset--; }
			break;
		case 6:
			if (ch == L'/' || ch == L'-' || ch == L' ' || ch == L'\0') { parserLastError = wstring(acceptedCommand) + L" ��Ҫ����x,y"; return -1; }
			else if (ch == L',')
			{
				if (valuetoken >= offset) { parserLastError = L"size.x ��ֵ����Ϊ��"; return -1; }
				wndSize.x = _wtoi(strCmdLine.substr(valuetoken, offset - valuetoken).c_str());
				if (wndSize.x > 2560) { parserLastError = L"size.x ��ֵ̫��"; return -1; }
				valuetoken = offset + 1;
				state = 7;
			}
			else if (ch < L'0' || ch > L'9') { parserLastError = wstring(acceptedCommand) + L" ��ֵֻ��Ϊ����"; return -1; }
			break;
		case 7:
			if (ch == L'/' || ch == L'-') { state = 0; }
			else if (ch == L' ' || ch == L'\0') { state = 0; }
			else if (ch < L'0' || ch > L'9') { parserLastError = wstring(acceptedCommand) + L" ��ֵֻ��Ϊ����"; return -1; }
			if (state == 0) {
				if (valuetoken >= offset) { parserLastError = L"size.y ��ֵ����Ϊ��"; return -1; }
				wndSize.y = _wtoi(strCmdLine.substr(valuetoken, offset - valuetoken).c_str());
				if (wndSize.y > 1440) { parserLastError = L"size.y ��ֵ̫��"; return -1; }
			}
			if (ch == L'/' || ch == L'-') { offset--; }
			break;
		}
		if (cmdLine[offset] == L'\0') break;
		offset++;
	}

	// Last check.
	if (addToCurrentInstance && mediaFile.empty()) { parserLastError = L"Undefined mediafile being added to the existing instance"; return -1; }
	return 0;
}

int VPInterfaces::Execute()
{
	int ret = 0;
	if (addToCurrentInstance)
	{
		ret = AddMediaFileToCurrentInstance();
		// û���ҵ�ʵ�����½�һ��
		if (ret == -2) ret = CreateVideoPlayerInstance();
	}
	else
	{
		ret = CreateVideoPlayerInstance();
	}
	return ret;
}

int VPInterfaces::CreateVideoPlayerInstance()
{
	HINSTANCE hInstance = GetModuleHandleW(nullptr);

	wchar_t szAppPath[MAX_PATH + 1] = { 0 };
	if (!GetModuleFileNameW(hInstance, szAppPath, MAX_PATH)) return -1;    // Fatal Error.

	wchar_t* whereslash = wcsrchr(szAppPath, L'\\');
	if (whereslash != NULL) whereslash[1] = 0;

	wstring strAppPath = szAppPath;

	strAppPath += L"..\\"; // ������
	_trace(L"AppPath: \"%s\"\n", szAppPath);

	DuiLib::CPaintManagerUI::SetInstance(hInstance);

	using cv = Star_VideoPlayer::CVideoWindow;
	unique_ptr<cv> VideoWindow(new cv(strAppPath, playlistFile));

	if (!mediaFile.empty())
	{
		using csw = Star_VideoPlayer::svplwrapper;
		using csi = Star_VideoPlayer::svpl_item;
		shared_ptr<csw> SvplWrapper(new csw());
		csi newItem;
		newItem.path = mediaFile;
		SvplWrapper->AddItem(newItem, 0);
		if ((wndSize.x == 0 || wndSize.y == 0) && keepWidth == 0) keepWidth = 1;  // Enable keepwidth for default.
		SvplWrapper->Get()->config.size = wndSize;
		SvplWrapper->Get()->config.alwaystop = alwaysOnTop;
		SvplWrapper->Get()->playlists.at(0).keepwidth = keepWidth;
		
		VideoWindow->SetNewWrapper(SvplWrapper);
	}

	VideoWindow->m_bSaveHistoryPlaylist = saveHistoryPlaylist;
	VideoWindow->m_bSavePlaylistWhenExit = savePlaylistWhenExit;

	if (VideoWindow->CreateVideoWindow(hInstance) != 0) return -1;   // Fatal Error.
	VideoWindow->BindControlWindow(true);

	DuiLib::CPaintManagerUI::MessageLoop();

	return 0;
}

int VPInterfaces::AddMediaFileToCurrentInstance()
{
	if (mediaFile.empty()) return -1;
	HWND instances[1] = { nullptr };
	FindInstance(instances, sizeof(instances) / sizeof(instances[0]));
	if (instances[0] == nullptr) { return -2; }
	HANDLE hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 2048, L"Local\\videopl{52DC5C06-AAA4-46C5-B753-3BC8E28099B1}");
	if (hMap == nullptr) return -1;
	wchar_t* pbuf = (wchar_t*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 2048);
	if (pbuf == nullptr) return -1;
	StringCbCopyW(pbuf, 2048, mediaFile.c_str());
	int ret = SendMessageW(instances[0], WM_AddMediaFile, 0xffff, 0xffff);
	UnmapViewOfFile(pbuf);
	CloseHandle(hMap);
	return ret;
}

int VPInterfaces::FindInstance(HWND dest[], int maxCount)
{
	vector<HWND> instances;
	EnumWindows(EnumWindowsCallback, (LPARAM)&instances);
	int count = 0;
	for (auto instance : instances)
	{
		if (count >= maxCount) break;
		dest[count] = instance;
		count++;
	}
	return count;
}

int __stdcall VPInterfaces::EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
	vector<HWND> *instances = (vector<HWND>*)lParam;

	wchar_t className[1024];
	if (!GetClassNameW(hwnd, className, sizeof(className) / sizeof(className[0])))
	{
		// Continue enumeration.
		return TRUE;
	}
	if (wcscmp(className, L"Star Video Player_Video Window") == 0)
	{
		instances->push_back(hwnd);
	}
	return TRUE;
}

int VPInterfaces::ParserTest()
{
#define t(x,exp) { \
	_trace(L"\nRunning Test %s\n", _T(x)); \
	int ret = Parse(_T(x));  \
	assert(exp); _trace(L"Test end with %s\n", parserLastError.c_str()); }

	t("/nooverwrite", ret == 0 && savePlaylistWhenExit == false && saveHistoryPlaylist == true && addToCurrentInstance == false && keepWidth == 0 && wndSize.x == 0 && wndSize.y == 0);
	savePlaylistWhenExit = true;
	t("/nooverwrite ", ret == 0 && savePlaylistWhenExit == false && saveHistoryPlaylist == true && addToCurrentInstance == false && keepWidth == 0 && wndSize.x == 0 && wndSize.y == 0);
	savePlaylistWhenExit = true;
	t("nosavehistory", ret == -1 && savePlaylistWhenExit == true && saveHistoryPlaylist == true && addToCurrentInstance == false && keepWidth == 0 && wndSize.x == 0 && wndSize.y == 0);
	t("/nosavehistory", ret == 0 && savePlaylistWhenExit == true && saveHistoryPlaylist == false && addToCurrentInstance == false && keepWidth == 0 && wndSize.x == 0 && wndSize.y == 0);

	t("/mediafile=3232", ret == -1);
	t("/mediafile=\"3asd:f/g\"", ret == 0 && mediaFile == L"3asd:f/g");
	t("/playlist=3232", ret == -1);
	t("/playlist=\"3asd:f/g\"", ret == 0 && playlistFile == L"3asd:f/g");
	t("/add", ret == 0 && addToCurrentInstance == true);
	t("/size=123", ret == -1);
	t("/size=123.", ret == -1);
	t("/size=123,", ret == -1);
	t("/size=123,1", ret == 0 && wndSize.x == 123 && wndSize.y == 1);
	t("/size=1,1", ret == 0 && wndSize.x == 1 && wndSize.y == 1);
	t("/size=4831,12", ret == -1 && wndSize.x == 4831);
	t("/size=1444,12", ret == 0 && wndSize.x == 1444 && wndSize.y == 12);
	t("/size=,12", ret == -1);
	t("/size=123a", ret == -1);
	t("/keepwidth=300", ret == 0);
	t("/keepwidth=32a", ret == -1);
	t("/keepwidth=32 ", ret == 0 && keepWidth == 32);

#undef t

#define tn(x,exp) { \
	_trace(L"\nRunning Test %s\n", _T(x)); \
	VPInterfaces* c = new VPInterfaces(); int ret = c->Parse(_T(x));  \
	assert(exp); _trace(L"Test end with %s\n", c->parserLastError.c_str()); delete c;}
	tn("/size=1444,12/mediafile=\"2345: dkg tt;:////\\\"", ret == 0 && c->wndSize.x == 1444 && c->wndSize.y == 12 && c->mediaFile == L"2345: dkg tt;:////\\");
	tn("/keepwidth=23444", ret == -1);
	tn("/kaa", ret == -1);
	tn("/playlist=\"2312312323", ret == -1);
	tn("/playlist=\"2312312323\"/kaa", ret == -1);
	tn("/playlist=\"2312312323\"/keepwidth=3", ret == 0 && c->keepWidth == 3 && c->playlistFile == L"2312312323");
	tn("/playlist=\"2312312323\"/keepwidth=3 ", ret == 0 && c->keepWidth == 3 && c->playlistFile == L"2312312323");
	tn("/playlist=\"2312312323\" /keepwidth=3", ret == 0 && c->keepWidth == 3 && c->playlistFile == L"2312312323");

	tn("/playlist=\"2312312323\" /keepwidth=", ret == -1 && c->playlistFile == L"2312312323");
	tn("/playlist=2312312323\" /keepwidth=", ret == -1);
	tn("/playlist=\"\" /keepwidth=", ret == -1);
	tn("/playlist=\" /keepwidth=", ret == -1);
	tn("/ ", ret == -1);

	tn(" /playlist=\"2312312323\"/keepwidth=3", ret == 0 && c->keepWidth == 3 && c->playlistFile == L"2312312323");
	tn("/playlist=\"2222\" /size=", ret == -1);
	tn("/size ", ret == -1);
	tn("/size=233, 234", ret == -1);

	tn("/size", ret == -1);
	tn("", ret == 0);
#undef tn

	return 0;
}
