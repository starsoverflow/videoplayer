#include "entry.h"
#include "cmdLine.h"

#ifdef _DEBUG
#    pragma comment(lib, "..\\bin\\Debug\\DuiLib.lib")
#else
#    pragma comment(lib, "..\\bin\\Lib\\DuiLib.lib")
#endif

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	int mainRetValue = 0;
	auto ivp = GetInterface();

	if (ivp->Parse(lpCmdLine) == 0)
	{
		mainRetValue = ivp->Execute();
	}
	else
	{
		wstring parserLastError = ivp->GetLastError();
		mainRetValue = -1;
		bool bAttached = true;
		if (!AttachConsole(ATTACH_PARENT_PROCESS))
		{
			if (!AllocConsole()) return mainRetValue;
			bAttached = false;
		}
		
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		
		WORD bgColor = 0x00f0 & info.wAttributes;
		
		if (!bAttached)
		{
			COORD beginCoord = { 0, 0 }; DWORD charsWritten = 0;
			bgColor = BACKGROUND_GREEN | BACKGROUND_BLUE;
			FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), bgColor, info.dwSize.X * info.dwSize.Y, beginCoord, &charsWritten);
			SetConsoleTitleW(L" Video Player Version 1.5");
		}

		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | bgColor);

		parserLastError += L"\n\n                Usage: videoplayer [/nooverwrite] [/nosavehistory] [/mediafile=\"Your media file path\"]"
			L"\n                                   [/playlist=\"Your playlist path\"] [/keepwidth=800] [/size=800,450] [/add]"
			L"\n                                   [/alwaysOnTop] [/windowAspectRatio=16:9]"
			L"\n\n                       /nooverwrite        �˳�ʱ�����沥���б�"
			L"\n                       /nosavehistory      �˳�ʱ��������ʷ��¼"
			L"\n                       /playlist           ָ�������б��ļ�"
			L"\n                       /mediafile          ָ��Ҫ���ŵ�ý���ļ�"
			L"\n\n                       ����ѡ��ֻ��ָ����/mediafile����Ч:"
			L"\n                       /keepwidth          ʹ���ڱ���ָ���Ŀ�Ⱥ���Ƶ���ݺ��"
			L"\n                       /size               ָ�����ڴ�С"
			L"\n                                           ��ѡ�����keepwidthͬʱ���ã����򽫺���sizeѡ��"
			L"\n                       /add                ��ý���ļ���ӵ��Ѿ������еĲ�����ʵ��"
			L"\n                       /alwaysOnTop        ��������Ϊ������ǰ"
			L"\n                       /windowAspectRatio  �趨���ڵĺ��ݱȣ��⽫ʹ�ô���������ʱ�����趨�ĺ��ݱ�"
			L"\n                                           ������size���趨��ʼ�Ĵ��ڿ�ȣ��߶�ֵ���ᱻ����"
			L"\n                                           ��ѡ�����keepwidthͬʱ���ã����򽫺���windowAspectRatioѡ��"
			L"\n\n                       ���ͬʱָ����/playlist��/mediafile��ֻ��/mediafile��Ч";

		if (bAttached)
		{
			parserLastError = L"  The command line is: " + (wstring)lpCmdLine + 
				L"\n\n                Error: " + parserLastError + L"\n";
		}
		else
		{
			parserLastError = L"\n\n     Error: " + parserLastError + L"\n\n     Press any key to exit......     \n";
		}

		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), parserLastError.c_str(), parserLastError.length(), nullptr, nullptr);

		if (!bAttached)
		{
			INPUT_RECORD ir = { 0 }; DWORD nlen = 0;
			while (true)
			{
				if (!ReadConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &nlen)) break;
				if (nlen == 0) break;
				if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) break;
			}
		}
		else
		{
			PostMessageW(GetConsoleWindow(), WM_KEYDOWN, VK_RETURN, 0);
		}
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), info.wAttributes);
		
		FreeConsole();
	}

	return mainRetValue;
}
