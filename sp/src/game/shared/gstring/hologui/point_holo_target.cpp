
#include "cbase.h"
#include "point_holo_target.h"


#ifdef GAME_DLL
BEGIN_DATADESC( CPointHoloTarget )

	DEFINE_THINKFUNC( Update ),
	DEFINE_FIELD( m_hHealthProxy, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hPositionProxy, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_strTargetName, FIELD_STRING, "HoloTargetName" ),

	DEFINE_KEYFIELD( m_iTargetType, FIELD_INTEGER, "TargetType" ),
	DEFINE_KEYFIELD( m_flSize, FIELD_FLOAT, "Size" ),
	DEFINE_KEYFIELD( m_flHealth, FIELD_FLOAT, "TargetHealth" ),
	DEFINE_KEYFIELD( m_flMaxDistance, FIELD_FLOAT, "MaxDistance" ),
	DEFINE_KEYFIELD( m_bEnabled, FIELD_BOOLEAN, "Enabled" ),
	DEFINE_KEYFIELD( m_strHealthProxyName, FIELD_STRING, "HealthProxy" ),
	DEFINE_KEYFIELD( m_strPositionProxyName, FIELD_STRING, "PositionProxy" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT( m_OnEnabled, "OnEnabled" ),
	DEFINE_OUTPUT( m_OnDisabled, "OnDisabled" ),

END_DATADESC()
#else

static CUtlVector< IHoloTarget* > g_HoloTargets;
const CUtlVector< IHoloTarget* > &GetHoloTargets()
{
	return g_HoloTargets;
}

void AddHoloTarget( IHoloTarget *pEntity )
{
	Assert( !g_HoloTargets.HasElement( pEntity ) );
	if ( !g_HoloTargets.HasElement( pEntity ) )
	{
		g_HoloTargets.AddToTail( pEntity );
	}
}

void RemoveHoloTarget( IHoloTarget *pEntity )
{
	if ( g_HoloTargets.HasElement( pEntity ) )
	{
		g_HoloTargets.FindAndRemove( pEntity );
	}
}

#endif

IMPLEMENT_NETWORKCLASS_DT( CPointHoloTarget, CPointHoloTarget_DT )

#ifdef GAME_DLL
	SendPropString( SENDINFO( m_szTargetName ) ),
	SendPropInt( SENDINFO( m_iTargetType ), 3, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flSize ) ),
	SendPropFloat( SENDINFO( m_flHealth ) ),
	SendPropFloat( SENDINFO( m_flMaxDistance ) ),
	SendPropEHandle( SENDINFO( m_hPositionProxy ) ),
#else
	RecvPropString( RECVINFO( m_szTargetName ) ),
	RecvPropInt( RECVINFO( m_iTargetType ) ),
	RecvPropFloat( RECVINFO( m_flSize ) ),
	RecvPropFloat( RECVINFO( m_flHealth ) ),
	RecvPropFloat( RECVINFO( m_flMaxDistance ) ),
	RecvPropEHandle( RECVINFO( m_hPositionProxy ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( point_holo_target, CPointHoloTarget );

CPointHoloTarget::CPointHoloTarget()
{
#ifdef GAME_DLL
	m_bEnabled = true;
	//m_bIsRestoring = false;
#endif
}

CPointHoloTarget::~CPointHoloTarget()
{
#ifdef CLIENT_DLL
	RemoveHoloTarget( this );
#endif
}

#ifdef GAME_DLL

void CPointHoloTarget::InputEnable( inputdata_t &inputdata )
{
	if ( !m_bEnabled )
	{
		m_bEnabled = true;
		m_OnEnabled.FireOutput( inputdata.pActivator, inputdata.pCaller );
		DispatchUpdateTransmitState();
	}
}

void CPointHoloTarget::InputDisable( inputdata_t &inputdata )
{
	if ( m_bEnabled )
	{
		m_bEnabled = false;
		m_OnDisabled.FireOutput( inputdata.pActivator, inputdata.pCaller );
		DispatchUpdateTransmitState();
	}
}

void CPointHoloTarget::Spawn()
{
	BaseClass::Spawn();

	m_bEnabled = HasSpawnFlags( 1 );
	m_flHealth = -1.0f;
}

void CPointHoloTarget::Activate()
{
	BaseClass::Activate();

	m_hHealthProxy = gEntList.FindEntityByName( NULL, m_strHealthProxyName, this );
	m_hPositionProxy = gEntList.FindEntityByName( NULL, m_strPositionProxyName, this );

	Q_strncpy( m_szTargetName.GetForModify(), STRING( m_strTargetName ), 32 );

	SetThink( &CPointHoloTarget::Update );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

int CPointHoloTarget::UpdateTransmitState()
{
	return SetTransmitState( m_bEnabled ? FL_EDICT_ALWAYS : FL_EDICT_DONTSEND );
}

void CPointHoloTarget::Update()
{
	if ( m_hHealthProxy.Get() )
	{
		m_flHealth = ceil( m_hHealthProxy->GetHealth() / (float)m_hHealthProxy->GetMaxHealth() * 100.0f );
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

#else

const char *CPointHoloTarget::GetName() const
{
	return m_szTargetName.Get();
}

float CPointHoloTarget::GetSize() const
{
	return m_flSize;
}

float CPointHoloTarget::GetHealthPercentage() const
{
	return m_flHealth * 0.01f;
}

IHoloTarget::TargetType CPointHoloTarget::GetType() const
{
	return (IHoloTarget::TargetType)m_iTargetType.Get();
}

float CPointHoloTarget::GetMaxDistance() const
{
	return m_flMaxDistance;
}

void CPointHoloTarget::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	// TODO: Called with SHOULDTRANSMIT_START on map shutdown? fucking source.
	BaseClass::NotifyShouldTransmit( state );
	switch( state )
	{
	case SHOULDTRANSMIT_START:
		AddHoloTarget( this );
		break;

	case SHOULDTRANSMIT_END:
		RemoveHoloTarget( this );
		break;
	}
}

#endif
