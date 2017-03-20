#pragma once

#include <string>
#include <memory>
using std::wstring;
using std::unique_ptr;

class IRunnable {
public:
	IRunnable() = default;
	IRunnable& operator= (const IRunnable &) = delete;
	IRunnable(const IRunnable &) = delete;

	virtual int Parse(wchar_t*) = 0;
	virtual int Execute() = 0;
	virtual wstring GetLastError() = 0;
};

unique_ptr<IRunnable> GetInterface();
