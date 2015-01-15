
#include "cbase.h"
#include "point_holo_conversation.h"
#include "gstring/cgstring_player.h"

#include "filesystem.h"

namespace
{
	KeyValues *g_pConversations;
	int g_iConversationReferenceCounter;
}

extern ISoundEmitterSystemBase *soundemitterbase;

BEGIN_DATADESC( CPointHoloConversation )

	DEFINE_THINKFUNC( AdvanceConversation ),
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

	DEFINE_INPUTFUNC( FIELD_VOID, "Start", InputStart ),
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
	if ( m_pConversation )
	{
		m_pMessage = m_pConversation->GetFirstTrueSubKey();
		if ( m_pMessage )
		{
			AdvanceConversation();
		}
		else
		{
			Warning( "No messages found in conversation (%s)!\n", STRING( m_strConversationName ) );
		}
	}
	else
	{
		Warning( "Conversation not found (%s)!\n", STRING( m_strConversationName ) );
	}
}

void CPointHoloConversation::Spawn()
{
	BaseClass::Spawn();

	m_pConversation = g_pConversations->FindKey( STRING( m_strConversationName ) );
	Precache();
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
				PrecacheScriptSound( pszSoundName );
			}
		}
	}
}

int CPointHoloConversation::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_DONTSEND );
}

void CPointHoloConversation::AdvanceConversation()
{
	Assert( m_pMessage );

	CGstringPlayer *pLocal = LocalGstringPlayer();

	const char *pszSoundName = m_pMessage->GetString( "soundname" );
	const char *pszDisplayName = m_pMessage->GetString( "displayname" );
	float flDuration = 0.0f;

	CSingleUserRecipientFilter filter( pLocal );
	EmitSound( filter, pLocal->entindex(), pszSoundName, 0, 0, &flDuration );

	CSoundParameters params;
	soundemitterbase->GetParametersForSound( pszSoundName, params, GENDER_NONE );

	CSingleUserRecipientFilter user( pLocal );
	user.MakeReliable();
	UserMessageBegin( user, "HoloMessage" );
		WRITE_STRING( pszDisplayName );
		WRITE_STRING( params.soundname );
		WRITE_FLOAT( flDuration );
	MessageEnd();

	m_pMessage = m_pMessage->GetNextTrueSubKey();
	if ( m_pMessage )
	{
		const float flDelay = m_pMessage->GetFloat( "delay" ) + flDuration;
		SetThink( &CPointHoloConversation::AdvanceConversation );
		SetNextThink( gpGlobals->curtime + flDelay );
	}
	else
	{
		SetThink( NULL );
	}
}
