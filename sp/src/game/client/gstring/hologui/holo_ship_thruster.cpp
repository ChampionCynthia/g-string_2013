
#include "cbase.h"
#include "holo_ship_thruster.h"
#include "holo_utilities.h"
#include "gstring/cspacecraft.h"

#include "materialsystem/imaterialvar.h"
#include "vgui_controls/Label.h"

using namespace vgui;

CHoloShipThruster::CHoloShipThruster( ISpacecraftData *pSpacecraftData ) :
	m_pSpacecraftData( pSpacecraftData ),
	m_flThrusterStrength( 0.0f )
{
	m_pLabelThruster = new Label( this, "", "#holo_gui_thruster" );
	CMatRenderContextPtr pRenderContext( materials );
	m_pMeshElement = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	CreateSlantedRect( m_pMeshElement, 0, 0, 0.2f, 0.3f );

	SetOrigin( Vector( -1.2f, -9.5f, -7.0f ) );
	SetAngles( QAngle( 0, 150, 0 ) );
	m_flScale = 0.015f;
}

CHoloShipThruster::~CHoloShipThruster()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DestroyStaticMesh( m_pMeshElement );
}

void CHoloShipThruster::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelThruster->SetFont( m_FontLarge );
	m_pLabelThruster->SetFgColor( Color( HOLO_COLOR255_HIGHLIGHT, 255 ) );
	m_pLabelThruster->SizeToContents();

	m_pLabelThruster->SetPos( 3, 0 );
	SetBounds( 0, 0, m_pLabelThruster->GetWide() + 3, m_pLabelThruster->GetTall() );
}

void CHoloShipThruster::Draw( IMatRenderContext *pRenderContext )
{
	BaseClass::Draw( pRenderContext );

	matrix3x4_t mat, up;
	SetIdentityMatrix( mat );
	SetIdentityMatrix( up );
	MatrixSetTranslation( Vector( 0, 0.2f, 0.2f ), mat );
	MatrixSetTranslation( Vector( 0, 0.0f, 0.4f ), up );
	pRenderContext->MultMatrixLocal( mat );

	GetColorVar()->SetVecValue( HOLO_COLOR_HIGHLIGHT );
	IMaterialVar *pAlpha = GetAlphaVar();

	const int iThrusterCount = m_pSpacecraftData->GetThrusterCount();
	const int iElementCount = 3;
	const float flAlphaMin = 0.02f;
	const float flAlphaDelta = 0.9f;
	const float flFadeRange = 1.0f / iElementCount;

	MatrixSetTranslation( Vector( 0, 0.3f, 0 ), mat );
	for ( int i = 0; i < iThrusterCount; ++i )
	{
		pRenderContext->PushMatrix();
		for ( int u = 0; u < iElementCount; ++u )
		{
			float flThrusterPower = m_pSpacecraftData->GetThrusterPower( i ) * 0.1f;
			flThrusterPower = clamp( flThrusterPower, 0, 1 );

			const float flFadeSelfStart = u / float( iElementCount );
			float flAlpha = ( flThrusterPower - flFadeSelfStart ) / flFadeRange;
			flAlpha = Clamp( flAlpha, 0.0f, 1.0f );
			pAlpha->SetFloatValue( flAlphaMin + flAlpha * flAlphaDelta );

			pRenderContext->Bind( GetMaterial() );
			m_pMeshElement->Draw();
			pRenderContext->MultMatrixLocal( up );
		}
		pRenderContext->PopMatrix();
		pRenderContext->MultMatrixLocal( mat );
	}
}

void CHoloShipThruster::Think( float frametime )
{
	BaseClass::Think( frametime );

	//const ISpacecraftData::EngineLevel_e engineLevel = m_pSpacecraftData->GetEngineLevel();
	//float flDesiredEngineStrength = 0.0f;
	//switch ( engineLevel )
	//{
	//case ISpacecraftData::ENGINELEVEL_IDLE:
	//	flDesiredEngineStrength = 0.1f;
	//	break;

	//case ISpacecraftData::ENGINELEVEL_NORMAL:
	//	flDesiredEngineStrength = 0.5f;
	//	break;

	//case ISpacecraftData::ENGINELEVEL_BOOST:
	//	flDesiredEngineStrength = 1.0f;
	//}

	//const float flApproachSpeed = flDesiredEngineStrength > m_flEngineStrength ? 4.0f : 1.2f;
	//m_flEngineStrength = Approach( flDesiredEngineStrength, m_flEngineStrength, frametime * flApproachSpeed );
}
