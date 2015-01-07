#ifndef HOLO_SHIP_AIM_INFO_H
#define HOLO_SHIP_AIM_INFO_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;
class IHoloTarget;

class CHoloShipAimInfo : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipAimInfo, CHoloPanelVGUI );

public:
	CHoloShipAimInfo( ISpacecraftData *pSpacecraftData );
	~CHoloShipAimInfo();

	virtual void PerformLayout();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	ISpacecraftData *m_pSpacecraftData;

	vgui::Label *m_pLabelTarget;
	vgui::Label *m_pLabelDash;
	vgui::Label *m_pLabelHealth;
	vgui::Label *m_pLabelDistance;

	int m_iHealth;
	int m_iDistance;
	const IHoloTarget *m_pTarget;
};

#endif
