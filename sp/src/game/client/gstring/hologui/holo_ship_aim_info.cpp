
#include "cbase.h"
#include "holo_ship_aim_info.h"
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

CHoloShipAimInfo::CHoloShipAimInfo( vgui::Panel *pParent, ISpacecraftData *pSpacecraftData ) :
	BaseClass( pParent, "info" ),
	m_pSpacecraftData( pSpacecraftData ),
	m_iHealth( -1 ),
	m_iDistance( 0.0f ),
	m_pTarget( NULL )
{
	m_pLabelTarget = new Label( this, "", "" );
	m_pLabelDash = new Label( this, "", "-" );
	m_pLabelHealth = new Label( this, "", "" );
	m_pLabelDistance = new Label( this, "", "" );
	m_pLabelDash->SetVisible( false );

	const int iWide = 270;
	m_flScale = 0.03f;
	SetOrigin( Vector( 4.0f, iWide * m_flScale * 0.5f, -4.7f ) );
	SetAngles( QAngle( 0, 180, 0 ) );
	SetWide( iWide );
}

CHoloShipAimInfo::~CHoloShipAimInfo()
{
}

void CHoloShipAimInfo::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelTarget->SetFont( m_FontSmall );
	m_pLabelTarget->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 255 ) );
	m_pLabelTarget->SizeToContents();
	m_pLabelDash->SetFont( m_FontSmall );
	m_pLabelDash->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelHealth->SetFont( m_FontSmallMono );
	m_pLabelHealth->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelHealth->SetContentAlignment( Label::a_east );
	m_pLabelDistance->SetFont( m_FontSmallMono );
	m_pLabelDistance->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );

	const int iFontTall = surface()->GetFontTall( m_FontSmall );

	SetTall( iFontTall * 2 );
	const int wide = GetWide();

	m_pLabelTarget->SetBounds( 0, 0, wide, iFontTall );

	m_pLabelDash->SizeToContents();
	m_pLabelDash->SetPos( wide - 70, 0 );

	m_pLabelHealth->SetBounds( wide - 70, 0, 70, iFontTall );
	m_pLabelDistance->SetBounds( 0, iFontTall, wide, iFontTall );

	m_pTarget = NULL;
	m_pLabelTarget->SetText( SafeLocalize( "holo_gui_notarget" ) );
	m_pLabelHealth->SetText( "" );
	m_pLabelTarget->SetContentAlignment( Label::a_center );
	m_pLabelDistance->SetContentAlignment( Label::a_center );
}

void CHoloShipAimInfo::Draw( IMatRenderContext *pRenderContext )
{
	BaseClass::Draw( pRenderContext );

	// Draw glow
	matrix3x4_t viewMatrixInv = CurrentHoloViewMatrixInverted();
	MatrixSetTranslation( Vector( 0, 270 * m_flScale * 0.5f, -0.5f ), viewMatrixInv );
	pRenderContext->MultMatrixLocal( viewMatrixInv );

	GetColorVar( MATERIALTYPE_GLOW )->SetVecValue( HOLO_COLOR_DEFAULT );
	SetHoloAlpha( 0.03f, MATERIALTYPE_GLOW );

	const float flScale = 3.0f;
	const float flRatio = 3.0f;
	IMesh *pMeshGlow = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_GLOW ) );
	CreateTexturedRect( pMeshGlow, flScale * -flRatio, -flScale, flScale * 2 * flRatio, flScale * 2 );
	pMeshGlow->Draw();
}

void CHoloShipAimInfo::Think( float frametime )
{
	BaseClass::Think( frametime );

	const float flWorldScaleInv = g_pGstringGlobals ? 1.0f / g_pGstringGlobals->GetWorldScale() : 1.0f;
	const IHoloTarget *pAutoAimTarget = GetGstringInput()->GetAutoAimTarget();
	const float flHealth = pAutoAimTarget ? ceil( pAutoAimTarget->GetHealthPercentage() * 100.0f ) : -1;
	const int iHealth = flHealth;
	if ( m_pTarget != pAutoAimTarget )
	{
		m_pTarget = pAutoAimTarget;
		wchar_t wszTarget[ 128 ] = { 0 };
		const wchar_t *pwsTarget;
		if ( pAutoAimTarget == NULL )
		{
			pwsTarget = SafeLocalize( "holo_gui_notarget" );
			m_pLabelTarget->SetContentAlignment( Label::a_center );
		}
		else
		{
			g_pVGuiLocalize->ConstructString( wszTarget, sizeof( wszTarget ),
				SafeLocalizeInline( "holo_gui_target" ).Get(),
				1, SafeLocalizeInline( pAutoAimTarget->GetName() ).Get() );
			pwsTarget = wszTarget;
			m_pLabelTarget->SetContentAlignment( flHealth >= 0.0f ? Label::a_west : Label::a_center );
		}
		m_pLabelTarget->SetText( pwsTarget );
		//m_pLabelDash->SetVisible( pAutoAimTarget != NULL );
	}

	if ( iHealth != m_iHealth )
	{
		m_iHealth = iHealth;
		m_pLabelHealth->SetText( iHealth >= 0 ? VarArgs( "%i%%", iHealth ) : "" );
		m_pLabelDistance->SetContentAlignment( iHealth >= 0 ? Label::a_east : Label::a_center );
	}

	const int iDistance = pAutoAimTarget ?
		UNITS2METERS( pAutoAimTarget->GetEntity()->WorldSpaceCenter() - m_pSpacecraftData->GetEntity()->WorldSpaceCenter() ).Length() * 0.01f * flWorldScaleInv :
		-1;
	if ( iDistance != m_iDistance )
	{
		m_iDistance = iDistance;
		float km = iDistance * 0.1f;
		m_pLabelDistance->SetText( iDistance >= 0 ? VarArgs( "%.1f km", km ) : "" );
	}
}
