
#include "cbase.h"
#include "gstring/hologui/env_holo_system.h"

#ifdef CLIENT_DLL
#include "gstring/hologui/holo_ship_health_graphic.h"
#include "gstring/hologui/holo_ship_health_text.h"
#include "gstring/hologui/holo_ship_model.h"
#include "gstring/hologui/holo_ship_engine.h"
#include "gstring/hologui/holo_ship_thruster.h"
#include "gstring/hologui/holo_ship_radar.h"
#include "gstring/hologui/holo_ship_aim.h"
#include "gstring/hologui/holo_ship_objectives.h"
#include "gstring/hologui/holo_ship_comm.h"
#include "gstring/hologui/holo_ship_aim_info.h"

#include "gstring/cspacecraft.h"
#include "gstring/gstring_rendertargets.h"
#include "gstring/cgstring_globals.h"

#include "iviewrender.h"
#include "viewrender.h"
#include "view_shared.h"
#include "model_types.h"
#include "view.h"
#include "c_user_message_register.h"

#include "materialsystem/itexture.h"
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CEnvHoloSystem )

	DEFINE_KEYFIELD( m_strAttachment, FIELD_STRING, "Attachment" ),

	// DEFINE_FIELD( m_bLensflareEnabled,		FIELD_BOOLEAN ),

END_DATADESC()
#else
static Vector g_vecHoloViewOrigin;
static matrix3x4_t g_matView;
static matrix3x4_t g_matViewInverted;
const Vector &CurrentHoloViewOrigin()
{
	return g_vecHoloViewOrigin;
}

const matrix3x4_t &CurrentHoloViewMatrix()
{
	return g_matView;
}

const matrix3x4_t &CurrentHoloViewMatrixInverted()
{
	return g_matViewInverted;
}

CUtlVector< CEnvHoloSystem* > g_HoloSystems;
void __MsgFunc_SpacecraftDamage( bf_read &msg )
{
	FOR_EACH_VEC( g_HoloSystems, i )
	{
		g_HoloSystems[ i ]->MsgFuncSpacecraftDamage( msg );
	}
}
USER_MESSAGE_REGISTER( SpacecraftDamage );
#endif

IMPLEMENT_NETWORKCLASS_DT( CEnvHoloSystem, CEnvHoloSystem_DT )

#ifdef GAME_DLL
	SendPropString( SENDINFO( m_szAttachment ) ),
#else
	RecvPropString( RECVINFO( m_szAttachment ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( env_holo_system, CEnvHoloSystem );

CEnvHoloSystem::CEnvHoloSystem()
#ifdef GAME_DLL
#else
	: m_iAttachment( -1 )
	, m_iEyes( -1 )
	, m_pAimPanel( NULL )
#endif
{
#ifdef GAME_DLL
#else
	g_HoloSystems.AddToTail( this );
#endif
}

CEnvHoloSystem::~CEnvHoloSystem()
{
#ifdef GAME_DLL
#else
	DestroyPanels();
	g_HoloSystems.FindAndRemove( this );
#endif
}

#ifdef GAME_DLL

void CEnvHoloSystem::Activate()
{
	BaseClass::Activate();

	Q_strncpy( m_szAttachment.GetForModify(), STRING( m_strAttachment ), 16 );
}

int CEnvHoloSystem::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

#else

int CEnvHoloSystem::DrawModel( int flags )
{
	if ( ( flags & STUDIO_SHADOWDEPTHTEXTURE ) != 0 ||
		( flags & STUDIO_SSAODEPTHTEXTURE ) != 0 ||
		( flags & STUDIO_TRANSPARENCY ) == 0 ||
		!GetOwnerEntity() )
	{
		return 0;
	}

	if ( view->GetViewSetup()->m_eStereoEye == GetFirstEye() )
	{
		PreRenderPanels();
	}

	matrix3x4_t attachmentMatrix;
	C_BaseAnimating *pAttachedModel = assert_cast< C_BaseAnimating* >( GetOwnerEntity() );
	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();

	// world
	//if ( pAttachedModel && m_iAttachment >= 0 && pAttachedModel->GetAttachment( m_iAttachment, attachmentMatrix ) )
	//{
	//	pRenderContext->LoadMatrix( attachmentMatrix );
	//}

	const float flWorldScale = g_pGstringGlobals ? g_pGstringGlobals->GetWorldScale() : 1.0f;

	// view model
	if ( pAttachedModel && m_iAttachment >= 0 )
	{
		const mstudioattachment_t &attachEyes = pAttachedModel->GetModelPtr()->pAttachment( m_iEyes - 1 );
		const mstudioattachment_t &attachGUI = pAttachedModel->GetModelPtr()->pAttachment( m_iAttachment - 1 );

		matrix3x4_t eyes, eyesInv;
		matrix3x4_t attachmentMatrix = attachGUI.local;
		MatrixInvert( attachEyes.local, eyesInv );
		ConcatTransforms( eyesInv, attachmentMatrix, eyes );

		matrix3x4_t matToDevice, matToDeviceInv;
		MatrixSetTranslation( vec3_origin, matToDevice );
		MatrixSetColumn( Vector( 0, -1, 0 ), 0, matToDevice );
		MatrixSetColumn( Vector( 0, 0, 1 ), 1, matToDevice );
		MatrixSetColumn( Vector( -1, 0, 0 ), 2, matToDevice );
		MatrixInvert( matToDevice, matToDeviceInv );

		pRenderContext->MatrixMode( MATERIAL_VIEW );

		Vector eyePosition;
		MatrixGetTranslation( eyes, eyePosition );
		g_vecHoloViewOrigin = -eyePosition;

		pRenderContext->LoadMatrix( eyes );

		// Apply view offset back
		QAngle eyeAngles;
		Vector attachmentPosition;
		pAttachedModel->GetAttachment( m_iEyes, attachmentPosition, eyeAngles );
		AngleMatrix( eyeAngles, attachmentPosition, eyes );

		matrix3x4_t camera, cameraOffset;
		AngleMatrix( CurrentViewAngles(), CurrentViewOrigin(), camera );
		MatrixInvert( camera, eyesInv );
		ConcatTransforms( eyesInv, eyes, cameraOffset );

		matrix3x4_t attachmentOrientation;
		AngleMatrix( eyeAngles, vec3_origin, attachmentOrientation );
		Vector temp = ( CurrentViewOrigin() - attachmentPosition );
		Vector localOffset;
		VectorITransform(temp, attachmentOrientation, localOffset);
		g_vecHoloViewOrigin += localOffset / flWorldScale;

		MatrixScaleBy( flWorldScale, cameraOffset );
		pRenderContext->MultMatrix( cameraOffset );
		pRenderContext->MultMatrix( matToDeviceInv );

		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->LoadIdentity();
	}

	pRenderContext->GetMatrix( MATERIAL_VIEW, &g_matView );
	MatrixScaleBy( 1.0f / flWorldScale, g_matView );
	MatrixInvert( g_matView, g_matViewInverted );

	DrawPanels();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PopMatrix();
	return 0;
}

RenderGroup_t CEnvHoloSystem::GetRenderGroup()
{
	if ( GetOwnerEntity() && assert_cast< C_BaseAnimating* >( GetOwnerEntity() )->IsViewModel() )
	{
		return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT;
	}
	return BaseClass::GetRenderGroup();
}

void CEnvHoloSystem::ClientThink()
{
	if ( GetOwnerEntity() == NULL )
	{
		DestroyPanels();
	}
	else if ( gpGlobals->frametime > 0.0f )
	{
		FOR_EACH_VEC( m_Panels, i )
		{
			m_Panels[ i ]->ThinkHierarchy( gpGlobals->frametime );
		}
	}
}

void CEnvHoloSystem::GetRenderBounds( Vector& mins, Vector& maxs )
{
	if ( GetOwnerEntity() )
	{
		GetOwnerEntity()->GetRenderBounds( mins, maxs );
	}
	else
	{
		BaseClass::GetRenderBounds( mins, maxs );
	}
}

void CEnvHoloSystem::GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
{
	if ( GetOwnerEntity() )
	{
		GetOwnerEntity()->GetRenderBoundsWorldspace( mins, maxs );
	}
	else
	{
		BaseClass::GetRenderBoundsWorldspace( mins, maxs );
	}
}

void CEnvHoloSystem::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		CreatePanels();
	}

	C_BaseAnimating *pAttachedModel = assert_cast< C_BaseAnimating* >( GetOwnerEntity() );
	if ( pAttachedModel && *m_szAttachment )
	{
		m_iAttachment = pAttachedModel->LookupAttachment( m_szAttachment );
		m_iEyes = pAttachedModel->LookupAttachment( "eyes" );
	}
	else
	{
		m_iAttachment = -1;
	}
}

void CEnvHoloSystem::CreatePanels()
{
	DestroyPanels();

	CSpacecraft *pSpacecraft = assert_cast< CSpacecraft* >( GetOwnerEntity() );

	m_Panels.AddToTail( new CHoloShipHealthGraphic( pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipHealthText( pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipModel( pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipEngine( pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipThruster( pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipRadar( pSpacecraft ) );
	m_pAimPanel = new CHoloShipAim( pSpacecraft );
	m_Panels.AddToTail( m_pAimPanel );
	m_Panels.AddToTail( new CHoloShipObjectives( pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipComm( pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipAimInfo( pSpacecraft ) );

	FOR_EACH_VEC( m_Panels, i )
	{
		m_Panels[ i ]->Setup();
	}
}

void CEnvHoloSystem::DestroyPanels()
{
	m_pAimPanel = NULL;
	m_Panels.PurgeAndDeleteElements();
}

void CEnvHoloSystem::MsgFuncSpacecraftDamage( bf_read &msg )
{
	if ( m_pAimPanel && GetOwnerEntity() )
	{
		m_pAimPanel->MsgFuncSpacecraftDamage( msg );
	}
}

void CEnvHoloSystem::PreRenderPanels()
{
	ITexture *pHoloGUITexture = g_pGstringRenderTargets->GetHoloGUITexture();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PushRenderTargetAndViewport( pHoloGUITexture );
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->ClearBuffers( true, false );

	Rect_t rect;
	rect.x = rect.y = rect.width = rect.height = 0;
	const int width = pHoloGUITexture->GetActualWidth();
	const int height = pHoloGUITexture->GetActualHeight();

	FOR_EACH_VEC( m_Panels, i )
	{
		m_Panels[ i ]->PreRenderHierarchy( pRenderContext, rect, width, height );
	}

	Assert( ( rect.x + rect.width ) <= width && ( rect.y + rect.height ) <= height );

	pRenderContext->PopRenderTargetAndViewport();
}

void CEnvHoloSystem::DrawPanels()
{
	CMatRenderContextPtr pRenderContext( materials );

	FOR_EACH_VEC( m_Panels, i )
	{
		m_Panels[ i ]->DrawHierarchy( pRenderContext );
	}
}

#endif
