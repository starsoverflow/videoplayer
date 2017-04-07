
#include "MyMenu.h"

namespace SVideoPlayer
{
	ConMenu::ConMenu(CDuiString xml)
		:CDuiMenu(xml)
	{
	}


	ConMenu::~ConMenu(void)
	{
	}


	CDuiString ConMenu::GetSkinFolder()
	{
		return L"";
	}
}