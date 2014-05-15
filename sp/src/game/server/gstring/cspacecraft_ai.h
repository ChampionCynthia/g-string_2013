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
	CBaseEntity *GetEnemy();

	CSpacecraft *m_pShip;

	CBaseEntity *m_pEnemy;

};

#endif