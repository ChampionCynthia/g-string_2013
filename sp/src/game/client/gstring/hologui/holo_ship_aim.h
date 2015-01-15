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

	void MsgFunc_SpacecraftDamage( bf_read &msg );

protected:
	virtual void Think( float frametime );
	virtual void Draw( IMatRenderContext *pRenderContext );

private:
	struct Target
	{
		Target() :
			m_Entity( NULL ),
			m_flBlinkTimer( 0.0f ),
			m_flFocusTimer( 0.0f )
		{}
		IHoloTarget *m_Entity;
		float m_flBlinkTimer;
		float m_flFocusTimer;
	};

	struct DamagePanel
	{
		float m_flAngle;
		float m_flAlpha;
		int m_iType;
	};

	void DrawTargets( IMatRenderContext *pRenderContext );
	void DrawReticule( IMatRenderContext *pRenderContext );

	ISpacecraftData *m_pSpacecraftData;
	int m_iAttachmentGUI;
	CUtlVector< Target > m_Targets;
	CUtlVector< IHoloTarget* > m_KnownTargetEntities;
	CUtlVector< DamagePanel > m_DamagePanels;

	IMesh *m_pMeshLargeReticule;
	IMesh *m_pMeshReticule;
	IMesh *m_pMeshTarget;
	IMesh *m_pMeshTargetThick;
	IMesh *m_pMeshTargetArrows;
	IMesh *m_pMeshPanel;
	IMesh *m_pMeshDamagePanel;
	IMesh *m_pMeshDamagePanelDecor;
	IMesh *m_pMeshDamagePanelInner;
	IMesh *m_pMeshDamagePanelOuter;
};

#endif
