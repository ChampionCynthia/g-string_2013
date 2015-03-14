#ifndef I_ACTION_H
#define I_ACTION_H

#include "cbase.h"

#include "icommand.h"

class IAction : public ICommand
{
public:
	virtual bool Execute( SpacecraftState_t &a_state );
};

#endif
