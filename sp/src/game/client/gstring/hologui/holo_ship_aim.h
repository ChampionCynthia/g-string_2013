#ifndef HOLO_SHIP_AIM_H
#define HOLO_SHIP_AIM_H

#include "holo_panel.h"

class ISpacecraftData;

class CHoloShipAim : public CHoloPanel
{
public:
	CHoloShipAim( ISpacecraftData *pSpacecraftData );
	virtual ~CHoloShipAim();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );

private:
	void DrawTargets( IMatRenderContext *pRenderContext );

	ISpacecraftData *m_pSpacecraftData;
	int m_iAttachmentGUI;

	IMesh *m_pMeshLargeReticule;
	IMesh *m_pMeshReticule;
	IMesh *m_pMeshTarget;
	IMesh *m_pMeshPanel;
};

#endif
