#include <windows.h>

#include "svplreader.h"
#include <algorithm>
#include <sstream>

namespace SVideoPlayer
{
	svplwrapper::svplwrapper()
	{
		InitializeCriticalSection(&m_cs);
		CreateDefaultSvpl();
		parserStatus = 0;
		_trace(L"svplwrapper Intialized\n");
	}

	svplwrapper::svplwrapper(wstring svplfilepath)
	{
		InitializeCriticalSection(&m_cs);
		parserStatus = svplparser::ParseSvpl(&m_svpl, svplfilepath);
		_trace(L"svplwrapper Intialized\n");
	}

	svplwrapper::svplwrapper(LPCWSTR buffer)
	{
		InitializeCriticalSection(&m_cs);
		parserStatus = svplparser::ParseSvpl(&m_svpl, buffer);
		_trace(L"svplwrapper Intialized\n");
	}

	svplwrapper::~svplwrapper()
	{
		DeleteCriticalSection(&m_cs);
		_trace(L"svplwrapper Destructed\n");
	}

	void svplwrapper::CreateDefaultSvpl()
	{
		EnterCriticalSection(&m_cs);
		m_svpl.playlists.clear();
		svpl_playlist newplaylist;
		m_svpl.playlists.push_back(newplaylist);
		svpl_config newconfig;
		m_svpl.config = newconfig;
		LeaveCriticalSection(&m_cs);
	}

	// 在 where 的前面插入元素
	void svplwrapper::AddItem(svpl_item _item, UINT where, int playlistindex/* = 0*/)
	{
		EnterCriticalSection(&m_cs);
		if (where >= 0 && where <= m_svpl.playlists.at(playlistindex).items.size() - 1)
			m_svpl.playlists.at(playlistindex).items.insert(m_svpl.playlists.at(playlistindex).items.begin() + where, _item);
		else if (where == m_svpl.playlists.at(playlistindex).items.size())
			m_svpl.playlists.at(playlistindex).items.push_back(_item);
		LeaveCriticalSection(&m_cs);
	}

	void svplwrapper::AddItem(svpl_item _item, int playlistindex/* = 0*/)
	{
		EnterCriticalSection(&m_cs);
		m_svpl.playlists.at(playlistindex).items.push_back(_item);
		LeaveCriticalSection(&m_cs);
	}

	void svplwrapper::DeleteItem(UINT itemindex, int playlistindex/* = 0*/)
	{
		EnterCriticalSection(&m_cs);
		if (m_svpl.playlists.size() != 0) {
			if (itemindex <= m_svpl.playlists.at(playlistindex).items.size() - 1)
				m_svpl.playlists.at(playlistindex).items.erase(m_svpl.playlists.at(playlistindex).items.begin() + itemindex);
		}
		LeaveCriticalSection(&m_cs);
	}

	// Called from DragListUI
	bool svplwrapper::GetItem(UINT itemindex, svpl_item * _item, int playlistindex)
	{
		bool ret = false;
		EnterCriticalSection(&m_cs);
		try {
			*_item = m_svpl.playlists.at(playlistindex).items.at(itemindex);
			ret = true;
		}
		catch (...)
		{
			ret = false;
		}
		LeaveCriticalSection(&m_cs);
		return ret;
	}

	svpl_item svplwrapper::GetItem(int playlistindex, UINT itemindex)
	{
		svpl_item copy;
		EnterCriticalSection(&m_cs);
		try {
			copy = m_svpl.playlists.at(playlistindex).items.at(itemindex);
		}
		catch (...)
		{

		}
		LeaveCriticalSection(&m_cs);
		return copy;
	}

	// --------------------- svpl parser ---------------------

	// 返回值：-1 无法访问   -2 文件内容无效   0 成功
	BOOL svplparser::ParseSvpl(svpl * svpldest, wstring filepath)
	{
		ios::sync_with_stdio(false);
		wifstream fin(filepath);
		fin.imbue(locale(locale::classic(), new codecvt_utf8<wchar_t>));   // utf-8编码
		
		if (!fin) return -1;

		wstringstream wss;
		wss << fin.rdbuf();

		BOOL ret = ParseSvpl(svpldest, wss);
		ios::sync_with_stdio(true);

		return ret;
	}

	BOOL svplparser::ParseSvpl(svpl* svpldest, LPCWSTR buffer)
	{
		wstringstream wss;
		wss << buffer;
		return ParseSvpl(svpldest, wss);
	}

	BOOL svplparser::ParseSvpl(svpl* svpldest, wstringstream& strstream)
	{
		wstring s;

		bool inplaylist = false;  // 在playlist中
		bool inadapt = false;

		while (getline(strstream, s))
		{
			if (s != L"\0" && s.substr(0, 3) != L"***") {   // 如果不是注释
				if (s.substr(0, 11) == L"</playlist>") {
					inplaylist = false;
				}
				else if (s.substr(0, 9) == L"<playlist") {
					inplaylist = true;
					svpl_playlist newplaylist;
					svpldest->playlists.push_back(newplaylist);
					GetAttribute(svpldest, s, 1);   // 1表示playlist;2表示item
				}
				else if (s.substr(0, 5) == L"<item" && inplaylist) {
					svpl_item newitem;
					svpldest->playlists.back().items.push_back(newitem);
					GetAttribute(svpldest, s, 2);
				}
				else if (s.substr(0, 2) == L"# ") {
					GetConfigAttribute(svpldest, s.substr(2, s.length() - 2));
				}
			}
		}

		// Check validity
		if (svpldest->config.normalplaylist < 0) return -2;
		if (svpldest->config.normalplaylist >= (int)svpldest->playlists.size()) return -2;
		if (svpldest->config.mousehidetime <= 0) return -2;
		return 0;
	}

	void svplparser::GetConfigAttribute(svpl* svpldest, wstring s)
	{
		wstring attribute = L"";
		wstring value = L"";
		bool invalue = false;
		for (unsigned int i = 0; i <= s.length() - 1; i++) {
			if (!invalue) {
				if (s[i] != '=') attribute = attribute + s[i]; else invalue = true;
			}
			else value = value + s[i];
		}
		transform(attribute.begin(), attribute.end(), attribute.begin(), ::tolower);
		if (attribute == L"normalplaylist" || attribute == L"defaultplaylist")
		{
			svpldest->config.normalplaylist = _wtoi(value.c_str());
		}
		else if (attribute == L"mousehidetime" || attribute == L"hidecursortimeout")
		{
			svpldest->config.mousehidetime = _wtoi(value.c_str());
		}
		else if (attribute == L"alwaystop" || attribute == L"alwaysontop")
		{
			transform(value.begin(), value.end(), value.begin(), ::tolower);
			if (value == L"true") svpldest->config.alwaystop = true;
			else svpldest->config.alwaystop = false;
		}
		else if (attribute == L"location")
		{
			size_t wh = value.find_first_of(L',');
			if (wh != wstring::npos)
			{
				svpldest->config.location.x = _wtoi(value.substr(0, wh).c_str());
				svpldest->config.location.y = _wtoi(value.substr(wh + 1).c_str());
			}
		}
		else if (attribute == L"size")
		{
			size_t wh = value.find_first_of(L',');
			if (wh != wstring::npos)
			{
				svpldest->config.size.x = _wtoi(value.substr(0, wh).c_str());
				svpldest->config.size.y = _wtoi(value.substr(wh + 1).c_str());
			}
		}
		else if (attribute == L"bordercolor")
		{
			svpldest->config.borderColor = wcstol(value.c_str(), nullptr, 16);
		}
		else if (attribute == L"windowaspectratio")
		{
			size_t wh = value.find_first_of(L':');
			if (wh != wstring::npos)
			{
				svpldest->config.windowAspectRatio.x = _wtoi(value.substr(0, wh).c_str());
				svpldest->config.windowAspectRatio.y = _wtoi(value.substr(wh + 1).c_str());
			}
		}
	}

	void svplparser::SetAttribute(svpl * svpldest, wstring name, wstring value, int type)
	{
		transform(name.begin(), name.end(), name.begin(), ::tolower);
		switch (type)
		{
		case 1: {
			auto n_playlist = &svpldest->playlists.back();
			if (name == L"name") n_playlist->name = value;
			else if (name == L"volume") n_playlist->volume = _wtoi(value.c_str());
			else if (name == L"currentindex") n_playlist->currentindex = _wtoi(value.c_str());
			else if (name == L"keepwidth") n_playlist->keepwidth = _wtoi(value.c_str());
			break;
		}
		case 2: {
			auto n_item = &svpldest->playlists.back().items.back();
			if (name == L"type") n_item->type = value;
			if (name == L"path") n_item->path = value;
			if (name == L"title") n_item->title = value;
			if (name == L"volume") n_item->volume = _wtoi(value.c_str());
			if (name == L"begintime") n_item->begintime = _wtof_l(value.c_str(), NULL);
			if (name == L"endtime") n_item->endtime = _wtof_l(value.c_str(), NULL);
			if (name == L"bordercolor") n_item->borderColor = wcstol(value.c_str(), nullptr, 16);
			break;
		}
		}
	}

	void svplparser::GetAttribute(svpl * svpldest, wstring s, int type)
	{

		bool inattribute = false; // 是否在属性中
		bool invalue = false;     // 是否在属性值中
		bool getquote = false;    // 是否得到了引号
		wstring attrname;  // 属性名称
		wstring attrvalue; // 属性值
		unsigned int i;

		for (i = 0; i <= s.length() - 1; i++) {
			if (inattribute) {
				// 在属性里
				if (s[i] == '=' && !getquote) {
					invalue = true;
				}
				else {
					if (!invalue) {
						attrname = attrname + s[i];
					}
					else {
						if (s[i] == '"') {
							if (getquote == true) {
								// 结束
								getquote = false;
								// 设定值
								SetAttribute(svpldest, attrname, attrvalue, type);
								invalue = false;
								inattribute = false;
								attrname = L"";
								attrvalue = L"";
							}
							else {
								getquote = true;
							}
						}
						else {
							attrvalue = attrvalue + s[i];
						}

					}
				}
			}

			if (s[i] == ' ' && !getquote) {
				// 进入了属性
				inattribute = true;
			}
		}

	}
}