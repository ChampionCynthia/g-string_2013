#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "cbase.h"
#include "UtlStringMap.h"

struct SpacecraftState_t;

class ICommand;
class IAction;
class ICondition;

class CStateMachine
{
public:
	static CStateMachine *LoadStateMachine( const char *pszFilePath );
	void Delete();

private:
	CStateMachine();
	~CStateMachine();

	struct State
	{
		CUtlVector< ICommand* > commands;
	};

	void LoadFromKeyValues( KeyValues *pKV );

	CUtlStringMap< State > m_States;
};

#endif