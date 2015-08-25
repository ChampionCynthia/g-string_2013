#ifndef SPACE_CRAFT_PROJECTILE_H
#define SPACE_CRAFT_PROJECTILE_H

#include "gstring_player_shared_forward.h"
#include "cspacecraft_config.h"

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

	virtual void Precache();
	virtual void Activate();
	virtual bool ShouldCollide( const CBaseEntity *pOther ) const;
	void Fire( string_t strSettingsName, CBaseEntity *pPlayer, CBaseEntity *pVehicle,
		const Vector &vecOrigin, const Vector &vecVelocity );
#else
	virtual void OnDataChanged( DataUpdateType_t t );
#endif

private:
	void UpdateConfig();

#ifdef GAME_DLL
	void OnTouch( CBaseEntity *pOther );
	void OnTimeout();

	EHANDLE m_hVehicleOwner;
	string_t m_strSettingsName;
#else
	CSmartPtr< CNewParticleEffect > m_hTrailParticle;

	int m_iImpactTypeLast;
#endif

	SpacecraftProjectileSettings_t m_Settings;

	CNetworkVar( int, m_iImpactType );
	CNetworkVar( UtlSymId_t, m_iSettingsIndex );
};

#endif