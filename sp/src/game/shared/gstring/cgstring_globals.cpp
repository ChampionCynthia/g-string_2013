
#include "cbase.h"
#include "cgstring_globals.h"
#include "gstring_player_shared.h"
#include "movevars_shared.h"

#define GSTRINGGLOBALSFLAGS_USERLIGHTSOURCE_ENABLED		0x01
#define GSTRINGGLOBALSFLAGS_NIGHTVISION_ENABLED			0x02
#define GSTRINGGLOBALSFLAGS_SPACE_MAP					0x04

CGstringGlobals *g_pGstringGlobals;

#ifdef GAME_DLL

BEGIN_DATADESC( CGstringGlobals )

	DEFINE_FIELD( m_bNightvisionEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bUserLightSourceEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsSpaceMap, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "nightvision_enable", InputNightvisionEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "nightvision_disable", InputNightvisionDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "nightvision_toggle", InputNightvisionToggle ),

	DEFINE_INPUTFUNC( FIELD_VOID, "userlightsource_enable", InputUserLightSourceEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "userlightsource_disable", InputUserLightSourceDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "userlightsource_toggle", InputUserLightSourceToggle ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "OnFireballFired", InputOnFireballFired ),

END_DATADESC()

#endif

IMPLEMENT_NETWORKCLASS_DT( CGstringGlobals, CGstringGlobals_DT )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bIsSpaceMap ) ),
#else
	RecvPropBool( RECVINFO( m_bIsSpaceMap ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( gstring_globals, CGstringGlobals );

CGstringGlobals::CGstringGlobals()
{
#ifdef GAME_DLL
	m_bNightvisionEnabled = true;
	m_bUserLightSourceEnabled = true;
#endif

	if ( g_pGstringGlobals == NULL )
	{
		g_pGstringGlobals = this;
	}
	else
	{
		AssertMsg( 0, "Multiple gstring_globals found." );
#ifndef DEBUG
		Error( "Only one 'gstring_globals' entity is allowed to be placed in a map." );
#endif
	}
}

CGstringGlobals::~CGstringGlobals()
{
	Assert( g_pGstringGlobals == this );

	if ( g_pGstringGlobals == this )
	{
		g_pGstringGlobals = NULL;
		sv_gravity.Revert();
	}
}

#ifdef GAME_DLL

void CGstringGlobals::Spawn()
{
	BaseClass::Spawn();

	SetUserLightSourceEnabled( HasSpawnFlags( GSTRINGGLOBALSFLAGS_USERLIGHTSOURCE_ENABLED ) );
	SetNightvisionEnabled( HasSpawnFlags( GSTRINGGLOBALSFLAGS_NIGHTVISION_ENABLED ) );
	m_bIsSpaceMap = HasSpawnFlags( GSTRINGGLOBALSFLAGS_SPACE_MAP );
}

void CGstringGlobals::Activate()
{
	BaseClass::Activate();

	if ( m_bIsSpaceMap )
	{
		if ( physenv )
		{
			physenv->SetGravity( vec3_origin );
		}
		sv_gravity.SetValue( "0" );
	}
}

int CGstringGlobals::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CGstringGlobals::SetNightvisionEnabled( bool bEnabled )
{
	m_bNightvisionEnabled = bEnabled;

	if ( !bEnabled )
	{
		CGstringPlayer *pPlayer = LocalGstringPlayer();

		if ( pPlayer )
		{
			pPlayer->SetNightvisionActive( false );

			if ( pPlayer->FlashlightIsOn() )
				pPlayer->FlashlightTurnOff();
		}
	}
}

bool CGstringGlobals::IsNightvisionEnabled() const
{
	return m_bNightvisionEnabled;
}

void CGstringGlobals::InputNightvisionEnable( inputdata_t &inputdata )
{
	SetNightvisionEnabled( true );
}

void CGstringGlobals::InputNightvisionDisable( inputdata_t &inputdata )
{
	SetNightvisionEnabled( false );
}

void CGstringGlobals::InputNightvisionToggle( inputdata_t &inputdata )
{
	SetNightvisionEnabled( !IsNightvisionEnabled() );
}

void CGstringGlobals::SetUserLightSourceEnabled( bool bEnabled )
{
	m_bUserLightSourceEnabled = bEnabled;

	if ( !bEnabled )
	{
		CGstringPlayer *pPlayer = LocalGstringPlayer();

		if ( pPlayer )
		{
			pPlayer->SetNightvisionActive( false );

			if ( pPlayer->FlashlightIsOn() )
				pPlayer->FlashlightTurnOff();
		}
	}
}

bool CGstringGlobals::IsUserLightSourceEnabled() const
{
	return m_bUserLightSourceEnabled;
}

void CGstringGlobals::InputUserLightSourceEnable( inputdata_t &inputdata )
{
	SetUserLightSourceEnabled( true );
}

void CGstringGlobals::InputUserLightSourceDisable( inputdata_t &inputdata )
{
	SetUserLightSourceEnabled( false );
}

void CGstringGlobals::InputUserLightSourceToggle( inputdata_t &inputdata )
{
	SetUserLightSourceEnabled( !IsUserLightSourceEnabled() );
}

void CGstringGlobals::InputOnFireballFired( inputdata_t &inputdata )
{
	if ( inputdata.value.Convert( FIELD_FLOAT ) )
	{
		CSingleUserRecipientFilter filter( UTIL_GetLocalPlayer() );
		filter.MakeReliable();
		UserMessageBegin( filter, "Fireball" );
			WRITE_FLOAT( inputdata.value.Float() );
		MessageEnd();
	}
}

#else

void CGstringGlobals::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( m_bIsSpaceMap && physenv )
	{
		physenv->SetGravity( vec3_origin );
	}
}

#endif
