#ifndef C_SPACECRAFT_CONFIG_H
#define C_SPACECRAFT_CONFIG_H

#include "cbase.h"
#include "UtlStringMap.h"

struct SpacecraftSettings_t
{
	SpacecraftSettings_t() :
		m_flAcceleration( 1.0f ),
		m_flAccelerationSide( 1.0f ),
		m_flAccelerationBoost( 1.0f ),
		m_iHealth( 100 ),
		m_flCollisionDamageScale( 0.0f ),
		m_flCollisionDamageMin( 0.0f ),
		m_flCollisionDamageMax( FLT_MAX )
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

	float m_flCollisionDamageScale;
	float m_flCollisionDamageMin;
	float m_flCollisionDamageMax;
};

class CSpacecraftConfig : public CAutoGameSystem
{
	static CSpacecraftConfig m_Instance;
public:

	CSpacecraftConfig();

	virtual bool Init();
	virtual void LevelInitPreEntity();

	static CSpacecraftConfig *GetInstance() { return &m_Instance; }

	void Precache();
	void ReloadConfig();

	UtlSymId_t GetSettingsIndex( const char *pszSettingsName ) const;
	const SpacecraftSettings_t &GetSettings( UtlSymId_t index ) const;

private:
	typedef CUtlStringMap< SpacecraftSettings_t > SpacecraftSettingsContrainer;
	SpacecraftSettingsContrainer m_Settings;
};

#endif
