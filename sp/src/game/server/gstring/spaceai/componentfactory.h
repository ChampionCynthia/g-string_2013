#ifndef COMPONENT_FACTORY_H
#define COMPONENT_FACTORY_H

#include "cbase.h"
#include "UtlStringMap.h"

#include "iaction.h"
#include "icondition.h"
//#include "imove.h"

class CComponentFactory
{
	static CComponentFactory m_Instance;
public:
	CComponentFactory();

	static CComponentFactory *GetInstance();

	void RegisterAction( const char *pszName, IAction *pAction );
	void RegisterCondition( const char *pszName, ICondition *pCondition );
	//void RegisterMove( const char *pszName, IMove *pMove );

	IAction *FindAction( const char *pszName );
	ICondition *FindCondition( const char *pszName );
	//IMove *FindMove( const char *pszName );

private:
	CUtlStringMap< IAction* > m_Actions;
	CUtlStringMap< ICondition* > m_Conditions;
	//CUtlStringMap< IMove* > m_Moves;

};

#endif