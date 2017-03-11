
#include "svplreader.h"

namespace Star_VideoPlayer
{
	class svplwriter {
	public:
		static BOOL WritePlayListToFile(wstring filepath, const svpl* m_svpl);
	};
}