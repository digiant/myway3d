#pragma once

#include "xApp.h"

class CPropertiesToolBar : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};



class xPropertyGrid : public IDockPane
{
	DECLARE_SINGLETON(xPropertyGrid);
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExpandAllProperties();
	afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
	afx_msg void OnSortProperties();
	afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);
	afx_msg void OnProperties1();
	afx_msg void OnUpdateProperties1(CCmdUI* pCmdUI);
	afx_msg void OnProperties2();
	afx_msg void OnUpdateProperties2(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point) {}

	afx_msg LRESULT OnPropertyChanged(WPARAM wParam, LPARAM lParam);

public:
	xPropertyGrid();
	virtual ~xPropertyGrid();

	void AdjustLayout();

protected:
	void _OnSelect(void * param0, void * param1);
	void _OnUnSelect(void * param0, void * param1);
	void Show(xObj * obj);
	void _ToCtrl(CMFCPropertyGridProperty * gp, xObj * obj, const Property * p);

// ����
public:
	void SetVSDotNetLook(BOOL bSet)
	{
		m_wndPropList.SetVSDotNetLook(bSet);
		m_wndPropList.SetGroupNameFullWidth(bSet);
	}

protected:
	CFont m_fntPropList;
	CPropertiesToolBar m_wndToolBar;
	CMFCPropertyGridCtrl m_wndPropList;

	void InitPropList();
	void SetPropListFont();

	tEventListener<xPropertyGrid> OnSelectObj;
	tEventListener<xPropertyGrid> OnUnSelectObj;
};

