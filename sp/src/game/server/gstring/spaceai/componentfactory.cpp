
#include "cbase.h"

#include "componentfactory.h"

CComponentFactory CComponentFactory::m_Instance;

CComponentFactory::CComponentFactory()
{
}

CComponentFactory *CComponentFactory::GetInstance()
{
	return &m_Instance;
}

void CComponentFactory::RegisterAction( const char *pszName, IAction *pAction )
{
	Assert( !m_Actions.Defined( pszName ) );
	m_Actions[ pszName ] = pAction;
}

void CComponentFactory::RegisterCondition( const char *pszName, ICondition *pCondition )
{
	Assert( !m_Conditions.Defined( pszName ) );
	m_Conditions[ pszName ] = pCondition;
}

//void CComponentFactory::RegisterMove( const char *pszName, IMove *pMove )
//{
//	Assert( !m_Moves.Defined( pszName ) );
//	m_Moves[ pszName ] = pMove;
//}

IAction *CComponentFactory::FindAction( const char *pszName )
{
	Assert( m_Actions.Defined( pszName ) );
	return m_Actions[ pszName ];
}

ICondition *CComponentFactory::FindCondition( const char *pszName )
{
	Assert( m_Conditions.Defined( pszName ) );
	return m_Conditions[ pszName ];
}

//IMove *CComponentFactory::FindMove( const char *pszName )
//{
//	Assert( m_Moves.Defined( pszName ) );
//	return m_Moves[ pszName ];
//}
