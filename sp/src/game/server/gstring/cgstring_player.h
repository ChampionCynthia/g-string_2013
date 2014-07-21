#ifndef CGSTRING_PLAYER_H
#define CGSTRING_PLAYER_H

#include "hl2_player.h"
#include "gstring/cspacecraft.h"

class CGstringPlayer : public CHL2_Player
{
	DECLARE_CLASS( CGstringPlayer, CHL2_Player );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
public:

	CGstringPlayer();

	void Precache();
	void Spawn();

	bool IsNightvisionActive() const;
	void SetNightvisionActive( bool bActive );

	virtual void ImpulseCommands();
	virtual void PhysicsSimulate();

	virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	virtual const Vector &GetViewOffset() const;

	virtual bool ShouldGib( const CTakeDamageInfo &info );
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual bool CanBecomeRagdoll();
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	virtual void DoReloadAnim();

	virtual bool ClientCommand( const CCommand &args );

	void EnterSpacecraft( CSpacecraft *pSpacecraft );
	bool IsInSpacecraft() const;
	void ExitSpacecraft();
	CSpacecraft *GetSpacecraft();

	virtual bool Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );
	virtual void StartAdmireGlovesAnimation();

	void BeginInteraction( CBaseAnimating *pInteractionBody );
	void EndInteraction();
	bool IsInInteraction() const;

protected:

private:
	void ToggleNightvision();
	CNetworkVar( bool, m_bNightvisionActive );

	CNetworkVar( bool, m_bHasUseEntity );
	CNetworkVar( unsigned char, m_nReloadParity );

	CNetworkHandle( CSpacecraft, m_hSpacecraft );
	CNetworkVar( bool, m_bSpacecraftDeath );

	CNetworkHandle( CBaseAnimating, m_hInteractionBody );
};

inline CGstringPlayer *ToGstringPlayer( CBaseEntity *pPlayer )
{
	return assert_cast< CGstringPlayer* >( pPlayer );
}

inline CGstringPlayer *LocalGstringPlayer()
{
	return ToGstringPlayer( UTIL_GetLocalPlayer() );
}

#endif