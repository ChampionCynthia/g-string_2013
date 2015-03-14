#ifndef I_CONDITION_H
#define I_CONDITION_H

#include "cbase.h"

#include "icommand.h"

class ICondition : public ICommand
{
public:
	virtual bool Execute( SpacecraftState_t &a_state );
};

#endif