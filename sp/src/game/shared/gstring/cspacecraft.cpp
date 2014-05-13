
#include "cbase.h"
#include "in_buttons.h"
#include "gstring/cspacecraft.h"
#include "gstring/gstring_player_shared.h"

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

static ConVar gstring_spacecraft_move_ang_approach_speed( "gstring_spacecraft_move_ang_approach_speed", "2.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_side( "gstring_spacecraft_move_drag_side", "2.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_fwd( "gstring_spacecraft_move_drag_fwd", "1.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_drag_up( "gstring_spacecraft_move_drag_up", "2.0", FCVAR_REPLICATED );

#ifdef GAME_DLL
BEGIN_DATADESC( CSpacecraft )

	//DEFINE_KEYFIELD( m_strOverlayMaterial, FIELD_STRING, "overlayname" ),
	//DEFINE_KEYFIELD( m_iRenderMode, FIELD_INTEGER, "rendermode" ),
	//DEFINE_KEYFIELD( m_iRenderIndex, FIELD_INTEGER, "renderindex" ),

	//DEFINE_FIELD( m_iMaterialIndex, FIELD_INTEGER ),

	//DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnterVehicle", InputEnterVehicle ),
	//DEFINE_INPUTFUNC( FIELD_VOID, "disable", InputDisable ),
	//DEFINE_INPUTFUNC( FIELD_VOID, "toggle", InputToggle ),

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
#else
	: m_iEngineLevelLast( 0 )
	, m_iProjectileParityLast( 0 )
	, m_iGUID_Engine( -1 )
	, m_iGUID_Boost( -1 )
	, m_flEngineVolume( ENGINE_VOLUME_LOW )
	, m_flShakeTimer( 0.0f )
#endif
{
}

CSpacecraft::~CSpacecraft()
{
}

#ifdef GAME_DLL
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
	const float flCollisionScale = 1.0f / 16.0f;
	SetModelScale( flCollisionScale );

	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );

	BaseClass::Activate();

	VPhysicsGetObject()->Wake();
	VPhysicsGetObject()->EnableGravity( false );
	VPhysicsGetObject()->EnableDrag( true );
}

void CSpacecraft::PhysicsSimulate()
{
	BaseClass::PhysicsSimulate();
	UpdatePhysicsShadowToCurrentPosition( gpGlobals->frametime );
}

void CSpacecraft::PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity )
{
}

void CSpacecraft::InputEnterVehicle( inputdata_t &inputdata )
{
	CGstringPlayer *pPlayer = LocalGstringPlayer();
	SetPlayerSimulated( pPlayer );
	pPlayer->EnterSpacecraft( this );
}

void CSpacecraft::SimulateFire( CMoveData &moveData )
{
	if ( m_WeaponAttachments.Count() < 1 )
	{
		return;
	}

	if ( m_flFireDelay > 0.0f )
	{
		m_flFireDelay -= gpGlobals->frametime;
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

		CBasePlayer *pPlayer = ToBasePlayer( gEntList.GetBaseEntity( moveData.m_nPlayerHandle.Get()->GetRefEHandle() ) );
		Assert( pPlayer );

		for ( ; i < m_WeaponAttachments.Count(); i += 2 )
		{
			Vector vecProjectileOrigin;
			QAngle angProjectileAngles;
			GetAttachment( m_WeaponAttachments[ i ], vecProjectileOrigin, angProjectileAngles );

			Vector vecProjectileVelocity;
			//AngleVectors( angProjectileAngles, &vecProjectileVelocity );
			const Vector vecShootTarget = pPlayer->GetLastUserCommand()->worldShootPosition;
			vecProjectileVelocity = vecShootTarget - vecProjectileOrigin;
			vecProjectileVelocity.NormalizeInPlace();

			CSpacecraftProjectile *pProjectile = assert_cast< CSpacecraftProjectile* >(
				CreateEntityByName( "prop_spacecraft_projectile" ) );
			Assert( pProjectile );

			vecProjectileVelocity *= 3000.0f;
			pProjectile->Fire( pPlayer, this, vecProjectileOrigin, vecProjectileVelocity );
		}

		m_flFireDelay = 0.05f;
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

	const int iActiveThrustersSoundsDesired = ( iActiveThrusters > 0 && iActiveThrusters < 2 ) ?
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

				if ( m_iEngineLevelLast == 1 )
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

		if ( m_iEngineLevel == 3 )
		{
			EmitSound( SPACECRAFT_SOUND_BOOST_START );
			m_iGUID_Boost = enginesound->GetGuidForLastSoundEmitted();
		}
		else if ( m_iEngineLevelLast == 3 )
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

	if ( m_iGUID_Engine >= 0 )
	{
		enginesound->SetVolumeByGuid( m_iGUID_Engine, m_flEngineVolume );
	}

	const bool bShouldShowSpaceField = m_PhysVelocity.Get().LengthSqr() > 2000.0f;
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

void CSpacecraft::SimulateMove( CMoveData &moveData )
{
	const bool bCutOffEngines = ( moveData.m_nButtons & IN_JUMP ) != 0;

	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	Assert( pPhysObject );

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

		if ( ( moveData.m_nButtons & IN_FORWARD ) != 0 )
		{
			vecImpulse += vecFwd * ( bBoosting ? flAccelerationBoost : flAcceleration );
			bAccelerationEffects = true;
			bBoostEffects = bBoosting;
		}

		if ( ( moveData.m_nButtons & IN_DUCK ) == 0 )
		{
			if ( ( moveData.m_nButtons & IN_MOVELEFT ) != 0 )
			{
				vecImpulse -= vecRight * flAccelerationSide;
				bAccelerationEffects = true;
			}

			if ( ( moveData.m_nButtons & IN_MOVERIGHT ) != 0 )
			{
				vecImpulse += vecRight * flAccelerationSide;
				bAccelerationEffects = true;
			}
		}

		if ( ( moveData.m_nButtons & IN_BACK ) != 0 )
		{
			vecImpulse -= vecFwd * flAcceleration;
			bAccelerationEffects = false;
			bBoostEffects = false;
		}
	}

	m_iEngineLevel = bCutOffEngines ? 0 : ( bAccelerationEffects ?
		( bBoostEffects ? 3 : 2 ) : 1 );

	QAngle deltaAngle;
	matrix3x4_t matModel, matModelInv, matTarget, matTargetLocal;

	SetIdentityMatrix( matModel );
	AngleMatrix( angPhysAngles, matModel );
	MatrixInvert( matModel, matModelInv );
	AngleMatrix( moveData.m_vecViewAngles, matTarget );
	ConcatTransforms( matModelInv, matTarget, matTargetLocal );
	MatrixAngles( matTargetLocal, deltaAngle );

	deltaAngle = ConvertSourceToPhysics( deltaAngle );
	AngularImpulse angImpulse( XYZ( deltaAngle ) );
	angImpulse *= gstring_spacecraft_move_ang_approach_speed.GetFloat();
	angImpulse -= angOldImpulse;

	if ( !bCutOffEngines )
	{
		const float flSideSpeed = DotProduct( vecRight, vecOldImpulse );
		if ( abs( flSideSpeed ) > 0.1f )
		{
			vecImpulse += vecRight * -flSideSpeed * MIN( 1.0f,
				gpGlobals->frametime * gstring_spacecraft_move_drag_side.GetFloat() );
		}

		const float flForwardSpeed = DotProduct( vecFwd, vecOldImpulse );
		if ( abs( flForwardSpeed ) > 0.1f )
		{
			vecImpulse += vecFwd * -flForwardSpeed * MIN( 1.0f,
				gpGlobals->frametime * gstring_spacecraft_move_drag_fwd.GetFloat() );
		}

		const float flUpSpeed = DotProduct( vecUp, vecOldImpulse );
		if ( abs( flUpSpeed ) > 0.1f )
		{
			vecImpulse += vecUp * -flUpSpeed * MIN( 1.0f,
				gpGlobals->frametime * gstring_spacecraft_move_drag_up.GetFloat() );
		}
	}

	if ( !vecImpulse.IsZero() || !angImpulse.IsZero() )
	{
		pPhysObject->AddVelocity( &vecImpulse, &angImpulse );
	}

	moveData.SetAbsOrigin( GetAbsOrigin() );

	pPhysObject->GetVelocity( &m_PhysVelocity.GetForModify(), NULL );

#ifdef GAME_DLL
	SimulateFire( moveData );
#endif
}
