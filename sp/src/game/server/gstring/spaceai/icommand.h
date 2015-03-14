#ifndef I_COMPONENT_H
#define I_COMPONENT_H

#include "cbase.h"

struct SpacecraftState_t;

class ICommand
{
public:
	virtual ~ICommand() {}

	virtual bool Execute( SpacecraftState_t &a_state ) = 0;
};

#endif
