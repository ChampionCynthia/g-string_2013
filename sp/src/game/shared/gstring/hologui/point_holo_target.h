#ifndef POINT_HOLO_TARGET_H
#define POINT_HOLO_TARGET_H

#include "cbase.h"

#ifdef CLIENT_DLL
#define HOLO_TARGET_MAX_DISTANCE 5000.0f //3000.0f
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
	virtual bool IsActive() const = 0;
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

	int GetTargetType() const;
	bool IsActive() const;

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
#else
	virtual const char *GetName() const;
	virtual float GetSize() const;
	virtual float GetHealthPercentage() const;
	virtual TargetType GetType() const;
	virtual float GetMaxDistance() const;
	virtual bool IsActive() const { return m_hPositionProxy.m_Value.IsValid() == ( m_hPositionProxy.Get() != NULL ); };
	virtual const C_BaseEntity *GetEntity() const { return m_hPositionProxy.Get() ? m_hPositionProxy.Get() : this; }

	virtual void NotifyShouldTransmit( ShouldTransmitState_t state );
#endif

public:
	CPointHoloTarget();
	virtual ~CPointHoloTarget();

#ifdef GAME_DLL
	virtual void Spawn();
	virtual void Activate();

	virtual int ObjectCaps()
	{
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();
#else
	//virtual int DrawModel( int flags );
	virtual bool ShouldDraw() { return false; }
	//virtual RenderGroup_t GetRenderGroup();
	//virtual void ClientThink();
#endif
	
private:
#ifdef GAME_DLL
	void Update();

	string_t m_strTargetName;
	string_t m_strHealthProxyName;
	string_t m_strPositionProxyName;
	bool m_bEnabled;
	EHANDLE m_hHealthProxy;

	COutputEvent m_OnEnabled;
	COutputEvent m_OnDisabled;
#else
#endif

	CNetworkString( m_szTargetName, 32 );
	CNetworkVar( int, m_iTargetType );
	CNetworkVar( float, m_flSize );
	CNetworkVar( float, m_flHealth );
	CNetworkVar( float, m_flMaxDistance );
	CNetworkHandle( CBaseEntity, m_hPositionProxy );
};

#endif
