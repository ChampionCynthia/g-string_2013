
#include "cbase.h"
#include "holo_ship_engine.h"
#include "holo_utilities.h"
#include "gstring/hologui/env_holo_system.h"
#include "gstring/cspacecraft.h"
#include "gstring/cgstring_globals.h"

#include "materialsystem/imaterialvar.h"
#include "vgui_controls/Label.h"
#include "vgui/ISurface.h"

using namespace vgui;

CHoloShipEngine::CHoloShipEngine( ISpacecraftData *pSpacecraftData ) :
	m_pSpacecraftData( pSpacecraftData ),
	m_flEngineStrength( 0.0f )
{
	SetIdentityMatrix( m_matrixUp );
	SetIdentityMatrix( m_matrixCenterOffset );
	SetIdentityMatrix( m_matrixEndOffset );
	MatrixSetTranslation( Vector( 0, 0.0f, 0.45f ), m_matrixUp );
	MatrixSetTranslation( Vector( 0, 0.0f, 0.15f ), m_matrixCenterOffset );
	MatrixSetTranslation( Vector( 0, 0.0f, -0.15f ), m_matrixEndOffset );

	m_pLabelEngine = new Label( this, "", "#holo_gui_engine" );
	m_pLabelSpeedLabel = new Label( this, "", "#holo_gui_speed" );
	m_pLabelSpeedValue = new Label( this, "", "" );
	CMatRenderContextPtr pRenderContext( materials );
	m_pMeshElement = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	CreateSlantedRect( m_pMeshElement, 0, 0, 1.2f, 0.35f, -0.125f );

	SetOrigin( Vector( 0, -5.5f, -7.5f ) );
	SetAngles( QAngle( 0, 160, 0 ) );
}

CHoloShipEngine::~CHoloShipEngine()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DestroyStaticMesh( m_pMeshElement );
}

void CHoloShipEngine::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelEngine->SetFont( m_FontLarge );
	m_pLabelEngine->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelEngine->SizeToContents();
	m_pLabelSpeedLabel->SetFont( m_FontSmall );
	m_pLabelSpeedLabel->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelSpeedLabel->SizeToContents();
	m_pLabelSpeedValue->SetFont( m_FontSmallMono );
	m_pLabelSpeedValue->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );

	const int iLargeTall = surface()->GetFontTall( m_FontLarge );
	const int iSmallTall = surface()->GetFontTall( m_FontSmall );

	const int iWide = 150;
	m_pLabelEngine->SetPos( 3, 0 );
	m_pLabelEngine->SetWide( iWide - 3 );
	m_pLabelEngine->SetContentAlignment( Label::a_center );

	m_pLabelSpeedLabel->SetPos( 0, iLargeTall );
	m_pLabelSpeedValue->SetPos( 0, iLargeTall );
	SetBounds( 0, 0, m_pLabelEngine->GetWide() + 3, iLargeTall + iSmallTall );
	m_pLabelSpeedValue->SetSize( iWide, iSmallTall );
	m_pLabelSpeedValue->SetContentAlignment( Label::a_east );
}

void CHoloShipEngine::Draw( IMatRenderContext *pRenderContext )
{
	m_vecPanelWorldOffset.x = -0.4f;
	BaseClass::Draw( pRenderContext );

	matrix3x4_t mat;
	SetIdentityMatrix( mat );
	MatrixSetTranslation( Vector( 0, -0.6f, 0.2f ), mat );
	pRenderContext->MultMatrixLocal( mat );

	GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	IMaterialVar *pAlpha = GetAlphaVar();

	const int iElementCount = 5;
	const float flAlphaMin = 0.02f;
	const float flAlphaDelta = 0.4f;
	const float flFadeRange = 1.0f / iElementCount;
	const bool bDoPulse = m_flEngineStrength >= 0.9f;

	MatrixSetTranslation( Vector( 0, 1.3f, 0 ), mat );
	for ( int i = 0; i < 3; ++i )
	{
		pRenderContext->PushMatrix();
		if ( i > 0 )
		{
			pRenderContext->MultMatrixLocal( i == 1 ? m_matrixCenterOffset : m_matrixEndOffset );
		}
		for ( int u = 0; u < iElementCount; ++u )
		{
			float flPulse = 0.0f;
			if ( bDoPulse )
			{
				flPulse = sin( gpGlobals->curtime * 4.0f - u * 0.5f ) - 0.9f;
				flPulse = MAX( 0.0f, flPulse );
				flPulse *= flPulse * 40.0f;
			}

			const float flFadeSelfStart = u / float( iElementCount );
			float flAlpha = ( m_flEngineStrength - flFadeSelfStart ) / flFadeRange;
			flAlpha = Clamp( flAlpha, 0.0f, 1.0f );
			pAlpha->SetFloatValue( flAlphaMin + flAlpha * flAlphaDelta + flPulse );

			pRenderContext->Bind( GetMaterial() );
			m_pMeshElement->Draw();
			pRenderContext->MultMatrixLocal( m_matrixUp );
		}
		pRenderContext->PopMatrix();
		pRenderContext->MultMatrixLocal( mat );
	}

	// Draw glow
	matrix3x4_t viewMatrixInv = CurrentHoloViewMatrixInverted();
	MatrixSetTranslation( Vector( -1, -2, 0.8f ), viewMatrixInv );
	pRenderContext->MultMatrixLocal( viewMatrixInv );

	GetColorVar( MATERIALTYPE_GLOW )->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar( MATERIALTYPE_GLOW )->SetFloatValue( 0.02f + 0.03f * m_flEngineStrength );

	const float flScale = 6.0f;
	IMesh *pMeshGlow = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_GLOW ) );
	CreateTexturedRect( pMeshGlow, -flScale, -flScale, flScale * 2, flScale * 2 );
	pMeshGlow->Draw();
}

void CHoloShipEngine::Think( float frametime )
{
	BaseClass::Think( frametime );

	const ISpacecraftData::EngineLevel_e engineLevel = m_pSpacecraftData->GetEngineLevel();
	float flDesiredEngineStrength = 0.0f;
	switch ( engineLevel )
	{
	case ISpacecraftData::ENGINELEVEL_IDLE:
		flDesiredEngineStrength = 0.1f;
		break;

	case ISpacecraftData::ENGINELEVEL_NORMAL:
		flDesiredEngineStrength = 0.5f;
		break;

	case ISpacecraftData::ENGINELEVEL_BOOST:
		flDesiredEngineStrength = 1.0f;
	}

	const float flApproachSpeed = flDesiredEngineStrength > m_flEngineStrength ? 4.0f : 1.2f;
	m_flEngineStrength = Approach( flDesiredEngineStrength, m_flEngineStrength, frametime * flApproachSpeed );

	const float flMetersPerSecondToKmH = 0.001f * 60.0f * 60.0f;
	const float flWorldScaleInv = g_pGstringGlobals ? 1.0f / g_pGstringGlobals->GetWorldScale() : 1.0f;
	const float flSpeed = UNITS2METERS( m_pSpacecraftData->GetPhysVelocity().Length() ) * flWorldScaleInv * flMetersPerSecondToKmH;
	m_pLabelSpeedValue->SetText( VarArgs( "%.0f km/h", flSpeed ) );
	m_pLabelSpeedValue->MakeReadyForUse();
}
