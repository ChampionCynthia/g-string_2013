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
			pEnemyShip->KeyValue( "settingsname", "ricepod" );

			CSpacecraftAIBase *pAI = new CSpacecraftAIBase( pEnemyShip );
			pEnemyShip->SetAI( pAI );

			DispatchSpawn( pEnemyShip );
			pEnemyShip->Activate();
		}
	}
}

extern ConVar gstring_spacecraft_move_mode;

CSpacecraftAIBase::CSpacecraftAIBase( CSpacecraft *pShip )
	: m_pShip( pShip )
	, m_ThinkFunc( NULL )
	, m_MoveFunc( NULL )
	, m_flNextThink( 0.0f )
	, m_vecMoveTarget( vec3_origin )
	, m_flRotationSuppressTimer( -1.0f )
	, m_flRotationSpeedBlend( 1.0f )
	, m_flSideTimer( 0.0f )
	, m_flSideScale( 1.0f )
{
	moveData.m_nButtons = 0;
	moveData.m_iAutoAimEntityIndex = 0;
	moveData.m_vecWorldShootPosition.Init();
	moveData.m_vecViewAngles.Init();
	moveData.m_flForwardMove = 0.0f;
	moveData.m_flSideMove = 0.0f;

	SetNextThink( 0.0f, &CSpacecraftAIBase::Think_ShootSalvoes );
	//SetMove( &CSpacecraftAIBase::Move_Follow );
	SetMove( &CSpacecraftAIBase::Move_Pursuit );
}

CSpacecraftAIBase::~CSpacecraftAIBase()
{
}

void CSpacecraftAIBase::Run( float flFrametime )
{
	if ( m_flNextThink >= 0.0f )
	{
		m_flNextThink -= flFrametime;
		if ( m_flNextThink <= 0.0f )
		{
			m_flNextThink = -1.0f;
			if ( m_ThinkFunc != NULL )
			{
				(this->*m_ThinkFunc)();
			}
		}
	}

	if ( m_MoveFunc != NULL )
	{
		(this->*m_MoveFunc)( flFrametime );
	}

	m_pShip->SimulateMove( moveData, flFrametime );
}

CBaseEntity *CSpacecraftAIBase::GetEnemy()
{
	CBaseEntity *pEnemy = m_pShip->GetEnemy();
	if ( pEnemy == NULL )
	{
		CGstringPlayer *pPlayer = LocalGstringPlayer();
		if ( pPlayer && pPlayer->IsInSpacecraft() )
		{
			pEnemy = pPlayer->GetSpacecraft();
			m_pShip->SetEnemy( pEnemy );
		}
	}

	return pEnemy;
}

void CSpacecraftAIBase::SetNextThink( float flDelay, AIThink thinkFunc )
{
	m_flNextThink = MAX( 0.0f, flDelay );
	if ( thinkFunc != NULL )
	{
		m_ThinkFunc = thinkFunc;
	}
}

void CSpacecraftAIBase::ClearThink()
{
	m_flNextThink = -1.0f;
	m_ThinkFunc = NULL;
}

void CSpacecraftAIBase::SetMove( AIMove moveFunc )
{
	m_MoveFunc = moveFunc;
}

void CSpacecraftAIBase::Think_ShootSalvoes()
{
	moveData.m_nButtons ^= IN_ATTACK;

	SetNextThink( RandomFloat( 1.0f, 4.0f ) );
}

void CSpacecraftAIBase::Move_Follow( float flFrametime )
{
	CBaseEntity *pEnemy( GetEnemy() );
	if ( pEnemy == NULL )
	{
		return;
	}

	Vector vecEnemy = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();
	VectorAngles( vecEnemy, Vector( 0.0f, 0.0f, 1.0f ), moveData.m_vecViewAngles );

	const float flDistance = vecEnemy.Length();

	moveData.m_flSideMove = 0.0f;
	moveData.m_flUpMove = 0.0f;

	if ( gstring_spacecraft_move_mode.GetBool() )
	{
		moveData.m_flForwardMove = 0.0f;
		if ( flDistance < 50.0f )
		{
			moveData.m_flSideMove = 50.0f;
		}
	}
	else
	{
		if ( flDistance > 300.0f )
		{
			moveData.m_flForwardMove = 200.0f;
		}
		else if ( flDistance < 50.0f )
		{
			moveData.m_flForwardMove = -200.0f;
			moveData.m_flSideMove = 50.0f;
		}
	}

	if ( flDistance > 400.0f )
	{
		moveData.m_nButtons |= IN_SPEED;
	}
	else
	{
		moveData.m_nButtons &= ~IN_SPEED;
	}

	const float flEnemyDistance = vecEnemy.Length();
	const float flSpread = RemapValClamped( flEnemyDistance, 5.0f, 800.0f, 0.0f, 50.0f );
	moveData.m_vecWorldShootPosition = pEnemy->GetAbsOrigin() + RandomVector( -flSpread, flSpread );
}

void CSpacecraftAIBase::Move_Pursuit( float flFrametime )
{
	CBaseEntity *pEnemy( GetEnemy() );
	if ( pEnemy == NULL )
	{
		return;
	}

	Vector vecOrigin = m_pShip->GetAbsOrigin();
	Vector vecEnemy = pEnemy->GetAbsOrigin() - vecOrigin;

	if ( vecEnemy.LengthSqr() > Sqr( 200.0f ) )
	{
		Vector vecEnemyToTarget = m_vecMoveTarget - pEnemy->GetAbsOrigin();
		Vector vecSelfToEnemyDirection = vecEnemy.Normalized();
		m_vecMoveTarget -= DotProduct( vecSelfToEnemyDirection, vecEnemyToTarget ) * vecSelfToEnemyDirection;

		DebugDrawLine( pEnemy->GetAbsOrigin(), vecOrigin, 255, 0, 0, true, -1.0f );
		DebugDrawLine( m_vecMoveTarget, vecOrigin, 0, 255, 0, true, -1.0f );
	}

	const float flDistanceTargetToEnemy = ( m_vecMoveTarget - pEnemy->GetAbsOrigin() ).Length();
	if ( flDistanceTargetToEnemy < 30.0f || flDistanceTargetToEnemy > 120.0f )
	{
		QAngle angEnemyOrientation;
		Vector vecEnemyUp;
		VectorAngles( vecEnemy, angEnemyOrientation );
		angEnemyOrientation.z += RandomFloat( 0.0f, 360.0f );
		AngleVectors( angEnemyOrientation, NULL, NULL, &vecEnemyUp );
		m_vecMoveTarget = pEnemy->GetAbsOrigin() + vecEnemyUp * RandomFloat( 40.0f, 80.0f );
	}

	vecEnemy = m_vecMoveTarget - m_pShip->GetAbsOrigin();
	QAngle angMove;
	VectorAngles( vecEnemy, angMove );

	if ( ( m_vecMoveTarget - m_pShip->GetAbsOrigin() ).LengthSqr() < Sqr( 50.0f ) )
	{
		m_flRotationSuppressTimer = gpGlobals->curtime + RandomFloatExp( 2.0f, 6.0f, 2.0f );
		m_flRotationSpeedBlend = 0.0f;

		if ( RandomInt( 0, 3 ) == 0 )
		{
			m_flSideTimer = gpGlobals->curtime + RandomFloat( 2.0f, 4.0f );
			m_flSideScale = RandomFloat( 0.0f, 1.0f ) > 0.5f ? 1.0f : -1.0f;
		}
	}

	const float flRotationSpeedBlendDesried = ( m_flRotationSuppressTimer < gpGlobals->curtime ) ? 1.0f : 0.0f;
	m_flRotationSpeedBlend = Approach( flRotationSpeedBlendDesried, m_flRotationSpeedBlend, gpGlobals->frametime * 0.5f );

	if ( m_flRotationSpeedBlend > 0.0f )
	{
		Quaternion qMoveDesired, qMoveCurrent;
		AngleQuaternion( angMove, qMoveDesired );
		AngleQuaternion( moveData.m_vecViewAngles, qMoveCurrent );
		QuaternionSlerp( qMoveCurrent, qMoveDesired, gpGlobals->frametime * m_flRotationSpeedBlend * 5.0f, qMoveCurrent );
		QuaternionAngles( qMoveCurrent, moveData.m_vecViewAngles );
	}

	if ( gstring_spacecraft_move_mode.GetBool() )
	{
		moveData.m_flForwardMove = 0.0f;
	}
	else
	{
		moveData.m_flForwardMove = 200.0f;
	}

	moveData.m_flUpMove = 0.0f;
	moveData.m_nButtons |= IN_SPEED;

	Vector vecOpponent = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();
	Vector vecFwd;
	AngleVectors( moveData.m_vecViewAngles, &vecFwd );

	const float flShootDistance = RemapValClamped( vecOpponent.Length(), 30.0f, 500.0f, 1.0f, 5.0f );
	moveData.m_vecWorldShootPosition = pEnemy->GetAbsOrigin() + RandomVector( -flShootDistance, flShootDistance );

	if ( DotProduct( vecOpponent, vecFwd ) < 0.0f )
	{
		moveData.m_nButtons &= ~IN_ATTACK;
		m_flSideTimer = 0.0f;
	}
	else if ( m_flSideTimer > gpGlobals->curtime )
	{
		moveData.m_flForwardMove = 0.0f;
		moveData.m_flSideMove = 200.0f * m_flSideScale;
	}
}
