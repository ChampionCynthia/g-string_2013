#ifndef SPACECRAFT_AI_H
#define SPACECRAFT_AI_H

#include "gstring/cspacecraft.h"

class CSpacecraftAIBase : public ISpacecraftAI
{
public:
	CSpacecraftAIBase( CSpacecraft *pShip );
	virtual ~CSpacecraftAIBase();

	virtual void Run( float flFrametime );

private:
	enum AIState_e
	{
		STATE_FOLLOW_AND_SHOOT = 0,
	};

	void StateFollowAndShoot( float flFrametime );

	CBaseEntity *GetEnemy();

	CSpacecraft *m_pShip;

	CBaseEntity *m_pEnemy;

	CMoveData moveData;
	AIState_e m_aiState;
	float m_flShootDelay;
	float m_flShootCooldown;
};

#endif