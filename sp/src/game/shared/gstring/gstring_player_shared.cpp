#include "cbase.h"
#include "gstring_player_shared.h"

const Vector &CSharedPlayer::GetViewOffset() const
{
	if ( IsInSpacecraft() )
	{
		return vec3_origin;
	}

	return BaseClass::GetViewOffset();
}

bool CSharedPlayer::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	if ( IsInSpacecraft()
		|| IsInInteraction() )
	{
		return false;
	}

	return BaseClass::Weapon_CanSwitchTo( pWeapon );
}

bool CSharedPlayer::IsInInteraction() const
{
	return m_hInteractionBody != NULL;
}
