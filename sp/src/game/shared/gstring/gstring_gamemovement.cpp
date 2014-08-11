#include "cbase.h"
#include "gstring_gamemovement.h"
#include "in_buttons.h"
#include "hl2_shareddefs.h"
#include "cspacecraft.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CGstringGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement );

CGstringGameMovement::CGstringGameMovement()
{
}

void CGstringGameMovement::Duck()
{
	CSharedPlayer *pPlayer = (CSharedPlayer*)player;

	if ( !pPlayer->IsInSpacecraft() )
	{
		BaseClass::Duck();
	}
}

void CGstringGameMovement::ProcessMoveType()
{
	CSharedPlayer *pPlayer = (CSharedPlayer*)player;

	if ( pPlayer->IsInInteraction() )
	{
		return;
	}

	if ( pPlayer->IsInSpacecraft() )
	{
		SpacecraftMove();
	}
	else
	{
		BaseClass::ProcessMoveType();
	}
}

void CGstringGameMovement::SpacecraftMove()
{
	float flFrametime = gpGlobals->frametime;
	if ( flFrametime <= 0.0f )
	{
		return;
	}

	flFrametime = MIN( 0.25f, flFrametime );

	CSharedPlayer *pPlayer = ( CSharedPlayer* )player;
	CSpacecraft *pSpacecraft = pPlayer->GetSpacecraft();
	pSpacecraft->SimulateMove( *mv, flFrametime );
}

