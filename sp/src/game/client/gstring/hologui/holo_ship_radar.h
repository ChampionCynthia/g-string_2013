#ifndef HOLO_SHIP_RADAR_H
#define HOLO_SHIP_RADAR_H

#include "holo_panel.h"

class ISpacecraftData;

class CHoloShipRadar : public CHoloPanel
{
	DECLARE_CLASS_GAMEROOT( CHoloShipRadar, CHoloPanel );

public:
	CHoloShipRadar( vgui::Panel *pParent, ISpacecraftData *pSpacecraftData );
	~CHoloShipRadar();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	virtual void DrawBlips( IMatRenderContext *pRenderContext ); //, bool bPassBelow );

	ISpacecraftData *m_pSpacecraftData;

	IMesh *m_pMeshRingLarge;
	IMesh *m_pMeshRingSmall;
	IMesh *m_pMeshRingGrid;
	IMesh *m_pMeshRingCenter;
	IMesh *m_pMeshRingLine;
	IMesh *m_pMeshCircle;
};

#endif
