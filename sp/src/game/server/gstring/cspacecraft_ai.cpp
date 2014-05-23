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
	, m_aiState( STATE_FOLLOW_AND_SHOOT )
	, m_flShootDelay( 0.0f )
	, m_flShootCooldown( -1.0f )
{
	moveData.m_nButtons = 0;
	moveData.m_iAutoAimEntityIndex = 0;
	moveData.m_vecWorldShootPosition.Init();
	moveData.m_vecViewAngles.Init();
}

CSpacecraftAIBase::~CSpacecraftAIBase()
{
}

void CSpacecraftAIBase::Run( float flFrametime )
{
	switch ( m_aiState )
	{
	case STATE_FOLLOW_AND_SHOOT:
		StateFollowAndShoot( flFrametime );
		break;
	}

	/*
	Vector vecEnemy = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();
	moveData.m_nButtons |= IN_FORWARD;

	moveData.m_nButtons |= IN_ATTACK;

	if ( m_flShootTime > 0.0f )
	{
		m_flShootTime -= flFrametime;

		moveData.m_vecWorldShootPosition = pEnemy->GetAbsOrigin() + RandomVector( -50, 50 );
	}

	if ( m_flEngineCutOffTime > 0.0f )
	{
		moveData.m_nButtons |= IN_JUMP;
		m_flEngineCutOffTime -= flFrametime;
	}

	VectorAngles( vecEnemy, moveData.m_vecViewAngles );
	*/

	m_pShip->SimulateMove( moveData, flFrametime );
}

CBaseEntity *CSpacecraftAIBase::GetEnemy()
{
	if ( m_pEnemy == NULL )
	{
		CGstringPlayer *pPlayer = LocalGstringPlayer();
		if ( pPlayer && pPlayer->IsInSpacecraft() )
		{
			m_pEnemy = pPlayer->GetSpacecraft();
		}
	}

	return m_pEnemy;
}

void CSpacecraftAIBase::StateFollowAndShoot( float flFrametime )
{
	CBaseEntity *pEnemy( GetEnemy() );

	Vector vecEnemy = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();
	moveData.m_nButtons |= IN_FORWARD;

	moveData.m_nButtons &= ~IN_ATTACK;

	m_flShootDelay -= flFrametime;
	if ( m_flShootDelay > 0.0f )
	{
		moveData.m_nButtons |= IN_ATTACK;
	}
	else if ( m_flShootDelay < m_flShootCooldown )
	{
		m_flShootDelay = RandomFloat( 1.0f, 4.0f );
		m_flShootCooldown = RandomFloat( -3.0f, -0.5f );
	}

	const float flEnemyDistance = vecEnemy.Length();
	const float flSpread = RemapValClamped( flEnemyDistance, 5.0f, 1000.0f, 0.0f, 50.0f );
	moveData.m_vecWorldShootPosition = pEnemy->GetAbsOrigin() + RandomVector( -flSpread, flSpread );

	VectorAngles( vecEnemy, moveData.m_vecViewAngles );
}
