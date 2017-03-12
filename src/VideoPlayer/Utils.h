#pragma once


#define RectGetWidth(x)           (x.right - x.left)
#define RectGetHeight(x)          (x.bottom - x.top)
#define RectClientToScreen(x, y) ::ClientToScreen(y, (LPPOINT)&x.left);\
                                 ::ClientToScreen(y, (LPPOINT)&x.right);

namespace Star_VideoPlayer
{
	void __inline string_replace(std::wstring &str, const std::wstring &old_value, const std::wstring &new_value)
	{
		while (true) {
			std::wstring::size_type pos(0);
			if ((pos = str.find(old_value)) != std::wstring::npos)
				str.replace(pos, old_value.length(), new_value);
			else break;
		}
	}

	std::wstring __inline GetFilename(const std::wstring sFull)
	{
		std::wstring ret = sFull;
		string_replace(ret, L"/", L"\\");   // 转换成正规格式
		size_t pos = ret.find_last_of(L"\\");
		if (pos >= 0) ret = ret.substr(pos + 1);
		pos = ret.find_last_of(L".");
		if (pos >= 0) ret = ret.substr(0, pos);

		return ret;
	}
}