#ifndef HOLO_PANEL_VGUI_H
#define HOLO_PANEL_VGUI_H

#include "holo_panel.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/Panel.h"

class CHoloPanelVGUI : public CHoloPanel //, public vgui::Panel
{
	DECLARE_CLASS_GAMEROOT( CHoloPanelVGUI, CHoloPanel );

public:
	CHoloPanelVGUI();
	virtual void Setup();

	virtual void Paint();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

protected:
	virtual void PreRender( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight );
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

	vgui::HFont m_FontLarge;
	vgui::HFont m_FontSmall;
	vgui::HFont m_FontSmallMono;
	Vector2D m_vecUVs[ 2 ];
	Vector2D m_vecPanelWorldOffset;
	float m_flScale;
	float m_flWidth;
	float m_flHeight;

};

#endif
