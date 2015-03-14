
#include "cbase.h"
#include "filesystem.h"

#include "statemachine.h"
#include "componentfactory.h"

CStateMachine::CStateMachine()
{
}

CStateMachine::~CStateMachine()
{
}

CStateMachine *CStateMachine::LoadStateMachine( const char *pszFilePath )
{
	KeyValues::AutoDelete pKV( new KeyValues( "" ) );
	if ( !pKV->LoadFromFile( g_pFullFileSystem, pszFilePath ) )
	{
		return NULL;
	}

	CStateMachine *machine = new CStateMachine();
	machine->LoadFromKeyValues( pKV );
	return machine;
}

void CStateMachine::Delete()
{
	delete this;
}

void CStateMachine::LoadFromKeyValues( KeyValues *pKV )
{
}
