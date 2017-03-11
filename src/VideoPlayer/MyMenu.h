#pragma once
#include "MenuUI.h"

namespace Star_VideoPlayer
{
	using namespace DuiLib;

	class ConMenuUI : public CMenuUI
	{
	public:
		void OnItemHot(CMenuElementUI* pMenuItem)
		{
			CLabelUI* pControl = (CLabelUI*)m_pManager->FindSubControlByName(pMenuItem, L"menu_label");
			pMenuItem->SetBkColor(0xffb8b8b8);
			pControl->SetTextColor(0xffffffff);

			CLabelUI* pControl2 = (CLabelUI*)m_pManager->FindSubControlByName(pMenuItem, L"menu_shortcut");
			if (pControl2)
			{
				pControl2->SetEnabled(true);
				pControl2->SetTextColor(0xffffffff);
			}

			CControlUI* pIco = m_pManager->FindSubControlByName(pMenuItem, L"ico");
			CControlUI* pIcoHover = m_pManager->FindSubControlByName(pMenuItem, L"ico_hover");
			if (pIco&&pIcoHover)
			{
				pIco->SetVisible(false);
				pIcoHover->SetVisible(true);
			}


			CControlUI* pExtend = m_pManager->FindSubControlByName(pMenuItem, L"extend");
			CControlUI* pExtend_Hover = m_pManager->FindSubControlByName(pMenuItem, L"extend_hover");

			if (pExtend&&pExtend_Hover)
			{
				pExtend->SetVisible(false);
				pExtend_Hover->SetVisible(true);
			}

		}


		void OnItemUnHot(CMenuElementUI* pMenuItem)
		{
			CLabelUI* pControl = (CLabelUI*)m_pManager->FindSubControlByName(pMenuItem, L"menu_label");
			pMenuItem->SetBkColor(0x00ffffff);
			pControl->SetTextColor(0xff000000);

			CLabelUI* pControl2 = (CLabelUI*)m_pManager->FindSubControlByName(pMenuItem, L"menu_shortcut");
			if (pControl2)
			{
				pControl2->SetEnabled(false);
			}

			CControlUI* pIco = m_pManager->FindSubControlByName(pMenuItem, L"ico");
			CControlUI* pIcoHover = m_pManager->FindSubControlByName(pMenuItem, L"ico_hover");
			if (pIco&&pIcoHover)
			{
				pIco->SetVisible(true);
				pIcoHover->SetVisible(false);
			}


			CControlUI* pExtend = m_pManager->FindSubControlByName(pMenuItem, L"extend");
			CControlUI* pExtend_Hover = m_pManager->FindSubControlByName(pMenuItem, L"extend_hover");

			if (pExtend&&pExtend_Hover)
			{
				pExtend->SetVisible(true);
				pExtend_Hover->SetVisible(false);
			}


		}

	};

	class ConMenu : public CDuiMenu
	{
	public:

		CControlUI* CreateControl(LPCTSTR pstrClass)
		{

			if (_tcscmp(pstrClass, L"Menu") == 0)
			{
				return new ConMenuUI;
			}
			return CDuiMenu::CreateControl(pstrClass);
		}

		ConMenu(CDuiString xml);
		~ConMenu(void);
		virtual CDuiString     GetSkinFolder();
	};
}