#include "cbase.h"

#ifdef CLIENT_DLL
#include "dlight.h"
#include "iefx.h"
#endif

#include "cspacecraftprojectile.h"
#include "decals.h"
#include "particle_parse.h"

extern ConVar gstring_space_exterior_sounds;

#ifdef GAME_DLL
BEGIN_DATADESC( CSpacecraftProjectile )

	DEFINE_FIELD( m_strSettingsName, FIELD_STRING ),

	DEFINE_FUNCTION( OnTouch ),
	DEFINE_THINKFUNC( OnTimeout ),

END_DATADESC()
#endif

IMPLEMENT_NETWORKCLASS_DT( CSpacecraftProjectile, CSpacecraftProjectile_DT )

#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_iImpactType ), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSettingsIndex ) ),
#else
	RecvPropInt( RECVINFO( m_iImpactType ) ),
	RecvPropInt( RECVINFO( m_iSettingsIndex ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( prop_spacecraft_projectile, CSpacecraftProjectile );
PRECACHE_REGISTER( prop_spacecraft_projectile );

CSpacecraftProjectile::CSpacecraftProjectile()
#ifdef CLIENT_DLL
	: m_iImpactTypeLast( 0 )
#endif
{
	m_iSettingsIndex = UTL_INVAL_SYMBOL;
}

CSpacecraftProjectile::~CSpacecraftProjectile()
{
#ifdef CLIENT_DLL
	if ( m_hTrailParticle.GetObject() != NULL )
	{
		m_hTrailParticle->StopEmission( false, true );
		m_hTrailParticle = NULL;
	}
#endif
}

void CSpacecraftProjectile::UpdateConfig()
{
	const CSpacecraftConfig *pConfig = CSpacecraftConfig::GetInstance();
#ifdef GAME_DLL
	m_iSettingsIndex = pConfig->GetSettingsIndex( STRING( m_strSettingsName ) );
#endif
	m_Settings = pConfig->GetProjectileSettings( m_iSettingsIndex );
}

#ifdef GAME_DLL
void CSpacecraftProjectile::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Spacecraft.Projectile.Fire" );
	PrecacheScriptSound( "Spacecraft.Projectile.Fire.Player" );
}

void CSpacecraftProjectile::Activate()
{
	BaseClass::Activate();
	UpdateConfig();
}

bool CSpacecraftProjectile::ShouldCollide( const CBaseEntity *pOther ) const
{
	if ( pOther == m_hVehicleOwner )
	{
		return false;
	}
	return BaseClass::ShouldCollide( pOther );
}

void CSpacecraftProjectile::Fire( string_t strSettingsName, CBaseEntity *pPlayer, CBaseEntity *pVehicle,
	const Vector &vecOrigin, const Vector &vecVelocity )
{
	m_strSettingsName = strSettingsName;
	UpdateConfig();

	SetOwnerEntity( pPlayer ? pPlayer : pVehicle );
	m_hVehicleOwner = pVehicle;

	Vector vecFinalVelocity = vecVelocity;
	const float flSpeed = vecFinalVelocity.NormalizeInPlace();
	vecFinalVelocity += RandomVector( -m_Settings.m_flSpread, m_Settings.m_flSpread );
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
	else if ( gstring_space_exterior_sounds.GetBool() )
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
	CBaseEntity *pHitEntity = NULL;
	bool bCheckReflect = false;
	if ( tr.DidHitWorld() )
	{
		const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if ( pdata->game.material == CHAR_TEX_METAL )
		{
			bCheckReflect = true;
			m_iImpactType = 1;
		}
		else if ( ( tr.surface.flags & SURF_SKY ) == 0 )
		{
			m_iImpactType = 1;
		}
	}
	else
	{
		const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if ( pdata != NULL && pdata->game.material == CHAR_TEX_METAL )
		{
			bCheckReflect = true;
		}

		if ( pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER ) &&
			( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) ) )
		{
			m_iImpactType = 1;
			SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
			return;
		}

		if ( pOther->m_takedamage != DAMAGE_NO )
		{
			m_iImpactType = 2;
			pHitEntity = pOther;
			Vector vecVelocity = GetAbsVelocity();
			vecVelocity.NormalizeInPlace();

			CTakeDamageInfo dmgInfo( this, GetOwnerEntity(), m_Settings.m_flDamage, DMG_BLAST );
			if ( GetOwnerEntity() != NULL && GetOwnerEntity()->IsPlayer() && pOther->IsNPC() )
			{
				dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			}
			if ( vecVelocity.LengthSqr() < 1.0f )
			{
				vecVelocity = RandomVector( -0.5f, 0.5f );
			}
			dmgInfo.SetDamageForce( vecVelocity * 7500.0f );
			//CalculateMeleeDamageForce( &dmgInfo, vecVelocity, tr.endpos, 2.0f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecVelocity, &tr );
			ApplyMultiDamage();

			if ( pOther->m_takedamage == DAMAGE_YES )
			{
				bCheckReflect = false;
			}
		}
		else
		{
			m_iImpactType = 1;
		}
	}

	if ( m_iImpactType != 0 )
	{
		tr.m_pEnt = pHitEntity;
		tr.endpos = tr.endpos + ( tr.endpos - tr.startpos ).Normalized() * 3.0f;
		ImpactTrace( &tr, DMG_BLAST );
	}

	if ( bCheckReflect )
	{
		Vector vecVelocity = GetAbsVelocity();
		float speed = VectorNormalize( vecVelocity );
		const float hitDot = DotProduct( tr.plane.normal, -vecVelocity );
		if ( speed > 0.0f && tr.plane.normal.LengthSqr() > 0.5f && hitDot < 0.707f )
		{
			Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecVelocity.Normalized();
			Assert( vReflection.LengthSqr() > 0.5f && speed > 0.0f );
			SetAbsVelocity( vReflection * speed );

			QAngle facing;
			VectorAngles( vReflection, facing );
			SetAbsAngles( facing );

			m_iImpactType = 0;
			return;
		}
	}

	SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
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
		UpdateConfig();
		m_hTrailParticle = ParticleProp()->Create( m_Settings.m_strParticleTrail, PATTACH_ABSORIGIN_FOLLOW );

		//dlight_t *el = effects->CL_AllocElight( entindex() );
		//el->origin = GetAbsOrigin();

		//el->color.r = 255;
		//el->color.g = 96;
		//el->color.b = 32;
		//el->color.exponent = 5.0f;

		//el->radius = random->RandomFloat( 30.0f, 100.0f );
		//el->decay = el->radius / 0.05f;
		//el->die = gpGlobals->curtime + 0.1f;

		// FX_TracerSound(  )
		extern bool FX_AffectRagdolls( Vector vecOrigin, Vector vecStart, int iDamageType );
		QAngle angles = GetAbsAngles();
		Vector vecForward;
		AngleVectors( angles, &vecForward );
		FX_AffectRagdolls( GetAbsOrigin() + vecForward * 512.0f, GetAbsOrigin(), DMG_BLAST );
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