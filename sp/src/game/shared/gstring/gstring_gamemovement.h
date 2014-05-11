#ifndef GSTRING_GAMEMOVEMENT_H
#define GSTRING_GAMEMOVEMENT_H

#include "hl_gamemovement.h"
#include "gstring_player_shared.h"

class CGstringGameMovement : public CHL2GameMovement
{
	typedef CHL2GameMovement BaseClass;
public:
	CGstringGameMovement();

	virtual void Duck();
	virtual void ProcessMoveType();

private:
	void SpacecraftMove();
};


#endif