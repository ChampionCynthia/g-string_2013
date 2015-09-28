
#include "cbase.h"
#include "cgstring_player.h"
#include "cspacecraft_config.h"
#include "gstring/cgstring_globals.h"
#include "obstacle_pushaway.h"
#include "shareddefs.h"

static ConVar gstring_nightvision_override( "gstring_nightvision_override", "0", FCVAR_CHEAT );

CON_COMMAND( gstring_spacecraft_forceexit, "" )
{
	if ( !sv_cheats->GetBool() )
	{
		return;
	}

	CGstringPlayer *pPlayer = LocalGstringPlayer();
	if ( pPlayer != NULL )
	{
		pPlayer->ExitSpacecraft();
	}
}

BEGIN_DATADESC( CGstringPlayer )

	DEFINE_FIELD( m_bNightvisionActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nReloadParity, FIELD_CHARACTER ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CGstringPlayer, DT_CGstringPlayer )

	SendPropBool( SENDINFO( m_bNightvisionActive ) ),
	SendPropBool( SENDINFO( m_bHasUseEntity ) ),
	SendPropInt( SENDINFO( m_nReloadParity ), EF_MUZZLEFLASH_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hSpacecraft ) ),
	SendPropBool( SENDINFO( m_bSpacecraftDeath ) ),
	SendPropEHandle( SENDINFO( m_hInteractionBody ) ),

END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( player, CGstringPlayer );

CGstringPlayer::CGstringPlayer()
{
	m_bHasUseEntity = false;
	m_bNightvisionActive = false;
	m_bSpacecraftDeath = false;
}

void CGstringPlayer::Precache()
{
	PrecacheScriptSound( "nightvision.on" );
	PrecacheScriptSound( "nightvision.off" );
	PrecacheScriptSound( "nightvision.unavailable" );

	PrecacheModel( "models/humans/group02/female_04.mdl" );

	UTIL_PrecacheDecal( "GString.Hammer.Impact", true );

	PrecacheParticleSystem( "blood_advisor_puncture_withdraw" );
	PrecacheParticleSystem( "blood_advisor_spray_strong" );
	PrecacheParticleSystem( "blood_advisor_spray_strong_2" );
	PrecacheParticleSystem( "blood_advisor_spray_strong_3" );

	BaseClass::Precache();
}

void CGstringPlayer::Spawn()
{
	BaseClass::Spawn();

	SetModel( "models/humans/group02/female_04.mdl" );

	m_Local.m_iHideHUD |= HIDEHUD_SPACECRAFT;
}

void CGstringPlayer::Activate()
{
	BaseClass::Activate();

	RemoveFlag( FL_GODMODE );
}

void CGstringPlayer::DoReloadAnim()
{
	m_nReloadParity = (m_nReloadParity+1) & ((1 << EF_MUZZLEFLASH_BITS) - 1);
}

bool CGstringPlayer::ClientCommand( const CCommand &args )
{
	const char *pszCmd = args[ 0 ];

	if ( Q_stricmp( "__reload_spacecraft_config", pszCmd ) == 0 )
	{
		CSpacecraftConfig::GetInstance()->ReloadConfig();
		return true;
	}
	//else if ( Q_stricmp( "reload_weapon_scripts", pszCmd ) == 0 )
	//{
	//	if ( !sv_cheats->GetBool() )
	//	{
	//		return false;
	//	}

	//	PrecacheFileWeaponInfoDatabase( filesystem, g_pGameRules->GetEncryptionKey() );
	//	return true;
	//}
	else
	{
		return BaseClass::ClientCommand( args );
	}
}

bool CGstringPlayer::IsNightvisionActive() const
{
	return m_bNightvisionActive;
}

void CGstringPlayer::SetNightvisionActive( bool bActive )
{
	if ( m_bNightvisionActive != bActive )
	{
		if ( bActive )
			EmitSound( "nightvision.on" );
		else
			EmitSound( "nightvision.off" );

		m_bNightvisionActive = bActive;
	}
}

void CGstringPlayer::ToggleNightvision()
{
	if ( g_pGstringGlobals != NULL
		&& !g_pGstringGlobals->IsNightvisionEnabled()
		&& !gstring_nightvision_override.GetBool()
		|| !IsSuitEquipped() )
	{
		EmitSound( "nightvision.unavailable" );
		return;
	}

	SetNightvisionActive( !IsNightvisionActive() );
}

void CGstringPlayer::ImpulseCommands()
{
	int iImpulse = GetImpulse();

	switch ( iImpulse )
	{
	case 100:
		{
			extern ConVar gstring_nightvision_override;
			const bool bOverrideNightvision = gstring_nightvision_override.GetBool();

			if ( g_pGstringGlobals != NULL
				|| bOverrideNightvision )
			{
				// neither flashlight nor nightvision
				if ( !bOverrideNightvision
					&& !g_pGstringGlobals->IsUserLightSourceEnabled() )
					break;

				if ( bOverrideNightvision
					|| g_pGstringGlobals->IsNightvisionEnabled() ) // use nightvision
				{
					if ( FlashlightIsOn() )
						FlashlightTurnOff();

					ToggleNightvision();
				}
				else // use flashlight
				{
					SetNightvisionActive( false );
					BaseClass::ImpulseCommands();
				}

				break;
			}

			BaseClass::ImpulseCommands();
		}
		break;
	default:
		BaseClass::ImpulseCommands();
		return;
	}

	ClearImpulse();
}

void CGstringPlayer::PhysicsSimulate()
{
	BaseClass::PhysicsSimulate();

	m_bHasUseEntity = GetUseEntity() != NULL;

	PerformObstaclePushaway( this );
}

void CGstringPlayer::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
#if 0
	if ( m_Local.m_bDucked )
	{
		if ( m_flStepSoundTime > 0 )
		{
			m_flStepSoundTime -= 1000.0f * gpGlobals->frametime;
			if ( m_flStepSoundTime < 0 )
			{
				m_flStepSoundTime = 0;
			}
		}

		const bool bStepTimeWasZero = m_flStepSoundTime <= 0.0f;

		if ( m_flStepSoundTime <= 0 )
		{
			BaseClass::UpdateStepSound( psurface, vecOrigin, vecVelocity );
		}

		if ( m_flStepSoundTime > 0.0f
			&& bStepTimeWasZero )
		{
			m_flStepSoundTime += 780.0f;
		}
	}
#endif
}

bool CGstringPlayer::ShouldGib( const CTakeDamageInfo &info )
{
	return false;
}

void CGstringPlayer::DeathSound( const CTakeDamageInfo &info )
{
	if ( IsInSpacecraft() )
	{
		m_bSpacecraftDeath = true;
	}
	else
	{
		BaseClass::DeathSound( info );
	}
}

int CGstringPlayer::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( IsInSpacecraft() && ( info.GetDamageType() & DMG_VEHICLE ) == 0 )
	{
		return 0;
	}
	return BaseClass::OnTakeDamage( info );
}

bool CGstringPlayer::CanBecomeRagdoll()
{
	if ( IsInSpacecraft() )
	{
		return false;
	}

	return true;
}

bool CGstringPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( IsInSpacecraft() )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

void CGstringPlayer::EnterSpacecraft( CSpacecraft *pSpacecraft )
{
	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if ( pActiveWeapon != NULL )
	{
		pActiveWeapon->Holster();
	}

	SetMoveType( MOVETYPE_NOCLIP );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_hSpacecraft.Set( pSpacecraft );
	pSpacecraft->OnPlayerEntered( this );

	m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	m_Local.m_iHideHUD &= ~HIDEHUD_SPACECRAFT;
}

bool CGstringPlayer::IsInSpacecraft() const
{
	return m_hSpacecraft != NULL;
}

void CGstringPlayer::ExitSpacecraft()
{
	Assert( m_hSpacecraft->GetOwnerEntity() == this );
	m_hSpacecraft->SetOwnerEntity( NULL );
	m_hSpacecraft.Set( NULL );

	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_WALK );

	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if ( pActiveWeapon != NULL )
	{
		pActiveWeapon->Deploy();
	}

	m_Local.m_iHideHUD |= HIDEHUD_SPACECRAFT;
	m_Local.m_iHideHUD &= ~HIDEHUD_HEALTH;

	if ( g_pGstringGlobals != NULL )
	{
		SetModelScale( g_pGstringGlobals->GetWorldScale() );
		SetGravity( g_pGstringGlobals->GetWorldScale() );
	}
}

CSpacecraft *CGstringPlayer::GetSpacecraft()
{
	return m_hSpacecraft;
}

void CGstringPlayer::StartAdmireGlovesAnimation()
{
	if ( IsInSpacecraft() )
		return;

	BaseClass::StartAdmireGlovesAnimation();
}

void CGstringPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	// Stubbed.
}

void CGstringPlayer::BeginInteraction( CBaseAnimating *pInteractionBody )
{
	LockPlayerInPlace();
	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if ( pActiveWeapon != NULL )
	{
		pActiveWeapon->Holster();
	}
	m_hInteractionBody.Set( pInteractionBody );
}

void CGstringPlayer::EndInteraction()
{
	m_hInteractionBody = NULL;
	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if ( pActiveWeapon != NULL )
	{
		pActiveWeapon->Deploy();
	}
	UnlockPlayer();

	// HACK: Fix broken view offset if interaction started while ducking (client version not in sync!)
	SetViewOffset( VEC_VIEW_SCALED( this ) - Vector( 0, 0, 0.2f ) );
}
