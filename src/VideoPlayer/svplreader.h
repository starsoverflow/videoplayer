#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <codecvt>

#include "../mydebug.h"

using namespace std;

// ��д 2016.10.7 ��һ��svplreader����װ���ж���svpl�ķ��� �ṩ���̰߳�ȫ
//                ԭ���� svplreader ����Ϊ svplparser ���ڶ�ȡ svpl �ļ�
//                �Ż� svplparser �Ĵ���

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
		double begintime = 0.0;  // ��ʼʱ��
		double endtime = 0.0;    // ����ʱ��
		DWORD borderColor = 0;   // ������Ը���ȫ������
		int errtime = 0;         // ���ش���Ĵ��������Ӧ�����浽�����б��У�ֻ�Ƿ��������
	};

	struct svpl_playlist
	{
		wstring name;
		int volume = 50;
		int currentindex = 0;
		vector<svpl_item> items;
		int keepwidth = 800;   // ������ȷ���岻ʹ��keepwidth(keepwidth=0) ���򱣳�Ϊ800
	};

	struct svpl_config   // ȫ������
	{
		int normalplaylist = 0;
		int mousehidetime = 10;
		bool alwaystop = true;
		POINT location = { 100,100 };
		POINT size = { 0 };
		DWORD borderColor = 0;
		POINT windowAspectRatio = { 0 };  // �����Ƿ�ǿ����ʾ�߿��ұ��ִ��ڳ���ȣ�����size������������keepwidthΪ0ʱ��Ч��
		                                  // Note: ��showBorder������⴦��
	};

	// �洢�����б�����
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
