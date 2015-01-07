#ifndef HOLO_SHIP_AIM_H
#define HOLO_SHIP_AIM_H

#include "holo_panel.h"

class ISpacecraftData;
class IHoloTarget;

class CHoloShipAim : public CHoloPanel
{
public:
	CHoloShipAim( ISpacecraftData *pSpacecraftData );
	virtual ~CHoloShipAim();

protected:
	virtual void Think( float frametime );
	virtual void Draw( IMatRenderContext *pRenderContext );

private:
	struct Target
	{
		Target() :
			m_Entity( NULL ),
			m_flBlinkTimer( 0.0f )
		{}
		IHoloTarget *m_Entity;
		float m_flBlinkTimer;
	};

	void DrawTargets( IMatRenderContext *pRenderContext );

	ISpacecraftData *m_pSpacecraftData;
	int m_iAttachmentGUI;
	CUtlVector< Target > m_Targets;
	CUtlVector< IHoloTarget* > m_KnownTargetEntities;

	IMesh *m_pMeshLargeReticule;
	IMesh *m_pMeshReticule;
	IMesh *m_pMeshTarget;
	IMesh *m_pMeshTargetThick;
	IMesh *m_pMeshTargetArrows;
	IMesh *m_pMeshPanel;
};

#endif
