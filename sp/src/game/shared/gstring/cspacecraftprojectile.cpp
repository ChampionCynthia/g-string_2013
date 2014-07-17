#include "cbase.h"

#ifdef CLIENT_DLL
#include "dlight.h"
#include "iefx.h"
#endif

#include "cspacecraftprojectile.h"
#include "decals.h"
#include "particle_parse.h"

static ConVar gstring_spacecraft_damage_player( "gstring_spacecraft_damage_player", "5.0", FCVAR_CHEAT );

#ifdef GAME_DLL
BEGIN_DATADESC( CSpacecraftProjectile )

	DEFINE_FUNCTION( OnTouch ),
	DEFINE_THINKFUNC( OnTimeout ),

END_DATADESC()
#endif

IMPLEMENT_NETWORKCLASS_DT( CSpacecraftProjectile, CSpacecraftProjectile_DT )

#ifdef GAME_DLL
	//SendPropBool( SENDINFO( m_bHadImpact ) ),
	SendPropInt( SENDINFO( m_iImpactType ), 2, SPROP_UNSIGNED ),
	//SendPropVector( SENDINFO( m_AngularImpulse ) ),
	//SendPropVector( SENDINFO( m_PhysVelocity ) ),
#else
	//RecvPropBool( RECVINFO( m_bHadImpact ) ),
	RecvPropInt( RECVINFO( m_iImpactType ) ),
	//RecvPropVector( RECVINFO( m_AngularImpulse ) ),
	//RecvPropVector( RECVINFO( m_PhysVelocity ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( prop_spacecraft_projectile, CSpacecraftProjectile );
PRECACHE_REGISTER( prop_spacecraft_projectile );

CSpacecraftProjectile::CSpacecraftProjectile()
#ifdef CLIENT_DLL
	: m_iImpactTypeLast( 0 )
#endif
{
}

CSpacecraftProjectile::~CSpacecraftProjectile()
{
}

#ifdef GAME_DLL
void CSpacecraftProjectile::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Spacecraft.Projectile.Fire" );
	PrecacheScriptSound( "Spacecraft.Projectile.Fire.Player" );
}

void CSpacecraftProjectile::Fire( CBaseEntity *pPlayer, CBaseEntity *pVehicle,
	const Vector &vecOrigin, const Vector &vecVelocity )
{
	SetOwnerEntity( pPlayer );
	m_hVehicleOwner = pVehicle;

	Vector vecFinalVelocity = vecVelocity;
	const float flSpeed = vecFinalVelocity.NormalizeInPlace();
	vecFinalVelocity += RandomVector( -0.008f, 0.008f );
	vecFinalVelocity.NormalizeInPlace();
	vecFinalVelocity *= flSpeed;

	QAngle angles;
	VectorAngles( vecFinalVelocity, angles );

	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetSolid( SOLID_BBOX );
	const float flProjectileSize = 3.0f;
	UTIL_SetSize( this, Vector( -flProjectileSize, -flProjectileSize, -flProjectileSize ),
		Vector( flProjectileSize, flProjectileSize, flProjectileSize ) );
	SetTouch( &CSpacecraftProjectile::OnTouch );
	SetThink( &CSpacecraftProjectile::OnTimeout );
	SetNextThink( gpGlobals->curtime + 3.0f );

	SetAbsOrigin( vecOrigin );
	SetAbsVelocity( vecFinalVelocity );
	SetAbsAngles( angles );

	if ( pPlayer )
	{
		pPlayer->EmitSound( "Spacecraft.Projectile.Fire.Player" );
	}
	else
	{
		EmitSound( "Spacecraft.Projectile.Fire" );
	}
}

void CSpacecraftProjectile::OnTouch( CBaseEntity *pOther )
{
	Assert( pOther != m_hVehicleOwner );
	Assert( pOther != GetOwnerEntity() );

	trace_t tr;
	tr = BaseClass::GetTouchTrace();
	if ( tr.DidHitWorld() )
	{
		const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if ( pdata->game.material == CHAR_TEX_METAL )
		{
			Vector vecVelocity = GetAbsVelocity();
			float speed = VectorNormalize( vecVelocity );
			const float hitDot = DotProduct( tr.plane.normal, -vecVelocity );
			if ( hitDot < 0.707f )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecVelocity.Normalized();
				SetAbsVelocity( vReflection * speed );

				QAngle facing;
				VectorAngles( vReflection, facing );
				SetAbsAngles( facing );

				tr.m_pEnt = NULL;
				ImpactTrace( &tr, DMG_BLAST );
				return;
			}
		}

		if ( ( tr.surface.flags & SURF_SKY ) == 0 )
		{
			m_iImpactType = 1;
		}

		SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + gpGlobals->frametime );

		if ( pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER ) &&
			( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) ) )
		{
			m_iImpactType = 1;
			return;
		}

		if ( pOther->m_takedamage != DAMAGE_NO )
		{
			m_iImpactType = 2;
			Vector vecVelocity = GetAbsVelocity();
			vecVelocity.NormalizeInPlace();

			CTakeDamageInfo dmgInfo( this, GetOwnerEntity(), gstring_spacecraft_damage_player.GetFloat(), DMG_NEVERGIB );
			if ( GetOwnerEntity() != NULL && GetOwnerEntity()->IsPlayer() && pOther->IsNPC() )
			{
				dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			}
			CalculateMeleeDamageForce( &dmgInfo, vecVelocity, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecVelocity, &tr );
			ApplyMultiDamage();
		}
		else
		{
			m_iImpactType = 1;
		}
	}

	if ( m_iImpactType != 0 )
	{
		tr.m_pEnt = NULL;
		ImpactTrace( &tr, DMG_BLAST );
	}
}

void CSpacecraftProjectile::OnTimeout()
{
	UTIL_Remove( this );
}
#else
void CSpacecraftProjectile::OnDataChanged( DataUpdateType_t t )
{
	BaseClass::OnDataChanged( t );

	if ( t == DATA_UPDATE_CREATED )
	{
		m_hTrailParticle = ParticleProp()->Create( "projectile_red", PATTACH_ABSORIGIN_FOLLOW );

		dlight_t *el = effects->CL_AllocElight( entindex() );
		el->origin = GetAbsOrigin();

		el->color.r = 255;
		el->color.g = 96;
		el->color.b = 32;
		el->color.exponent = 9.0f;

		el->radius = random->RandomFloat( 30.0f, 100.0f );
		el->decay = el->radius / 0.05f;
		el->die = gpGlobals->curtime + 0.1f;

		// FX_TracerSound(  )
	}
	else
	{
		if ( m_iImpactTypeLast != m_iImpactType )
		{
			if ( m_hTrailParticle.GetObject() != NULL )
			{
				m_hTrailParticle->StopEmission( false, true );
				m_hTrailParticle = NULL;
			}

			if ( m_iImpactType > 0 )
			{
				const char *pszParticleName = ( m_iImpactType == 2 ) ? "projectile_red_hit_enemy" : "projectile_red_hit";
				DispatchParticleEffect( pszParticleName, GetAbsOrigin(), GetAbsAngles() );

				dlight_t *el = effects->CL_AllocElight( entindex() );
				el->origin = GetAbsOrigin();

				el->color.r = 255;
				el->color.g = 96;
				el->color.b = 32;
				el->color.exponent = 9.0f;

				el->radius = random->RandomFloat( 30.0f, 130.0f );
				el->decay = el->radius / 0.05f;
				el->die = gpGlobals->curtime + 0.1f;
			}
			m_iImpactTypeLast = m_iImpactType;
		}
	}
}
#endif