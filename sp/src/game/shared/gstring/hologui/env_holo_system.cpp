
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

#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui_controls/AnimationController.h"

#include "materialsystem/itexture.h"
#include "vgui/ISurface.h"
#include "sourcevr/isourcevirtualreality.h"
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CEnvHoloSystem )

	DEFINE_FIELD( m_hHoloEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_strAttachment, FIELD_STRING, "Attachment" ),
	DEFINE_KEYFIELD( m_strHoloEntity, FIELD_STRING, "HoloEntity" ),
	DEFINE_KEYFIELD( m_iUIState, FIELD_INTEGER, "UIState" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "PlayAnimation", InputPlayAnimation ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// DEFINE_FIELD( m_bLensflareEnabled,		FIELD_BOOLEAN ),

END_DATADESC()
#else

static CEnvHoloSystem *g_pLastHoloSystem;
CON_COMMAND( gstring_holo_play_animation, "" )
{
	if ( g_pLastHoloSystem != NULL && args.ArgC() > 1 )
	{
		DevMsg( "Starting holo animation: %s\n", args[ 1 ] );
		g_pLastHoloSystem->StartAnimation( args[ 1 ] );
	}
}

CON_COMMAND( gstring_holo_reload_animation_script, "" )
{
	if ( g_pLastHoloSystem != NULL )
	{
		g_pLastHoloSystem->ReloadAnimationScript();
	}
}

static void __MsgFunc_HoloAnimation( bf_read &msg )
{
	int entindex = msg.ReadShort();
	C_BaseEntity *pEntity = ClientEntityList().GetBaseEntity( entindex );
	CEnvHoloSystem *pHoloSystem = dynamic_cast<CEnvHoloSystem*>( pEntity );
	if ( pHoloSystem != NULL )
	{
		char szAnimationName[ 128 ];
		msg.ReadString( szAnimationName, sizeof( szAnimationName ) );
		pHoloSystem->StartAnimation( szAnimationName );
	}
}
USER_MESSAGE_REGISTER( HoloAnimation );

static ConVar gstring_holoui_draw( "gstring_holoui_draw", "1" );

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

class CSpacecraftDataStub : public ISpacecraftData
{
public:
	CSpacecraftDataStub( CBaseEntity *pEntity )
		: m_vecVelocity( vec3_origin )
		, m_iAttachment( -1 )
	{
		m_hEntity.Set( pEntity );
	}

	void Update()
	{
		if ( m_hEntity.Get() != NULL )
		{
			if ( m_iAttachment < 0 )
			{
				m_iAttachment = m_hEntity->LookupAttachment( "engine_2" );
			}
			Quaternion angularVelocity;
			m_hEntity->GetAttachmentVelocity( m_iAttachment, m_vecVelocity, angularVelocity );
		}
	}

	virtual int GetShield() const
	{
		return 100;
	}

	virtual int GetMaxShield() const
	{
		return 100;
	}

	virtual int GetHull() const
	{
		return 100;
	}

	virtual int GetMaxHull() const
	{
		return 100;
	}

	virtual CBaseEntity *GetEntity()
	{
		return m_hEntity;
	}

	virtual const QAngle &GetAngularImpulse() const
	{
		return vec3_angle;
	}

	virtual const Vector &GetPhysVelocity() const
	{
		return m_vecVelocity;
	}

	virtual EngineLevel_e GetEngineLevel() const
	{
		return m_vecVelocity.LengthSqr() > 1.0f ? ISpacecraftData::ENGINELEVEL_NORMAL : ISpacecraftData::ENGINELEVEL_IDLE;
	}

	virtual bool IsBoostSuspended() const
	{
		return false;
	}

	virtual int GetThrusterCount() const
	{
		return 8;
	}

	virtual float GetThrusterPower( int index ) const
	{
		return 0.0f;
	}

private:
	EHANDLE m_hEntity;
	int m_iAttachment;
	Vector m_vecVelocity;
};
#endif

IMPLEMENT_NETWORKCLASS_DT( CEnvHoloSystem, CEnvHoloSystem_DT )

#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_iUIState ) ),
	SendPropString( SENDINFO( m_szAttachment ) ),
	SendPropEHandle( SENDINFO( m_hHoloEntity ) ),
#else
	RecvPropInt( RECVINFO( m_iUIState ) ),
	RecvPropString( RECVINFO( m_szAttachment ) ),
	RecvPropEHandle( RECVINFO( m_hHoloEntity ) ),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( env_holo_system, CEnvHoloSystem );

CEnvHoloSystem::CEnvHoloSystem()
#ifdef CLIENT_DLL
	: m_iAttachment( -1 )
	, m_iEyes( -1 )
	, m_iViewportWidth( 0 )
	, m_iViewportHeight( 0 )
	, m_bIsAnimating( false )
	, m_pSpacecraftDataAdapter( NULL )
	, m_pRoot( NULL )
	, m_pAnimationController( NULL )
#else
	: m_bEnabled( false )
#endif
{
#ifdef CLIENT_DLL
	if ( g_pLastHoloSystem == NULL )
	{
		g_pLastHoloSystem = this;
	}
#endif
}

CEnvHoloSystem::~CEnvHoloSystem()
{
#ifdef CLIENT_DLL
	DestroyPanels();
	delete m_pSpacecraftDataAdapter;

	if ( g_pLastHoloSystem == this )
	{
		g_pLastHoloSystem = NULL;
	}
#endif
}

#ifdef GAME_DLL

void CEnvHoloSystem::Spawn()
{
	BaseClass::Spawn();

	m_bEnabled = HasSpawnFlags( 1 );
}

void CEnvHoloSystem::Activate()
{
	BaseClass::Activate();

	Q_strncpy( m_szAttachment.GetForModify(), STRING( m_strAttachment ), 16 );

	m_hHoloEntity = gEntList.FindEntityByName( NULL, m_strHoloEntity, this );
	if ( m_hHoloEntity.Get() != NULL )
	{
		SetOwnerEntity( m_hHoloEntity );
	}
}

int CEnvHoloSystem::UpdateTransmitState()
{
	return SetTransmitState( m_bEnabled ? FL_EDICT_ALWAYS : FL_EDICT_DONTSEND );
}

void CEnvHoloSystem::InputPlayAnimation( inputdata_t &inputdata )
{
	if ( inputdata.value.FieldType() == FIELD_STRING || inputdata.value.Convert( FIELD_STRING ) )
	{
		CBasePlayer *pLocal = UTIL_GetLocalPlayer();

		CSingleUserRecipientFilter user( pLocal );
		user.MakeReliable();
		UserMessageBegin( user, "HoloAnimation" );
			WRITE_SHORT( entindex() );
			WRITE_STRING( inputdata.value.String() );
		MessageEnd();
	}
}

void CEnvHoloSystem::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
	DispatchUpdateTransmitState();
}

void CEnvHoloSystem::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	DispatchUpdateTransmitState();
}

#else

int CEnvHoloSystem::DrawModel( int flags )
{
	if ( ( flags & STUDIO_SHADOWDEPTHTEXTURE ) != 0 ||
		( flags & STUDIO_SSAODEPTHTEXTURE ) != 0 ||
		( flags & STUDIO_TRANSPARENCY ) == 0 ||
		!GetOwnerEntity() ||
		!gstring_holoui_draw.GetBool() )
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
	return RENDER_GROUP_TRANSLUCENT_ENTITY;
}

void CEnvHoloSystem::ClientThink()
{
	if ( m_pSpacecraftDataAdapter != NULL )
	{
		m_pSpacecraftDataAdapter->Update();
	}

	if ( m_pAnimationController != NULL )
	{
		m_pAnimationController->UpdateAnimations( gpGlobals->curtime );

		const bool bIsAnimating = m_pAnimationController->GetNumActiveAnimations();
		if ( bIsAnimating || m_bIsAnimating )
		{
			FOR_EACH_VEC( m_Panels, i )
			{
				m_Panels[ i ]->MakeDirty();
			}
		}
		m_bIsAnimating = bIsAnimating;
	}

	if ( GetOwnerEntity() == NULL )
	{
		DestroyPanels();
	}
	else if ( gpGlobals->frametime > 0.0f )
	{
		int x, y, width, height;
		vgui::surface()->GetFullscreenViewport( x, y, width, height );

		if ( width != m_iViewportWidth || height != m_iViewportHeight )
		{
			m_iViewportWidth = width;
			m_iViewportHeight = height;
			FOR_EACH_VEC( m_Panels, i )
			{
				m_Panels[ i ]->PerformLayout3DHierarchy( width, height, UseVR() );
			}
		}

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

		if ( m_pAnimationController != NULL )
		{
			m_pAnimationController->UpdateAnimations( gpGlobals->curtime );
			switch ( m_iUIState )
			{
				// Parked ship
				case 1:
					{
						StartAnimation( "shipparked" );
					}
					break;
			}
		}
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

	ISpacecraftData *pSpacecraft = dynamic_cast< CSpacecraft* >( GetOwnerEntity() );

	if ( pSpacecraft == NULL )
	{
		if ( m_pSpacecraftDataAdapter == NULL )
		{
			m_pSpacecraftDataAdapter = new CSpacecraftDataStub( m_hHoloEntity );
		}
		pSpacecraft = m_pSpacecraftDataAdapter;
	}

	ITexture *pHoloGUITexture = g_pGstringRenderTargets->GetHoloGUITexture();

	m_pRoot = new vgui::Panel();
	m_pRoot->SetBounds( 0, 0, pHoloGUITexture->GetActualWidth(), pHoloGUITexture->GetActualHeight() );
	m_pRoot->MakeReadyForUse();

	m_Panels.AddToTail( new CHoloShipHealthGraphic( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipHealthText( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipModel( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipEngine( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipThruster( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipRadar( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipAim( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipObjectives( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipComm( m_pRoot, pSpacecraft ) );
	m_Panels.AddToTail( new CHoloShipAimInfo( m_pRoot, pSpacecraft ) );

	m_pAnimationController = new vgui::AnimationController( m_pRoot );
	ReloadAnimationScript();

	int x, y, width, height;
	vgui::surface()->GetFullscreenViewport( x, y, width, height );

	FOR_EACH_VEC( m_Panels, i )
	{
		m_Panels[ i ]->PerformLayout3DHierarchy( width, height, UseVR() );
	}

	FOR_EACH_VEC( m_Panels, i )
	{
		m_Panels[ i ]->Setup();
	}
}

void CEnvHoloSystem::DestroyPanels()
{
	FOR_EACH_VEC( m_Panels, i )
	{
		m_Panels[ i ]->SetParent( (vgui::Panel*)NULL );
	}
	m_Panels.PurgeAndDeleteElements();
	if ( m_pRoot != NULL )
	{
		m_pRoot->DeletePanel();
	}
	m_pRoot = NULL;
	m_pAnimationController = NULL;
}

void CEnvHoloSystem::StartAnimation( const char *pszName )
{
	if ( m_pAnimationController != NULL )
	{
		if ( !m_pAnimationController->StartAnimationSequence( pszName ) )
		{
			DevMsg( "Failed to play holo animation: %s\n", pszName );
		}
	}
}

void CEnvHoloSystem::ReloadAnimationScript()
{
	if ( !m_pAnimationController || !m_pAnimationController->SetScriptFile( m_pRoot->GetVPanel(), "scripts/holoanimations.txt", true ) )
	{
		DevMsg( "Failed loading holo animations!\n" );
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

	if ( m_pRoot != NULL )
	{
		vgui::ipanel()->PerformApplySchemeSettings( m_pRoot->GetVPanel() );
		vgui::surface()->SolveTraverse( m_pRoot->GetVPanel() );
	}

	FOR_EACH_VEC( m_Panels, i )
	{
		m_Panels[ i ]->PreRenderHierarchy( pRenderContext, rect, width, height );
	}

	vgui::surface()->DrawSetAlphaMultiplier( 1.0f );

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
