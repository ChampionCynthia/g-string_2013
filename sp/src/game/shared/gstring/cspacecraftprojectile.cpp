#include "cbase.h"
#include "cspacecraftprojectile.h"
#include "particle_parse.h"
#include "decals.h"

static ConVar gstring_spacecraft_damage_player( "gstring_spacecraft_damage_player", "5.0", FCVAR_CHEAT );

#ifdef GAME_DLL
BEGIN_DATADESC( CSpacecraftProjectile )

	DEFINE_FUNCTION( OnTouch ),
	DEFINE_THINKFUNC( OnTimeout ),

END_DATADESC()
#endif

IMPLEMENT_NETWORKCLASS_DT( CSpacecraftProjectile, CSpacecraftProjectile_DT )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bHadImpact ) ),
	//SendPropInt( SENDINFO( m_iEngineLevel ) ),
	//SendPropVector( SENDINFO( m_AngularImpulse ) ),
	//SendPropVector( SENDINFO( m_PhysVelocity ) ),
#else
	RecvPropBool( RECVINFO( m_bHadImpact ) ),
	//RecvPropInt( RECVINFO( m_iEngineLevel ) ),
	//RecvPropVector( RECVINFO( m_AngularImpulse ) ),
	//RecvPropVector( RECVINFO( m_PhysVelocity ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( prop_spacecraft_projectile, CSpacecraftProjectile );
PRECACHE_REGISTER( prop_spacecraft_projectile );

CSpacecraftProjectile::CSpacecraftProjectile()
#ifdef CLIENT_DLL
	: m_bHadImpactLast( false )
#endif
{
}

CSpacecraftProjectile::~CSpacecraftProjectile()
{
}

#ifdef GAME_DLL
void CSpacecraftProjectile::Fire( CBaseEntity *pPlayer, CBaseEntity *pVehicle,
	const Vector &vecOrigin, const Vector &vecVelocity )
{
	SetOwnerEntity( pPlayer );
	m_hVehicleOwner = pVehicle;

	Vector vecFinalVelocity = vecVelocity;
	const float flSpeed = vecFinalVelocity.NormalizeInPlace();
	vecFinalVelocity += RandomVector( -0.01f, 0.01f );
	vecFinalVelocity.NormalizeInPlace();
	vecFinalVelocity *= flSpeed;

	QAngle angles;
	VectorAngles( vecFinalVelocity, angles );

	//DebugDrawLine( vecOrigin, vecOrigin + vecVelocity, 0, 255, 0, true, 1.0f );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetSolid( SOLID_BBOX );
	const float flProjectileSize = 3.0f;
	UTIL_SetSize( this, Vector( -flProjectileSize, -flProjectileSize, -flProjectileSize ),
		Vector( flProjectileSize, flProjectileSize, flProjectileSize ) );
	SetTouch( &CSpacecraftProjectile::OnTouch );
	SetThink( &CSpacecraftProjectile::OnTimeout );
	SetNextThink( gpGlobals->curtime + 5.0f );

	Vector vecSafePosition = vecOrigin + vecFinalVelocity.Normalized() * 20.0f;
	Teleport( &vecSafePosition, &angles, &vecFinalVelocity );
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
				return;
			}
		}

		if ( ( tr.surface.flags & SURF_SKY ) == 0 )
		{
			m_bHadImpact = true;
		}

		SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
	}
	else
	{
		m_bHadImpact = true;
		SetNextThink( gpGlobals->curtime + gpGlobals->frametime );

		if ( pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER ) &&
			( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) ) )
		{
			return;
		}

		if ( pOther->m_takedamage != DAMAGE_NO )
		{
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
		}
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
	}
	else
	{
		if ( m_bHadImpactLast != m_bHadImpact )
		{
			if ( m_hTrailParticle.GetObject() != NULL )
			{
				m_hTrailParticle->StopEmission( false, true );
				m_hTrailParticle = NULL;
			}

			if ( m_bHadImpact )
			{
				DispatchParticleEffect( "projectile_red_hit", GetAbsOrigin(), GetAbsAngles() );
			}

			m_bHadImpactLast = m_bHadImpact;
		}
	}
}
#endif