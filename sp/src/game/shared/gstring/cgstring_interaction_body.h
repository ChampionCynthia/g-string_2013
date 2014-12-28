#ifndef CGSTRING_INTERACTION_BODY_H
#define CGSTRING_INTERACTION_BODY_H

#include "cbase.h"

class CGstringInteraction;

class CGstringInteractionBody : public CBaseAnimating
{
	DECLARE_CLASS( CGstringInteractionBody, CBaseAnimating );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CGstringInteractionBody();
	~CGstringInteractionBody();

#ifdef GAME_DLL
	virtual void Spawn();
	virtual void Activate();

	virtual int ObjectCaps( void ){
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();
	virtual void HandleAnimEvent( animevent_t *pEvent );

	void SetInteractionEntity( CGstringInteraction *pInteractionEntity );
#else
	void SetTransitionBlend( float flBlend );

	virtual ShadowType_t ShadowCastType() { return SHADOWS_NONE; }
	virtual int DrawModel( int flags );

	virtual void OnDataChanged( DataUpdateType_t type );
	virtual CStudioHdr *OnNewModel();

	void GetCamera( Vector &origin, QAngle &angles );
#endif

private:
#ifdef GAME_DLL
	void Think();

	CHandle< CGstringInteraction > m_hInteraction;
#else
	float m_flPlayerTransitionBlend;
	int m_iEyesAttachment;
#endif
};

#endif