#ifndef HOLO_SHIP_THRUSTER_H
#define HOLO_SHIP_THRUSTER_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;

class CHoloShipThruster : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipThruster, CHoloPanelVGUI );

public:
	CHoloShipThruster( ISpacecraftData *pSpacecraftData );
	~CHoloShipThruster();

	virtual void PerformLayout();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	ISpacecraftData *m_pSpacecraftData;

	float m_flThrusterStrength;

	vgui::Label *m_pLabelThruster;
	IMesh *m_pMeshElement;
};

#endif
