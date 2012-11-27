#pragma once

#include "xTerrainHeightDlg.h"
#include "xTerrainLayerDlg.h"
#include "xEditTerrainHeight.h"
#include "xEditTerrainLayer.h"

class xTerrainPane : public CDockablePane
{
	DECLARE_MESSAGE_MAP()

public:
	xTerrainPane();
	virtual ~xTerrainPane();

	void AdjustLayout();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	void _Init(void *);
	void _Shutdown(void *);
	void _Update(void *);
	void _Render(void *);
	void _RenderUI(void *);

protected:
	tEventListener<xTerrainPane> OnInit;
	tEventListener<xTerrainPane> OnShutdown;
	tEventListener<xTerrainPane> OnUpdate;
	tEventListener<xTerrainPane> OnRender;
	tEventListener<xTerrainPane> OnRenderUI;

	CMFCTabCtrl mTab;

	xTerrainHeightDlg mHeightDlg;
	xTerrainLayerDlg mLayerDlg;

	xEditTerrainHeight mEditHeight;
	xEditTerrainLayer mEditLayer;
};