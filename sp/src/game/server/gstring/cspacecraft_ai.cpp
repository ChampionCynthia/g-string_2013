#include "cbase.h"
#include "cgstring_player.h"
#include "cspacecraft_ai.h"
#include "in_buttons.h"

CON_COMMAND( gstring_spaceshipai_debug_spawn, "" )
{
	CGstringPlayer *pPlayer = LocalGstringPlayer();

	if ( pPlayer == NULL )
		return;

	Vector vecFwd;
	AngleVectors( pPlayer->GetAbsAngles(), &vecFwd );

	Vector start = pPlayer->GetAbsOrigin();
	Vector end = start + vecFwd * 400.0f;

	const Vector vecHull( 40, 40, 40 );

	CTraceFilterSkipTwoEntities filter( pPlayer, pPlayer->GetSpacecraft(), COLLISION_GROUP_NPC );

	trace_t tr;
	UTIL_TraceHull( start, end, -vecHull, vecHull, MASK_SOLID, &filter, &tr );

	if ( !tr.startsolid && tr.fraction > 0.2f )
	{
		CSpacecraft *pEnemyShip = assert_cast< CSpacecraft* >( CreateEntityByName( "prop_vehicle_spacecraft" ) );
		if ( pEnemyShip != NULL )
		{
			pEnemyShip->SetAbsOrigin( tr.endpos );
			pEnemyShip->KeyValue( "model", "models/bioproto/spacecraft.mdl" );

			CSpacecraftAIBase *pAI = new CSpacecraftAIBase( pEnemyShip );
			pEnemyShip->SetAI( pAI );

			DispatchSpawn( pEnemyShip );
			pEnemyShip->Activate();
		}
	}
}

CSpacecraftAIBase::CSpacecraftAIBase( CSpacecraft *pShip )
	: m_pShip( pShip )
	, m_pEnemy( NULL )
{
}

CSpacecraftAIBase::~CSpacecraftAIBase()
{
}

void CSpacecraftAIBase::Run( float flFrametime )
{
	CBaseEntity *pEnemy( GetEnemy() );

	Vector vecEnemy = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();

	CMoveData moveData;
	moveData.m_nButtons = 0;

	moveData.m_nButtons |= IN_FORWARD;

	VectorAngles( vecEnemy, moveData.m_vecViewAngles );

	m_pShip->SimulateMove( moveData, flFrametime );
}

CBaseEntity *CSpacecraftAIBase::GetEnemy()
{
	if ( m_pEnemy == NULL )
	{
		m_pEnemy = LocalGstringPlayer();
	}

	return m_pEnemy;
}
