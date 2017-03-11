#pragma once

#include <stdio.h>
#include <windows.h>
#include <tchar.h>

#ifdef _DEBUG
#define _trace debug_trace
inline bool debug_trace(const TCHAR * strOutputString, ...)
{
	TCHAR strBuffer[4096] = { 0 };
	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	_vsntprintf_s(strBuffer, sizeof(strBuffer)-1, strOutputString, vlArgs);
	va_end(vlArgs);
	OutputDebugString(strBuffer);
	return true;
}
#else
#define _trace
#endif
