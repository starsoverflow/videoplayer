#pragma once

#include "../DuiLib/UIlib.h"

namespace DuiLib
{
    class CMultiLineLabelUI : public CLabelUI
    {
    public:
		CMultiLineLabelUI();
        virtual ~CMultiLineLabelUI();

		SIZE EstimateSize(SIZE szAvailable);

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);

		void DrawMultiLineText(HDC hDC, CPaintManagerUI * pManager, RECT & rc, LPCTSTR pstrText, DWORD dwTextColor, int iFont, UINT uStyle);

		void PaintText(HDC hDC);
    };
}
