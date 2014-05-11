#include "cbase.h"
#include "cspacecraftprojectile.h"


IMPLEMENT_NETWORKCLASS_DT( CSpacecraftProjectile, CSpacecraftProjectile_DT )

#ifdef GAME_DLL
	//SendPropInt( SENDINFO( m_iEngineLevel ) ),
	//SendPropVector( SENDINFO( m_AngularImpulse ) ),
	//SendPropVector( SENDINFO( m_PhysVelocity ) ),
#else
	//RecvPropInt( RECVINFO( m_iEngineLevel ) ),
	//RecvPropVector( RECVINFO( m_AngularImpulse ) ),
	//RecvPropVector( RECVINFO( m_PhysVelocity ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( prop_spacecraft_projectile, CSpacecraftProjectile );
PRECACHE_REGISTER( prop_spacecraft_projectile );

CSpacecraftProjectile::CSpacecraftProjectile()
{

}

CSpacecraftProjectile::~CSpacecraftProjectile()
{

}