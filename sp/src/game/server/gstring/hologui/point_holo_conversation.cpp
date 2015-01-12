
#include "cbase.h"
#include "point_holo_conversation.h"
#include "filesystem.h"

namespace
{
	KeyValues *g_pConversations;
	int g_iConversationReferenceCounter;
}

BEGIN_DATADESC( CPointHoloConversation )

	//DEFINE_THINKFUNC( Update ),
	//DEFINE_FIELD( m_hHealthProxy, FIELD_EHANDLE ),
	//DEFINE_FIELD( m_hPositionProxy, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_strConversationName, FIELD_STRING, "Conversation" ),

	//DEFINE_KEYFIELD( m_iTargetType, FIELD_INTEGER, "TargetType" ),
	//DEFINE_KEYFIELD( m_flSize, FIELD_FLOAT, "Size" ),
	//DEFINE_KEYFIELD( m_flHealth, FIELD_FLOAT, "TargetHealth" ),
	//DEFINE_KEYFIELD( m_flMaxDistance, FIELD_FLOAT, "MaxDistance" ),
	//DEFINE_KEYFIELD( m_bEnabled, FIELD_BOOLEAN, "Enabled" ),
	//DEFINE_KEYFIELD( m_strHealthProxyName, FIELD_STRING, "HealthProxy" ),
	//DEFINE_KEYFIELD( m_strPositionProxyName, FIELD_STRING, "PositionProxy" ),

	//DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	//DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	//DEFINE_OUTPUT( m_OnEnabled, "OnEnabled" ),
	//DEFINE_OUTPUT( m_OnDisabled, "OnDisabled" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( point_holo_conversation, CPointHoloConversation );

PRECACHE_REGISTER( point_holo_conversation );

CPointHoloConversation::CPointHoloConversation() :
	m_pConversation( NULL ),
	m_pMessage( NULL )
{
	if ( g_iConversationReferenceCounter == 0 )
	{
		g_pConversations = new KeyValues( "" );
		g_pConversations->LoadFromFile( g_pFullFileSystem, "scripts/holo_conversations.txt" );
	}
	++g_iConversationReferenceCounter;
}

CPointHoloConversation::~CPointHoloConversation()
{
	--g_iConversationReferenceCounter;
	if ( g_iConversationReferenceCounter == 0 )
	{
		g_pConversations->deleteThis();
	}
}

void CPointHoloConversation::InputStart( inputdata_t &inputdata )
{
}

void CPointHoloConversation::Spawn()
{
	BaseClass::Spawn();

	m_pConversation = g_pConversations->FindKey( STRING( m_strConversationName ) );
	if ( m_pConversation )
	{
		m_pMessage = m_pConversation->GetFirstTrueSubKey();
	}
}

void CPointHoloConversation::Precache()
{
	BaseClass::Precache();

	if ( m_pConversation )
	{
		for ( KeyValues *pMessage = m_pConversation->GetFirstTrueSubKey();
			pMessage;
			pMessage = pMessage->GetNextTrueSubKey() )
		{
			const char *pszSoundName = pMessage->GetString( "soundname" );
			if ( *pszSoundName )
			{
				PrecacheSound( pszSoundName );
			}
		}
	}
}

int CPointHoloConversation::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_DONTSEND );
}
