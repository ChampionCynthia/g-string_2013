
#include "cbase.h"
#include "holo_ship_autopilot.h"
#include "holo_utilities.h"
#include "gstring/hologui/env_holo_system.h"
#include "gstring/hologui/point_holo_target.h"
#include "gstring/cspacecraft.h"
#include "gstring/gstring_in_main.h"
#include "gstring/cgstring_globals.h"
#include "gstring/c_gstring_util.h"

#include "materialsystem/imaterialvar.h"
#include "vgui_controls/Label.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"

using namespace vgui;

CHoloShipAutopilot::CHoloShipAutopilot( vgui::Panel *pParent, ISpacecraftData *pSpacecraftData ) :
	BaseClass( pParent, "autopilot" ),
	m_pSpacecraftData( pSpacecraftData )
{
	m_pLabelAutopilot = new Label( this, "", "#holo_gui_autopilot" );

	m_MaterialAutopilotIcon.Init( materials->FindMaterial( "hud/holo_autopilot_icon", TEXTURE_GROUP_OTHER ) );

	const int iWide = 270;
	m_flScale = 0.03f;
	SetOrigin( Vector( 0.0f, iWide * m_flScale * 0.5f, 1.0f ) );
	SetAngles( QAngle( 0, 180.0f, 0 ) );
	SetWide( iWide );

	CMatRenderContextPtr pRenderContext( materials );
	m_pMeshIcon = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, m_MaterialAutopilotIcon );
	m_pMeshOutline = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );

	CreateTexturedRectHolo( m_pMeshIcon, 0, 0, 0.8f, 0.8f );
	CreateLineRect( m_pMeshOutline, 3.0f, 1.65f, 0.2f );
}

CHoloShipAutopilot::~CHoloShipAutopilot()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DestroyStaticMesh( m_pMeshIcon );
	pRenderContext->DestroyStaticMesh( m_pMeshOutline );
}

void CHoloShipAutopilot::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelAutopilot->SetFont( m_FontLarge );
	m_pLabelAutopilot->SetFgColor( Color( HOLO_COLOR255_WARNING, 255 ) );
	m_pLabelAutopilot->SetContentAlignment( Label::a_center );

	const int iFontTall = surface()->GetFontTall( m_FontLarge );
	SetTall( iFontTall * 2 );

	m_pLabelAutopilot->SetBounds( 0, 0, GetWide(), GetTall() );

	//m_pLabelTarget->SizeToContents();
	//m_pLabelDash->SetFont( m_FontSmall );
	//m_pLabelDash->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	//m_pLabelHealth->SetFont( m_FontSmallMono );
	//m_pLabelHealth->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	//m_pLabelHealth->SetContentAlignment( Label::a_east );
	//m_pLabelDistance->SetFont( m_FontSmallMono );
	//m_pLabelDistance->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );

	//const int iFontTall = surface()->GetFontTall( m_FontSmall );

	//SetTall( iFontTall * 2 );
	//const int wide = GetWide();

	//m_pLabelTarget->SetBounds( 0, 0, wide, iFontTall );

	//m_pLabelDash->SizeToContents();
	//m_pLabelDash->SetPos( wide - 70, 0 );

	//m_pLabelHealth->SetBounds( wide - 70, 0, 70, iFontTall );
	//m_pLabelDistance->SetBounds( 0, iFontTall, wide, iFontTall );

	//m_pTarget = NULL;
	//m_pLabelTarget->SetText( SafeLocalize( "holo_gui_notarget" ) );
	//m_pLabelHealth->SetText( "" );
	//m_pLabelTarget->SetContentAlignment( Label::a_center );
	//m_pLabelDistance->SetContentAlignment( Label::a_center );
}

void CHoloShipAutopilot::Draw( IMatRenderContext *pRenderContext )
{
	BaseClass::Draw( pRenderContext );

	// Draw glow
	//matrix3x4_t viewMatrixInv = CurrentHoloViewMatrixInverted();
	//MatrixSetTranslation( Vector( 0, 270 * m_flScale * 0.5f, -0.5f ), viewMatrixInv );
	//pRenderContext->MultMatrixLocal( viewMatrixInv );

	//GetColorVar( MATERIALTYPE_GLOW )->SetVecValue( HOLO_COLOR_DEFAULT );
	//SetHoloAlpha( 0.03f, MATERIALTYPE_GLOW );

	//const float flScale = 3.0f;
	//const float flRatio = 3.0f;
	//IMesh *pMeshGlow = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_GLOW ) );
	//CreateTexturedRect( pMeshGlow, flScale * -flRatio, -flScale, flScale * 2 * flRatio, flScale * 2 );
	//pMeshGlow->Draw();

	// Draw outline
	pRenderContext->PushMatrix();

	matrix3x4_t mat;
	SetIdentityMatrix( mat );
	MatrixSetTranslation( Vector( 0.0f, GetOrigin().y, -GetOrigin().z ), mat );
	pRenderContext->MultMatrixLocal( mat );

	pRenderContext->Bind( GetMaterial() );
	SetHoloAlpha( sin( gpGlobals->curtime * 4.0f ) * 0.35f + 0.5f );
	GetColorVar()->SetVecValue( HOLO_COLOR_WARNING );
	m_pMeshOutline->Draw();

	pRenderContext->PopMatrix();

	// Draw icons
	pRenderContext->Bind( m_MaterialAutopilotIcon );
	Vector vecIconColor( HOLO_COLOR_WARNING );
	static unsigned int siIconColor = 0;
	static unsigned int siIconAlpha = 0;
	IMaterialVar *pIconColorVar = m_MaterialAutopilotIcon->FindVarFast( "$color", &siIconColor );
	pIconColorVar->SetVecValue( vecIconColor.Base(), 3 );
	IMaterialVar *pIconAlphaVar = m_MaterialAutopilotIcon->FindVarFast( "$alpha", &siIconAlpha );
	pIconAlphaVar->SetFloatValue( GetHoloAlpha() );

	pRenderContext->PushMatrix();

	MatrixSetTranslation( Vector( 0.0f, -0.3f, -1.3f ), mat );
	pRenderContext->MultMatrixLocal( mat );
	m_pMeshIcon->Draw();

	MatrixSetTranslation( Vector( 0.0f, 7.2f, 0.0f ), mat );
	pRenderContext->MultMatrixLocal( mat );
	m_pMeshIcon->Draw();

	pRenderContext->PopMatrix();
}

void CHoloShipAutopilot::Think( float frametime )
{
	BaseClass::Think( frametime );
}
