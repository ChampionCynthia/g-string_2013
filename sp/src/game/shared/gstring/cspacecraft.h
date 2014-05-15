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

#else
class ISpacecraftAI
{
public:
	virtual ~ISpacecraftAI() {}

	virtual void Run( float flFrametime ) = 0;
};
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

	bool IsPlayerControlled();

#ifdef GAME_DLL
	void SetAI( ISpacecraftAI *pSpacecraftAI );

	virtual void Precache();
	virtual void Spawn();
	virtual void Activate();

	//virtual bool IsNPC() const;
	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );

	void InputEnterVehicle( inputdata_t &inputdata );
#else
	//virtual bool IsTransparent() { return false; }
	virtual bool IsTwoPass() { return true; }
	virtual RenderGroup_t GetRenderGroup() { return RENDER_GROUP_TWOPASS; }
	virtual ShadowType_t ShadowCastType() { return SHADOWS_SIMPLE; }
	virtual bool ShouldReceiveProjectedTextures( int flags ) { return true; }

	virtual void OnDataChanged( DataUpdateType_t t );
	virtual void UpdateOnRemove();
	virtual void ClientThink();

	void UpdateCrosshair( CHudCrosshair *pCrosshair );
#endif

	virtual CStudioHdr *OnNewModel();
	virtual void SimulateMove( CMoveData &moveData, float flFrametime );

private:
#ifdef GAME_DLL
	void SimulateFire( CMoveData &moveData, float flFrametime );

	float m_flFireDelay;
	bool m_bAlternatingWeapons;

	ISpacecraftAI *m_pAI;
	float m_flLastAIThinkTime;
#else
	CUtlVector< int > m_ThrusterAttachments;
	CUtlVector< int > m_ThrusterSounds;
	CUtlVector< int > m_EngineAttachments;

	CUtlVector< CSmartPtr< CNewParticleEffect > > m_ThrusterParticles;
	CUtlVector< CSmartPtr< CNewParticleEffect > > m_EngineParticles;

	CSmartPtr< CNewParticleEffect > m_SpaceFieldParticles;

	int m_iEngineLevelLast;
	int m_iProjectileParityLast;
	int m_iGUID_Engine;
	int m_iGUID_Boost;

	float m_flEngineVolume;
	float m_flShakeTimer;
#endif
	CUtlVector< int > m_WeaponAttachments;

	CNetworkQAngle( m_AngularImpulse );
	CNetworkVector( m_PhysVelocity );
	CNetworkVar( int, m_iEngineLevel );
	CNetworkVar( int, m_iProjectileParity );
};

#endif