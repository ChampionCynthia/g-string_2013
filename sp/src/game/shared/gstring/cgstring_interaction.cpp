
#include "cbase.h"
#include "cgstring_interaction.h"
#include "gstring_player_shared.h"

#ifdef GAME_DLL
#include "cgstring_interaction_body.h"
#include "props.h"
#endif

#define FIRSTPERSON_BODY_MODEL "models/bioproto/myo_firstperson.mdl"

#ifdef GAME_DLL

BEGIN_DATADESC( CGstringInteraction )

	DEFINE_FIELD( m_hFinalPosition, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hInteractiveObject, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_strFinalPositionName, FIELD_STRING, "final_position" ),
	DEFINE_KEYFIELD( m_strInteractiveObjectName, FIELD_STRING, "interactive_object" ),
	DEFINE_KEYFIELD( m_strPlayerSequenceName, FIELD_STRING, "player_sequence_name" ),
	DEFINE_KEYFIELD( m_strObjectSequenceName, FIELD_STRING, "object_sequence_name" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "start_interaction", InputStartInteraction ),

END_DATADESC()

#endif

IMPLEMENT_NETWORKCLASS_DT( CGstringInteraction, CGstringInteraction_DT )

#ifdef GAME_DLL
	SendPropEHandle( SENDINFO( m_hInteractiveObject ) ),
#else
	RecvPropEHandle( RECVINFO( m_hInteractiveObject ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( gstring_interaction, CGstringInteraction );

PRECACHE_REGISTER( gstring_interaction );

CGstringInteraction::CGstringInteraction()
#ifdef GAME_DLL
	: m_bInteractionActive( false )
#endif
{
}

CGstringInteraction::~CGstringInteraction()
{
}

#ifdef GAME_DLL

void CGstringInteraction::Precache()
{
	BaseClass::Precache();

	PrecacheModel( FIRSTPERSON_BODY_MODEL );
}

void CGstringInteraction::Spawn()
{
	BaseClass::Spawn();
}

void CGstringInteraction::Activate()
{
	m_hInteractiveObject = dynamic_cast< CBaseAnimating* >( gEntList.FindEntityByName( NULL, m_strInteractiveObjectName, this ) );
	m_hFinalPosition = gEntList.FindEntityByName( NULL, m_strFinalPositionName, this );

	BaseClass::Activate();
}

int CGstringInteraction::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CGstringInteraction::InputStartInteraction( inputdata_t &inputdata )
{
	if ( m_bInteractionActive )
	{
		Warning( "Can't start interaction while there is one active!\n" );
		return;
	}

	if ( m_hInteractiveObject.Get() == NULL )
	{
		Warning( "Interaction entity has no interactive object assigned!\n" );
		return;
	}

	if ( !inputdata.pActivator->IsPlayer() )
	{
		Warning( "Interaction must be triggered by a player.\n" );
		return;
	}

	CDynamicProp *pInteractiveObject = dynamic_cast< CDynamicProp* >( m_hInteractiveObject.Get() );

	if ( pInteractiveObject != NULL )
	{
		inputdata_t data;
		data.value.SetString( m_strObjectSequenceName );
		pInteractiveObject->InputSetAnimation( data );
		pInteractiveObject->SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		Warning( "Interaction entity must be a prop_dynamic!\n" );
		return;
	}

	CGstringInteractionBody *pFirstpersonBody = assert_cast< CGstringInteractionBody* >( CreateEntityByName( "gstring_interaction_body" ) );
	Assert( pFirstpersonBody );

	pFirstpersonBody->SetAbsOrigin( pInteractiveObject->GetAbsOrigin() );
	pFirstpersonBody->SetAbsAngles( pInteractiveObject->GetAbsAngles() );
	pFirstpersonBody->SetModel( FIRSTPERSON_BODY_MODEL );

	DispatchSpawn( pFirstpersonBody );

	const int iPlayerSequence = pFirstpersonBody->LookupSequence( m_strPlayerSequenceName.ToCStr() );
	if ( iPlayerSequence >= 0 )
	{
		pFirstpersonBody->ResetSequence( iPlayerSequence );
	}

	CGstringPlayer *pPlayer = assert_cast< CGstringPlayer* >( inputdata.pActivator );
	pPlayer->BeginInteraction( pFirstpersonBody );
	m_hPlayer.Set( pPlayer );

	pFirstpersonBody->SetInteractionEntity( this );
	m_bInteractionActive = true;
}

void CGstringInteraction::OnBodyAnimationFinished()
{
	CGstringPlayer *pPlayer = m_hPlayer;
	if ( pPlayer != NULL )
	{
		pPlayer->EndInteraction();

		CBaseEntity *pFinalPosition = m_hFinalPosition;
		if ( pFinalPosition != NULL )
		{
			pPlayer->Teleport( &pFinalPosition->GetAbsOrigin(),
				&pFinalPosition->GetAbsAngles(), NULL );
		}
	}
	m_bInteractionActive = false;
}

#else

//void CGstringGlobals::OnDataChanged( DataUpdateType_t type )
//{
//	BaseClass::OnDataChanged( type );
//
//	//if ( type == DATA_UPDATE_DATATABLE_CHANGED )
//	{
//		if ( m_bCascadedShadowMappingEnabled && physenv )
//		{
//			physenv->SetGravity( vec3_origin );
//		}
//	}
//}

#endif
