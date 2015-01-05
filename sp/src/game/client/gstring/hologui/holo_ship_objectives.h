#ifndef HOLO_SHIP_OBJECTIVES_H
#define HOLO_SHIP_OBJECTIVES_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;

class CHoloShipObjectives : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipObjectives, CHoloPanelVGUI );

public:
	CHoloShipObjectives( ISpacecraftData *pSpacecraftData );
	~CHoloShipObjectives();

	virtual void PerformLayout();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	struct Objective
	{
		int m_iEntindex;
		vgui::Label *m_pDescription;
		float m_flPosition;
	};

	ISpacecraftData *m_pSpacecraftData;

	vgui::Label *m_pLabelHeader;
	CUtlVector< Objective > m_Objectives;
};

#endif