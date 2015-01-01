#ifndef HOLO_SHIP_HEALTH_GRAPHIC_H
#define HOLO_SHIP_HEALTH_GRAPHIC_H

#include "holo_panel.h"

class ISpacecraftData;

class CHoloShipHealthGraphic : public CHoloPanel
{
	DECLARE_CLASS_GAMEROOT( CHoloShipHealthGraphic, CHoloPanel );

public:
	CHoloShipHealthGraphic( ISpacecraftData *pSpacecraftData );
	virtual ~CHoloShipHealthGraphic();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	ISpacecraftData *m_pSpacecraftData;
	IMesh *m_pHullElement;
	float m_flHullFraction;
	float m_flShieldFraction;
};

#endif
