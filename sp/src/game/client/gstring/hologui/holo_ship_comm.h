#ifndef HOLO_SHIP_COMM_H
#define HOLO_SHIP_COMM_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;

class CHoloShipComm : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipComm, CHoloPanelVGUI );

public:
	CHoloShipComm( ISpacecraftData *pSpacecraftData );
	~CHoloShipComm();

	virtual void PerformLayout();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	//virtual void Think( float frametime );

private:
	ISpacecraftData *m_pSpacecraftData;

	vgui::Label *m_pLabelHeader;
};

#endif