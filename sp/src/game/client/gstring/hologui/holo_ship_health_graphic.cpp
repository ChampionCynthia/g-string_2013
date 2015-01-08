
#include "cbase.h"
#include "holo_ship_health_graphic.h"
#include "holo_utilities.h"
#include "gstring/cspacecraft.h"

#include "materialsystem/imaterialvar.h"

const float flStepAngle = 180.0f / 13.0f;

CHoloShipHealthGraphic::CHoloShipHealthGraphic( ISpacecraftData *pSpacecraftData ) :
	m_pSpacecraftData( pSpacecraftData ),
	m_flHullFraction( 0.0f ),
	m_flShieldFraction( 0.0f )
{
	SetAngles( QAngle( -25, 20, -3 ) );
	SetOrigin( Vector( 0, 8, -7 ) );

	CMatRenderContextPtr pRenderContext( materials );
	m_pHullElement = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );

	CreateArc( m_pHullElement, 8, 3.0f, 0.5f, DEG2RAD( 0.5f ), DEG2RAD( flStepAngle - 0.5f ) );
}

CHoloShipHealthGraphic::~CHoloShipHealthGraphic()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DestroyStaticMesh( m_pHullElement );
}

void CHoloShipHealthGraphic::Draw( IMatRenderContext *pRenderContext )
{
	pRenderContext->PushMatrix();
	matrix3x4_t dst;
	MatrixBuildRotationAboutAxis( Vector( 0, 0, 1 ), -flStepAngle, dst);

	pRenderContext->Bind( GetMaterial() );

	IMaterialVar *pColor = GetColorVar();
	IMaterialVar *pAlpha = GetAlphaVar();
	pColor->SetVecValue( HOLO_COLOR_DEFAULT );

	const float flHealthPercentage = m_flHullFraction * 7.0f;
	for ( int s = 0; s < 2; ++s )
	{
		for ( int i = 0; i < 13; ++i )
		{
			const int index = abs( i - 6 );
			if ( index <= flHealthPercentage )
			{
				float flAlpha = 0.6f;
				flAlpha *= 1.0f - abs( index ) / 10.0f;
				const float flLocalPercentage = flHealthPercentage - index;
				if ( flLocalPercentage >= 0.0f && flLocalPercentage < 1.0f )
				{
					flAlpha *= fmodf( flLocalPercentage, 1.0f );
				}
				pAlpha->SetFloatValue( flAlpha );

				m_pHullElement->Draw();
			}
			pRenderContext->MultMatrixLocal(dst);
		}
	}
	pRenderContext->PopMatrix();

	if ( m_flShieldFraction > 0.0f )
	{
		const float flFadeRange = 0.5f * m_flShieldFraction * ( 1.0f - m_flShieldFraction * 0.9f );

		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_VERTEXCOLOR ) );
		CreateArcFaded( pMesh, 25, 3.4f, 0.1f, 0.0f, DEG2RAD( 180.0f ), m_flShieldFraction - 0.001f, m_flShieldFraction + flFadeRange );
		GetColorVar( MATERIALTYPE_VERTEXCOLOR )->SetVecValue( HOLO_COLOR_HIGHLIGHT );
		GetAlphaVar( MATERIALTYPE_VERTEXCOLOR )->SetFloatValue( 1.0f );

		pMesh->Draw();

		MatrixBuildRotationAboutAxis( Vector( 0, 0, 1 ), 180, dst );
		pRenderContext->MultMatrixLocal(dst);
		pMesh->Draw();
	}
}

void CHoloShipHealthGraphic::Think( float frametime )
{
	const float flHullDesired = m_pSpacecraftData->GetHull() / (float)m_pSpacecraftData->GetMaxHull();
	m_flHullFraction += ( flHullDesired - m_flHullFraction ) * frametime * 5.0f;

	const float flShieldDesired = m_pSpacecraftData->GetShield() / (float)m_pSpacecraftData->GetMaxShield();
	m_flShieldFraction = Approach( flShieldDesired, m_flShieldFraction, frametime * 1.5f );
}

