#ifndef SPACECRAFT_AI_H
#define SPACECRAFT_AI_H

#include "gstring/cspacecraft.h"
#include "gstring/spaceai/spacecraftstate.h"

class CStateMachine;

class CSpacecraftAIBase : public ISpacecraftAI
{
public:

	CSpacecraftAIBase(CSpacecraft *pShip);
	virtual ~CSpacecraftAIBase();

	virtual void EnterState(AISTATE_e state);

	virtual void Run(float flFrametime);

	//void SetSquadLeader(CSpacecraft *pShip);

private:
	typedef void (CSpacecraftAIBase::*AIThink)(void);
	void SetNextThink(float flDelay, AIThink thinkFunc = NULL);
	void ClearThink();

	typedef void (CSpacecraftAIBase::*AIMove)(float flFrametime);
	void SetMove(AIMove moveFunc);
	
	void Think_Idle();
	void Think_ShootSalvoes();
	
	void Move_Idle(float flFrametime);
	void Move_Follow(float flFrametime);
	void Move_Pursuit(float flFrametime);
	void Move_AttackStationary(float flFrametime);
	//void Move_FollowLeader(float flFrametime);
	void Move_FollowPath(float flFrametime);

	void FindPathTo( Vector &target );
	void PerformAvoidance(bool bAvoidPlayer, float flMaxDistance);

	void Fire_UpdateProjectilePosition(float frametime);

	void UpdateEnemy(float &flUpdateDelay);

	CBaseEntity *GetEnemy();

	CSpacecraft *m_pShip;

	//SpacecraftState_t m_state;
	//CStateMachine *m_pStateMachine;
	float m_flEnemyUpdateTimer;

	AIThink m_ThinkFunc;
	AIMove m_MoveFunc;

	float m_flNextThink;

	CMoveData moveData;
	Vector m_vecPushMove;
	EHANDLE m_hTemporaryEnemy;
	Vector m_vecMoveTarget;
	float m_flRotationSuppressTimer;
	float m_flRotationSpeedBlend;
	float m_flRotationLockTimer;
	float m_flSideTimer;
	float m_flSideScale;
	float m_flForwardTimer;

	// Squads
	CHandle< CSpacecraft > m_hLeader;
	CUtlVector< CSpacecraft* > m_hSquadMembers;
};

#endif