#ifndef POINT_HOLO_OBJECTIVE_H
#define POINT_HOLO_OBJECTIVE_H

#include "cbase.h"

#ifdef CLIENT_DLL
class CPointHoloObjective;
const CUtlVector< CPointHoloObjective* > &GetHoloObjectives();
void AddHoloObjective( CPointHoloObjective *pEntity );
void RemoveHoloObjective( CPointHoloObjective *pEntity );
#endif

class CPointHoloObjective : public CBaseEntity
{
	DECLARE_CLASS( CPointHoloObjective, CBaseEntity );
	DECLARE_NETWORKCLASS();

	enum ObjectiveState
	{
		OBJECTIVESTATE_DISABLED = 0,
		OBJECTIVESTATE_ACTIVE,
		OBJECTIVESTATE_COMPLETED,
	};

#ifdef GAME_DLL
	DECLARE_DATADESC();

	void InputSetStateDisabled( inputdata_t &inputdata );
	void InputSetStateActive( inputdata_t &inputdata );
	void InputSetStateCompleted( inputdata_t &inputdata );
	void InputSetCountCurrent( inputdata_t &inputdata );
#else
	const char *GetDescription() const;
	ObjectiveState GetObjectiveState() const;
	int GetCountMax() const;
	int GetCountCurrent() const;
#endif

public:
	CPointHoloObjective();
	virtual ~CPointHoloObjective();

#ifdef GAME_DLL
	//virtual void Spawn();
	virtual void Activate();

	virtual int ObjectCaps()
	{
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();
#else
	virtual void OnDataChanged( DataUpdateType_t type );

	virtual bool ShouldDraw() { return false; }
#endif

private:
#ifdef GAME_DLL
	void CompleteObjective( CBaseEntity *pActivator, CBaseEntity *pCaller );

	string_t m_strDescription;
	COutputEvent m_OnObjectiveCompleted;
#else
#endif

	CNetworkString( m_szDescription, 32 );
	CNetworkVar( int, m_iObjectiveState );
	CNetworkVar( int, m_iCountMax );
	CNetworkVar( int, m_iCountCurrent );
};

#endif
