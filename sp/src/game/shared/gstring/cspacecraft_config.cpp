#include "cbase.h"
#include "filesystem.h"
#include "props_shared.h"
#include "cspacecraft_config.h"

#ifdef CLIENT_DLL
CON_COMMAND( gstring_spacecraft_reload_config, "" )
{
	CSpacecraftConfig::GetInstance()->ReloadConfig();
	engine->ServerCmd( "__reload_spacecraft_config" );
}
#endif

static void PrecacheSoundIfSet( const CUtlString &strSound )
{
	if ( !strSound.IsEmpty() )
	{
		CBaseEntity::PrecacheScriptSound( strSound );
	}
}

CSpacecraftConfig CSpacecraftConfig::m_Instance;

CSpacecraftConfig::CSpacecraftConfig()
{
}

bool CSpacecraftConfig::Init()
{
	ReloadConfig();

	return true;
}

void CSpacecraftConfig::LevelInitPreEntity()
{
	Precache();
}

void CSpacecraftConfig::Precache()
{
	if ( m_Settings.GetNumStrings() == 0 )
	{
		ReloadConfig();
	}

	for ( int i = 0; i < m_Settings.GetNumStrings(); i++ )
	{
		const SpacecraftSettings_t &settings = m_Settings[ i ];

		const int iModelIndex = CBaseEntity::PrecacheModel( settings.m_strModel );
		PrecacheGibsForModel( iModelIndex );

		PrecacheSoundIfSet( settings.m_strSoundEngineStart );
		PrecacheSoundIfSet( settings.m_strSoundEngineStop );
		PrecacheSoundIfSet( settings.m_strSoundBoostStart );
		PrecacheSoundIfSet( settings.m_strSoundBoostStop );
		PrecacheSoundIfSet( settings.m_strSoundThruster );
		PrecacheSoundIfSet( settings.m_strSoundDamage );
		PrecacheSoundIfSet( settings.m_strSoundDeath );
	}
}

void CSpacecraftConfig::ReloadConfig()
{
	m_Settings.Clear();

	KeyValues::AutoDelete pKV( new KeyValues( "" ) );

	if ( pKV->LoadFromFile( g_pFullFileSystem, "scripts/manifest_spacecraft.txt" ) )
	{
		for ( KeyValues *pChild = pKV->GetFirstTrueSubKey();
			pChild;
			pChild = pChild->GetNextTrueSubKey() )
		{
			const char *pszName = pChild->GetName();
			if ( !pszName || !*pszName )
			{
				continue;
			}

			SpacecraftSettings_t settings;
			settings.m_strModel = pChild->GetString( "model" );

			settings.m_strParticleDamage = pChild->GetString( "particle_damage" );
			settings.m_strParticleDeath = pChild->GetString( "particle_death" );
			settings.m_strParticleGib = pChild->GetString( "particle_gib" );

			for ( KeyValues *pValue = pChild->GetFirstValue();
				pValue;
				pValue = pValue->GetNextValue() )
			{
				const char *pszValueName = pValue->GetName();
				if ( FStrEq( pszValueName, "particle_gib_connect" ) )
				{
					const char *pszValue = pValue->GetString();
					if ( *pszValue )
					{
						settings.m_ParticleGibConnect.AddToTail( pszValue );
					}
				}
			}

			settings.m_strSoundEngineStart = pChild->GetString( "sound_engine_start" );
			settings.m_strSoundEngineStop = pChild->GetString( "sound_engine_stop" );
			settings.m_strSoundBoostStart = pChild->GetString( "sound_boost_start" );
			settings.m_strSoundBoostStop = pChild->GetString( "sound_boost_stop" );
			settings.m_strSoundThruster = pChild->GetString( "sound_thruster" );
			settings.m_strSoundDamage = pChild->GetString( "sound_damage" );
			settings.m_strSoundDeath = pChild->GetString( "sound_death" );

			settings.m_flAcceleration = pChild->GetFloat( "acceleration", settings.m_flAcceleration );
			settings.m_flAccelerationSide = pChild->GetFloat( "acceleration_side", settings.m_flAccelerationSide );
			settings.m_flAccelerationBoost = pChild->GetFloat( "acceleration_boost", settings.m_flAccelerationBoost );

			settings.m_iHealth = pChild->GetInt( "health", settings.m_iHealth );

			settings.m_flCollisionDamageScale = pChild->GetFloat( "collision_damage_scale", settings.m_flCollisionDamageScale );
			settings.m_flCollisionDamageMin = pChild->GetFloat( "collision_damage_min", settings.m_flCollisionDamageMin );
			settings.m_flCollisionDamageMax = pChild->GetFloat( "collision_damage_max", settings.m_flCollisionDamageMax );

			m_Settings[ pszName ] = settings;
		}
	}
}

UtlSymId_t CSpacecraftConfig::GetSettingsIndex( const char *pszSettingsName ) const
{
	return m_Settings.Find( pszSettingsName );
}

const SpacecraftSettings_t &CSpacecraftConfig::GetSettings( UtlSymId_t index ) const
{
	if ( index == m_Settings.InvalidIndex() )
	{
		static SpacecraftSettings_t invalidSettings;
		return invalidSettings;
	}
	return m_Settings[ index ];
}
