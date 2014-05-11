#ifndef C_SPACE_CRAFT_H
#define C_SPACE_CRAFT_H

#ifdef CLIENT_DLL
#include "c_baseanimating.h"
#define CBaseAnimating C_BaseAnimating
#else
#include "baseanimating.h"
#endif

#include "igamemovement.h"
#include "gstring_player_shared_forward.h"

#ifdef CLIENT_DLL
class CHudCrosshair;
#endif

class CSpacecraft : public CBaseAnimating
{
	DECLARE_CLASS( CSpacecraft, CBaseAnimating );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CSpacecraft();
	virtual ~CSpacecraft();

#ifdef GAME_DLL
	virtual void Precache();
	virtual void Spawn();
	virtual void Activate();

	virtual void PhysicsSimulate();
	virtual void PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity );

	void InputEnterVehicle( inputdata_t &inputdata );
#else
	//virtual bool IsTransparent() { return false; }
	virtual bool IsTwoPass() { return true; }
	virtual RenderGroup_t GetRenderGroup() { return RENDER_GROUP_TWOPASS; }
	virtual ShadowType_t ShadowCastType() { return SHADOWS_SIMPLE; }
	virtual bool ShouldReceiveProjectedTextures( int flags ) { return true; }

	virtual CStudioHdr *OnNewModel();

	virtual void OnDataChanged( DataUpdateType_t t );
	virtual void UpdateOnRemove();
	virtual void ClientThink();

	void UpdateCrosshair( CHudCrosshair *pCrosshair );
#endif

	virtual void SimulateMove( CMoveData &moveData );

private:
#ifdef GAME_DLL
#else
	CUtlVector< int > m_ThrusterAttachments;
	CUtlVector< int > m_ThrusterSounds;
	CUtlVector< int > m_EngineAttachments;

	CUtlVector< CSmartPtr< CNewParticleEffect > > m_ThrusterParticles;
	CUtlVector< CSmartPtr< CNewParticleEffect > > m_EngineParticles;

	CSmartPtr< CNewParticleEffect > m_SpaceFieldParticles;

	int m_iEngineLevelLast;
	int m_iGUID_Engine;
	int m_iGUID_Boost;

	float m_flEngineVolume;
	float m_flShakeTimer;
#endif

	CNetworkQAngle( m_AngularImpulse );
	CNetworkVector( m_PhysVelocity );
	CNetworkVar( int, m_iEngineLevel );
};

#endif