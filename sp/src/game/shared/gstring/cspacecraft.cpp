
#include "cbase.h"
#include "in_buttons.h"
#include "gstring/cspacecraft.h"
#include "gstring/gstring_player_shared.h"
#include "gstring/gstring_util.h"
#include "gstring/hologui/env_holo_system.h"
#include "props_shared.h"
#include "particle_parse.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
#include "view_scene.h"
#include "hud_crosshair.h"
#include "engine/IEngineSound.h"
#include "ivieweffects.h"
#include "gstring_cvars.h"
#else
#include "cspacecraftprojectile.h"
#include "cspacecraft_ai.h"
#include "datacache/imdlcache.h"
#include "cgstring_globals.h"
#endif

#include "igamemovement.h"

template< typename V >
V ConvertSourceToPhysics( const V &v )
{
	return V( v.z, v.x, v.y );
}

template< typename V >
V ConvertPhysicsToSource( const V &v )
{
	return V( v.y, v.z, v.x );
}

inline void EmitSoundIfSet( CBaseEntity *pEntity, const CUtlString &strSound )
{
	if ( !strSound.IsEmpty() )
	{
		pEntity->EmitSound( strSound );
	}
}

#ifdef GAME_DLL

CON_COMMAND( kill_spacecraft, "" )
{
	CGstringPlayer *pPlayer = ToGstringPlayer( UTIL_GetCommandClient() );
	if ( pPlayer != NULL && pPlayer->IsInSpacecraft() )
	{
		CSpacecraft *pSpacecraft = pPlayer->GetSpacecraft();

		Vector vecVelocity;
		pSpacecraft->VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

		if ( vecVelocity.IsZero() )
		{
			vecVelocity = RandomVector( -10.0f, 10.0f );
		}

		CTakeDamageInfo info( NULL, NULL, NULL, vecVelocity, pSpacecraft->GetAbsOrigin(), 9999.0f, DMG_BLAST );
		pSpacecraft->OnTakeDamage( info );
	}
}

#endif

#define ENGINE_VOLUME_LOW 0.4f
#define ENGINE_VOLUME_HIGH 1.0f

static ConVar gstring_spacecraft_physics_drag( "gstring_spacecraft_physics_drag", "10.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_physics_angulardrag( "gstring_spacecraft_physics_angulardrag", "5000.0", FCVAR_REPLICATED );

static ConVar gstring_spacecraft_move_ang_approach_speed( "gstring_spacecraft_move_ang_approach_speed", "3.5", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_side( "gstring_spacecraft_move_drag_side", "2.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_fwd( "gstring_spacecraft_move_drag_fwd", "1.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_up( "gstring_spacecraft_move_drag_up", "2.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_boost_duration( "gstring_spacecraft_boost_duration", "10.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_boost_cooldown( "gstring_spacecraft_boost_cooldown", "5.0", FCVAR_REPLICATED );
ConVar gstring_spacecraft_move_mode( "gstring_spacecraft_move_mode", "0", FCVAR_REPLICATED );
ConVar gstring_space_exterior_sounds( "gstring_space_exterior_sounds", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE );

#ifdef GAME_DLL
BEGIN_DATADESC( CSpacecraft )

	DEFINE_FIELD( m_iShield, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMaxShield, FIELD_INTEGER ),
	DEFINE_FIELD( m_hHoloSystem, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_strSettingsName, FIELD_STRING, "settingsname" ),
	DEFINE_KEYFIELD( m_strShipName, FIELD_STRING, "shipname" ),

	// AI
	DEFINE_KEYFIELD( m_iAIControlled, FIELD_INTEGER, "aicontrolled" ),
	DEFINE_KEYFIELD( m_iAIState, FIELD_INTEGER, "aistate" ),
	DEFINE_KEYFIELD( m_iAITeam, FIELD_INTEGER, "aiteam" ),
	DEFINE_KEYFIELD( m_flSpeedMultiplier, FIELD_FLOAT, "aispeedmultiplier" ),

	DEFINE_KEYFIELD( m_strInitialEnemy, FIELD_STRING, "initialenemy" ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetAIState", InputSetAIState ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetEnemy", InputSetEnemy ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearEnemy", InputClearEnemy ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPath", InputSetPathEntity ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeedMultiplier", InputSetSpeedMultiplier ),
	DEFINE_FIELD( m_bInvincible, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsFrozen, FIELD_BOOLEAN ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFrozen", InputSetFrozen ),

	DEFINE_FIELD( m_hPathEntity, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_strPathStartName, FIELD_STRING, "pathstartname" ),

	// Player
	DEFINE_INPUTFUNC( FIELD_VOID, "EnterVehicle", InputEnterVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ExitVehicle", InputExitVehicle ),

	// Events
	DEFINE_OUTPUT( m_OnKilled, "OnKilled" ),
	DEFINE_OUTPUT( m_OnPlayerTeamAttack, "OnPlayerTeamAttack" ),
	DEFINE_OUTPUT( m_OnPlayerTeamAttackRetaliation, "OnPlayerTeamAttackRetaliation" ),

END_DATADESC()
#endif

IMPLEMENT_NETWORKCLASS_DT( CSpacecraft, CSpacecraft_DT )

#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_iHealth ), -1, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iMaxHealth ) ),
	SendPropInt( SENDINFO( m_iShield ), 16, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iMaxShield ), 16 ),

	SendPropInt( SENDINFO( m_iEngineLevel ), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iProjectileParity ), 8, SPROP_UNSIGNED ),

	SendPropVector( SENDINFO( m_AngularImpulse ) ),
	SendPropVector( SENDINFO( m_PhysVelocity ) ),
	SendPropInt( SENDINFO( m_iSettingsIndex ) ),

	SendPropFloat( SENDINFO( m_flMoveX ) ),
	SendPropFloat( SENDINFO( m_flMoveY ) ),
	SendPropBool( SENDINFO( m_bBoostSuspended ) ),
	SendPropInt( SENDINFO( m_iAITeam ), CSpacecraft::AITEAM_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hHoloSystem ) ),
	SendPropString( SENDINFO( m_szShipName ) )
#else
	RecvPropInt( RECVINFO( m_iHealth ) ),
	RecvPropInt( RECVINFO( m_iMaxHealth ) ),
	RecvPropInt( RECVINFO( m_iShield ) ),
	RecvPropInt( RECVINFO( m_iMaxShield ) ),

	RecvPropInt( RECVINFO( m_iEngineLevel ) ),
	RecvPropInt( RECVINFO( m_iProjectileParity ) ),

	RecvPropVector( RECVINFO( m_AngularImpulse ) ),
	RecvPropVector( RECVINFO( m_PhysVelocity ) ),
	RecvPropInt( RECVINFO( m_iSettingsIndex ) ),

	RecvPropFloat( RECVINFO( m_flMoveX ) ),
	RecvPropFloat( RECVINFO( m_flMoveY ) ),
	RecvPropBool( RECVINFO( m_bBoostSuspended ) ),
	RecvPropInt( RECVINFO( m_iAITeam ) ),
	RecvPropEHandle( RECVINFO( m_hHoloSystem ) ),
	RecvPropString( RECVINFO( m_szShipName ) )
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( prop_vehicle_spacecraft, CSpacecraft );

CSpacecraft::CSpacecraft()
#ifdef GAME_DLL
	: m_flFireDelay( 0.0f )
	, m_iNextWeaponIndex( 0 )
	, m_pAI( NULL )
	, m_bHasExternalHoloSystem( false )
	, m_iAIControlled( 0 )
	, m_iAIState( 0 )
	, m_flSpeedMultiplier( 1.0f )
	, m_bInvincible( false )
	, m_bIsFrozen( false )
	, m_flCollisionDamageProtection( 0.0f )
	, m_flShieldRegenerationTimer( 0.0f )
	, m_flShieldRegeneratedTimeStamp( 0.0f )
	, m_flHealthRegenerationTimer( 0.0f )
	, m_flHealthRegeneratedTimeStamp( 0.0f )
	, m_flPlayerTeamAttackCooldown( 0.0f )
	, m_flPlayerTeamAttackToleration( 0.0f )
	, m_bShouldTeamkillPlayer( false )
#else
	: m_iMaxHealth( 0 )
	, m_iEngineLevelLast( ENGINELEVEL_STALLED )
	, m_iProjectileParityLast( 0 )
	, m_iGUID_Engine( -1 )
	, m_iGUID_Boost( -1 )
	, m_flEngineAlpha( 0.0f )
	, m_flEngineVolume( ENGINE_VOLUME_LOW )
	, m_flShakeTimer( 0.0f )
	, m_flThrusterPower( NULL )
	, m_iAttachmentGUI( 0 )
#endif
	, m_flBoostUsage( 0.0f )
{
	m_iAITeam = AITEAM_MARTIAN;
	m_iSettingsIndex = UTL_INVAL_SYMBOL;
}

CSpacecraft::~CSpacecraft()
{
#ifdef GAME_DLL
	if ( !m_bHasExternalHoloSystem && m_hHoloSystem.Get() != NULL )
	{
		m_hHoloSystem->SetThink( &CBaseEntity::SUB_Remove );
		m_hHoloSystem->SetNextThink( gpGlobals->curtime + 0.1f );
	}
	delete m_pAI;
#else
	RemoveHoloTarget( this );
	if ( m_hHoloSystem.Get() != NULL )
	{
		m_hHoloSystem->DestroyPanels();
	}
	delete [] m_flThrusterPower;

	if ( m_iGUID_Engine >= 0 && enginesound->IsSoundStillPlaying( m_iGUID_Engine ) )
	{
		enginesound->StopSoundByGuid( m_iGUID_Engine );
	}

	if ( m_iGUID_Boost >= 0 && enginesound->IsSoundStillPlaying( m_iGUID_Boost ) )
	{
		enginesound->StopSoundByGuid( m_iGUID_Boost );
	}
#endif
}

#ifdef CLIENT_DLL
const char *CSpacecraft::GetName() const
{
	if ( *m_szShipName.Get() )
	{
		return m_szShipName.Get();
	}

	return GetType() == IHoloTarget::ENEMY ? "holo_gui_generic_enemy" : "holo_gui_generic_friend";
}

float CSpacecraft::GetSize() const
{
	return 1.0f;
}

float CSpacecraft::GetHealthPercentage() const
{
	return ( GetHull() + GetShield() ) / float( GetMaxHull() + GetMaxShield() );
}

IHoloTarget::TargetType CSpacecraft::GetType() const
{
	return (m_iAITeam == AITEAM_NATO) ? IHoloTarget::ENEMY : IHoloTarget::FRIENDLY;
}

float CSpacecraft::GetMaxDistance() const
{
	return 2048.0f;
}

bool CSpacecraft::IsActive() const
{
	return !IsPlayerControlled();
}

const C_BaseEntity *CSpacecraft::GetEntity() const
{
	return this;
}
#endif

bool CSpacecraft::IsPlayerControlled() const
{
	return GetOwnerEntity() && GetOwnerEntity()->IsPlayer();
}

bool CSpacecraft::ShouldPlaySounds() const
{
	return IsPlayerControlled() || gstring_space_exterior_sounds.GetBool();
}

#ifdef GAME_DLL
void CSpacecraft::SetAI( ISpacecraftAI *pSpacecraftAI )
{
	delete m_pAI;
	m_pAI = pSpacecraftAI;
}

void CSpacecraft::Precache()
{
	BaseClass::Precache();

	CSpacecraftConfig::GetInstance()->Precache();

	PrecacheScriptSound( "Spacecraft.Boost.Deny" );
}

void CSpacecraft::Spawn()
{
	BaseClass::Spawn();

	m_bInvincible = HasSpawnFlags( SPACECRAFT_SPAWNFLAG_INVINCIBLE );

	m_hPathEntity = dynamic_cast<CPathTrack*>(gEntList.FindEntityByName( NULL, m_strPathStartName, this ));
	m_hEnemy = gEntList.FindEntityByName( NULL, m_strInitialEnemy, this );
}

void CSpacecraft::Activate()
{
	Precache();

	if ( m_iAIControlled == 1 )
	{
		CSpacecraftAIBase *pAI = new CSpacecraftAIBase( this );
		SetAI( pAI );
	}

	if ( m_pAI != NULL )
	{
		m_pAI->EnterState((ISpacecraftAI::AISTATE_e) m_iAIState);
	}

	if ( STRING( m_strShipName ) )
	{
		Q_strncpy( m_szShipName.GetForModify(), STRING( m_strShipName ), SHIPNAME_MAX_LENGTH );
	}

	const CSpacecraftConfig *pConfig = CSpacecraftConfig::GetInstance();
	m_iSettingsIndex = pConfig->GetSettingsIndex( STRING( m_strSettingsName ) );
	m_Settings = pConfig->GetSettings( m_iSettingsIndex );

	SetModel( m_Settings.m_strModel );

	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );

	if ( g_pGstringGlobals != NULL )
	{
		SetModelScale( g_pGstringGlobals->GetWorldScale() );
	}

	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );

	BaseClass::Activate();

	VPhysicsGetObject()->Wake();
	VPhysicsGetObject()->EnableGravity( false );
	VPhysicsGetObject()->EnableDrag( true );

	m_takedamage = DAMAGE_YES;
	SetMaxHealth( m_Settings.m_iHealth );
	SetHealth( GetMaxHealth() );

	m_iMaxShield = m_Settings.m_iShield;
	m_iShield = m_iMaxShield;

	// Not called when restoring...
	MDLCACHE_CRITICAL_SECTION();
	OnNewModel();

	// Restore controlling player
	if ( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() )
	{
		inputdata_t d;
		InputEnterVehicle( d );
	}
}

void CSpacecraft::OnPlayerEntered( CGstringPlayer *pPlayer )
{
	m_iAIControlled = 0;
	SetAI( NULL );

	SetOwnerEntity( pPlayer );

	if ( !m_hHoloSystem )
	{
		CEnvHoloSystem *pHoloSystem = assert_cast< CEnvHoloSystem* >( CreateEntityByName( "env_holo_system" ) );
		Assert( pHoloSystem );
		pHoloSystem->SetOwnerEntity( this );
		pHoloSystem->KeyValue( "Attachment", "gui" );
		pHoloSystem->KeyValue( "spawnflags", "1" ); // Start enabled
		DispatchSpawn( pHoloSystem );
		pHoloSystem->Activate();
		m_hHoloSystem = pHoloSystem;
	}
	else
	{
		m_hHoloSystem->SetEnabled( true );
	}
}

void CSpacecraft::InputEnterVehicle( inputdata_t &inputdata )
{
	CGstringPlayer *pPlayer = LocalGstringPlayer();
	SetPlayerSimulated( pPlayer );
	pPlayer->EnterSpacecraft( this );
}

void CSpacecraft::InputExitVehicle( inputdata_t &inputdata )
{
	CGstringPlayer *pPlayer = LocalGstringPlayer();
	SetPlayerSimulated( pPlayer );
	pPlayer->ExitSpacecraft();
}

void CSpacecraft::PhysicsSimulate()
{
	BaseClass::PhysicsSimulate();

	if ( gpGlobals->frametime > 0.0f )
	{
		if ( m_pAI != NULL )
		{
			m_pAI->Run( gpGlobals->frametime );
		}

		if ( GetShield() < GetMaxShield() && m_Settings.m_flShieldRegenerationRate > 0 )
		{
			if ( m_flShieldRegenerationTimer > 0.0f )
			{
				m_flShieldRegenerationTimer -= gpGlobals->frametime;
				m_flShieldRegeneratedTimeStamp = gpGlobals->curtime;
			}
			else if ( m_flShieldRegeneratedTimeStamp < gpGlobals->curtime )
			{
				int iAddedShield = ( gpGlobals->curtime - m_flShieldRegeneratedTimeStamp ) * m_Settings.m_flShieldRegenerationRate;
				if ( iAddedShield > 0 )
				{
					m_flShieldRegeneratedTimeStamp = gpGlobals->curtime;
					//TakeHealth( iAddedHealth, DMG_GENERIC );
					iAddedShield = MIN( iAddedShield, GetMaxShield() - GetShield() );
					m_iShield += iAddedShield;
				}
			}
		}

		if ( GetHealth() < GetMaxHealth() && m_Settings.m_flHealthRegenerationRate > 0 )
		{
			if ( m_flHealthRegenerationTimer > 0.0f )
			{
				m_flHealthRegenerationTimer -= gpGlobals->frametime;
				m_flHealthRegeneratedTimeStamp = gpGlobals->curtime;
			}
			else if ( m_flHealthRegeneratedTimeStamp < gpGlobals->curtime )
			{
				int iAddedHealth = ( gpGlobals->curtime - m_flHealthRegeneratedTimeStamp ) * m_Settings.m_flHealthRegenerationRate;
				if ( iAddedHealth > 0 )
				{
					m_flHealthRegeneratedTimeStamp = gpGlobals->curtime;
					TakeHealth( iAddedHealth, DMG_GENERIC );
				}
			}
		}
	}
}

void CSpacecraft::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	if ( m_flCollisionDamageProtection > gpGlobals->curtime )
	{
		return;
	}

	const int iSelfIndex = ( pEvent->pObjects[ 0 ] == VPhysicsGetObject() ) ? 0 : 1;
	float flDamage = ( pEvent->preVelocity[ iSelfIndex ] - pEvent->postVelocity[ iSelfIndex ] ).LengthSqr() *
		m_Settings.m_flCollisionDamageScale;

	if ( flDamage >= m_Settings.m_flCollisionDamageMin )
	{
		m_flCollisionDamageProtection = gpGlobals->curtime + 0.5f;

		flDamage = MIN( m_Settings.m_flCollisionDamageMax, flDamage );
		Vector vecContactPoint, vecSurfaceNormal;
		pEvent->pInternalData->GetContactPoint( vecContactPoint );
		pEvent->pInternalData->GetSurfaceNormal( vecSurfaceNormal );

		if ( iSelfIndex == 0 )
		{
			vecSurfaceNormal = -vecSurfaceNormal;
		}

		CTakeDamageInfo info( this, this, flDamage, DMG_DIRECT );
		info.SetDamageForce( pEvent->preVelocity[ iSelfIndex ] );
		info.SetDamagePosition( vecContactPoint + vecSurfaceNormal * 3.0f );
		// TakeDamage( info );
		PhysCallbackDamage( this, info );
	}
}

void CSpacecraft::VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit )
{
	if ( ShouldPlaySounds() )
	{
		BaseClass::VPhysicsFriction(pObject, energy, surfaceProps, surfacePropsHit);
	}
}

int CSpacecraft::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newDamage( info );

	if ( GetShield() > 0.0f )
	{
		float flAbsorbed = ceil( newDamage.GetDamage() );
		flAbsorbed = MIN( GetShield(), flAbsorbed );

		newDamage.SetDamage( MAX( 0.0f, newDamage.GetDamage() - flAbsorbed ) );
		m_iShield = MAX( 0, GetShield() - flAbsorbed );
	}

	const int ret = IsInvincible() ? 0 : BaseClass::OnTakeDamage( newDamage );

	DispatchParticleEffect( m_Settings.m_strParticleDamage, newDamage.GetDamagePosition(), GetAbsAngles() );
	if (ShouldPlaySounds())
	{
		EmitSoundIfSet( this, m_Settings.m_strSoundDamage );
	}

	m_flShieldRegenerationTimer = m_Settings.m_flShieldRegenerationDelay;
	m_flShieldRegeneratedTimeStamp = gpGlobals->curtime;
	m_flHealthRegenerationTimer = m_Settings.m_flHealthRegenerationDelay;
	m_flHealthRegeneratedTimeStamp = gpGlobals->curtime;

	if ( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), 1.5f, 7.0f, 0.8f, 128.0f, SHAKE_START_NORUMBLE, true );
		UTIL_ScreenShake( GetAbsOrigin(), 20.0f, 5.0f, 0.8f, 128.0f, SHAKE_START_RUMBLEONLY, true );

		CSingleUserRecipientFilter user( ToGstringPlayer( GetOwnerEntity() ) );
		user.MakeReliable();
		UserMessageBegin( user, "SpacecraftDamage" );
			WRITE_VEC3COORD( newDamage.GetDamagePosition() );
		MessageEnd();
	}

	// Check for friendly fire
	CBaseEntity *pAttacker = info.GetAttacker();
	if ( pAttacker != NULL &&
		pAttacker->IsPlayer() &&
		GetTeam() == AITEAM_MARTIAN )
	{
		OnPlayerTeamAttack(info);
	}
	return ret;
}

void CSpacecraft::Event_Killed( const CTakeDamageInfo &info )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
	if ( pPlayer != NULL )
	{
		CTakeDamageInfo playerDamage( info );
		playerDamage.SetAttacker( pPlayer );
		playerDamage.SetInflictor( pPlayer );
		playerDamage.SetDamage( pPlayer->GetHealth() * 10 );
		playerDamage.SetDamageType( DMG_GENERIC | DMG_VEHICLE | DMG_REMOVENORAGDOLL );
		pPlayer->TakeDamage( playerDamage );

		pPlayer->SetMoveType( MOVETYPE_NONE );
		pPlayer->SetAbsVelocity( vec3_origin );
	}

	float flRumbleScale = 1.0f;

	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( pPhysics != NULL )
	{
		AngularImpulse angVelocity;
		pPhysics->GetVelocity( NULL, &angVelocity );

		breakablepropparams_t params( GetAbsOrigin(), GetAbsAngles(), info.GetDamageForce(), angVelocity * 1.5f );
		params.impactEnergyScale = 1.0f;
		params.defCollisionGroup = COLLISION_GROUP_INTERACTIVE;
		params.defBurstScale = 1.0f;
		params.randomAngularVelocity = RandomFloat( 500.0f, 600.0f );
		params.pszGibParticleSystemName = m_Settings.m_strParticleGib;
		FOR_EACH_VEC( m_Settings.m_ParticleGibConnect, i )
		{
			params.connectingParticleNames.AddToTail( m_Settings.m_ParticleGibConnect[ i ] );
		}
		params.particleChance = 0.5f;
		params.velocityScale = info.GetDamageForce().Length() / 100.0f;
		params.velocityScale = MIN( 1.0f, params.velocityScale );
		params.velocityScale = powf( params.velocityScale, 1.2f );

		const int iDamageType = info.GetDamageType();
		if ( ( iDamageType & DMG_DIRECT ) != 0 )
		{
			params.burstScale = 0.0f;
			flRumbleScale = 6.0f;
		}
		else if ( ( iDamageType & DMG_BLAST ) != 0 )
		{
			params.burstScale = 2.5f;
		}

		params.fadeTimeOverride = ( pPlayer != NULL ) ? 5.0f : 0.5f;

		MDLCACHE_CRITICAL_SECTION();
		PropBreakableCreateAll( GetModelIndex(), pPhysics, params, this, -1, false, true );
	}

	if ( ShouldPlaySounds() )
	{
		EmitSoundIfSet( this, m_Settings.m_strSoundDeath );
	}

	DispatchParticleEffect( m_Settings.m_strParticleDeath, GetAbsOrigin(), GetAbsAngles() );
	//const int iRingCount = RandomInt( 2, 4 );
	//for ( int i = 0; i < iRingCount; i++ )
	//{
	//	QAngle angles( RandomFloat( 0.0f, 90.0f ), RandomFloat( 0, 180.0f ), 0.0f );
	//	DispatchParticleEffect( "ship_explosion", GetAbsOrigin(), angles );
	//}

	BaseClass::Event_Killed( info );

	AddSolidFlags( FSOLID_NOT_SOLID );
	VPhysicsDestroyObject();

	UTIL_ScreenShake( GetAbsOrigin(), 8.0f, 8.0f, 1.2f, 96.0f, SHAKE_START_NORUMBLE, true );
	UTIL_ScreenShake( GetAbsOrigin(), 40.0f * flRumbleScale, 5.0f * flRumbleScale, 1.2f,
		96.0f * flRumbleScale, SHAKE_START_RUMBLEONLY, true );

	m_OnKilled.FireOutput(info.GetAttacker(), this);
}

void CSpacecraft::RegisterHoloSystem( CEnvHoloSystem *pHoloSystem )
{
	m_hHoloSystem.Set( pHoloSystem );
	m_bHasExternalHoloSystem = true;
}

void CSpacecraft::InputSetAIState(inputdata_t &inputdata)
{
	if (m_pAI != NULL && (inputdata.value.FieldType() == FIELD_INTEGER || inputdata.value.Convert(FIELD_INTEGER)))
	{
		m_iAIState = inputdata.value.Int();
		m_pAI->EnterState((ISpacecraftAI::AISTATE_e)m_iAIState);
	}
}

CBaseEntity *CSpacecraft::GetEnemy() const
{
	return m_hEnemy;
}

void CSpacecraft::SetEnemy( CBaseEntity *pEnemy )
{
	m_hEnemy.Set( pEnemy );
}

void CSpacecraft::InputSetEnemy( inputdata_t &inputdata )
{
	if ( inputdata.value.Convert( FIELD_EHANDLE ) )
	{
		SetEnemy( inputdata.value.Entity() );
	}
}

void CSpacecraft::InputClearEnemy( inputdata_t &inputdata )
{
	SetEnemy( NULL );
}

CPathTrack *CSpacecraft::GetPathEntity() const
{
	return m_hPathEntity;
}

void CSpacecraft::SetPathEntity(CPathTrack *pPathEntity)
{
	m_hPathEntity.Set( pPathEntity );
}

void CSpacecraft::InputSetPathEntity(inputdata_t &inputdata)
{
	if (inputdata.value.FieldType() == FIELD_STRING)
	{
		SetPathEntity(dynamic_cast<CPathTrack*>(gEntList.FindEntityByName( NULL, inputdata.value.String(), this )));
	}
}

void CSpacecraft::InputSetSpeedMultiplier(inputdata_t &inputdata)
{
	if (inputdata.value.FieldType() == FIELD_FLOAT || inputdata.value.Convert(FIELD_FLOAT))
	{
		SetSpeedMultiplier(inputdata.value.Float());
	}
}

void CSpacecraft::InputSetFrozen( inputdata_t &inputdata )
{
	if (inputdata.value.FieldType() == FIELD_INTEGER || inputdata.value.Convert(FIELD_INTEGER))
	{
		m_bIsFrozen = inputdata.value.Int() != 0;
	}
}

void CSpacecraft::SimulateFire( CMoveData &moveData, float flFrametime )
{
	if ( m_WeaponAttachments.Count() < 1 )
	{
		return;
	}

	if ( m_flFireDelay > 0.0f )
	{
		m_flFireDelay -= flFrametime;
	}

	if ( ( moveData.m_nButtons & IN_ATTACK ) != 0 && m_flFireDelay <= 0.0f )
	{
		int i = m_iNextWeaponIndex;
		if ( i >= m_WeaponAttachments.Count() )
		{
			i = m_iNextWeaponIndex = 0;
		}
		m_iProjectileParity++;

		CBasePlayer *pPlayer = moveData.m_nPlayerHandle.IsValid() ?
			ToBasePlayer( gEntList.GetBaseEntity( moveData.m_nPlayerHandle.Get()->GetRefEHandle() ) ) : NULL;

		Vector vecEntityFwd;
		AngleVectors( GetAbsAngles(), &vecEntityFwd );

		CBaseEntity *pAutoAimEntity = NULL;
		Vector vecInitialTarget = GetAbsOrigin() + vecEntityFwd * 1024.0f;

		IHandleEntity *pAutoAimHandle = moveData.m_iAutoAimEntityIndex > 0 ?
			gEntList.LookupEntityByNetworkIndex( moveData.m_iAutoAimEntityIndex ) : NULL;
		pAutoAimEntity = pAutoAimHandle ? gEntList.GetBaseEntity( pAutoAimHandle->GetRefEHandle() ) : NULL;

		QAngle angSpacecraft = GetAbsAngles();
		Vector vecSpacecraftForward;
		AngleVectors( angSpacecraft, &vecSpacecraftForward );

		const int iShotsMax = i + m_Settings.m_iProjectileCount;
		const float flProjectileSpeed = m_Settings.m_ProjectileSettings.m_flSpeed;
		for ( ; i < iShotsMax; ++i )
		{
			Vector vecProjectileOrigin;
			QAngle angProjectileAngles;
			GetAttachment( m_WeaponAttachments[ i % m_WeaponAttachments.Count() ], vecProjectileOrigin, angProjectileAngles );

			//Vector vecAttachmentFwd;
			//AngleVectors( angProjectileAngles, &vecAttachmentFwd );
			//vecProjectileOrigin += vecAttachmentFwd * 20.0f;

			Vector vecTarget = moveData.m_vecWorldShootPosition;

			if ( pAutoAimEntity != NULL )
			{
				vecTarget = pAutoAimEntity->WorldSpaceCenter();
				Vector vecVelocity( pAutoAimEntity->GetAbsVelocity() );

				IPhysicsObject *pPhysicsObject( pAutoAimEntity->VPhysicsGetObject() );
				if ( pPhysicsObject != NULL )
				{
					pPhysicsObject->GetVelocity( &vecVelocity, NULL );
				}

				Vector vecPredictedTarget;
				if ( UTIL_PredictProjectileTarget( vecProjectileOrigin, vecTarget, vecVelocity,
					flProjectileSpeed, vecPredictedTarget ) )
				{
					//DebugDrawLine( vecProjectileOrigin, vecTarget, 0, 0, 255, true, 1.0f );
					vecTarget = vecPredictedTarget;
					//DebugDrawLine( vecProjectileOrigin, vecTarget, 255, 0, 0, true, 1.0f );
				}
			}

			Vector vecShootDirection = vecTarget - vecProjectileOrigin;
			vecShootDirection.NormalizeInPlace();

			const float flSpacecraftDot = DotProduct( vecShootDirection, vecSpacecraftForward );
			const float flMinimumTraceDistance = 0.7071f;
			if ( flSpacecraftDot < flMinimumTraceDistance )
			{
				vecShootDirection -= vecSpacecraftForward * flSpacecraftDot;
				vecShootDirection.NormalizeInPlace();

				Vector vecSign( Sign( vecShootDirection.x ), Sign( vecShootDirection.y ),
					Sign( vecShootDirection.z ) );

				vecShootDirection *= vecShootDirection;
				vecShootDirection *= 1.0f - flMinimumTraceDistance * flMinimumTraceDistance;
				vecShootDirection.x = FastSqrt( vecShootDirection.x );
				vecShootDirection.y = FastSqrt( vecShootDirection.y );
				vecShootDirection.z = FastSqrt( vecShootDirection.z );

				vecShootDirection *= vecSign;
				vecShootDirection += vecSpacecraftForward * flMinimumTraceDistance;

				vecTarget = vecProjectileOrigin + vecShootDirection * 128.0f;
			}

			Vector vecProjectileVelocity;
			const Vector vecShootTarget = vecTarget;
			vecProjectileVelocity = vecShootTarget - vecProjectileOrigin;
			vecProjectileVelocity.NormalizeInPlace();

			CSpacecraftProjectile *pProjectile = assert_cast< CSpacecraftProjectile* >(
				CreateEntityByName( "prop_spacecraft_projectile" ) );
			Assert( pProjectile );

			vecProjectileVelocity *= flProjectileSpeed;
			pProjectile->Fire( m_strSettingsName, pPlayer, this, vecProjectileOrigin, vecProjectileVelocity );

			++m_iNextWeaponIndex;
		}

		m_flFireDelay += m_Settings.m_flFireRate;
	}
}

void CSpacecraft::OnPlayerTeamAttack(const CTakeDamageInfo &info)
{
	// Already team attacking player as penalty
	if ( m_bShouldTeamkillPlayer )
	{
		return;
	}

	// Tolerate immediate follow ups
	if ( m_flPlayerTeamAttackToleration > gpGlobals->curtime )
	{
		return;
	}

	m_flPlayerTeamAttackToleration = gpGlobals->curtime + 3.0f;

	// Two attacks, fight back
	if ( m_flPlayerTeamAttackCooldown > gpGlobals->curtime )
	{
		m_OnPlayerTeamAttackRetaliation.FireOutput( this, this );

		CGstringPlayer *pPlayer = dynamic_cast<CGstringPlayer*>(info.GetAttacker());
		if (pPlayer != NULL &&
			pPlayer->GetSpacecraft() != NULL &&
			m_pAI != NULL)
		{
			m_bShouldTeamkillPlayer = true;
			SetEnemy(pPlayer->GetSpacecraft());
			m_pAI->EnterState(ISpacecraftAI::AISTATE_ATTACK_AND_CHASE);
		}
	}
	else
	{
		// First attack, wait
		m_flPlayerTeamAttackCooldown = gpGlobals->curtime + 5.0f;
		m_OnPlayerTeamAttack.FireOutput( this, this );
	}
}
#else
void CSpacecraft::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit( state );
	switch( state )
	{
	case SHOULDTRANSMIT_START:
		AddHoloTarget( this );
		break;

	case SHOULDTRANSMIT_END:
		RemoveHoloTarget( this );
		break;
	}
}

RenderGroup_t CSpacecraft::GetRenderGroup()
{
	if ( IsViewModel() )
	{
		return RENDER_GROUP_VIEW_MODEL_OPAQUE;
	}

	return RENDER_GROUP_TWOPASS;
}

bool CSpacecraft::IsViewModel() const
{
	return GetOwnerEntity() == C_BasePlayer::GetLocalPlayer() &&
		gstring_spacecraft_firstperson.GetBool();
}

void CSpacecraft::OnDataChanged( DataUpdateType_t t )
{
	BaseClass::OnDataChanged( t );

	if ( t == DATA_UPDATE_CREATED )
	{
		CSpacecraftConfig *pConfig = CSpacecraftConfig::GetInstance();
		pConfig->Precache();
		m_Settings = pConfig->GetSettings( m_iSettingsIndex );

		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
	else if ( GetOwnerEntity() == C_BasePlayer::GetLocalPlayer() )
	{
		m_nSkin = gstring_spacecraft_firstperson.GetBool() ? 3 : 0;
	}
}

void CSpacecraft::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	for ( int i = 0; i < m_ThrusterSounds.Count(); i++ )
	{
		const int guid = m_ThrusterSounds[ i ];
		if ( enginesound->IsSoundStillPlaying( guid ) )
		{
			enginesound->StopSoundByGuid( guid );
		}
	}
}

void CSpacecraft::ClientThink()
{
	Assert( m_ThrusterAttachments.Count() == m_ThrusterParticles.Count() );
	const bool bIsPlayerControlled = IsPlayerControlled();

	QAngle angPhysImpulse( m_AngularImpulse.Get() );
	for ( int i = 0; i < 3; i++ )
	{
		angPhysImpulse[ i ] = clamp( angPhysImpulse[ i ], -45.0f, 45.0f );
	}
	matrix3x4_t matAngularImpulse;
	AngleMatrix( angPhysImpulse, matAngularImpulse );

	int iActiveThrusters = 0;

	Vector vecShipFwd, vecShipRight, vecShipUp;
	AngleVectors( GetRenderAngles(), &vecShipFwd, &vecShipRight, &vecShipUp );
	Vector vecMoveDelta( Vector( 0.0f, -1.0f, 0.0 ) * m_flMoveY );
	if ( gstring_spacecraft_move_mode.GetBool() )
	{
		vecMoveDelta += Vector( 0.0f, 0.0f, 1.0 ) * m_flMoveX;
	}
	vecMoveDelta *= -1000.0f;

	FOR_EACH_VEC( m_ThrusterAttachments, i )
	{
		const int iAttachmentIndex = m_ThrusterAttachments[ i ];

		Vector vecPos, vecFwd;
		QAngle ang;
		GetAttachment( iAttachmentIndex, vecPos, ang );
		AngleVectors( ang, &vecFwd );

		Vector vecPosLocal, vecPosLocalImpulse, vecFwdLocal;
		QAngle angLocal;
		GetAttachmentLocal( iAttachmentIndex, vecPosLocal, angLocal );
		AngleVectors( angLocal, &vecFwdLocal );

		VectorRotate( vecPosLocal, matAngularImpulse, vecPosLocalImpulse );
		Vector vecImpulseDelta = vecPosLocal - vecPosLocalImpulse + vecMoveDelta;

		const float flLocalImpulseStrength = DotProduct( vecImpulseDelta, vecFwdLocal );
		const bool bShouldShowThruster = flLocalImpulseStrength > 1.5f;
		const bool bIsShowingThruster = m_ThrusterParticles[ i ].GetObject() != NULL;
		m_flThrusterPower[ i ] = flLocalImpulseStrength;
		if ( bShouldShowThruster != bIsShowingThruster )
		{
			if ( bShouldShowThruster )
			{
				m_ThrusterParticles[ i ] = ParticleProp()->Create( "thruster_small", PATTACH_POINT_FOLLOW, iAttachmentIndex );
				m_ThrusterParticles[ i ]->SetClientViewModelEffect( IsViewModel(), true );
				ClientLeafSystem()->EnableAlternateSorting( m_ThrusterParticles[ i ]->RenderHandle(), true );
			}
			else
			{
				m_ThrusterParticles[ i ].GetObject()->StopEmission();
				m_ThrusterParticles[ i ] = NULL;
			}
		}

		if ( bShouldShowThruster )
		{
			iActiveThrusters++;
		}
	}

	int iActiveThrustersSoundsDesired = ( iActiveThrusters > 0 && iActiveThrusters < 2 ) ?
			1 : MIN( 2, iActiveThrusters / 2 );
	if (!bIsPlayerControlled)
	{
		iActiveThrustersSoundsDesired = 0;
	}
	const int iActiveThrusterSounds = m_ThrusterSounds.Count();
	if ( iActiveThrustersSoundsDesired > iActiveThrusterSounds )
	{
		const int iSoundsToAdd = iActiveThrustersSoundsDesired - iActiveThrusterSounds;
		for ( int i = 0; i < iSoundsToAdd; i++ )
		{
			EmitSoundIfSet( this, m_Settings.m_strSoundThruster );
			m_ThrusterSounds.AddToTail( enginesound->GetGuidForLastSoundEmitted() );
		}
	}
	else if ( iActiveThrustersSoundsDesired < iActiveThrusterSounds )
	{
		for ( int i = iActiveThrusterSounds - 1; i >= iActiveThrustersSoundsDesired; i-- )
		{
			const int guid = m_ThrusterSounds[ i ];
			if ( enginesound->IsSoundStillPlaying( guid ) )
			{
				enginesound->StopSoundByGuid( guid );
			}
		}
		m_ThrusterSounds.RemoveMultipleFromTail( iActiveThrusterSounds - iActiveThrustersSoundsDesired );
	}

	const float flEngineVolumeTarget = ( m_iEngineLevel == ENGINELEVEL_NORMAL ) ?
		ENGINE_VOLUME_HIGH : ENGINE_VOLUME_LOW;

	if ( m_iEngineLevelLast != m_iEngineLevel )
	{
		const bool bEngineRunning = m_iEngineLevel > ENGINELEVEL_STALLED;
		const bool bEngineWasRunning = m_iEngineLevelLast > ENGINELEVEL_STALLED;

		if ( ( bEngineRunning != bEngineWasRunning || bEngineRunning && m_iGUID_Engine < 0 ) &&
			ShouldPlaySounds() )
		{
			m_flEngineVolume = flEngineVolumeTarget;
			if ( bEngineRunning )
			{
				EmitSoundIfSet( this, m_Settings.m_strSoundEngineStart );
				m_iGUID_Engine = enginesound->GetGuidForLastSoundEmitted();
			}
			else
			{
				enginesound->StopSoundByGuid( m_iGUID_Engine );
				m_iGUID_Engine = -1;
				EmitSoundIfSet( this, m_Settings.m_strSoundEngineStop );

				if ( m_iEngineLevelLast == ENGINELEVEL_IDLE )
				{
					enginesound->SetVolumeByGuid( enginesound->GetGuidForLastSoundEmitted(), 0.4f );
				}
			}
		}

		Assert( m_EngineParticles.Count() == m_EngineAttachments.Count() );

		FOR_EACH_VEC( m_EngineParticles, i )
		{
			CSmartPtr< CNewParticleEffect > &particle = m_EngineParticles[ i ];

			if ( particle.GetObject() != NULL )
			{
				particle->StopEmission();
				particle = NULL;
			}

			const char *pszEngineParticle = NULL;
			switch ( m_iEngineLevel.Get() )
			{
			case 1:
				pszEngineParticle = "idle_loop";
				break;

			case 2:
				pszEngineParticle = "move_loop";
				break;

			case 3:
				pszEngineParticle = "boost_loop";
				break;
			}

			if ( pszEngineParticle != NULL )
			{
				particle = ParticleProp()->Create( pszEngineParticle,
					PATTACH_POINT_FOLLOW, m_EngineAttachments[ i ] );
			}
		}

		if (ShouldPlaySounds())
		{
			if ( m_iEngineLevel == ENGINELEVEL_BOOST )
			{
				EmitSoundIfSet( this, m_Settings.m_strSoundBoostStart );
				m_iGUID_Boost = enginesound->GetGuidForLastSoundEmitted();
			}
			else if ( m_iEngineLevelLast == ENGINELEVEL_BOOST )
			{
				enginesound->StopSoundByGuid( m_iGUID_Boost );
				m_iGUID_Boost = -1;
				EmitSoundIfSet( this, m_Settings.m_strSoundBoostStop );
			}
		}

		m_iEngineLevelLast = m_iEngineLevel;
	}

	if ( abs( m_flEngineVolume - flEngineVolumeTarget ) > 0.01f )
	{
		m_flEngineVolume = Approach( flEngineVolumeTarget, m_flEngineVolume, gpGlobals->frametime * 2.0f );
	}
	else
	{
		m_flEngineVolume = flEngineVolumeTarget;
	}

	float flEngineAlphaTarget = 1.0f;
	switch ( m_iEngineLevel )
	{
	case ENGINELEVEL_STALLED:
		flEngineAlphaTarget = 0.0f;
		break;

	case ENGINELEVEL_IDLE:
		flEngineAlphaTarget = 0.4f;
		break;
	}

	if ( abs( m_flEngineAlpha - flEngineAlphaTarget ) > 0.01f )
	{
		m_flEngineAlpha = Approach( flEngineAlphaTarget, m_flEngineAlpha, gpGlobals->frametime * 0.8f );
	}
	else
	{
		m_flEngineAlpha = flEngineAlphaTarget;
	}

	if ( m_iGUID_Engine >= 0 && enginesound->IsSoundStillPlaying( m_iGUID_Engine ) )
	{
		enginesound->SetVolumeByGuid( m_iGUID_Engine, m_flEngineVolume );
	}

	const bool bShouldShowSpaceField = IsPlayerControlled() && m_PhysVelocity.Get().LengthSqr() > 2000.0f;
	const bool bIsShowingSpaceField = m_SpaceFieldParticles.GetObject() != NULL;
	if ( bShouldShowSpaceField != bIsShowingSpaceField )
	{
		if ( bShouldShowSpaceField )
		{
			m_SpaceFieldParticles = ParticleProp()->Create( "spacefield", PATTACH_ABSORIGIN_FOLLOW );
		}
		else
		{
			m_SpaceFieldParticles->StopEmission();
			m_SpaceFieldParticles = NULL;
		}
	}

	if ( bIsPlayerControlled )
	{
		if ( m_iEngineLevel == 3 && m_flShakeTimer <= 0.0f )
		{
			ScreenShake_t shake;
			shake.amplitude = 0.8f;
			shake.command = SHAKE_START;
			shake.duration = 0.4f;
			shake.frequency = 10.0f;
			vieweffects->Shake( shake );
			m_flShakeTimer = 0.2f;
		}
		else if ( m_flShakeTimer > 0.0f )
		{
			m_flShakeTimer -= gpGlobals->frametime;
		}
	}

	if ( m_iProjectileParityLast != m_iProjectileParity )
	{
		int i = 0;
		if ( ( m_iProjectileParity % 2 ) != 0 && m_WeaponAttachments.Count() > 1 )
		{
			i = 1;
		}

		for ( ; i < m_WeaponAttachments.Count(); i += 2 )
		{
			DispatchParticleEffect( m_Settings.m_ProjectileSettings.m_strParticleSpawn, PATTACH_POINT_FOLLOW, this, m_WeaponAttachments[ i ] );
		}

		m_iProjectileParityLast = m_iProjectileParity;
	}
}

void CSpacecraft::UpdateCrosshair( CHudCrosshair *pCrosshair )
{
	CHudTexture *pCrosshairTexture = gHUD.GetIcon( "crosshair_spacecraft" );

	const Color white( 255, 255, 255, 255 );
	pCrosshair->SetCrosshair( pCrosshairTexture, white );
}

//const Vector &CSpacecraft::GetPhysVelocitySmooth() const
//{
//	return m_iv_vecVelocity.GetHead
//}
#endif

CStudioHdr *CSpacecraft::OnNewModel()
{
	CStudioHdr *pRet = BaseClass::OnNewModel();

#ifdef CLIENT_DLL
	m_ThrusterAttachments.RemoveAll();
	m_EngineAttachments.RemoveAll();
	delete [] m_flThrusterPower;

	if ( pRet != NULL )
	{
		for ( int i = 0; i < pRet->GetNumAttachments(); i++ )
		{
			const char *pszAttachmentName = pRet->pAttachment( i ).pszName();
			if ( Q_stristr( pszAttachmentName, "thruster" ) == pszAttachmentName )
			{
				m_ThrusterAttachments.AddToTail( i + 1 );
			}
			else if ( Q_stristr( pszAttachmentName, "engine" ) == pszAttachmentName )
			{
				m_EngineAttachments.AddToTail( i + 1 );
			}
		}
	}

	m_flThrusterPower = new float[ m_ThrusterAttachments.Count() ];
	Plat_FastMemset( m_flThrusterPower, 0, sizeof( float ) * m_ThrusterAttachments.Count() );

	FOR_EACH_VEC( m_ThrusterParticles, i )
	{
		if ( m_ThrusterParticles[ i ].GetObject() != NULL )
		{
			m_ThrusterParticles[ i ]->StopEmission();
		}
	}

	m_ThrusterParticles.SetCount( m_ThrusterAttachments.Count() );
	m_EngineParticles.SetCount( m_EngineAttachments.Count() );
	m_iAttachmentGUI = LookupAttachment( "gui" );
#endif

	m_WeaponAttachments.RemoveAll();
	if ( pRet != NULL )
	{
		for ( int i = 0; i < pRet->GetNumAttachments(); i++ )
		{
			const char *pszAttachmentName = pRet->pAttachment( i ).pszName();
			if ( Q_stristr( pszAttachmentName, "weapon" ) == pszAttachmentName )
			{
				m_WeaponAttachments.AddToTail( i + 1 );
			}
		}
	}
	return pRet;
}

void CSpacecraft::SimulateMove( CMoveData &moveData, float flFrametime )
{
	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	if ( pPhysObject == NULL )
	{
		return;
	}

	const bool bCutOffEngines = ( moveData.m_nButtons & ( IN_JUMP | IN_DUCK ) ) != 0;
	float flAngularDrag = gstring_spacecraft_physics_angulardrag.GetFloat();
	if ( bCutOffEngines )
	{
		float flZeroDrag = 0.0f;
		pPhysObject->SetDragCoefficient( &flZeroDrag, &flAngularDrag );
	}
	else
	{
		float flPositionalDrag = gstring_spacecraft_physics_drag.GetFloat();
		pPhysObject->SetDragCoefficient( &flPositionalDrag, &flAngularDrag );
	}

	Vector vecFwd, vecRight, vecUp;
	//AngleVectors( moveData.m_vecViewAngles, &vecFwd, &vecRight, &vecUp );

	Vector vecOldImpulse( vec3_origin );
	AngularImpulse angOldImpulse( vec3_origin );

	Vector vecPhysPosition;
	QAngle angPhysAngles;

	pPhysObject->GetVelocity( &vecOldImpulse, &angOldImpulse );
	pPhysObject->GetPosition( &vecPhysPosition, &angPhysAngles );

	QAngle viewangles = moveData.m_vecViewAngles;
	float flForwardMove = moveData.m_flForwardMove;
	float flSideMove = moveData.m_flSideMove;
	float flUpMove = moveData.m_flUpMove;

#ifdef GAME_DLL
	if ( m_bIsFrozen )
	{
		viewangles = angPhysAngles;
		flForwardMove = 0.0f;
		flSideMove = 0.0f;
		flUpMove = 0.0f;

		if ( IsPlayerControlled() )
		{
			CBasePlayer *pPlayer = assert_cast< CBasePlayer* >( GetOwnerEntity() );
			pPlayer->SnapEyeAngles( viewangles );
		}
	}
#endif

	AngleVectors( viewangles, NULL, &vecRight, &vecUp );
	AngleVectors( angPhysAngles, &vecFwd /*, &vecRight, &vecUp*/ );

	m_AngularImpulse.SetX( angOldImpulse.x );
	m_AngularImpulse.SetY( angOldImpulse.y );
	m_AngularImpulse.SetZ( angOldImpulse.z );
	m_AngularImpulse = ConvertPhysicsToSource( m_AngularImpulse.Get() );

	Vector vecImpulse( vec3_origin );
	bool bAccelerationEffects = false;
	bool bBoostEffects = false;

	if ( !bCutOffEngines )
	{
		const bool bUseScreenMove = gstring_spacecraft_move_mode.GetBool();
		const float flMaxMove = 200.0f;
		Vector vecMove( vec3_origin );
		vecMove.x = clamp( flForwardMove / flMaxMove, -1.0f, 1.0f );
		vecMove.y = clamp( flSideMove / flMaxMove, -1.0f, 1.0f );
		vecMove.z = clamp( flUpMove / flMaxMove, -1.0f, 1.0f );
		if ( vecMove.LengthSqr() > 1.0f )
		{
			vecMove.NormalizeInPlace();
		}
		m_flMoveX = vecMove.x;
		m_flMoveY = vecMove.y;

		const bool bBoostPressed = ( moveData.m_nButtons & IN_SPEED ) != 0;
		const bool bBoosting = bBoostPressed && !m_bBoostSuspended;
		if ( !bBoosting && bBoostPressed &&
			( moveData.m_nOldButtons & IN_SPEED ) == 0 && IsPlayerControlled() )
		{
			EmitSound( "Spacecraft.Boost.Deny" );
		}

		const float flSideAccel = IsPlayerControlled() ? m_Settings.m_flAccelerationSide : m_Settings.m_flAcceleration;
		if ( bUseScreenMove )
		{
			if ( !bCutOffEngines )
			{
				vecImpulse += 1.0f * vecFwd * ( bBoosting ? m_Settings.m_flAccelerationBoost :
					m_Settings.m_flAcceleration );
				bAccelerationEffects = true;
				bBoostEffects = bBoosting;
			}

			if ( vecMove.x != 0.0f )
			{
				vecImpulse += vecMove.x * vecUp * flSideAccel;
				bAccelerationEffects = true;
			}
		}
		else //if ( !bCutOffEngines )
		{
			if ( vecMove.x > 0.0f )
			{
				vecImpulse += vecMove.x * vecFwd * ( bBoosting ? m_Settings.m_flAccelerationBoost :
					m_Settings.m_flAcceleration );
				bAccelerationEffects = true;
				bBoostEffects = bBoosting;
			}
			else if ( vecMove.x < 0.0f )
			{
				vecImpulse += vecMove.x * vecFwd * m_Settings.m_flAcceleration;
				bAccelerationEffects = false;
				bBoostEffects = false;
			}
		}

		//if ( ( moveData.m_nButtons & IN_DUCK ) == 0 )
		{
			if ( vecMove.y != 0.0f )
			{
				vecImpulse += vecMove.y * vecRight * flSideAccel;
				bAccelerationEffects = true;
			}

			if ( vecMove.z != 0.0f )
			{
				vecImpulse += vecMove.z * vecUp * flSideAccel;
				bAccelerationEffects = true;
			}
		}
	}
	else
	{
		m_flMoveX = 0.0f;
		m_flMoveY = 0.0f;
	}

	const float flBoostDuration = gstring_spacecraft_boost_duration.GetFloat();
	const float flBoostCooldown = gstring_spacecraft_boost_cooldown.GetFloat();

	if ( bBoostEffects && IsPlayerControlled() )
	{
		m_flBoostUsage += flFrametime;
		if ( m_flBoostUsage > flBoostDuration )
		{
			m_flBoostUsage = flBoostDuration;
			m_bBoostSuspended = true;
		}
	}
	else if ( m_flBoostUsage > 0.0f )
	{
		m_flBoostUsage -= flBoostDuration / flBoostCooldown * flFrametime;
		if ( m_flBoostUsage <= 0.0f )
		{
			m_flBoostUsage = 0.0f;
			m_bBoostSuspended = false;
		}
	}

	m_iEngineLevel = bCutOffEngines ? ENGINELEVEL_STALLED : ( bAccelerationEffects ?
		( bBoostEffects ? ENGINELEVEL_BOOST : ENGINELEVEL_NORMAL ) : ENGINELEVEL_IDLE );

	QAngle deltaAngle;
	matrix3x4_t matModel, matModelInv, matTarget, matTargetLocal;

	SetIdentityMatrix( matModel );
	AngleMatrix( angPhysAngles, matModel );
	MatrixInvert( matModel, matModelInv );
	AngleMatrix( viewangles, matTarget );
	ConcatTransforms( matModelInv, matTarget, matTargetLocal );
	MatrixAngles( matTargetLocal, deltaAngle );

	deltaAngle.z = clamp( deltaAngle.z, -89.0f, 89.0f );

	deltaAngle = ConvertSourceToPhysics( deltaAngle );
	AngularImpulse angImpulse( XYZ( deltaAngle ) );
	angImpulse *= gstring_spacecraft_move_ang_approach_speed.GetFloat();

	AngularImpulse angDelta = angImpulse - angOldImpulse;
	angImpulse -= angOldImpulse;

	//if ( bCutOffEngines )
	//{
	//	vecOldImpulse -= vecFwd * DotProduct( vecFwd, vecOldImpulse );
	//	vecOldImpulse = vecOldImpulse - ( vecImpulse / gpGlobals->frametime );
	//}

	if ( !bCutOffEngines )
	{
		const float flForwardSpeed = DotProduct( vecFwd, vecOldImpulse );
		if ( abs( flForwardSpeed ) > 0.1f )
		{
			vecImpulse += vecFwd * -flForwardSpeed * MIN( 1.0f,
				flFrametime * gstring_spacecraft_move_drag_fwd.GetFloat() );
		}

		const float flSideSpeed = DotProduct( vecRight, vecOldImpulse );
		if ( abs( flSideSpeed ) > 0.1f )
		{
			vecImpulse += vecRight * -flSideSpeed * MIN( 1.0f,
				flFrametime * gstring_spacecraft_move_drag_side.GetFloat() );
		}

		const float flUpSpeed = DotProduct( vecUp, vecOldImpulse );
		if ( abs( flUpSpeed ) > 0.1f )
		{
			vecImpulse += vecUp * -flUpSpeed * MIN( 1.0f,
				flFrametime * gstring_spacecraft_move_drag_up.GetFloat() );
		}
	}

	if ( !vecImpulse.IsZero() || !angImpulse.IsZero() )
	{
		pPhysObject->AddVelocity( &vecImpulse, &angImpulse );
	}

	moveData.SetAbsOrigin( GetAbsOrigin() );

	pPhysObject->GetVelocity( &m_PhysVelocity.GetForModify(), NULL );

#ifdef GAME_DLL
	if ( !m_bIsFrozen )
	{
		SimulateFire( moveData, flFrametime );
	}
#endif
}
