#ifndef C_SPACECRAFT_CONFIG_H
#define C_SPACECRAFT_CONFIG_H

#include "cbase.h"
#include "UtlStringMap.h"

struct SpacecraftProjectileSettings_t
{
	SpacecraftProjectileSettings_t()
		: m_flDamage( 5.0f )
		, m_flSpeed( 4000.0f )
		, m_flSpread( 0.008f )
	{
	}

	float m_flDamage;
	float m_flSpeed;
	float m_flSpread;
	
	CUtlString m_strParticleSpawn;
	CUtlString m_strParticleTrail;
	CUtlString m_strParticleHitWorld;
	CUtlString m_strParticleHitShip;
};

struct SpacecraftSettings_t
{
	SpacecraftSettings_t() :
		m_flAcceleration( 1.0f ),
		m_flAccelerationSide( 1.0f ),
		m_flAccelerationBoost( 1.0f ),
		m_iHealth( 100 ),
		m_iShield( 100 ),
		m_flShieldRegenerationDelay( 1.0f ),
		m_flShieldRegenerationRate( 1.0f ),
		m_flHealthRegenerationDelay( 1.0f ),
		m_flHealthRegenerationRate( 1.0f ),
		m_flCollisionDamageScale( 0.0f ),
		m_flCollisionDamageMin( 0.0f ),
		m_flCollisionDamageMax( FLT_MAX ),
		m_flFireRate( 0.1f ),
		m_iProjectileCount( 1 ),
		m_flFireDurationMin( 1.0f ),
		m_flFireDurationMax( 4.0f ),
		m_flIdleDurationMin( 1.0f ),
		m_flIdleDurationMax( 4.0f )
	{
	}

	CUtlString m_strModel;

	CUtlString m_strParticleDamage;
	CUtlString m_strParticleDeath;
	CUtlString m_strParticleGib;
	CUtlVector< CUtlString > m_ParticleGibConnect;

	CUtlString m_strSoundEngineStart;
	CUtlString m_strSoundEngineStop;
	CUtlString m_strSoundBoostStart;
	CUtlString m_strSoundBoostStop;
	CUtlString m_strSoundThruster;
	CUtlString m_strSoundDamage;
	CUtlString m_strSoundDeath;

	float m_flAcceleration;
	float m_flAccelerationSide;
	float m_flAccelerationBoost;

	int m_iHealth;
	int m_iShield;
	float m_flShieldRegenerationDelay;
	float m_flShieldRegenerationRate;
	float m_flHealthRegenerationDelay;
	float m_flHealthRegenerationRate;

	float m_flCollisionDamageScale;
	float m_flCollisionDamageMin;
	float m_flCollisionDamageMax;

	float m_flFireRate;
	int m_iProjectileCount;
	float m_flFireDurationMin;
	float m_flFireDurationMax;
	float m_flIdleDurationMin;
	float m_flIdleDurationMax;
	SpacecraftProjectileSettings_t m_ProjectileSettings;
};

class CSpacecraftConfig : public CAutoGameSystem
{
	static CSpacecraftConfig m_Instance;
public:

	CSpacecraftConfig();

	static CSpacecraftConfig *GetInstance() { return &m_Instance; }

	virtual void LevelInitPreEntity();
	void Precache();
	void ReloadConfig();

	UtlSymId_t GetSettingsIndex( const char *pszSettingsName ) const;
	const SpacecraftSettings_t &GetSettings( UtlSymId_t index ) const;
	const SpacecraftProjectileSettings_t &GetProjectileSettings( UtlSymId_t index ) const;

private:
	typedef CUtlStringMap< SpacecraftSettings_t > SpacecraftSettingsContrainer;
	SpacecraftSettingsContrainer m_Settings;

	bool m_bPrecached;
};

#endif
