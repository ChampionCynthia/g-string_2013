
#include "cbase.h"
#include "point_holo_target.h"


#ifdef GAME_DLL
BEGIN_DATADESC( CPointHoloTarget )

	DEFINE_KEYFIELD( m_strTargetName, FIELD_STRING, "HoloTargetName" ),

	DEFINE_KEYFIELD( m_iTargetType, FIELD_INTEGER, "TargetType" ),
	DEFINE_KEYFIELD( m_flSize, FIELD_FLOAT, "Size" ),
	DEFINE_KEYFIELD( m_flHealth, FIELD_FLOAT, "Health" ),
	DEFINE_KEYFIELD( m_flMaxDistance, FIELD_FLOAT, "MaxDistance" ),
	DEFINE_KEYFIELD( m_bEnabled, FIELD_BOOLEAN, "Enabled" ),

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
	g_HoloTargets.AddToTail( pEntity );
}

void RemoveHoloTarget( IHoloTarget *pEntity )
{
	Assert( g_HoloTargets.HasElement( pEntity ) );
	g_HoloTargets.FindAndRemove( pEntity );
}

#endif

IMPLEMENT_NETWORKCLASS_DT( CPointHoloTarget, CPointHoloTarget_DT )

#ifdef GAME_DLL
	SendPropString( SENDINFO( m_szTargetName ) ),
	SendPropInt( SENDINFO( m_iTargetType ), 3, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flSize ) ),
	SendPropFloat( SENDINFO( m_flHealth ) ),
	SendPropFloat( SENDINFO( m_flMaxDistance ) ),
#else
	RecvPropString( RECVINFO( m_szTargetName ) ),
	RecvPropInt( RECVINFO( m_iTargetType ) ),
	RecvPropFloat( RECVINFO( m_flSize ) ),
	RecvPropFloat( RECVINFO( m_flHealth ) ),
	RecvPropFloat( RECVINFO( m_flMaxDistance ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( point_holo_target, CPointHoloTarget );

CPointHoloTarget::CPointHoloTarget()
{
#ifdef CLIENT_DLL
	AddHoloTarget( this );
#else
	m_bEnabled = true;
#endif
}

CPointHoloTarget::~CPointHoloTarget()
{
#ifdef CLIENT_DLL
	RemoveHoloTarget( this );
#endif
}

#ifdef GAME_DLL

void CPointHoloTarget::Activate()
{
	BaseClass::Activate();

	m_bEnabled = HasSpawnFlags( 1 );

	Q_strncpy( m_szTargetName.GetForModify(), STRING( m_strTargetName ), 32 );
}

int CPointHoloTarget::UpdateTransmitState()
{
	return SetTransmitState( m_bEnabled ? FL_EDICT_ALWAYS : FL_EDICT_DONTSEND );
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
	return m_flHealth;
}

IHoloTarget::TargetType CPointHoloTarget::GetType() const
{
	return (IHoloTarget::TargetType)m_iTargetType.Get();
}

float CPointHoloTarget::GetMaxDistance() const
{
	return m_flMaxDistance;
}

void CPointHoloTarget::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );
}

#endif
