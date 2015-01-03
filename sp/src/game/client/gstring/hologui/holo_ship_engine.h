#ifndef HOLO_SHIP_ENGINE_H
#define HOLO_SHIP_ENGINE_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;

class CHoloShipEngine : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipEngine, CHoloPanelVGUI );

public:
	CHoloShipEngine( ISpacecraftData *pSpacecraftData );
	~CHoloShipEngine();

	virtual void PerformLayout();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	ISpacecraftData *m_pSpacecraftData;

	float m_flEngineStrength;

	vgui::Label *m_pLabelEngine;
	vgui::Label *m_pLabelSpeedLabel;
	vgui::Label *m_pLabelSpeedValue;
	IMesh *m_pMeshElement;
};

#endif