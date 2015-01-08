
#include "cbase.h"
#include "holo_ship_radar.h"
#include "holo_utilities.h"
#include "gstring/cspacecraft.h"
#include "gstring/hologui/env_holo_system.h"
#include "gstring/hologui/point_holo_target.h"
#include "gstring/cgstring_globals.h"

#include "materialsystem/imaterialvar.h"

void GetHoloTargetTypeColor( IHoloTarget::TargetType type, Vector &color, float &alpha )
{
	switch ( type )
	{
	case IHoloTarget::FRIENDLY:
		color.Init( HOLO_COLOR_FRIENDLY );
		alpha = 1;
		break;

	case IHoloTarget::ENEMY:
		color.Init( HOLO_COLOR_WARNING );
		alpha = 1.5f;
		break;

	default:
		color.Init( HOLO_COLOR_DEFAULT );
		alpha = 1;
		break;
	}
}

namespace
{
	const float g_flRadarSize = 3.8f;
	const float g_flRadarScale = g_flRadarSize / HOLO_TARGET_MAX_DISTANCE;
	const float g_flRadarFOV = 48.0f;
	const float g_flRadarFieldSize = 0.1f;
}

CHoloShipRadar::CHoloShipRadar( ISpacecraftData *pSpacecraftData ) :
	m_pSpacecraftData( pSpacecraftData )
{
	CMatRenderContextPtr pRenderContext( materials );
	m_pMeshRingLarge = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshRingSmall = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshRingCenter = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshRingLine = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshCircle = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );

	const float flFOVRadians = DEG2RAD( g_flRadarFOV );

	CreateArc( m_pMeshRingLarge, 64, g_flRadarSize, 0.1f, flFOVRadians, (M_PI_F * 2.0f - flFOVRadians) );
	CreateArc( m_pMeshRingSmall, 16, g_flRadarSize, 0.1f, -flFOVRadians, flFOVRadians );
	CreateArc( m_pMeshRingCenter, 16, 0.1f, 0.1f, 0.0f, M_PI_F * 2.0f );
	CreateArc( m_pMeshRingLine, 1, g_flRadarSize, g_flRadarSize, M_PI_F * -0.005f, M_PI_F * 0.005f );
	CreateArc( m_pMeshCircle, 64, g_flRadarSize, g_flRadarSize, 0, M_PI_F * 2.0f );

	SetOrigin( Vector( 2.0f, 0, -8.0f ) );
	SetAngles( QAngle( -20, 0, 0 ) );
}

CHoloShipRadar::~CHoloShipRadar()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DestroyStaticMesh( m_pMeshRingLarge );
	pRenderContext->DestroyStaticMesh( m_pMeshRingSmall );
	pRenderContext->DestroyStaticMesh( m_pMeshRingCenter );
	pRenderContext->DestroyStaticMesh( m_pMeshRingLine );
	pRenderContext->DestroyStaticMesh( m_pMeshCircle );
}

void CHoloShipRadar::Draw( IMatRenderContext *pRenderContext )
{
	const int iRingCount = 4;

	// Draw rings
	matrix3x4_t matrixTemp;
	SetIdentityMatrix( matrixTemp );

	pRenderContext->PushMatrix();
	pRenderContext->Bind( GetMaterial() );
	for ( int i = 0; i < iRingCount; ++i )
	{
		GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
		GetAlphaVar()->SetFloatValue( 0.5f );
		m_pMeshRingLarge->Draw();

		GetColorVar()->SetVecValue( HOLO_COLOR_HIGHLIGHT );
		GetAlphaVar()->SetFloatValue( 1.0f );
		m_pMeshRingSmall->Draw();

		MatrixScaleBy( 0.85f, matrixTemp );
		pRenderContext->MultMatrixLocal( matrixTemp );
	}
	pRenderContext->PopMatrix();

	// Draw center blip
	m_pMeshRingCenter->Draw();

	// Draw FOV borders
	pRenderContext->PushMatrix();
	SetIdentityMatrix( matrixTemp );
	MatrixBuildRotationAboutAxis( Vector( 0, 0, 1 ), g_flRadarFOV, matrixTemp );
	pRenderContext->MultMatrixLocal( matrixTemp );

	m_pMeshRingLine->Draw();

	MatrixBuildRotationAboutAxis( Vector( 0, 0, 1 ), g_flRadarFOV * -2.0f, matrixTemp );
	pRenderContext->MultMatrixLocal( matrixTemp );
	m_pMeshRingLine->Draw();
	pRenderContext->PopMatrix();

	// Draw blips
	GetAlphaVar()->SetFloatValue( 1.0f );

	pRenderContext->ClearBuffers( false, false, true );
	pRenderContext->SetStencilEnable( true );
	pRenderContext->SetStencilWriteMask( 1 );
	pRenderContext->SetStencilTestMask( 1 );
	pRenderContext->SetStencilReferenceValue( 1 );
	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_NEVER );
	pRenderContext->SetStencilFailOperation( STENCILOPERATION_REPLACE );
	pRenderContext->SetStencilPassOperation( STENCILOPERATION_KEEP );
	pRenderContext->SetStencilZFailOperation( STENCILOPERATION_KEEP );
	m_pMeshCircle->Draw();

	GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar()->SetFloatValue( 0.2f );

	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_EQUAL );

	pRenderContext->SetStencilWriteMask( 0 );
	DrawBlips( pRenderContext );

	pRenderContext->SetStencilEnable( false );

	// Draw glow
	matrix3x4_t viewMatrixInv = CurrentHoloViewMatrixInverted();
	MatrixSetTranslation( vec3_origin, viewMatrixInv );
	pRenderContext->MultMatrixLocal( viewMatrixInv );

	GetColorVar( MATERIALTYPE_GLOW )->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar( MATERIALTYPE_GLOW )->SetFloatValue( 0.04f );

	const float flScale = 8.0f;
	IMesh *pMeshGlow = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_GLOW ) );
	CreateTexturedRect( pMeshGlow, -flScale, -flScale, flScale * 2, flScale * 2 );
	pMeshGlow->Draw();
}

void CHoloShipRadar::DrawBlips( IMatRenderContext *pRenderContext )
{
	matrix3x4_t viewMatrix = CurrentHoloViewMatrix();
	matrix3x4_t viewMatrixInv = CurrentHoloViewMatrixInverted();
	MatrixSetTranslation( vec3_origin, viewMatrix );
	MatrixSetTranslation( vec3_origin, viewMatrixInv );

	matrix3x4_t modelMatrix;
	pRenderContext->GetMatrix( MATERIAL_MODEL, &modelMatrix );
	Vector modelOrigin;
	MatrixGetTranslation( modelMatrix, modelOrigin );

	matrix3x4_t modelInverted;
	MatrixInvert( modelMatrix, modelInverted );
	MatrixSetTranslation( vec3_origin, modelInverted );

	matrix3x4_t matrixTemp;
	SetIdentityMatrix( matrixTemp );

	const CUtlVector< IHoloTarget* > &targets = GetHoloTargets();
	FOR_EACH_VEC( targets, i )
	{
		const IHoloTarget *pTarget = targets[ i ];
		const C_BaseEntity *pEntity = pTarget->GetEntity();

		Vector vecDelta;
		VectorITransform( pEntity->GetAbsOrigin(), m_pSpacecraftData->GetEntity()->EntityToWorldTransform(), vecDelta );

		const float flBlipSize = 0.02f;
		vecDelta *= g_flRadarScale;
		const float flVisibleDistance = g_flRadarSize + g_flRadarSize * g_flRadarFieldSize * pTarget->GetSize();

		if ( vecDelta.LengthSqr() > flVisibleDistance * flVisibleDistance )
		{
			continue;
		}

		const bool bCenterVisible = vecDelta.Length2DSqr() < g_flRadarSize * g_flRadarSize;

		// Exponential falloff
		//if ( vecDelta.Length2DSqr() > 0.00001f )
		//{
		//	float length = powf( vecDelta.Length2D() / g_flRadarSize, 0.5f ) * g_flRadarSize;
		//	vecDelta = vecDelta.Normalized() * length;
		//}

		Vector color;
		float flAlphaScale;
		GetHoloTargetTypeColor( pTarget->GetType(), color, flAlphaScale );

		GetColorVar()->SetVecValue( XYZ( color ) );

		pRenderContext->PushMatrix();

		SetIdentityMatrix( matrixTemp );
		MatrixSetTranslation( Vector( vecDelta.x, vecDelta.y, 0.0f ), matrixTemp );
		pRenderContext->MultMatrixLocal( matrixTemp );

		// Begin drawing blip
		pRenderContext->PushMatrix();

		// Draw center
		pRenderContext->PushMatrix();
		SetIdentityMatrix( matrixTemp );
		MatrixScaleBy( flBlipSize, matrixTemp );
		pRenderContext->MultMatrixLocal( matrixTemp );

		GetAlphaVar()->SetFloatValue( 0.8f * flAlphaScale );
		m_pMeshCircle->Draw();
		m_pMeshCircle->Draw();

		pRenderContext->PopMatrix();

		// Draw range
		SetIdentityMatrix( matrixTemp );
		MatrixScaleBy( g_flRadarFieldSize * pTarget->GetSize(), matrixTemp );
		pRenderContext->MultMatrixLocal( matrixTemp );

		GetAlphaVar()->SetFloatValue( 0.05f * flAlphaScale );
		m_pMeshCircle->Draw();

		pRenderContext->PopMatrix();

		// Draw blip line
		if ( bCenterVisible && abs( vecDelta.z ) > 0.1f )
		{
			pRenderContext->SetStencilEnable( false );

			const float flStripeWidth = 0.02f;
			const float flZPosition = clamp( vecDelta.z, -g_flRadarSize, g_flRadarSize );

			IMesh *pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial() );
			CreateSlantedRect( pMesh, -flStripeWidth, 0.0f, flStripeWidth * 2.0f, flZPosition );

			GetAlphaVar()->SetFloatValue( ( vecDelta.z < 0.0f ? 0.1f : 0.3f ) * flAlphaScale );
			pMesh->Draw();

			SetIdentityMatrix( matrixTemp );
			MatrixSetTranslation( Vector( 0, 0, vecDelta.z ), matrixTemp );
			MatrixScaleBy( 0.02f, matrixTemp );

			matrix3x4_t mat;
			ConcatTransforms( matrixTemp, modelInverted, mat );
			ConcatTransforms( mat, viewMatrixInv, matrixTemp );

			// Offset Z and scale
			pRenderContext->MultMatrixLocal( matrixTemp );

			// reverse model orientation
			//pRenderContext->MultMatrixLocal( modelInverted );

			// reverse view orientation
			//pRenderContext->MultMatrixLocal( viewMatrixInv );

			m_pMeshCircle->Draw();

			pRenderContext->SetStencilEnable( true );
		}

		pRenderContext->PopMatrix();
	}
}

void CHoloShipRadar::Think( float frametime )
{
}
