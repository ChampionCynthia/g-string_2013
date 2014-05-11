
#include "cbase.h"
#include "c_gstring_player.h"
#include "view_shared.h"
#include "view.h"
#include "ivieweffects.h"
#include "flashlighteffect.h"
#include "c_muzzleflash_effect.h"
#include "c_bobmodel.h"
#include "c_gstring_player_ragdoll.h"

#define FLASHLIGHT_DISTANCE		1000

static ConVar gstring_firstpersonbody_forwardoffset_min( "gstring_firstpersonbody_forwardoffset_min", "13.0" );
static ConVar gstring_firstpersonbody_forwardoffset_max( "gstring_firstpersonbody_forwardoffset_max", "18.0" );

ConVar gstring_firstpersonbody_enable( "gstring_firstpersonbody_enable", "1", FCVAR_ARCHIVE );
ConVar gstring_firstpersonbody_shadow_enable( "gstring_firstpersonbody_shadow_enable", "1", FCVAR_ARCHIVE );

static ConVar gstring_viewbob_walk_dist( "gstring_viewbob_walk_dist", "2.5" );
static ConVar gstring_viewbob_walk_scale( "gstring_viewbob_walk_scale", "1.5" );
static ConVar gstring_viewbob_model_scale( "gstring_viewbob_model_scale", "1" );

IMPLEMENT_CLIENTCLASS_DT( C_GstringPlayer, DT_CGstringPlayer, CGstringPlayer )

	RecvPropBool( RECVINFO( m_bNightvisionActive ) ),
	RecvPropBool( RECVINFO( m_bHasUseEntity ) ),
	RecvPropInt( RECVINFO( m_nReloadParity ) ),
	RecvPropEHandle( RECVINFO( m_hSpacecraft ) ),

END_RECV_TABLE()

C_GstringPlayer::C_GstringPlayer()
	: m_flNightvisionFraction( 0.0f )
	, m_flMuzzleFlashTime( 0.0f )
	, m_pMuzzleFlashEffect( NULL )
	, m_flMuzzleFlashDuration( 0.0f )
	, m_bFlashlightVisible( false )
	, m_pBobViewModel( NULL )
	, m_flBobModelAmount( 0.0f )
	, m_angLastBobAngle( vec3_angle )
	, m_pBodyModel( NULL )
	, m_flMuzzleFlashRoll( 0.0f )
	, m_flLandBobTime( 0.0f )
	, m_flBodyYawLast( 0.0f )
	, m_bBodyWasMoving( false )
	, m_bBobWasInAir( false )
	, m_flLandBobDynamicScale( 0.0f )
	, m_bBodyWasInAir( false )
	, m_bBodyPlayingLandAnim( false )
	, m_bBodyWasHidden( false )
	, m_nOldReloadParity( 0 )
	, m_iBodyNextAttackLayer( 0 )
	, m_flBodyStepSoundHack( 0.0f )
	, m_flLastFallVelocity( 0.0f )
{
	m_nReloadParity = 0;
	m_bHasUseEntity = false;
	m_vecBodyOffset.Init( -gstring_firstpersonbody_forwardoffset_min.GetFloat(), 0, 0 );
}

C_GstringPlayer::~C_GstringPlayer()
{
	delete m_pMuzzleFlashEffect;

	if ( m_pBobViewModel != NULL )
	{
		m_pBobViewModel->Release();
	}

	if ( m_pBodyModel != NULL )
	{
		m_pBodyModel->Release();
	}
}

bool C_GstringPlayer::IsNightvisionActive() const
{
	if ( render && render->GetViewEntity() > gpGlobals->maxClients )
		return false;

	return m_bNightvisionActive;
}

float C_GstringPlayer::GetNightvisionFraction() const
{
	return m_flNightvisionFraction;
}

void C_GstringPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		ConVarRef scissor( "r_flashlightscissor" );
		scissor.SetValue( "0" );
	}
}

void C_GstringPlayer::ClientThink()
{
	BaseClass::ClientThink();

	const float flNightvisionTarget = IsNightvisionActive() ? 1.0f : 0.0f;

	if ( flNightvisionTarget != m_flNightvisionFraction )
	{
		m_flNightvisionFraction = Approach( flNightvisionTarget, m_flNightvisionFraction, gpGlobals->frametime * 5.0f );

		unsigned char r, g, b;
		g_pClientShadowMgr->GetShadowColor( &r, &g, &b );

		Vector v( r / 255.0f, g / 255.0f, b / 255.0f );
		v = Lerp( m_flNightvisionFraction * 0.7f, v, Vector( 1, 1, 1 ) );

		g_pClientShadowMgr->SetShadowColorMaterialsOnly( XYZ( v ) );
	}

	if ( ShouldMuzzleFlash() )
	{
		DisableMuzzleFlash();

		ProcessMuzzleFlashEvent();
	}

	if ( m_nOldReloadParity != m_nReloadParity )
	{
		m_nOldReloadParity = m_nReloadParity;

		C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

		if ( m_pBodyModel != NULL
			&& pWeapon != NULL )
		{
			const int iSequence = m_pBodyModel->SelectWeightedSequence( pWeapon->ActivityOverride( ACT_GESTURE_RELOAD, NULL ) );

			if ( iSequence >= 0 )
			{
				C_AnimationLayer *pLayer = m_pBodyModel->GetAnimOverlay( 3 );

				pLayer->m_nSequence = iSequence;

				pLayer->m_flWeight = 1.0f;
				pLayer->m_flCycle = 0.0f;

				C_BaseAnimating *pViewModel = GetViewModel( 0 );

				if ( pViewModel != NULL
					&& pViewModel->GetSequence() >= 0
					&& pViewModel->GetSequenceActivity( pViewModel->GetSequence() ) == ACT_VM_RELOAD )
				{
					pLayer->m_flPlaybackRate = m_pBodyModel->SequenceDuration( pLayer->m_nSequence ) / pViewModel->SequenceDuration();
				}
				else
				{
					pLayer->m_flPlaybackRate = 1.0f;
				}
			}
		}
	}

	UpdateBodyModel();
}

void C_GstringPlayer::OverrideView( CViewSetup *pSetup )
{
	if ( m_pRagdollEntity.Get() != NULL )
	{
		m_pRagdollEntity->GetAttachment( "eyes", pSetup->origin, pSetup->angles );
		pSetup->fov += 10.0f;
		return;
	}

	if ( IsInSpacecraft() )
	{
		GetSpacecraftCamera( pSetup->origin, pSetup->angles, pSetup->fov );
		return;
	}

	Vector velocity;
	EstimateAbsVelocity( velocity );
	float speed = velocity.NormalizeInPlace();

	static float amt = 0.0f;
	static float amtSide = 0.0f;
	float amtGoal = RemapValClamped( speed, 200, 300, 0, 1.5f );

	if ( (GetFlags() & FL_ONGROUND) == 0 ||
		GetMoveType() != MOVETYPE_WALK )
		amtGoal = 0.0f;

	float amtGoalSide = amtGoal;

	float dot_fwd = DotProduct( MainViewForward(), velocity );
	float dot_side = DotProduct( MainViewRight(), velocity );
	amtGoal *= abs( dot_fwd );
	amtGoalSide *= dot_side * 2.0f;

	if ( amt != amtGoal )
		amt = Approach( amtGoal, amt, gpGlobals->frametime * 3.0f );

	if ( amtSide != amtGoalSide )
		amtSide = Approach( amtGoalSide, amtSide, gpGlobals->frametime * 4.0f );

	if ( amt > 0.0f )
	{
		float sine = sin( gpGlobals->curtime * 10.0f ) * amt;
		float sineY = sin( gpGlobals->curtime * 5.0f + M_PI * 0.5f ) * amt;

		pSetup->origin += Vector( 0, 0, gstring_viewbob_walk_dist.GetFloat() ) * sine;
		pSetup->angles.x += sine * gstring_viewbob_walk_scale.GetFloat();
		pSetup->angles.y += sineY * 2.0f * gstring_viewbob_walk_scale.GetFloat();
	}

	if ( amtSide != 0.0f )
	{
		pSetup->angles.z += amtSide;
	}

	// land bob anim
	const bool bIsInAir = ( GetFlags() & FL_ONGROUND ) == 0;

	if ( m_bBobWasInAir != bIsInAir )
	{
		if ( !bIsInAir && m_flLastFallVelocity > 70.0f )
		{
			m_flLandBobDynamicScale = RemapValClamped( m_flLastFallVelocity, 70, 200, 0.0f, 2.0f );

			m_flLandBobTime = M_PI;
			m_flLastFallVelocity = 0.0f;
		}

		m_bBobWasInAir = bIsInAir;
	}

	if ( bIsInAir )
	{
		m_flLastFallVelocity = m_Local.m_flFallVelocity;
	}

	if ( m_flLandBobTime > 0.0f )
	{
		//pSetup->origin.z -= ( 1.0f - abs( ( ( m_flLandBobTime / 500.0f ) - 0.5f ) * 2.0f ) ) * 10.0f;
		float flLandOffset = sin( m_flLandBobTime );

		pSetup->origin.z -= flLandOffset * 2.0f * m_flLandBobDynamicScale;

		float flRate = 2.0f + 10.0f * ( m_flLandBobTime / M_PI );

		m_flLandBobTime -= gpGlobals->frametime * flRate;

		if ( m_flLandBobTime < 0.0f )
		{
			m_flLandBobTime = 0.0f;
		}
	}

	// shake derived from viewmodel
	C_BaseViewModel *pViewModel = GetViewModel();

	if ( pViewModel != NULL
		&& pViewModel->GetModelPtr() != NULL
		&& pViewModel->GetWeapon() != NULL )
	{
		if ( m_pBobViewModel == NULL )
		{
			const char *pszName = modelinfo->GetModelName( pViewModel->GetModel() );

			if ( pszName && *pszName )
			{
				m_pBobViewModel = new C_BobModel();
			
				m_pBobViewModel->InitializeAsClientEntity( pszName, RENDER_GROUP_OTHER );
			}
		}

		if ( m_pBobViewModel->GetModelIndex() != pViewModel->GetModelIndex() )
		{
			const char *pszName = modelinfo->GetModelName( pViewModel->GetModel() );

			if ( pszName && *pszName )
			{
				m_pBobViewModel->SetModel( pszName );

				m_pBobViewModel->SetAttachmentInfo( pViewModel->GetWeapon()->GetWpnData() );
			}
		}

		if ( m_pBobViewModel->IsDirty() )
		{
			m_pBobViewModel->UpdateDefaultTransforms();
			m_pBobViewModel->SetDirty( false );
		}

		//extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

		if ( !m_pBobViewModel->IsInvalid() )
		{
			m_pBobViewModel->SetSequence( pViewModel->GetSequence() );
			m_pBobViewModel->SetCycle( pViewModel->GetCycle() );

			QAngle ang;
			m_pBobViewModel->GetDeltaTransforms( ang );
			m_angLastBobAngle = ang * 0.15f;
		}
	}

	float flGoalBobAmount = ( m_pBobViewModel
		&& !m_pBobViewModel->IsInvalid()
		&& !m_bHasUseEntity )
		? 1.0f : 0.0f;

	if ( m_flBobModelAmount != flGoalBobAmount )
	{
		m_flBobModelAmount = Approach( flGoalBobAmount, m_flBobModelAmount, gpGlobals->frametime * 5.0f );
	}

	if ( !m_bHasUseEntity
		&& render->GetViewEntity() == entindex() )
	{
		pSetup->angles += m_angLastBobAngle
			* gstring_viewbob_model_scale.GetFloat()
			* m_flBobModelAmount;
	}
}

int C_GstringPlayer::DrawModel( int flags )
{
	if ( IsInSpacecraft() )
	{
		return 0;
	}

	return BaseClass::DrawModel( flags );
}

void C_GstringPlayer::ProcessMuzzleFlashEvent()
{
	m_flMuzzleFlashDuration = RandomFloat( 0.025f, 0.045f );
	m_flMuzzleFlashTime = gpGlobals->curtime + m_flMuzzleFlashDuration;
	m_flMuzzleFlashRoll = RandomFloat( 0, 360.0f );

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( m_pBodyModel != NULL
		&& pWeapon != NULL )
	{
		const int iSequence = m_pBodyModel->SelectWeightedSequence( pWeapon->ActivityOverride( ACT_GESTURE_RANGE_ATTACK1, NULL ) );

		if ( iSequence >= 0 )
		{
			C_AnimationLayer *pLayer = m_pBodyModel->GetAnimOverlay( m_iBodyNextAttackLayer );

			m_iBodyNextAttackLayer++;
			if ( m_iBodyNextAttackLayer >= 3 )
			{
				m_iBodyNextAttackLayer = 0;
			}

			pLayer->m_nSequence = iSequence;
			pLayer->m_flWeight = 1.0f;
			pLayer->m_flCycle = 0.0f;
			pLayer->m_flPlaybackRate = 1.0f;
		}
	}
}

void C_GstringPlayer::UpdateFlashlight()
{
	const bool bDoMuzzleflash = m_flMuzzleFlashTime > gpGlobals->curtime || m_flMuzzleFlashDuration > 0.0f;
	const bool bDoFlashlight = !bDoMuzzleflash && IsEffectActive( EF_DIMLIGHT );

	Vector vecForward, vecRight, vecUp, vecPos;
	vecPos = EyePosition();
	EyeVectors( &vecForward, &vecRight, &vecUp );

	if ( m_flMuzzleFlashTime <= gpGlobals->curtime
		&& m_flMuzzleFlashDuration > 0.0f )
	{
		m_flMuzzleFlashDuration = 0.0f;
	}

	m_bFlashlightVisible = bDoFlashlight || bDoMuzzleflash;
	bool bUseFlashlightOffset = true;

	if ( m_bFlashlightVisible )
	{
		C_BaseViewModel *pViewModel = GetViewModel();

		if ( pViewModel != NULL
			&& pViewModel->GetModelPtr() != NULL
			&& pViewModel->GetModelPtr()->GetNumAttachments() >= 1 )
		{
			extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

			matrix3x4_t viewModel;
			pViewModel->GetAttachment( 1, viewModel );

			QAngle ang;
			MatrixAngles( viewModel, ang, vecPos );
			FormatViewModelAttachment( vecPos, false );
			AngleVectors( ang, &vecForward, &vecRight, &vecUp );

			trace_t tr;
			Vector vecIdealPos = vecPos - vecForward * 40.0f;

			const float flFlashlightHullSize = 10.0f;
			UTIL_TraceHull( EyePosition(), vecIdealPos,
				Vector( -flFlashlightHullSize, -flFlashlightHullSize, -flFlashlightHullSize ),
				Vector( flFlashlightHullSize, flFlashlightHullSize, flFlashlightHullSize ),
				MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );

			vecPos = tr.endpos;
			bUseFlashlightOffset = false;
		}
	}

	m_vecFlashlightPosition = vecPos;
	m_vecFlashlightForward = vecForward;

#define FLASHLIGHT_FOV_ADJUST 15.0f
#define FLASHLIGHT_FOV_ADJUSTMUZZLEFLASH 25.0f
#define FLASHLIGHT_FOV_MIN 5.0f

	if ( bDoFlashlight )
	{
		if (!m_pFlashlight)
		{
			// Turned on the headlight; create it.
			m_pFlashlight = new CFlashlightEffect(index);

			if (!m_pFlashlight)
				return;

			m_pFlashlight->TurnOn();
		}

		// Update the light with the new position and direction.
		m_pFlashlight->UpdateLight( vecPos, vecForward, vecRight, vecUp, FLASHLIGHT_DISTANCE, bUseFlashlightOffset );

		m_flFlashlightDot = m_pFlashlight->GetHorizontalFOV() - FLASHLIGHT_FOV_ADJUST;
		m_flFlashlightDot = MAX( m_flFlashlightDot, FLASHLIGHT_FOV_MIN );
		m_flFlashlightDot = cos( DEG2RAD( m_flFlashlightDot ) );
	}
	else if ( m_pFlashlight )
	{
		// Turned off the flashlight; delete it.
		delete m_pFlashlight;
		m_pFlashlight = NULL;
	}

	if ( bDoMuzzleflash )
	{
		if (!m_pMuzzleFlashEffect)
		{
			// Turned on the headlight; create it.
			m_pMuzzleFlashEffect = new C_MuzzleflashEffect();

			if (!m_pMuzzleFlashEffect)
				return;
		}

		float flStrength = ( m_flMuzzleFlashTime - gpGlobals->curtime ) / m_flMuzzleFlashDuration;

		QAngle ang;
		VectorAngles( vecForward, vecUp, ang );
		ang.z = m_flMuzzleFlashRoll;
		AngleVectors( ang, &vecForward, &vecRight, &vecUp );

		// Update the light with the new position and direction.
		m_pMuzzleFlashEffect->UpdateLight( vecPos, vecForward, vecRight, vecUp, flStrength * flStrength );
		
		m_flFlashlightDot = m_pMuzzleFlashEffect->GetHorizontalFOV() - FLASHLIGHT_FOV_ADJUSTMUZZLEFLASH;
		m_flFlashlightDot = MAX( m_flFlashlightDot, FLASHLIGHT_FOV_MIN );
		m_flFlashlightDot = cos( DEG2RAD( m_flFlashlightDot ) );
	}
	else
	{
		delete m_pMuzzleFlashEffect;
		m_pMuzzleFlashEffect = NULL;
	}
}

ShadowHandle_t C_GstringPlayer::GetFlashlightHandle()
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();

	if ( pPlayer )
	{
		if ( pPlayer->m_pFlashlight )
			return pPlayer->m_pFlashlight->GetFlashlightHandle();

		if ( pPlayer->m_pMuzzleFlashEffect )
			return pPlayer->m_pMuzzleFlashEffect->GetFlashlightHandle();
	}

	return SHADOW_HANDLE_INVALID;
}

bool C_GstringPlayer::ShouldFirstpersonModelCastShadow()
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();

	if ( pPlayer )
	{
		if ( pPlayer->m_pFlashlight )
		{
			ClientShadowHandle_t clientHandle = pPlayer->m_pFlashlight->GetFlashlightHandle();

			ShadowHandle_t shadowHandle = ( clientHandle != CLIENTSHADOW_INVALID_HANDLE ) ?
				g_pClientShadowMgr->GetShadowHandle( clientHandle ) : SHADOW_HANDLE_INVALID;

			if ( shadowHandle != SHADOW_HANDLE_INVALID
				&& shadowHandle == g_pClientShadowMgr->GetActiveDepthTextureHandle() )
			{
				return false;
			}
		}

		if ( pPlayer->m_pMuzzleFlashEffect )
		{
			ClientShadowHandle_t clientHandle = pPlayer->m_pMuzzleFlashEffect->GetFlashlightHandle();

			ShadowHandle_t shadowHandle = ( clientHandle != CLIENTSHADOW_INVALID_HANDLE ) ?
				g_pClientShadowMgr->GetShadowHandle( clientHandle ) : SHADOW_HANDLE_INVALID;

			if ( shadowHandle != SHADOW_HANDLE_INVALID
				&& shadowHandle == g_pClientShadowMgr->GetActiveDepthTextureHandle() )
			{
				return false;
			}
		}

		return gstring_firstpersonbody_shadow_enable.GetBool();
	}

	return false;
}

bool C_GstringPlayer::IsRenderingFlashlight() const
{
	return m_bFlashlightVisible;
}

void C_GstringPlayer::GetFlashlightPosition( Vector &vecPos ) const
{
	vecPos = m_vecFlashlightPosition;
}

void C_GstringPlayer::GetFlashlightForward( Vector &vecForward ) const
{
	vecForward = m_vecFlashlightForward;
}

float C_GstringPlayer::GetFlashlightDot() const
{
	return m_flFlashlightDot;
}

void C_GstringPlayer::UpdateBodyModel()
{
	const bool bFirstpersonBodyEnabled = gstring_firstpersonbody_enable.GetBool();

	if ( !bFirstpersonBodyEnabled
		|| !IsAlive()
		|| IsInSpacecraft() )
	{
		if ( m_pBodyModel != NULL )
		{
			m_pBodyModel->Release();
			m_pBodyModel = NULL;
		}

		if ( !bFirstpersonBodyEnabled )
		{
			UpdateCustomStepSound();
		}

		return;
	}

	if ( m_pBodyModel == NULL )
	{
		const char *pszModel = "models/humans/group02/female_04.mdl";

		m_pBodyModel = new C_FirstpersonBody();
		m_pBodyModel->InitializeAsClientEntity( pszModel, RENDER_GROUP_OPAQUE_ENTITY );
		m_pBodyModel->SetModelName( pszModel );

		m_pBodyModel->SetNumAnimOverlays( 4 );

		for ( int i = 0; i < m_pBodyModel->GetNumAnimOverlays(); i++ )
		{
			m_pBodyModel->GetAnimOverlay( i )->Reset();
			m_pBodyModel->GetAnimOverlay( i )->SetOrder( i );
		}

		//m_pBodyModel->Spawn();
		m_pBodyModel->AddEffects( EF_NOINTERP );
		m_pBodyModel->AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

		m_pBodyModel->UpdatePartitionListEntry();
		m_pBodyModel->UpdateVisibility();

		m_pBodyModel->DestroyShadow();
	}

	QAngle angle = GetRenderAngles();

	const float flViewPitch = angle.x;

	angle.x = 0;
	angle.z = 0;

	Vector fwd, right, up;
	AngleVectors( angle, &fwd, &right, &up );

	const float flMovingMinSpeed = 10.0f;

	const float flSpeed = GetAbsVelocity().Length2D();
	const bool bInAir = ( GetFlags() & FL_ONGROUND ) == 0;
	const bool bDuck = m_Local.m_bDucked
		|| m_Local.m_bDucking;
	const bool bMoving = flSpeed > flMovingMinSpeed;
	bool bIsHidden = false;
	const bool bFalling = GetAbsVelocity().z < -300;

	// move body backwards while ducked
	float flBackOffsetSpeed = 35.0f;
	float flBackOffsetSpeedQuad = 5.0f;
	Vector vecOffsetDesired( bDuck ?
		-gstring_firstpersonbody_forwardoffset_max.GetFloat()
		: -gstring_firstpersonbody_forwardoffset_min.GetFloat(),
		0, 0 );

	// hide body while falling/swimming
	if ( /*GetAbsVelocity().z < -300
		||*/ GetWaterLevel() >= WL_Eyes )
	{
		vecOffsetDesired.x = -110.0f;
		vecOffsetDesired.z = 200.0f;

		flBackOffsetSpeedQuad = 1.0f;
		bIsHidden = true;
	}

	Vector vecOffsetDelta = vecOffsetDesired - m_vecBodyOffset;
	if ( vecOffsetDelta.LengthSqr() > 2.0f )
	{
		float len = vecOffsetDelta.NormalizeInPlace();
		float deltaLen = MAX( gpGlobals->frametime * flBackOffsetSpeed,
			len * gpGlobals->frametime * flBackOffsetSpeedQuad );
		deltaLen = MIN( len, deltaLen );

		vecOffsetDelta *= deltaLen;

		m_vecBodyOffset += vecOffsetDelta;
	}
	else
	{
		m_vecBodyOffset = vecOffsetDesired;
	}

	// fix z position when ducking, duck-jumping etc
	Vector origin = GetRenderOrigin() + fwd * m_vecBodyOffset.x
		+ right * m_vecBodyOffset.y
		+ up * m_vecBodyOffset.z;
	Vector playerOrigin = GetRenderOrigin();

	if ( !bDuck
		|| m_Local.m_bInDuckJump && bInAir )
	{
		origin.z += GetViewOffset().z - VEC_VIEW.z;
		playerOrigin.z += GetViewOffset().z - VEC_VIEW.z;
	}
	else if ( bDuck && flViewPitch < -20.0f ) // hide body when ducking and looking up
	{
		origin.z -= 50.0f;
	}

	m_pBodyModel->m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;
	m_pBodyModel->SetAbsOrigin( origin );
	m_pBodyModel->SetAbsAngles( angle );
	m_pBodyModel->SetPlayerOrigin( playerOrigin );

	Activity actDesired = ACT_IDLE;
	float flPlaybackrate = 1.0f;

	if ( m_bBodyWasInAir != bInAir )
	{
		// current land anim sucks for this :(
		//if ( !bInAir )
		//{
		//	m_bBodyPlayingLandAnim = true;

		//	m_pBodyModel->SetCycle( 0.0f );
		//}

		m_bBodyWasInAir = bInAir;
	}

	if ( bInAir )
	{
		if ( bFalling )
		{
			actDesired = ACT_WALK;
		}
		else
		{
			actDesired = ACT_JUMP;
		}

		m_bBodyPlayingLandAnim = false;
	}
	else
	{
		if ( bMoving )
		{
			actDesired = bDuck ? ACT_RUN_CROUCH : ACT_RUN;

			m_bBodyPlayingLandAnim = false;
		}
		else
		{
			actDesired = bDuck ? ACT_COVER_LOW : ACT_IDLE;

			if ( m_bBodyPlayingLandAnim )
			{
				if ( m_pBodyModel->GetCycle() >= 1.0f )
				{
					m_bBodyPlayingLandAnim = false;
				}
				else
				{
					actDesired = ACT_LAND;
				}
			}
		}
	}

	Vector vecVelocity = GetAbsVelocity();
	vecVelocity.z = 0.0f;
	float flLength = vecVelocity.NormalizeInPlace();

	const bool bDoMoveYaw = flLength > flMovingMinSpeed
		|| bFalling;

	if ( bDoMoveYaw
		&& m_pBodyModel->m_iPoseParam_MoveYaw >= 0 )
	{
		float flYaw = 0.0f;

		if ( !bFalling )
		{
			VectorYawRotate( vecVelocity, -angle.y, vecVelocity );

			flYaw = atan2( vecVelocity.y, vecVelocity.x );
			flYaw = AngleNormalizePositive( flYaw );
		}

		//if ( m_bBodyWasMoving )
		//{
		//	m_flBodyYawLast = ApproachAngle( flYaw, m_flBodyYawLast, gpGlobals->frametime * 10.0f );
		//}
		//else
		{
			m_flBodyYawLast = flYaw;
		}

		m_pBodyModel->SetPoseParameter( m_pBodyModel->m_iPoseParam_MoveYaw, RAD2DEG( AngleNormalize( m_flBodyYawLast ) ) );
	}

	m_bBodyWasMoving = bDoMoveYaw;

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( pWeapon != NULL )
	{
		actDesired = pWeapon->ActivityOverride( actDesired, NULL );
	}

	if ( m_pBodyModel->GetSequenceActivity( m_pBodyModel->GetSequence() )
		!= actDesired )
	{
		int sequence = m_pBodyModel->SelectWeightedSequence( actDesired );

		Assert( sequence >= 0 );

		if ( sequence >= 0 )
		{
			m_pBodyModel->SetSequence( sequence );
		}
	}

	if ( bFalling )
	{
		flPlaybackrate = 0.5f;
	}
	else if ( !bInAir && bMoving )
	{
		float flGroundSpeed = m_pBodyModel->GetSequenceGroundSpeed( m_pBodyModel->GetSequence() );

		if ( flGroundSpeed > 0.0f )
		{
			flPlaybackrate = flSpeed / flGroundSpeed;

			flPlaybackrate = MIN( 1.5f, flPlaybackrate );
		}
	}

	m_pBodyModel->SetPlaybackRate( flPlaybackrate );
	m_pBodyModel->StudioFrameAdvance();

	if ( m_pBodyModel->GetModel() != NULL
		&& m_pBodyModel->GetShadowHandle() == CLIENTSHADOW_INVALID_HANDLE )
	{
		m_pBodyModel->CreateShadow();
	}

	if ( actDesired == ACT_RUN_AIM_PISTOL )
	{
		UpdateCustomStepSound();
	}
	else
	{
		m_flBodyStepSoundHack = 0.0f;
	}
}

void C_GstringPlayer::UpdateCustomStepSound()
{
	Vector vecVelocity = GetAbsVelocity();
	vecVelocity.z = 0.0f;
	const float flSpeedSqr = vecVelocity.LengthSqr();

	if ( flSpeedSqr > 1600.0f)
	{
		m_flBodyStepSoundHack -= gpGlobals->frametime;

		if ( m_flBodyStepSoundHack < 0.0f )
		{
			const Vector &vecVelocity = GetAbsVelocity();
			m_flBodyStepSoundHack = ( flSpeedSqr > 150.0f * 150.0f ) ? 0.25f : 0.45f;

			BaseClass::UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vecVelocity );
		}
	}
}

surfacedata_t* C_GstringPlayer::GetGroundSurface()
{
	return BaseClass::GetGroundSurface();
}

void C_GstringPlayer::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
}

void C_GstringPlayer::UpdateStepSoundOverride( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
	BaseClass::UpdateStepSound( psurface, vecOrigin, vecVelocity );
}

void C_GstringPlayer::GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt )
{
	if ( m_pBodyModel != NULL )
	{
		m_pBodyModel->SetAbsOrigin( GetAbsOrigin() );
		m_pBodyModel->InvalidateBoneCache();

		m_pBodyModel->GetRagdollInitBoneArrays( pDeltaBones0, pDeltaBones1, pCurrentBones, boneDt );
		return;
	}

	BaseClass::GetRagdollInitBoneArrays( pDeltaBones0, pDeltaBones1, pCurrentBones, boneDt );
}

C_ClientRagdoll *C_GstringPlayer::CreateRagdollCopyInstance()
{
	return new C_GStringPlayerRagdoll();
}

bool C_GstringPlayer::IsInSpacecraft() const
{
	return m_hSpacecraft != NULL;
}

CSpacecraft *C_GstringPlayer::GetSpacecraft()
{
	return m_hSpacecraft;
}

void C_GstringPlayer::GetSpacecraftCamera( Vector &origin, QAngle &angles, float &flFov )
{
	Assert( GetSpacecraft() );
	CSpacecraft *pSpacecraft = GetSpacecraft();

	const float flDistance = 40.0f;

	Vector vecFwd, vecRight, vecUp;
	AngleVectors( angles, &vecFwd, &vecRight, &vecUp );

	Vector vecPivot = origin + vecFwd * 5000.0f;
	origin = pSpacecraft->GetRenderOrigin() - vecFwd * flDistance;

	origin += vecUp * 10.0f;

	static Vector s_originSmooth( origin );
	Vector delta = origin - s_originSmooth;
	if ( delta.LengthSqr() > 10000.0f )
	{
		s_originSmooth = origin;
	}
	else
	{
		s_originSmooth += delta * gpGlobals->frametime * 50.0f;
		origin = s_originSmooth;
	}

	vieweffects->CalcShake();
	vieweffects->ApplyShake( origin, angles, 1.0f );
}
