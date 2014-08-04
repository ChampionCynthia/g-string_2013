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
#include "cspacecraft_config.h"

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
	enum EngineLevel_e
	{
		ENGINELEVEL_STALLED = 0,
		ENGINELEVEL_IDLE,
		ENGINELEVEL_NORMAL,
		ENGINELEVEL_BOOST,
	};

	CSpacecraft();
	virtual ~CSpacecraft();

	bool IsPlayerControlled();

#ifdef GAME_DLL
	void SetAI( ISpacecraftAI *pSpacecraftAI );

	virtual void Activate();

	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );

	void InputEnterVehicle( inputdata_t &inputdata );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

	CBaseEntity *GetEnemy() const;
	void SetEnemy( CBaseEntity *pEnemy );
	void InputSetEnemy( inputdata_t &inputdata );
	void InputClearEnemy( inputdata_t &inputdata );
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

	float GetEngineAlpha() const { return m_flEngineAlpha; }
#endif

	virtual CStudioHdr *OnNewModel();
	virtual void SimulateMove( CMoveData &moveData, float flFrametime );

	const Vector &GetPhysVelocity() const;
	EngineLevel_e GetEngineLevel() const;

private:
#ifdef GAME_DLL
	void SimulateFire( CMoveData &moveData, float flFrametime );

	string_t m_strSettingsName;

	float m_flFireDelay;
	bool m_bAlternatingWeapons;

	ISpacecraftAI *m_pAI;
	float m_flLastAIThinkTime;

	string_t m_strInitialEnemy;
	EHANDLE m_hEnemy;

	int m_iAIControlled;
	int m_iAIAttackState;
	int m_iAITeam;
	string_t m_strPathStartName;
	EHANDLE m_hPathEntity;
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

	float m_flEngineAlpha;
	float m_flEngineVolume;
	float m_flShakeTimer;
#endif
	CUtlVector< int > m_WeaponAttachments;

	CNetworkQAngle( m_AngularImpulse );
	CNetworkVector( m_PhysVelocity );
	CNetworkVar( int, m_iEngineLevel );
	CNetworkVar( int, m_iProjectileParity );

	CNetworkVar( UtlSymId_t, m_iSettingsIndex );
	SpacecraftSettings_t m_Settings;
};

#endif