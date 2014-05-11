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
	CSharedPlayer *pPlayer = ( CSharedPlayer* )player;
	CSpacecraft *pSpacecraft = pPlayer->GetSpacecraft();
	pSpacecraft->SimulateMove( *mv );
}

