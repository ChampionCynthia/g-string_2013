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

static void PrecacheParticleIfSet( const CUtlString &strParticle )
{
	if ( !strParticle.IsEmpty() )
	{
		PrecacheParticleSystem( strParticle );
	}
}

CSpacecraftConfig CSpacecraftConfig::m_Instance;

CSpacecraftConfig::CSpacecraftConfig() :
	m_bPrecached( false )
{
}

void CSpacecraftConfig::LevelInitPreEntity()
{
	m_bPrecached = false;
}

void CSpacecraftConfig::Precache()
{
	if ( m_Settings.GetNumStrings() == 0 )
	{
		ReloadConfig();
	}

	if ( !m_bPrecached )
	{
		m_bPrecached = true;
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

			PrecacheParticleIfSet( settings.m_strParticleDamage );
			PrecacheParticleIfSet( settings.m_strParticleDeath );
			PrecacheParticleIfSet( settings.m_strParticleGib );

			FOR_EACH_VEC( settings.m_ParticleGibConnect, p )
			{
				PrecacheParticleSystem( settings.m_ParticleGibConnect[ p ] );
			}

			PrecacheParticleIfSet( settings.m_ProjectileSettings.m_strParticleSpawn );
			PrecacheParticleIfSet( settings.m_ProjectileSettings.m_strParticleTrail );
			PrecacheParticleIfSet( settings.m_ProjectileSettings.m_strParticleHitShip );
			PrecacheParticleIfSet( settings.m_ProjectileSettings.m_strParticleHitWorld );
		}

		PrecacheParticleSystem( "spacefield" );

		PrecacheParticleSystem( "thruster_small" );
		PrecacheParticleSystem( "idle_loop" );
		PrecacheParticleSystem( "move_loop" );
		PrecacheParticleSystem( "boost_loop" );
	}
}

void CSpacecraftConfig::ReloadConfig()
{
	m_bPrecached = false;
	m_Settings.Clear();

	KeyValues::AutoDelete pKV( new KeyValues( "" ) );

	if ( pKV->LoadFromFile( g_pFullFileSystem, "scripts/manifest_spacecraft.txt" ) )
	{
		KeyValues *pKVProjectiles = pKV->FindKey( "projectiles" );
		KeyValues *pKVSpacecrafts = pKV->FindKey( "spacecrafts" );

		typedef CUtlStringMap< SpacecraftProjectileSettings_t > SpacecraftProjectileSettingsContrainer;
		SpacecraftProjectileSettingsContrainer projectileSettings;

		if ( pKVProjectiles != NULL )
		{
			for ( KeyValues *pChild = pKVProjectiles->GetFirstTrueSubKey();
				pChild;
				pChild = pChild->GetNextTrueSubKey() )
			{
				const char *pszName = pChild->GetName();
				if ( !pszName || !*pszName )
				{
					continue;
				}

				SpacecraftProjectileSettings_t settings;
				settings.m_flDamage = pChild->GetFloat( "damage", settings.m_flDamage );
				settings.m_flSpeed = pChild->GetFloat( "speed", settings.m_flSpread );
				settings.m_flSpread = pChild->GetFloat( "spread", settings.m_flSpread );
				
				settings.m_strParticleSpawn = pChild->GetString( "particle_spawn" );
				settings.m_strParticleTrail = pChild->GetString( "particle_trail" );
				settings.m_strParticleHitWorld = pChild->GetString( "particle_hitworld" );
				settings.m_strParticleHitShip = pChild->GetString( "particle_hitship" );

				projectileSettings[ pszName ] = settings;
			}
		}

		if ( pKVSpacecrafts != NULL )
		{
			for ( KeyValues *pChild = pKVSpacecrafts->GetFirstTrueSubKey();
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
				settings.m_iShield = pChild->GetInt( "shield", settings.m_iShield );
				settings.m_flShieldRegenerationDelay = pChild->GetFloat( "shield_regeneration_delay", settings.m_flShieldRegenerationDelay );
				settings.m_flShieldRegenerationRate = pChild->GetFloat( "shield_regeneration_rate", settings.m_flShieldRegenerationRate );
				settings.m_flHealthRegenerationDelay = pChild->GetFloat( "health_regeneration_delay", settings.m_flHealthRegenerationDelay );
				settings.m_flHealthRegenerationRate = pChild->GetFloat( "health_regeneration_rate", settings.m_flHealthRegenerationRate );

				settings.m_flCollisionDamageScale = pChild->GetFloat( "collision_damage_scale", settings.m_flCollisionDamageScale );
				settings.m_flCollisionDamageMin = pChild->GetFloat( "collision_damage_min", settings.m_flCollisionDamageMin );
				settings.m_flCollisionDamageMax = pChild->GetFloat( "collision_damage_max", settings.m_flCollisionDamageMax );

				settings.m_flFireRate = pChild->GetFloat( "firerate", settings.m_flFireRate );
				settings.m_iProjectileCount = pChild->GetInt( "projectilecount", settings.m_iProjectileCount );
				settings.m_flFireDurationMin = pChild->GetFloat( "fire_duration_min", settings.m_flFireDurationMin );
				settings.m_flFireDurationMax = pChild->GetFloat( "fire_duration_max", settings.m_flFireDurationMax );
				settings.m_flIdleDurationMin = pChild->GetFloat( "idle_duration_min", settings.m_flIdleDurationMin );
				settings.m_flIdleDurationMax = pChild->GetFloat( "idle_duration_max", settings.m_flIdleDurationMax );

				UtlSymId_t projectileIndex = projectileSettings.Find( pChild->GetString( "projectile" ) );
				if ( projectileIndex != projectileSettings.InvalidIndex() )
				{
					settings.m_ProjectileSettings = projectileSettings[ projectileIndex ];
				}

				m_Settings[ pszName ] = settings;
			}
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

const SpacecraftProjectileSettings_t &CSpacecraftConfig::GetProjectileSettings( UtlSymId_t index ) const
{
	if ( index == m_Settings.InvalidIndex() )
	{
		static SpacecraftProjectileSettings_t invalidSettings;
		return invalidSettings;
	}
	return m_Settings[ index ].m_ProjectileSettings;
}
