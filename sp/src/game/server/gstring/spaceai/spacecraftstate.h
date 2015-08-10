#include "usercmd.h"
#ifndef SPACE_CRAFT_STATE_H
#define SPACE_CRAFT_STATE_H

#include "cbase.h"
#include "igamemovement.h"

class CSpacecraft;

struct SpacecraftState_t
{
	CSpacecraft *pOuter;

	//Vector vecCurrentOrigin;
	//QAngle angCurrentAngles;
	//Vector vecCurrentVelocity;

	CMoveData moveData;
};

#endif
