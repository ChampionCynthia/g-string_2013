#ifndef SPACE_CRAFT_PROJECTILE_H
#define SPACE_CRAFT_PROJECTILE_H

#include "gstring_player_shared_forward.h"

class CSpacecraftProjectile : public CBaseEntity
{
	DECLARE_CLASS( CSpacecraftProjectile, CBaseEntity );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CSpacecraftProjectile();
	virtual ~CSpacecraftProjectile();

#ifdef GAME_DLL
	virtual int UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }

	void Fire( CBaseEntity *pPlayer, CBaseEntity *pVehicle,
		const Vector &vecOrigin, const Vector &vecVelocity );
#else
	virtual void OnDataChanged( DataUpdateType_t t );
#endif

private:
#ifdef GAME_DLL
	void OnTouch( CBaseEntity *pOther );
	void OnTimeout();

	EHANDLE m_hVehicleOwner;
#else
	CSmartPtr< CNewParticleEffect > m_hTrailParticle;

	bool m_bHadImpactLast;
#endif

	CNetworkVar( bool, m_bHadImpact );
};

#endif