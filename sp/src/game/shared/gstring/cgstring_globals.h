#ifndef CGSTRING_GLOBALS_H
#define CGSTRING_GLOBALS_H

#include "cbase.h"

class CGstringGlobals : public CBaseEntity
{
	DECLARE_CLASS( CGstringGlobals, CBaseEntity );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CGstringGlobals();
	~CGstringGlobals();

#ifdef GAME_DLL
	virtual void Spawn();

	virtual int ObjectCaps( void ){
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();

	void InputNightvisionEnable( inputdata_t &inputdata );
	void InputNightvisionDisable( inputdata_t &inputdata );
	void InputNightvisionToggle( inputdata_t &inputdata );

	void SetNightvisionEnabled( bool bEnabled );
	void SetNightvisionOverride( bool bEnabled );
	bool IsNightvisionEnabled() const;

	void InputUserLightSourceEnable( inputdata_t &inputdata );
	void InputUserLightSourceDisable( inputdata_t &inputdata );
	void InputUserLightSourceToggle( inputdata_t &inputdata );

	void SetUserLightSourceEnabled( bool bEnabled );
	bool IsUserLightSourceEnabled() const;
#else
	virtual void OnDataChanged( DataUpdateType_t type );
#endif

	bool IsCascadedShadowMappingEnabled() const;

private:
#ifdef GAME_DLL
	bool m_bNightvisionEnabled;
	bool m_bUserLightSourceEnabled;
#endif

	CNetworkVar( bool, m_bCascadedShadowMappingEnabled );
};

extern CGstringGlobals *g_pGstringGlobals;

#endif