
#include "cbase.h"
#include "cgstring_interaction_body.h"
#include "gstring_player_shared.h"

#ifdef GAME_DLL
#include "npcevent.h"
#include "eventlist.h"
#include "cgstring_interaction.h"

#include "props.h"
#else
#include "model_types.h"
#endif

#ifdef GAME_DLL

BEGIN_DATADESC( CGstringInteractionBody )

	//DEFINE_FIELD( m_hInteractiveObject, FIELD_EHANDLE ),

	//DEFINE_KEYFIELD( m_strInteractiveObjectName, FIELD_STRING, "interactive_object" ),
	//DEFINE_KEYFIELD( m_strPlayerSequenceName, FIELD_STRING, "player_sequence_name" ),
	//DEFINE_KEYFIELD( m_strObjectSequenceName, FIELD_STRING, "object_sequence_name" ),

	DEFINE_THINKFUNC( Think ),

END_DATADESC()

#endif

IMPLEMENT_NETWORKCLASS_DT( CGstringInteractionBody, CGstringInteractionBody_DT )

#ifdef GAME_DLL
	//SendPropEHandle( SENDINFO( m_hInteractiveObject ) ),
#else
	//RecvPropEHandle( RECVINFO( m_hInteractiveObject ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( gstring_interaction_body, CGstringInteractionBody );

CGstringInteractionBody::CGstringInteractionBody()
#ifdef CLIENT_DLL
	: m_flPlayerTransitionBlend( 0.0f )
	, m_iEyesAttachment( -1 )
#endif
{
}

CGstringInteractionBody::~CGstringInteractionBody()
{
}

#ifdef GAME_DLL

void CGstringInteractionBody::Spawn()
{
	BaseClass::Spawn();

	SetThink( &CGstringInteractionBody::Think );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGstringInteractionBody::Activate()
{
	BaseClass::Activate();
}

int CGstringInteractionBody::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CGstringInteractionBody::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case AE_SV_GSTRING_INTERACTION_PLAYER_1:
	case AE_SV_GSTRING_INTERACTION_PLAYER_2:
	case AE_SV_GSTRING_INTERACTION_PLAYER_3:
	case AE_SV_GSTRING_INTERACTION_PLAYER_4:
	case AE_SV_GSTRING_INTERACTION_PLAYER_5:
		{
			if ( m_hInteraction.Get() != NULL )
			{
				m_hInteraction->OnBodyEvent( pEvent->event - AE_SV_GSTRING_INTERACTION_PLAYER_1 );
			}
		}
		break;

	default:
		{
			BaseClass::HandleAnimEvent( pEvent );
		}
		break;
	}
}

void CGstringInteractionBody::SetInteractionEntity( CGstringInteraction *pInteractionEntity )
{
	m_hInteraction.Set( pInteractionEntity );
}

void CGstringInteractionBody::Think()
{
	StudioFrameAdvance();
	DispatchAnimEvents( this );

	if ( GetCycle() >= 1.0f )
	{
		if ( m_hInteraction.Get() != NULL )
		{
			m_hInteraction->OnBodyAnimationFinished();
		}

		UTIL_Remove( this );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

#else

void CGstringInteractionBody::SetTransitionBlend( float flBlend )
{
	m_flPlayerTransitionBlend = flBlend;
}

int CGstringInteractionBody::DrawModel( int flags )
{
	if ( m_flPlayerTransitionBlend < 1.0f )
	{
		return 0;
	}

	if ( ( flags & STUDIO_SHADOWDEPTHTEXTURE ) != 0 )
	{
		return 0;
	}

	return BaseClass::DrawModel( flags );
}

void CGstringInteractionBody::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
	}
}

CStudioHdr *CGstringInteractionBody::OnNewModel()
{
	CStudioHdr *pRet = BaseClass::OnNewModel();

	m_iEyesAttachment = LookupAttachment( "eyes" );

	return pRet;
}

void CGstringInteractionBody::GetCamera( Vector &origin, QAngle &angles )
{
	GetAttachment( m_iEyesAttachment, origin, angles );
}

#endif
