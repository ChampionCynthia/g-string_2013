
#include "cbase.h"
#include "point_holo_conversation.h"
#include "gstring/cgstring_player.h"
#include "gstring/cspacecraft.h"

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
	DEFINE_INPUTFUNC( FIELD_VOID, "Interrupt", InputInterrupt ),
	//DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	//DEFINE_OUTPUT( m_OnEnabled, "OnEnabled" ),
	//DEFINE_OUTPUT( m_OnDisabled, "OnDisabled" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( point_holo_conversation, CPointHoloConversation );

PRECACHE_REGISTER( point_holo_conversation );

CPointHoloConversation::CPointHoloConversation() :
	m_pConversation( NULL ),
	m_pMessage( NULL ),
	m_pCurrentMessage( NULL ),
	m_flNextMessage( 0.0f ),
	m_flCurrentDuration( 0.0f ),
	m_bCanInterrupt( false )
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
			CSpacecraft *pSpacecraftCaller = dynamic_cast<CSpacecraft*>(inputdata.pCaller);
			CSpacecraft *pSpacecraftActivator = dynamic_cast<CSpacecraft*>(inputdata.pActivator);
			m_bCanInterrupt = pSpacecraftCaller != NULL || pSpacecraftActivator != NULL;
			if (m_bCanInterrupt)
			{
				m_hEmittingEntity = pSpacecraftCaller ? pSpacecraftCaller : pSpacecraftActivator;
			}

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

void CPointHoloConversation::InputInterrupt( inputdata_t &inputdata )
{
	SetThink( NULL );
	m_bCanInterrupt = false;

	if ( m_pCurrentMessage != NULL )
	{
		const char *pszInterruptName = m_pCurrentMessage->GetString( "interrupt" );
		const char *pszDisplayName = m_pCurrentMessage->GetString( "displayname" );
		if ( pszInterruptName != NULL )
		{
			PlayHoloSound( pszInterruptName, pszDisplayName );
		}
		else
		{
			Warning( "No interrupt defined in current message of conversation (%s)!\n", STRING( m_strConversationName ) );
		}
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
			
			pszSoundName = pMessage->GetString( "interrupt" );
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
	// Check interrupt
	if ( m_flCurrentDuration > gpGlobals->curtime )
	{
		if (!m_bCanInterrupt)
		{
			SetNextThink( gpGlobals->curtime + 0.1f );
			return;
		}

		CBaseEntity *pEmittingEntity = m_hEmittingEntity.Get();
		if (pEmittingEntity == NULL ||
			m_hEmittingEntity->GetHealth() <= 0)
		{
			// Do interrupt and end.
			SetThink( NULL );
			m_bCanInterrupt = false;

			if ( m_pCurrentMessage != NULL )
			{
				const char *pszInterruptName = m_pCurrentMessage->GetString( "interrupt" );
				const char *pszDisplayName = m_pCurrentMessage->GetString( "displayname" );
				if ( pszInterruptName != NULL )
				{
					PlayHoloSound( pszInterruptName, pszDisplayName );
				}
			}
			return;
		}
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	// No further message, stop thinking
	if ( m_pMessage == NULL )
	{
		if ( m_flCurrentDuration <= gpGlobals->curtime )
		{
			SetThink( NULL );
		}
		return;
	}

	// Wait for next message
	if ( m_flNextMessage > gpGlobals->curtime )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	Assert( m_pMessage );

	m_pCurrentMessage = m_pMessage;
	const char *pszSoundName = m_pMessage->GetString( "soundname" );
	const char *pszDisplayName = m_pMessage->GetString( "displayname" );
	
	float flDuration = PlayHoloSound( pszSoundName, pszDisplayName );

	m_pMessage = m_pMessage->GetNextTrueSubKey();
	if ( m_pMessage )
	{
		const float flDelay = m_pMessage->GetFloat( "delay" ) + flDuration;
		m_flNextMessage = gpGlobals->curtime + flDelay;
	}

	SetThink( &CPointHoloConversation::AdvanceConversation );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

float CPointHoloConversation::PlayHoloSound( const char *pszSoundName, const char *pszDisplayName )
{
	float flDuration = 0.0f;
	CGstringPlayer *pLocal = LocalGstringPlayer();

	if ( pLocal != NULL )
	{
		CSingleUserRecipientFilter filter( pLocal );
		EmitSound( filter, pLocal->entindex(), pszSoundName, 0, 0, &flDuration );
		m_flCurrentDuration = gpGlobals->curtime + flDuration;

		CSoundParameters params;
		soundemitterbase->GetParametersForSound( pszSoundName, params, GENDER_NONE );

		CSingleUserRecipientFilter user( pLocal );
		user.MakeReliable();
		UserMessageBegin( user, "HoloMessage" );
			WRITE_STRING( pszDisplayName );
			WRITE_STRING( params.soundname );
			WRITE_FLOAT( flDuration );
		MessageEnd();
	}
	return flDuration;
}
