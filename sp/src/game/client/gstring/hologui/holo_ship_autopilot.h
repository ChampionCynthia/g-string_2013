#ifndef HOLO_SHIP_AUTOPILOT_H
#define HOLO_SHIP_AUTOPILOT_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;
class IHoloTarget;

class CHoloShipAutopilot : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipAutopilot, CHoloPanelVGUI );

public:
	CHoloShipAutopilot( vgui::Panel *pParent, ISpacecraftData *pSpacecraftData );
	~CHoloShipAutopilot();

	virtual void PerformLayout();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	ISpacecraftData *m_pSpacecraftData;

	IMesh *m_pMeshIcon;
	IMesh *m_pMeshOutline;
	CMaterialReference m_MaterialAutopilotIcon;
	vgui::Label *m_pLabelAutopilot;
	//vgui::Label *m_pLabelDash;
	//vgui::Label *m_pLabelHealth;
	//vgui::Label *m_pLabelDistance;

	//int m_iHealth;
	//int m_iDistance;
	//const IHoloTarget *m_pTarget;
};

#endif
