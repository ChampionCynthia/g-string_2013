#include "cbase.h"
#include "cgstring_player.h"
#include "cspacecraft_ai.h"
#include "in_buttons.h"
#include "gstring/gstring_util.h"

#include "spaceai/statemachine.h"

extern bool g_bAIDisabledByUser;

CON_COMMAND(gstring_spaceshipai_debug_spawn, "")
{
	CGstringPlayer *pPlayer = LocalGstringPlayer();

	if (pPlayer == NULL)
		return;

	bool bFollowLeader = args.ArgC() > 1 && args.FindArg("-follow") != NULL;

	Vector vecFwd;
	AngleVectors(pPlayer->GetAbsAngles(), &vecFwd);

	Vector start = pPlayer->GetAbsOrigin();
	Vector end = start + vecFwd * 400.0f;

	const Vector vecHull(40, 40, 40);

	CTraceFilterSkipTwoEntities filter(pPlayer, pPlayer->GetSpacecraft(), COLLISION_GROUP_NPC);

	trace_t tr;
	UTIL_TraceHull(start, end, -vecHull, vecHull, MASK_SOLID, &filter, &tr);

	if (!tr.startsolid && tr.fraction > 0.2f)
	{
		CSpacecraft *pEnemyShip = assert_cast<CSpacecraft*>(CreateEntityByName("prop_vehicle_spacecraft"));
		if (pEnemyShip != NULL)
		{
			pEnemyShip->SetAbsOrigin(tr.endpos);
			pEnemyShip->KeyValue("settingsname", "ricepod");

			CSpacecraftAIBase *pAI = new CSpacecraftAIBase(pEnemyShip);
			if (bFollowLeader)
			{
				//pAI->
			}
			pEnemyShip->SetAI(pAI);

			DispatchSpawn(pEnemyShip);
			pEnemyShip->Activate();
		}
	}
}

extern ConVar gstring_spacecraft_move_mode;

CSpacecraftAIBase::CSpacecraftAIBase(CSpacecraft *pShip)
	: m_pShip(pShip)
	, m_flEnemyUpdateTimer(0.0f)
	//, m_pStateMachine( NULL )
	, m_ThinkFunc(NULL)
	, m_MoveFunc(NULL)
	, m_flNextThink(0.0f)
	, m_vecMoveTarget(vec3_origin)
	, m_flRotationSuppressTimer(-1.0f)
	, m_flRotationSpeedBlend(1.0f)
	, m_flRotationLockTimer(0.0f)
	, m_flSideTimer(0.0f)
	, m_flSideScale(1.0f)
	, m_flForwardTimer(0.0f)
{
	moveData.m_nButtons = 0;
	moveData.m_iAutoAimEntityIndex = 0;
	moveData.m_vecWorldShootPosition.Init();
	moveData.m_vecViewAngles.Init();
	moveData.m_flForwardMove = 0.0f;
	moveData.m_flSideMove = 0.0f;

	//SetNextThink(0.0f, &CSpacecraftAIBase::Think_ShootSalvoes);
	//SetMove( &CSpacecraftAIBase::Move_Follow );
	//SetMove(&CSpacecraftAIBase::Move_Pursuit);

	//m_state.moveData.m_nButtons = 0;
	//m_state.moveData.m_flForwardMove = 0.0f;
	//m_state.moveData.m_flSideMove = 0.0f;
	//m_state.moveData.m_flUpMove = 0.0f;
	//m_state.moveData.m_vecViewAngles = pShip->GetAbsAngles();
	//m_state.moveData.m_iAutoAimEntityIndex = 0;
	//m_state.moveData.m_vecWorldShootPosition.Init();
}

CSpacecraftAIBase::~CSpacecraftAIBase()
{
	//if ( m_pStateMachine )
	//{
	//	m_pStateMachine->Delete();
	//}
}

void CSpacecraftAIBase::EnterState(AISTATE_e state)
{
	switch (state)
	{
	case AISTATE_IDLE:
		SetNextThink(RandomFloat(0.25f, 0.6f), &CSpacecraftAIBase::Think_Idle);
		SetMove(&CSpacecraftAIBase::Move_Idle);
		break;

	case AISTATE_ATTACK_AND_CHASE:
		SetNextThink(0.0f, &CSpacecraftAIBase::Think_ShootSalvoes);
		SetMove(&CSpacecraftAIBase::Move_Pursuit);
		break;

	case AISTATE_ATTACK_AND_IDLE:
		SetNextThink(0.0f, &CSpacecraftAIBase::Think_ShootSalvoes);
		SetMove(&CSpacecraftAIBase::Move_AttackStationary);
		break;

	case AISTATE_APPROACH_TARGET:
		SetNextThink(RandomFloat(0.25f, 0.6f), &CSpacecraftAIBase::Think_Idle);
		SetMove(&CSpacecraftAIBase::Move_FollowPath);
		break;

	default:
		Assert(0);
	}
}

void CSpacecraftAIBase::Run(float flFrametime)
{
	if (!g_bAIDisabledByUser)
	{
		if (m_flEnemyUpdateTimer < gpGlobals->curtime)
		{
			float flUpdateDelay = 2.0f;
			UpdateEnemy(flUpdateDelay);
			m_flEnemyUpdateTimer = gpGlobals->curtime + flUpdateDelay;
		}

		if (m_flNextThink >= 0.0f)
		{
			m_flNextThink -= flFrametime;
			if (m_flNextThink <= 0.0f)
			{
				m_flNextThink = -1.0f;
				if (m_ThinkFunc != NULL)
				{
					(this->*m_ThinkFunc)();
				}
			}
		}

		if (m_MoveFunc != NULL)
		{
			(this->*m_MoveFunc)(flFrametime);
		}
	}
	else
	{
		moveData.m_flUpMove = 0.0f;
		moveData.m_flSideMove = 0.0f;
		moveData.m_flForwardMove = 0.0f;
		moveData.m_nButtons = 0;
	}

	Fire_UpdateProjectilePosition(flFrametime);

	m_pShip->SimulateMove(moveData, flFrametime);

	moveData.m_nOldButtons = moveData.m_nButtons;
}

CBaseEntity *CSpacecraftAIBase::GetEnemy()
{
	CBaseEntity *pEnemy = m_pShip->GetEnemy();
	if (pEnemy == NULL)
	{
		pEnemy = m_hTemporaryEnemy;
	}
	return pEnemy;
}

void CSpacecraftAIBase::SetNextThink(float flDelay, AIThink thinkFunc)
{
	m_flNextThink = MAX(0.0f, flDelay);
	if (thinkFunc != NULL)
	{
		m_ThinkFunc = thinkFunc;
	}
}

void CSpacecraftAIBase::ClearThink()
{
	m_flNextThink = -1.0f;
	m_ThinkFunc = NULL;
}

void CSpacecraftAIBase::SetMove(AIMove moveFunc)
{
	m_MoveFunc = moveFunc;
}

void CSpacecraftAIBase::Think_Idle()
{
	moveData.m_nButtons &= ~IN_ATTACK;
}

void CSpacecraftAIBase::Think_ShootSalvoes()
{
	if (GetEnemy() == NULL)
	{
		moveData.m_nButtons &= ~IN_ATTACK;
		SetNextThink(RandomFloat(1.0f, 1.5f));
		return;
	}

	moveData.m_nButtons ^= IN_ATTACK;
	SetNextThink(RandomFloat(1.0f, 4.0f));
}

void CSpacecraftAIBase::Move_Idle(float flFrametime)
{
	moveData.m_flSideMove = 0.0f;
	moveData.m_flUpMove = 0.0f;
	moveData.m_flForwardMove = 0.0f;

	if (GetEnemy() != NULL)
	{
		EnterState(ISpacecraftAI::AISTATE_ATTACK_AND_CHASE);
	}
}

void CSpacecraftAIBase::Move_Follow(float flFrametime)
{
	CBaseEntity *pEnemy(GetEnemy());
	if (pEnemy == NULL)
	{
		return;
	}

	Vector vecEnemy = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();
	VectorAngles(vecEnemy, Vector(0.0f, 0.0f, 1.0f), moveData.m_vecViewAngles);

	const float flDistance = vecEnemy.Length();

	moveData.m_flSideMove = 0.0f;
	moveData.m_flUpMove = 0.0f;

	if (gstring_spacecraft_move_mode.GetBool())
	{
		moveData.m_flForwardMove = 0.0f;
		if (flDistance < 50.0f)
		{
			moveData.m_flSideMove = 50.0f;
		}
	}
	else
	{
		if (flDistance > 300.0f)
		{
			moveData.m_flForwardMove = 200.0f;
		}
		else if (flDistance < 50.0f)
		{
			moveData.m_flForwardMove = -200.0f;
			moveData.m_flSideMove = 50.0f;
		}
	}

	if (flDistance > 400.0f)
	{
		moveData.m_nButtons |= IN_SPEED;
	}
	else
	{
		moveData.m_nButtons &= ~IN_SPEED;
	}

	const float flEnemyDistance = vecEnemy.Length();
	const float flSpread = RemapValClamped(flEnemyDistance, 5.0f, 800.0f, 0.0f, 50.0f);
	moveData.m_vecWorldShootPosition = pEnemy->GetAbsOrigin() + RandomVector(-flSpread, flSpread);
}

void CSpacecraftAIBase::Move_Pursuit(float flFrametime)
{
	CBaseEntity *pEnemy(GetEnemy());
	if (pEnemy == NULL)
	{
		EnterState(ISpacecraftAI::AISTATE_IDLE);
		return;
	}

	Vector vecOrigin = m_pShip->GetAbsOrigin();
	Vector vecEnemy = pEnemy->GetAbsOrigin() - vecOrigin;

	if (vecEnemy.LengthSqr() > Sqr(200.0f))
	{
		Vector vecEnemyToTarget = m_vecMoveTarget - pEnemy->GetAbsOrigin();
		Vector vecSelfToEnemyDirection = vecEnemy.Normalized();
		m_vecMoveTarget -= DotProduct(vecSelfToEnemyDirection, vecEnemyToTarget) * vecSelfToEnemyDirection;

		//DebugDrawLine( pEnemy->GetAbsOrigin(), vecOrigin, 255, 0, 0, true, -1.0f );
		//DebugDrawLine( m_vecMoveTarget, vecOrigin, 0, 255, 0, true, -1.0f );
	}

	const float flDistanceTargetToEnemy = (m_vecMoveTarget - pEnemy->GetAbsOrigin()).Length();
	if (flDistanceTargetToEnemy < 30.0f || flDistanceTargetToEnemy > 120.0f)
	{
		QAngle angEnemyOrientation;
		Vector vecEnemyUp;
		VectorAngles(vecEnemy, angEnemyOrientation);
		angEnemyOrientation.z += RandomFloat(0.0f, 360.0f);
		AngleVectors(angEnemyOrientation, NULL, NULL, &vecEnemyUp);
		m_vecMoveTarget = pEnemy->GetAbsOrigin() + vecEnemyUp * RandomFloat(40.0f, 80.0f);
	}

	vecEnemy = m_vecMoveTarget - m_pShip->GetAbsOrigin();
	QAngle angMove;
	VectorAngles(vecEnemy, angMove);

	if ((m_vecMoveTarget - m_pShip->GetAbsOrigin()).LengthSqr() < Sqr(50.0f))
	{
		m_flRotationSuppressTimer = gpGlobals->curtime + RandomFloatExp(2.0f, 6.0f, 2.0f);
		m_flRotationSpeedBlend = 0.0f;

		if (RandomInt(0, 3) == 0)
		{
			m_flSideTimer = gpGlobals->curtime + RandomFloat(2.0f, 4.0f);
			m_flSideScale = RandomFloat(0.0f, 1.0f) > 0.5f ? 1.0f : -1.0f;
		}
	}

	const float flRotationSpeedBlendDesried = (m_flRotationSuppressTimer < gpGlobals->curtime) ? 1.0f : 0.0f;
	m_flRotationSpeedBlend = Approach(flRotationSpeedBlendDesried, m_flRotationSpeedBlend, gpGlobals->frametime * 0.5f);

	if (m_flRotationSpeedBlend > 0.0f)
	{
		float flRotationSpeedMultiplier = 5.0f;
		if (m_flRotationLockTimer > gpGlobals->curtime)
		{
			flRotationSpeedMultiplier = 0.0f;
		}
		Quaternion qMoveDesired, qMoveCurrent;
		AngleQuaternion(angMove, qMoveDesired);
		AngleQuaternion(moveData.m_vecViewAngles, qMoveCurrent);
		QuaternionSlerp(qMoveCurrent, qMoveDesired, gpGlobals->frametime * m_flRotationSpeedBlend * flRotationSpeedMultiplier, qMoveCurrent);
		QuaternionAngles(qMoveCurrent, moveData.m_vecViewAngles);
	}

	if (gstring_spacecraft_move_mode.GetBool())
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
	AngleVectors(moveData.m_vecViewAngles, &vecFwd);

	if (DotProduct(vecOpponent, vecFwd) < 0.0f)
	{
		moveData.m_nButtons &= ~IN_ATTACK;
		m_flSideTimer = 0.0f;
	}
	else if (m_flSideTimer > gpGlobals->curtime)
	{
		moveData.m_flForwardMove = 0.0f;
		moveData.m_flSideMove = 200.0f * m_flSideScale;
	}
	else if (m_flSideTimer + 2.0f < gpGlobals->curtime)
	{
		if (RandomInt(0, 1) == 0)
		{
			m_flForwardTimer = gpGlobals->curtime + RandomFloat(1.5f, 3.5f);
			m_flSideScale = -m_flSideScale;
			if (RandomInt(0, 1) == 0)
			{
				m_flForwardTimer += 4.0f;
			}
			m_flRotationLockTimer = gpGlobals->curtime + RandomFloat(1.0f, 2.0f);
		}
		else
		{
			m_flSideTimer = gpGlobals->curtime + RandomFloat(0.0f, 2.0f);
		}
	}

	if (m_flForwardTimer > gpGlobals->curtime)
	{
		moveData.m_flForwardMove = 200.0f;
		moveData.m_flSideMove *= 0.1f;
	}
}

void CSpacecraftAIBase::Move_AttackStationary(float flFrametime)
{
	CBaseEntity *pEnemy(GetEnemy());
	if (pEnemy == NULL)
	{
		EnterState(ISpacecraftAI::AISTATE_IDLE);
		return;
	}

	Vector vecOrigin = m_pShip->WorldSpaceCenter();
	Vector vecEnemy = pEnemy->WorldSpaceCenter() - vecOrigin;

	//if (vecEnemy.LengthSqr() > Sqr(200.0f))
	//{
	//	Vector vecEnemyToTarget = m_vecMoveTarget - pEnemy->GetAbsOrigin();
	//	Vector vecSelfToEnemyDirection = vecEnemy.Normalized();
	//	m_vecMoveTarget -= DotProduct(vecSelfToEnemyDirection, vecEnemyToTarget) * vecSelfToEnemyDirection;
	//}

	//vecEnemy = m_vecMoveTarget - m_pShip->GetAbsOrigin();
	QAngle angMove;
	VectorAngles(vecEnemy, angMove);

	Quaternion qMoveDesired, qMoveCurrent;
	AngleQuaternion(angMove, qMoveDesired);
	AngleQuaternion(moveData.m_vecViewAngles, qMoveCurrent);
	QuaternionSlerp(qMoveCurrent, qMoveDesired, gpGlobals->frametime * m_flRotationSpeedBlend * 5.0f, qMoveCurrent);
	QuaternionAngles(qMoveCurrent, moveData.m_vecViewAngles);

	Vector vecOpponent = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();
	Vector vecFwd;
	AngleVectors(moveData.m_vecViewAngles, &vecFwd);

	if (DotProduct(vecOpponent, vecFwd) < 0.0f)
	{
		moveData.m_nButtons &= ~IN_ATTACK;
		m_flSideTimer = 0.0f;
	}

	moveData.m_nButtons &= ~IN_SPEED;

	moveData.m_flUpMove = 0.0f;

	const float flDistanceTargetToEnemy = vecEnemy.Length();
	moveData.m_flForwardMove = RemapValClamped(flDistanceTargetToEnemy, 196.0f, 380.0f, -100.0f, 200.0f);
}

void CSpacecraftAIBase::Move_FollowPath(float flFrametime)
{
	CPathTrack *pPathTrack = m_pShip->GetPathEntity();
	if (pPathTrack == NULL)
	{
		EnterState(ISpacecraftAI::AISTATE_IDLE);
		return;
	}

	Vector vecOrigin = m_pShip->GetAbsOrigin();
	Vector vecEnemy = pPathTrack->GetAbsOrigin() - vecOrigin;

	QAngle angMove;
	VectorAngles(vecEnemy, angMove);

	Quaternion qMoveDesired, qMoveCurrent;
	AngleQuaternion(angMove, qMoveDesired);
	AngleQuaternion(moveData.m_vecViewAngles, qMoveCurrent);
	QuaternionSlerp(qMoveCurrent, qMoveDesired, MIN(1.0f, gpGlobals->frametime * m_flRotationSpeedBlend * 250.0f), qMoveCurrent);
	QuaternionAngles(qMoveCurrent, moveData.m_vecViewAngles);

	moveData.m_nButtons &= ~IN_ATTACK;
	moveData.m_nButtons &= ~IN_SPEED;

	moveData.m_flUpMove = 0.0f;

	const float flDistanceTargetToEnemy = vecEnemy.Length();
	CPathTrack *pNext = pPathTrack->GetNext();

	if (pNext == NULL)
	{
		moveData.m_flForwardMove = RemapValClamped(flDistanceTargetToEnemy, 0.0f, 400.0f, 0.0f, 1.0f);
		moveData.m_flForwardMove *= moveData.m_flForwardMove;
		moveData.m_flForwardMove *= 200.0f;

		if (flDistanceTargetToEnemy < 32.0f)
		{
			m_pShip->SetPathEntity(NULL);
		}
	}
	else if (flDistanceTargetToEnemy < 32.0f)
	{
		moveData.m_flForwardMove = 200.0f;
		m_pShip->SetPathEntity(pNext);
	}
	else
	{
		moveData.m_flForwardMove = 200.0f;
	}
}

//void CSpacecraftAIBase::Move_FollowLeader(float flFrametime)
//{
//}

void CSpacecraftAIBase::Fire_UpdateProjectilePosition(float frametime)
{
	const CBaseEntity *pEnemy(GetEnemy());
	if (pEnemy == NULL)
	{
		return;
	}

	Vector vecOpponent = pEnemy->GetAbsOrigin() - m_pShip->GetAbsOrigin();

	float flMinOffset = 1.0f;
	float flMaxOffset = 2.0f;
	const CSpacecraft *pEnemySpacecraft = dynamic_cast<const CSpacecraft*>(pEnemy);
	if (pEnemySpacecraft != NULL && pEnemySpacecraft->IsPlayerControlled())
	{
		flMinOffset = 2.5f;
		flMaxOffset = 7.0f;
	}

	Vector vecTarget = pEnemy->WorldSpaceCenter();
	Vector vecPredictedTarget, vecVelocity = pEnemy->GetAbsVelocity();

	IPhysicsObject *pPhysicsObject(pEnemy->VPhysicsGetObject());
	if (pPhysicsObject != NULL)
	{
		pPhysicsObject->GetVelocity(&vecVelocity, NULL);
	}

	if (UTIL_PredictProjectileTarget(m_pShip->GetAbsOrigin(), vecTarget, vecVelocity,
		4000.0f, vecPredictedTarget))
	{
		vecTarget = vecPredictedTarget;
	}

	const float flShootDistance = RemapValClamped(vecOpponent.Length(), 30.0f, 500.0f, flMinOffset, flMaxOffset);
	moveData.m_vecWorldShootPosition = vecTarget + RandomVector(-flShootDistance, flShootDistance);
}

void CSpacecraftAIBase::UpdateEnemy(float &flUpdateDelay)
{
	//m_pShip->GetEnemy()
	CBaseEntity *pEnemy = NULL;

	const CSpacecraft::AITEAM_e ownteam = m_pShip->GetTeam();
	Vector ownOrigin = m_pShip->GetAbsOrigin();
	const bool bIgnorePlayer = m_pShip->HasSpawnFlags(SPACECRAFT_SPAWNFLAG_IGNORE_PLAYER);

	CBaseEntity *pList[1024];
	float flBestDistanceSqr = FLT_MAX;

	const int count = UTIL_EntitiesInSphere(pList, 1024, m_pShip->GetAbsOrigin(), 1024.0f, 0);
	for (int i = 0; i < count; ++i)
	{
		CBaseEntity *pEntity = pList[i];
		CSpacecraft *pSpaceCraft = dynamic_cast<CSpacecraft*>(pEntity);
		if (pSpaceCraft == NULL || pSpaceCraft == m_pShip)
		{
			continue;
		}

		if (bIgnorePlayer && pSpaceCraft->IsPlayerControlled())
		{
			continue;
		}

		//CSpacecraftAIBase *pAI = (CSpacecraftAIBase*)pSpaceCraft->GetAI();

		const CSpacecraft::AITEAM_e team = pSpaceCraft->GetTeam();
		if (team == ownteam)
		{
			continue;
		}

		Vector shipOrigin = pSpaceCraft->GetAbsOrigin();
		float flDistanceSqr = (shipOrigin - ownOrigin).LengthSqr();

		if (flDistanceSqr > flBestDistanceSqr)
		{
			continue;
		}

		flBestDistanceSqr = flDistanceSqr;
		pEnemy = pSpaceCraft;
	}

	//CGstringPlayer *pPlayer = LocalGstringPlayer();
	//if (pPlayer && pPlayer->IsInSpacecraft())
	//{
	//	pEnemy = pPlayer->GetSpacecraft();
	//}

	if (pEnemy != NULL)
	{
		m_hTemporaryEnemy = pEnemy;
	}

	// Attack last dmg inflictor
	// Attack enemy/target/player nearby
}
