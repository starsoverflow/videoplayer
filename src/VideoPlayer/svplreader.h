#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <codecvt>

#include "../mydebug.h"

using namespace std;

// 重写 2016.10.7 用一个svplreader来封装所有对于svpl的访问 提供多线程安全
//                原来的 svplreader 更名为 svplparser 用于读取 svpl 文件
//                优化 svplparser 的代码

namespace Star_VideoPlayer
{
	template<typename _T> class LockHolder
	{
	public:
		LockHolder(_T* PointerToData, CRITICAL_SECTION Initialized_Critical_Section)
		{
			_cs = Initialized_Critical_Section;
			_data = PointerToData;
			EnterCriticalSection(&_cs);
			_trace(L"Svpl CriticalSection Entered\n");
		}

		~LockHolder() noexcept
		{
			LeaveCriticalSection(&_cs);
			_trace(L"Svpl CriticalSection Left\n");
		}

		LockHolder(LockHolder && src)
		{
			_trace(L"Svpl Moved\n");
			EnterCriticalSection(&src._cs);
			_trace(L"Svpl CriticalSection entered\n");
			_cs = src._cs;
			_data = src._data;
		}

		_T* GetPtr()
		{
			return _data;
		}

	private:
		CRITICAL_SECTION _cs;
		_T* _data;

	public:
		LockHolder() = delete;
		LockHolder(const LockHolder &) = delete;
		LockHolder& operator= (const LockHolder &) = delete;

		_T* operator->() const noexcept
		{
			return _data;
		}
	};

	struct svpl_item
	{
		wstring type;
		wstring path;
		wstring title;
		int volume = 100;
		double begintime = 0.0;  // 开始时间
		double endtime = 0.0;    // 结束时间
		DWORD borderColor = 0;   // 这里可以覆盖全局设置
		int errtime = 0;         // 加载错误的次数（此项不应被保存到播放列表中，只是方便错误处理）
	};

	struct svpl_playlist
	{
		wstring name;
		int volume = 50;
		int currentindex = 0;
		vector<svpl_item> items;
		int keepwidth = 800;   // 除非明确定义不使用keepwidth(keepwidth=0) 否则保持为800
	};

	struct svpl_config   // 全局设置
	{
		int normalplaylist = 0;
		int mousehidetime = 10;
		bool alwaystop = true;
		POINT location = { 100,100 };
		POINT size = { 0 };
		DWORD borderColor = 0;
		POINT windowAspectRatio = { 0 };  // 定义是否强制显示边框并且保持窗口长宽比（长由size决定），仅在keepwidth为0时有效。
		                                  // Note: 对showBorder无需额外处理。
	};

	// 存储播放列表数据
	struct svpl {
		vector<svpl_playlist> playlists;
		svpl_config config;
	};

	class svplwrapper {
	public:
		svplwrapper();
		svplwrapper(wstring svplfilepath);
		svplwrapper(LPCWSTR buffer);
		~svplwrapper();

		void CreateDefaultSvpl();

		void AddItem(svpl_item _item, UINT where, int playlistindex);
		void AddItem(svpl_item _item, int playlistindex);
		void DeleteItem(UINT itemindex, int playlistindex);
		bool GetItem(UINT itemindex, svpl_item * _item, int playlistindex);

		svpl_item GetItem(int playlistindex, UINT itemindex);

		auto GetItemNum(int playlistindex)
		{
			EnterCriticalSection(&m_cs);
			if (m_svpl.playlists.size() == 0) {
				LeaveCriticalSection(&m_cs); return (size_t)0;
			}
			auto num = m_svpl.playlists.at(playlistindex).items.size();
			LeaveCriticalSection(&m_cs);
			return num;
		}

		auto Get()
		{
			return LockHolder<svpl>(&m_svpl, m_cs);
		}

		BOOL parserStatus;

	private:
		CRITICAL_SECTION m_cs;
		svpl m_svpl;
	};


	class svplparser {
	public:
		static BOOL ParseSvpl(svpl* svpldest, wstring filepath);
		static BOOL ParseSvpl(svpl* svpldest, LPCWSTR buffer);
	private:
		static BOOL ParseSvpl(svpl* svpldest, wstringstream& strstream);
		static void GetConfigAttribute(svpl* svpldest, wstring s);
		static void SetAttribute(svpl * svpldest, wstring name, wstring value, int type);
		static void GetAttribute(svpl* svpldest, wstring s, int type);
	};
}
