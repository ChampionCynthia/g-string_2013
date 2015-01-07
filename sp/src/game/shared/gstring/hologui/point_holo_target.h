#ifndef POINT_HOLO_TARGET_H
#define POINT_HOLO_TARGET_H

#include "cbase.h"

#ifdef CLIENT_DLL
class IHoloTarget
{
public:
	virtual ~IHoloTarget() {}

	enum TargetType
	{
		NEUTRAL = 0,
		FRIENDLY,
		ENEMY
	};

	virtual const char *GetName() const = 0;
	virtual float GetSize() const = 0;
	virtual float GetHealthPercentage() const = 0;
	virtual TargetType GetType() const = 0;
	virtual float GetMaxDistance() const = 0;
	virtual const C_BaseEntity *GetEntity() const = 0;
};

const CUtlVector< IHoloTarget* > &GetHoloTargets();
void AddHoloTarget( IHoloTarget *pEntity );
void RemoveHoloTarget( IHoloTarget *pEntity );
#endif

class CPointHoloTarget : public CBaseEntity
#ifdef CLIENT_DLL
	, public IHoloTarget
#endif
{
	DECLARE_CLASS( CPointHoloTarget, CBaseEntity );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
#else
	virtual const char *GetName() const;
	virtual float GetSize() const;
	virtual float GetHealthPercentage() const;
	virtual TargetType GetType() const;
	virtual float GetMaxDistance() const;
	virtual const C_BaseEntity *GetEntity() const { return this; }

	virtual void NotifyShouldTransmit( ShouldTransmitState_t state );
#endif

public:
	CPointHoloTarget();
	virtual ~CPointHoloTarget();

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

	//virtual int DrawModel( int flags );
	virtual bool ShouldDraw() { return false; }
	//virtual RenderGroup_t GetRenderGroup();
	//virtual void ClientThink();
#endif
	
private:
#ifdef GAME_DLL
	string_t m_strTargetName;
	bool m_bEnabled;

	COutputEvent m_OnEnabled;
	COutputEvent m_OnDisabled;
#else
#endif

	CNetworkString( m_szTargetName, 32 );
	CNetworkVar( int, m_iTargetType );
	CNetworkVar( float, m_flSize );
	CNetworkVar( float, m_flHealth );
	CNetworkVar( float, m_flMaxDistance );
};

#endif
