#include "cbase.h"
#include "c_lights.h"

C_EnvLight *g_pCSMEnvLight;

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_EnvLight, DT_CEnvLight, CEnvLight )
	RecvPropQAngles( RECVINFO( m_angSunAngles ) ),
	RecvPropVector( RECVINFO( m_vecLight ) ),
	RecvPropVector( RECVINFO( m_vecAmbient ) ),
	RecvPropBool( RECVINFO( m_bCascadedShadowMappingEnabled ) ),
END_RECV_TABLE()

C_EnvLight::C_EnvLight()
	: m_angSunAngles( vec3_angle )
	, m_vecLight( vec3_origin )
	, m_vecAmbient( vec3_origin )
	, m_bCascadedShadowMappingEnabled( false )
{
}

C_EnvLight::~C_EnvLight()
{
	if ( g_pCSMEnvLight == this )
	{
		g_pCSMEnvLight = NULL;
	}
}

void C_EnvLight::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( m_bCascadedShadowMappingEnabled && g_pCSMEnvLight == NULL )
	{
		g_pCSMEnvLight = this;
	}
}
