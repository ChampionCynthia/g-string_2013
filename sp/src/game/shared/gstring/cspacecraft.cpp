
#include "cbase.h"
#include "in_buttons.h"
#include "gstring/cspacecraft.h"
#include "gstring/gstring_player_shared.h"
#include "gstring/gstring_util.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
#include "view_scene.h"
#include "hud_crosshair.h"
#include "engine/IEngineSound.h"
#include "ivieweffects.h"
#else
#include "cspacecraftprojectile.h"
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

#define SPACECRAFT_SOUND_THRUSTER_SMALL "Spacecraft.Thruster.Small"
#define SPACECRAFT_SOUND_ENGINE_START "Spacecraft.Engine.Start"
#define SPACECRAFT_SOUND_ENGINE_STOP "Spacecraft.Engine.Stop"
#define SPACECRAFT_SOUND_BOOST_START "Spacecraft.Boost.Start"
#define SPACECRAFT_SOUND_BOOST_STOP "Spacecraft.Boost.Stop"

#define ENGINE_VOLUME_LOW 0.4f
#define ENGINE_VOLUME_HIGH 1.0f

static ConVar gstring_spacecraft_acceleration( "gstring_spacecraft_acceleration", "3.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_acceleration_boost( "gstring_spacecraft_acceleration_boost", "5.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_acceleration_side( "gstring_spacecraft_acceleration_side", "2.2", FCVAR_REPLICATED );

static ConVar gstring_spacecraft_physics_drag( "gstring_spacecraft_physics_drag", "10.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_physics_angulardrag( "gstring_spacecraft_physics_angulardrag", "5000.0", FCVAR_REPLICATED );

static ConVar gstring_spacecraft_move_ang_approach_speed( "gstring_spacecraft_move_ang_approach_speed", "4.5", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_side( "gstring_spacecraft_move_drag_side", "2.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_fwd( "gstring_spacecraft_move_drag_fwd", "1.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_up( "gstring_spacecraft_move_drag_up", "2.0", FCVAR_REPLICATED );

#ifdef GAME_DLL
BEGIN_DATADESC( CSpacecraft )

	// AI
	DEFINE_KEYFIELD( m_iAIControlled, FIELD_INTEGER, "aicontrolled" ),
	DEFINE_KEYFIELD( m_iAIAttackState, FIELD_INTEGER, "aiattackstate" ),
	DEFINE_KEYFIELD( m_iAITeam, FIELD_INTEGER, "aiteam" ),

	DEFINE_KEYFIELD( m_strInitialEnemy, FIELD_STRING, "initialenemy" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "setenemy", InputSetEnemy ),
	DEFINE_INPUTFUNC( FIELD_VOID, "clearenemy", InputClearEnemy ),

	DEFINE_FIELD( m_hPathEntity, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_strPathStartName, FIELD_STRING, "pathstartname" ),

	// Player
	DEFINE_INPUTFUNC( FIELD_VOID, "EnterVehicle", InputEnterVehicle ),

END_DATADESC()
#endif

IMPLEMENT_NETWORKCLASS_DT( CSpacecraft, CSpacecraft_DT )

#ifdef GAME_DLL
	//SendPropInt( SENDINFO( m_iMaterialIndex ) ),

	//SendPropInt( SENDINFO( m_iRenderMode ) ),
	SendPropInt( SENDINFO( m_iEngineLevel ) ),
	SendPropInt( SENDINFO( m_iProjectileParity ), 8, SPROP_UNSIGNED ),

	//SendPropBool( SENDINFO( m_bEngineRunning ) ),
	SendPropVector( SENDINFO( m_AngularImpulse ) ),
	SendPropVector( SENDINFO( m_PhysVelocity ) ),
#else
	//RecvPropInt( RECVINFO( m_iMaterialIndex ) ),

	//RecvPropInt( RECVINFO( m_iRenderMode ) ),
	RecvPropInt( RECVINFO( m_iEngineLevel ) ),
	RecvPropInt( RECVINFO( m_iProjectileParity ) ),

	//RecvPropBool( RECVINFO( m_bEngineRunning ) ),
	RecvPropVector( RECVINFO( m_AngularImpulse ) ),
	RecvPropVector( RECVINFO( m_PhysVelocity ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( prop_vehicle_spacecraft, CSpacecraft );
PRECACHE_REGISTER( prop_vehicle_spacecraft );

CSpacecraft::CSpacecraft()
#ifdef GAME_DLL
	: m_flFireDelay( 0.0f )
	, m_bAlternatingWeapons( false )
	, m_pAI( NULL )
	, m_flLastAIThinkTime( 0.0f )
	, m_iAIControlled( 0 )
	, m_iAIAttackState( 0 )
	, m_iAITeam( 0 )
#else
	: m_iEngineLevelLast( 0 )
	, m_iProjectileParityLast( 0 )
	, m_iGUID_Engine( -1 )
	, m_iGUID_Boost( -1 )
	, m_flEngineAlpha( 0.0f )
	, m_flEngineVolume( ENGINE_VOLUME_LOW )
	, m_flShakeTimer( 0.0f )
#endif
{
}

CSpacecraft::~CSpacecraft()
{
#ifdef GAME_DLL
	delete m_pAI;
#else
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

bool CSpacecraft::IsPlayerControlled()
{
	return GetOwnerEntity() && GetOwnerEntity()->IsPlayer();
}

#ifdef GAME_DLL
void CSpacecraft::SetAI( ISpacecraftAI *pSpacecraftAI )
{
	delete m_pAI;
	m_pAI = pSpacecraftAI;

	m_flLastAIThinkTime = gpGlobals->curtime;
}

void CSpacecraft::Precache()
{
	BaseClass::Precache();

	PrecacheModel( STRING( GetModelName() ) );

	PrecacheScriptSound( SPACECRAFT_SOUND_THRUSTER_SMALL );
	PrecacheScriptSound( SPACECRAFT_SOUND_ENGINE_START );
	PrecacheScriptSound( SPACECRAFT_SOUND_ENGINE_STOP );
	PrecacheScriptSound( SPACECRAFT_SOUND_BOOST_START );
	PrecacheScriptSound( SPACECRAFT_SOUND_BOOST_STOP );
}

void CSpacecraft::Spawn()
{
	PrecacheModel( STRING( GetModelName() ) );

	SetModel( STRING( GetModelName() ) );

	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );

	BaseClass::Spawn();
}

void CSpacecraft::Activate()
{
	m_hPathEntity = gEntList.FindEntityByName( NULL, m_strPathStartName, this );
	m_hEnemy = gEntList.FindEntityByName( NULL, m_strInitialEnemy, this );

	const float flCollisionScale = 1.0f / 16.0f;
	SetModelScale( flCollisionScale );

	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );

	BaseClass::Activate();

	VPhysicsGetObject()->Wake();
	VPhysicsGetObject()->EnableGravity( false );
	VPhysicsGetObject()->EnableDrag( true );

	m_takedamage = DAMAGE_YES;
	SetMaxHealth( 100 );
	SetHealth( GetMaxHealth() );
}

void CSpacecraft::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );
	if ( m_pAI != NULL )
	{
		float flDeltaTime = gpGlobals->curtime - m_flLastAIThinkTime;
		flDeltaTime = MIN( 0.25f, flDeltaTime );

		if ( flDeltaTime > 0.0f )
		{
			m_pAI->Run( flDeltaTime );
			m_flLastAIThinkTime = gpGlobals->curtime;
		}
	}
}

void CSpacecraft::InputEnterVehicle( inputdata_t &inputdata )
{
	CGstringPlayer *pPlayer = LocalGstringPlayer();
	SetPlayerSimulated( pPlayer );
	pPlayer->EnterSpacecraft( this );
}

int CSpacecraft::OnTakeDamage( const CTakeDamageInfo &info )
{
	int ret = BaseClass::OnTakeDamage( info );

	return ret;
}

void CSpacecraft::Event_Killed( const CTakeDamageInfo &info )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
	if ( pPlayer != NULL )
	{
		CTakeDamageInfo playerDamage( info );
		playerDamage.SetDamage( pPlayer->GetHealth() + 1 );
		playerDamage.SetDamageType( playerDamage.GetDamageType() | DMG_NEVERGIB );
		pPlayer->TakeDamage( playerDamage );

		pPlayer->SetMoveType( MOVETYPE_NONE );
		pPlayer->SetAbsVelocity( vec3_origin );
	}

	BaseClass::Event_Killed( info );
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
		int i = 0;
		if ( m_bAlternatingWeapons && m_WeaponAttachments.Count() > 1 )
		{
			i = 1;
		}
		m_bAlternatingWeapons = !m_bAlternatingWeapons;
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

		for ( ; i < m_WeaponAttachments.Count(); i += 2 )
		{
			const float flProjectileSpeed = 4000.0f;

			Vector vecProjectileOrigin;
			QAngle angProjectileAngles;
			GetAttachment( m_WeaponAttachments[ i ], vecProjectileOrigin, angProjectileAngles );

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
			pProjectile->Fire( pPlayer, this, vecProjectileOrigin, vecProjectileVelocity );
		}

		m_flFireDelay = 0.1f;
	}
}
#else
void CSpacecraft::OnDataChanged( DataUpdateType_t t )
{
	BaseClass::OnDataChanged( t );

	if ( t == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

void CSpacecraft::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
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
		Vector vecImpulseDelta = vecPosLocal - vecPosLocalImpulse;

		const float flLocalImpulseStrength = DotProduct( vecImpulseDelta, vecFwdLocal );
		const bool bShouldShowThruster = flLocalImpulseStrength > 1.5f;
		const bool bIsShowingThruster = m_ThrusterParticles[ i ].GetObject() != NULL;
		if ( bShouldShowThruster != bIsShowingThruster )
		{
			if ( bShouldShowThruster )
			{
				m_ThrusterParticles[ i ] = ParticleProp()->Create( "thruster_small", PATTACH_POINT_FOLLOW, iAttachmentIndex );
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

	const int iActiveThrustersSoundsDesired = ( iActiveThrusters > 0 && iActiveThrusters < 2 ||
		!bIsPlayerControlled ) ?
			1 : MIN( 2, iActiveThrusters / 2 );
	const int iActiveThrusterSounds = m_ThrusterSounds.Count();
	if ( iActiveThrustersSoundsDesired > iActiveThrusterSounds )
	{
		const int iSoundsToAdd = iActiveThrustersSoundsDesired - iActiveThrusterSounds;
		for ( int i = 0; i < iSoundsToAdd; i++ )
		{
			EmitSound( SPACECRAFT_SOUND_THRUSTER_SMALL );
			m_ThrusterSounds.AddToTail( enginesound->GetGuidForLastSoundEmitted() );
		}
	}
	else if ( iActiveThrustersSoundsDesired < iActiveThrusterSounds )
	{
		for ( int i = iActiveThrusterSounds - 1; i >= iActiveThrustersSoundsDesired; i-- )
		{
			enginesound->StopSoundByGuid( m_ThrusterSounds[ i ] );
		}
		m_ThrusterSounds.RemoveMultipleFromTail( iActiveThrusterSounds - iActiveThrustersSoundsDesired );
	}

	const float flEngineVolumeTarget = ( m_iEngineLevel == 2 ) ? ENGINE_VOLUME_HIGH : ENGINE_VOLUME_LOW;

	if ( m_iEngineLevelLast != m_iEngineLevel )
	{
		const bool bEngineRunning = m_iEngineLevel > 0;
		const bool bEngineWasRunning = m_iEngineLevelLast > 0;

		if ( bEngineRunning != bEngineWasRunning )
		{
			m_flEngineVolume = flEngineVolumeTarget;
			if ( bEngineRunning )
			{
				EmitSound( SPACECRAFT_SOUND_ENGINE_START );
				m_iGUID_Engine = enginesound->GetGuidForLastSoundEmitted();
			}
			else
			{
				enginesound->StopSoundByGuid( m_iGUID_Engine );
				m_iGUID_Engine = -1;
				EmitSound( SPACECRAFT_SOUND_ENGINE_STOP );

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

		if ( m_iEngineLevel == ENGINELEVEL_BOOST )
		{
			EmitSound( SPACECRAFT_SOUND_BOOST_START );
			m_iGUID_Boost = enginesound->GetGuidForLastSoundEmitted();
		}
		else if ( m_iEngineLevelLast == ENGINELEVEL_BOOST )
		{
			enginesound->StopSoundByGuid( m_iGUID_Boost );
			m_iGUID_Boost = -1;
			EmitSound( SPACECRAFT_SOUND_BOOST_STOP );
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
			DispatchParticleEffect( "projectile_red_spawn", PATTACH_POINT_FOLLOW, this, m_WeaponAttachments[ i ] );
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

	FOR_EACH_VEC( m_ThrusterParticles, i )
	{
		if ( m_ThrusterParticles[ i ].GetObject() != NULL )
		{
			m_ThrusterParticles[ i ]->StopEmission();
		}
	}

	m_ThrusterParticles.SetCount( m_ThrusterAttachments.Count() );
	m_EngineParticles.SetCount( m_EngineAttachments.Count() );
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

	const bool bCutOffEngines = ( moveData.m_nButtons & IN_JUMP ) != 0;
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
	AngleVectors( moveData.m_vecViewAngles, &vecFwd, &vecRight, &vecUp );

	Vector vecOldImpulse( vec3_origin );
	AngularImpulse angOldImpulse( vec3_origin );

	Vector vecPhysPosition;
	QAngle angPhysAngles;

	pPhysObject->GetVelocity( &vecOldImpulse, &angOldImpulse );
	pPhysObject->GetPosition( &vecPhysPosition, &angPhysAngles );

	m_AngularImpulse.SetX( angOldImpulse.x );
	m_AngularImpulse.SetY( angOldImpulse.y );
	m_AngularImpulse.SetZ( angOldImpulse.z );
	m_AngularImpulse = ConvertPhysicsToSource( m_AngularImpulse.Get() );

	Vector vecImpulse( vec3_origin );
	bool bAccelerationEffects = false;
	bool bBoostEffects = false;

	if ( !bCutOffEngines )
	{
		const bool bBoosting = ( moveData.m_nButtons & IN_SPEED ) != 0;
		const float flAcceleration = gstring_spacecraft_acceleration.GetFloat();
		const float flAccelerationBoost = gstring_spacecraft_acceleration_boost.GetFloat();
		const float flAccelerationSide = gstring_spacecraft_acceleration_side.GetFloat();
		const float flMaxMove = 200.0f;
		Vector vecMove( vec3_origin );
		vecMove.x = clamp( moveData.m_flForwardMove / flMaxMove, -1.0f, 1.0f );
		vecMove.y = clamp( moveData.m_flSideMove / flMaxMove, -1.0f, 1.0f );

		if ( vecMove.LengthSqr() > 1.0f )
		{
			vecMove.NormalizeInPlace();
		}

		if ( vecMove.x > 0.0f )
		{
			vecImpulse += vecMove.x * vecFwd * ( bBoosting ? flAccelerationBoost : flAcceleration );
			bAccelerationEffects = true;
			bBoostEffects = bBoosting;
		}

		if ( ( moveData.m_nButtons & IN_DUCK ) == 0 )
		{
			if ( vecMove.y < 0.0f )
			{
				vecImpulse += vecMove.y * vecRight * flAccelerationSide;
				bAccelerationEffects = true;
			}

			if ( vecMove.y > 0.0f )
			{
				vecImpulse += vecMove.y * vecRight * flAccelerationSide;
				bAccelerationEffects = true;
			}
		}

		if ( vecMove.x < 0.0f )
		{
			vecImpulse += vecMove.x * vecFwd * flAcceleration;
			bAccelerationEffects = false;
			bBoostEffects = false;
		}
	}

	m_iEngineLevel = bCutOffEngines ? ENGINELEVEL_STALLED : ( bAccelerationEffects ?
		( bBoostEffects ? ENGINELEVEL_BOOST : ENGINELEVEL_NORMAL ) : ENGINELEVEL_IDLE );

	QAngle deltaAngle;
	matrix3x4_t matModel, matModelInv, matTarget, matTargetLocal;

	SetIdentityMatrix( matModel );
	AngleMatrix( angPhysAngles, matModel );
	MatrixInvert( matModel, matModelInv );
	AngleMatrix( moveData.m_vecViewAngles, matTarget );
	ConcatTransforms( matModelInv, matTarget, matTargetLocal );
	MatrixAngles( matTargetLocal, deltaAngle );

	deltaAngle.z = clamp( deltaAngle.z, -89.0f, 89.0f );

	deltaAngle = ConvertSourceToPhysics( deltaAngle );
	AngularImpulse angImpulse( XYZ( deltaAngle ) );
	angImpulse *= gstring_spacecraft_move_ang_approach_speed.GetFloat();

	AngularImpulse angDelta = angImpulse - angOldImpulse;
	angImpulse -= angOldImpulse;

	if ( !bCutOffEngines )
	{
		const float flSideSpeed = DotProduct( vecRight, vecOldImpulse );
		if ( abs( flSideSpeed ) > 0.1f )
		{
			vecImpulse += vecRight * -flSideSpeed * MIN( 1.0f,
				flFrametime * gstring_spacecraft_move_drag_side.GetFloat() );
		}

		const float flForwardSpeed = DotProduct( vecFwd, vecOldImpulse );
		if ( abs( flForwardSpeed ) > 0.1f )
		{
			vecImpulse += vecFwd * -flForwardSpeed * MIN( 1.0f,
				flFrametime * gstring_spacecraft_move_drag_fwd.GetFloat() );
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
	SimulateFire( moveData, flFrametime );
#endif
}

const Vector &CSpacecraft::GetPhysVelocity() const
{
	return m_PhysVelocity.Get();
}

CSpacecraft::EngineLevel_e CSpacecraft::GetEngineLevel() const
{
	return ( EngineLevel_e )m_iEngineLevel.Get();
}
