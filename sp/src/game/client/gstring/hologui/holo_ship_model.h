#ifndef HOLO_SHIP_MODEL_H
#define HOLO_SHIP_MODEL_H

#include "holo_panel.h"

class ISpacecraftData;

class CHoloShipModel : public CHoloPanel
{
public:
	CHoloShipModel( ISpacecraftData *pSpacecraftData );
	virtual ~CHoloShipModel();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

private:
	CMaterialReference m_MaterialHoloModel;
	ISpacecraftData *m_pSpacecraftData;

	CHandle< C_BaseAnimating > m_hModel;
	int m_iHull;
	float m_flDamageTimer;
	int m_iShield;
	float m_flShieldTimer;
};

#endif
