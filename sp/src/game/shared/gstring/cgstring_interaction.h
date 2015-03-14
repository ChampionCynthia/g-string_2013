#ifndef CGSTRING_INTERACTION_H
#define CGSTRING_INTERACTION_H

#include "cbase.h"
#include "gstring_player_shared_forward.h"

class CGstringInteraction : public CBaseEntity
{
	DECLARE_CLASS( CGstringInteraction, CBaseEntity );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CGstringInteraction();
	~CGstringInteraction();

#ifdef GAME_DLL
	virtual void Precache();
	virtual void Spawn();
	virtual void Activate();

	virtual int ObjectCaps( void ){
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();

	void InputStartInteraction( inputdata_t &inputdata );

	void OnObjectEvent( int iEventIndex );
	void OnBodyEvent( int iEventIndex );
	void OnBodyAnimationFinished();
#else
	//virtual void OnDataChanged( DataUpdateType_t type );
#endif

private:
#ifdef GAME_DLL
	string_t m_strFinalPositionName;
	string_t m_strInteractiveObjectName;
	string_t m_strPlayerSequenceName;
	string_t m_strObjectSequenceName;

	CHandle< CBaseEntity > m_hFinalPosition;
	CHandle< CGstringPlayer > m_hPlayer;

	bool m_bInteractionActive;

	COutputEvent m_PlayerEvents[ 5 ];
	COutputEvent m_ObjectEvents[ 5 ];
	COutputEvent m_InteractionStartEvent;
	COutputEvent m_InteractionEndEvent;
#endif

	CNetworkHandle( CBaseAnimating, m_hInteractiveObject );
	//CNetworkVar( bool, m_bCascadedShadowMappingEnabled );
};

#endif