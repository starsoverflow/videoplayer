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
			L"\n\n                       /nooverwrite        退出时不保存播放列表"
			L"\n                       /nosavehistory      退出时不保存历史记录"
			L"\n                       /playlist           指定播放列表文件"
			L"\n                       /mediafile          指定要播放的媒体文件"
			L"\n\n                       以下选项只有指定了/mediafile才有效:"
			L"\n                       /keepwidth          使窗口保持指定的宽度和视频的纵横比"
			L"\n                       /size               指定窗口大小"
			L"\n                                           该选项不能与keepwidth同时设置，否则将忽略size选项"
			L"\n                       /add                将媒体文件添加到已经在运行的播放器实例"
			L"\n                       /alwaysOnTop        将窗口设为总在最前"
			L"\n                       /windowAspectRatio  设定窗口的横纵比，这将使得窗口在拉伸时保持设定的横纵比"
			L"\n                                           可以在size中设定初始的窗口宽度，高度值将会被忽略"
			L"\n                                           该选项不能与keepwidth同时设置，否则将忽略windowAspectRatio选项"
			L"\n\n                       如果同时指定了/playlist和/mediafile，只有/mediafile生效";

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
