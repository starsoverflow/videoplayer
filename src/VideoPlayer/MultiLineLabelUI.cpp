
#include "MultiLineLabelUI.h"

namespace DuiLib
{
	CMultiLineLabelUI::CMultiLineLabelUI()
    {
    }

	LPCTSTR CMultiLineLabelUI::GetClass() const
	{
		return _T("MultiLineLabelUI");
	}
	
	LPVOID CMultiLineLabelUI::GetInterface(LPCTSTR pstrName)
	{
		if (_tcscmp(pstrName, _T("MultiLineLabelUI")) == 0) return static_cast<CMultiLineLabelUI*>(this);
		return CControlUI::GetInterface(pstrName);
	}

	CMultiLineLabelUI::~CMultiLineLabelUI()
    {
    }

	SIZE CMultiLineLabelUI::EstimateSize(SIZE szAvailable)
	{
		RECT rcText = { 0,0,this->GetFixedWidth(),GetFixedHeight() };

		DrawMultiLineText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, m_dwTextColor, m_iFont, DT_WORDBREAK | DT_EDITCONTROL | DT_CALCRECT | m_uTextStyle);
		m_cxyFixed.cy = rcText.bottom - rcText.top + m_rcTextPadding.bottom + m_rcTextPadding.top;
		
		return CControlUI::EstimateSize(szAvailable);
	}

	void CMultiLineLabelUI::DrawMultiLineText(HDC hDC, CPaintManagerUI* pManager, RECT& rc, LPCTSTR pstrText, DWORD dwTextColor, int iFont, UINT uStyle)
	{
		ASSERT(::GetObjectType(hDC) == OBJ_DC || ::GetObjectType(hDC) == OBJ_MEMDC);
		if (pstrText == NULL || pManager == NULL) return;

		::SetBkMode(hDC, TRANSPARENT);
		::SetTextColor(hDC, RGB(GetBValue(dwTextColor), GetGValue(dwTextColor), GetRValue(dwTextColor)));
		HFONT hOldFont = (HFONT)::SelectObject(hDC, pManager->GetFont(iFont));
		::DrawText(hDC, pstrText, -1, &rc, uStyle | DT_NOPREFIX | DT_WORDBREAK | DT_EDITCONTROL);
		::SelectObject(hDC, hOldFont);
	}

    void CMultiLineLabelUI::PaintText(HDC hDC)
    {
		if (m_dwTextColor == 0) m_dwTextColor = m_pManager->GetDefaultFontColor();
		if (m_dwDisabledTextColor == 0) m_dwDisabledTextColor = m_pManager->GetDefaultDisabledColor();

		RECT rc = m_rcItem;
		rc.left += m_rcTextPadding.left;
		rc.right -= m_rcTextPadding.right;
		rc.top += m_rcTextPadding.top;
		rc.bottom -= m_rcTextPadding.bottom;

		if (!GetEnabledEffect())
		{
			if (m_sText.IsEmpty()) return;
			int nLinks = 0;
			if (IsEnabled()) {
				DrawMultiLineText(hDC, m_pManager, rc, m_sText, m_dwTextColor, m_iFont, m_uTextStyle);
			}
			else {
				DrawMultiLineText(hDC, m_pManager, rc, m_sText, m_dwDisabledTextColor, m_iFont, m_uTextStyle);
			}
		}
		else
		{
			ASSERT(FALSE);
		}
    }
}
