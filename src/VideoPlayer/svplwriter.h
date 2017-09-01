
#include "svplreader.h"

namespace SVideoPlayer
{
	class svplwriter {
	public:
		static BOOL WritePlayListToFile(wstring filepath, const svpl* m_svpl);
		static BOOL GetPlaylistData(wstringstream& stream, const svpl* m_svpl);
	};
}
