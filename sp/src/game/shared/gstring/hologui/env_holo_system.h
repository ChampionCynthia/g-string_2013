#ifndef ENV_HOLO_SYSTEM_H
#define ENV_HOLO_SYSTEM_H

#include "cbase.h"

#ifdef CLIENT_DLL
class CHoloPanel;
class CHoloShipAim;
#endif

const Vector &CurrentHoloViewOrigin();
const matrix3x4_t &CurrentHoloViewMatrix();
const matrix3x4_t &CurrentHoloViewMatrixInverted();

class CEnvHoloSystem : public CBaseEntity
{
	DECLARE_CLASS( CEnvHoloSystem, CBaseEntity );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CEnvHoloSystem();
	virtual ~CEnvHoloSystem();

#ifdef GAME_DLL
	//virtual void Spawn();
	virtual void Activate();

	virtual int ObjectCaps()
	{
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();
#else
	void DestroyPanels();

	virtual void OnDataChanged( DataUpdateType_t type );

	virtual int DrawModel( int flags );
	virtual bool ShouldDraw() { return true; }
	virtual RenderGroup_t GetRenderGroup();
	virtual void ClientThink();

	virtual void GetRenderBounds( Vector& mins, Vector& maxs );
	virtual void GetRenderBoundsWorldspace( Vector& mins, Vector& maxs );
#endif
	
private:
#ifdef GAME_DLL
	string_t m_strAttachment;
#else
	void CreatePanels();
	void PreRenderPanels();
	void DrawPanels();

	int m_iAttachment;
	int m_iEyes;
	CUtlVector< CHoloPanel* > m_Panels;

	int m_iViewportWidth;
	int m_iViewportHeight;
#endif

	CNetworkString( m_szAttachment, 16 );
	//CNetworkVar( bool, m_bIsSpaceMap );
};

#endif
