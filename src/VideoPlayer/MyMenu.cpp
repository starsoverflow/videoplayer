
#include "MyMenu.h"

namespace Star_VideoPlayer
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