#ifndef SPACECRAFT_AI_H
#define SPACECRAFT_AI_H

#include "gstring/cspacecraft.h"

class CSpacecraftAIBase : public ISpacecraftAI
{
public:
	CSpacecraftAIBase( CSpacecraft *pShip );
	virtual ~CSpacecraftAIBase();

	virtual void Run( float flFrametime );

protected:
	enum AITEAM_e
	{
		AITEAM_MARTIAN = 0,
		AITEAM_NATO,
	};

	enum AIATTACKSTATE_e
	{
		AIATTACKSTATE_PACIFIST = 0,
		AIATTACKSTATE_OTHER_TEAM,
		AIATTACKSTATE_OTHER_TEAM_IGNORE_PLAYER,
	};

private:
	typedef void ( CSpacecraftAIBase::*AIThink )( void );
	void SetNextThink( float flDelay, AIThink thinkFunc = NULL );
	void ClearThink();

	typedef void ( CSpacecraftAIBase::*AIMove )( float flFrametime );
	void SetMove( AIMove moveFunc );

	void Think_ShootSalvoes();

	void Move_Follow( float flFrametime );
	void Move_Pursuit( float flFrametime );

	CBaseEntity *GetEnemy();

	CSpacecraft *m_pShip;

	AIThink m_ThinkFunc;
	AIMove m_MoveFunc;

	float m_flNextThink;

	CMoveData moveData;
	Vector m_vecMoveTarget;
	float m_flRotationSuppressTimer;
	float m_flRotationSpeedBlend;
	float m_flSideTimer;
	float m_flSideScale;
};

#endif