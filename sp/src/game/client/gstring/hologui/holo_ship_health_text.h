#ifndef HOLO_SHIP_HEALTH_TEXT_H
#define HOLO_SHIP_HEALTH_TEXT_H

#include "holo_panel_vgui.h"

class ISpacecraftData;

class CHoloShipHealthText : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipHealthText, CHoloPanelVGUI );

public:
	CHoloShipHealthText( ISpacecraftData *pSpacecraftData );

	virtual void PerformLayout();

protected:
	virtual void Think( float frametime );

private:
	ISpacecraftData *m_pSpacecraftData;

	vgui::Label *m_pLabelShieldText;
	vgui::Label *m_pLabelHullText;
	vgui::Label *m_pLabelShieldValue;
	vgui::Label *m_pLabelHullValue;

	int m_iShield;
	int m_iHull;
};


#endif
