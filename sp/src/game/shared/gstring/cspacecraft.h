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

class CEnvHoloSystem;
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

class ISpacecraftData
{
public:
	virtual ~ISpacecraftData() {}

	enum EngineLevel_e
	{
		ENGINELEVEL_STALLED = 0,
		ENGINELEVEL_IDLE,
		ENGINELEVEL_NORMAL,
		ENGINELEVEL_BOOST,
	};

	virtual int GetShield() const = 0;
	virtual int GetMaxShield() const = 0;
	virtual int GetHull() const = 0;
	virtual int GetMaxHull() const = 0;

	virtual CBaseEntity *GetEntity() = 0;

	virtual const QAngle &GetAngularImpulse() const = 0;
	virtual const Vector &GetPhysVelocity() const = 0;
	virtual EngineLevel_e GetEngineLevel() const = 0;

#ifdef CLIENT_DLL
	virtual int GetThrusterCount() const = 0;
	virtual float GetThrusterPower( int index ) const = 0;
#endif
};

class CSpacecraft : public CBaseAnimating, public ISpacecraftData
{
	DECLARE_CLASS( CSpacecraft, CBaseAnimating );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CSpacecraft();
	virtual ~CSpacecraft();

	bool IsPlayerControlled() const;

	// ISpacecraftData implementation
	virtual int GetShield() const { return m_iShield; }
	virtual int GetMaxShield() const { return m_iMaxShield; }
	virtual int GetHull() const { return GetHealth(); }
	virtual int GetMaxHull() const { return GetMaxHealth(); }
	virtual CBaseEntity *GetEntity() { return this; }
	virtual const QAngle &GetAngularImpulse() const { return m_AngularImpulse.Get(); }
	virtual const Vector &GetPhysVelocity() const { return m_PhysVelocity.Get(); }
	virtual EngineLevel_e GetEngineLevel() const { return ( EngineLevel_e )m_iEngineLevel.Get(); }
#ifdef CLIENT_DLL
	virtual int GetThrusterCount() const { return m_ThrusterAttachments.Count(); }
	virtual float GetThrusterPower( int index ) const { return m_flThrusterPower[ index ]; }
#endif

#ifdef GAME_DLL
	void SetAI( ISpacecraftAI *pSpacecraftAI );


	virtual int UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); } // GSTRING_INF

	virtual void Precache();
	virtual void Activate();
	void OnPlayerEntered( CGstringPlayer *pPlayer );

	void InputEnterVehicle( inputdata_t &inputdata );
	virtual void PhysicsSimulate();
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual bool WillSimulateGamePhysics() { return true; }
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

	CBaseEntity *GetEnemy() const;
	void SetEnemy( CBaseEntity *pEnemy );
	void InputSetEnemy( inputdata_t &inputdata );
	void InputClearEnemy( inputdata_t &inputdata );
#else
	virtual int GetHealth() const { return m_iHealth; }
	virtual int GetMaxHealth() const { return m_iMaxHealth; }

	//virtual bool IsTransparent() { return false; }
	virtual bool IsTwoPass() { return true; }
	virtual ShadowType_t ShadowCastType() { return SHADOWS_SIMPLE; }
	virtual bool ShouldReceiveProjectedTextures( int flags ) { return true; }
	virtual RenderGroup_t GetRenderGroup();
	virtual bool IsViewModel() const;
	//virtual int DrawModel( int flags );

	virtual void OnDataChanged( DataUpdateType_t t );
	virtual void UpdateOnRemove();
	virtual void ClientThink();

	void UpdateCrosshair( CHudCrosshair *pCrosshair );

	float GetEngineAlpha() const { return m_flEngineAlpha; }

	int m_iAttachmentGUI;
#endif

	virtual CStudioHdr *OnNewModel();
	virtual void SimulateMove( CMoveData &moveData, float flFrametime );

private:
#ifdef GAME_DLL
	void SimulateFire( CMoveData &moveData, float flFrametime );

	string_t m_strSettingsName;

	float m_flFireDelay;
	bool m_bAlternatingWeapons;

	ISpacecraftAI *m_pAI;

	string_t m_strInitialEnemy;
	EHANDLE m_hEnemy;

	int m_iAIControlled;
	int m_iAIAttackState;
	int m_iAITeam;
	string_t m_strPathStartName;
	EHANDLE m_hPathEntity;

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iMaxHealth );
	float m_flCollisionDamageProtection;
	float m_flShieldRegenerationTimer;
	float m_flShieldRegeneratedTimeStamp;
	float m_flHealthRegenerationTimer;
	float m_flHealthRegeneratedTimeStamp;
#else
	CUtlVector< int > m_ThrusterAttachments;
	CUtlVector< int > m_ThrusterSounds;
	CUtlVector< int > m_EngineAttachments;
	float *m_flThrusterPower;

	CUtlVector< CSmartPtr< CNewParticleEffect > > m_ThrusterParticles;
	CUtlVector< CSmartPtr< CNewParticleEffect > > m_EngineParticles;

	CSmartPtr< CNewParticleEffect > m_SpaceFieldParticles;

	int m_iMaxHealth;

	int m_iEngineLevelLast;
	int m_iProjectileParityLast;
	int m_iGUID_Engine;
	int m_iGUID_Boost;

	float m_flEngineAlpha;
	float m_flEngineVolume;
	float m_flShakeTimer;
#endif
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );

	CUtlVector< int > m_WeaponAttachments;

	CNetworkVar( int, m_iShield );
	CNetworkVar( int, m_iMaxShield );
	CNetworkQAngle( m_AngularImpulse );
	CNetworkVector( m_PhysVelocity );
	CNetworkVar( int, m_iEngineLevel );
	CNetworkVar( int, m_iProjectileParity );
	CNetworkVar( float, m_flMoveX );
	CNetworkVar( float, m_flMoveY );

	CNetworkVar( UtlSymId_t, m_iSettingsIndex );
	SpacecraftSettings_t m_Settings;

	CNetworkHandle( CEnvHoloSystem, m_hHoloSystem );
};


#endif