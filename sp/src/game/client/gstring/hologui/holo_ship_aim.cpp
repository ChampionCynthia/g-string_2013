
#include "cbase.h"
#include "holo_ship_aim.h"
#include "holo_utilities.h"
#include "gstring/hologui/point_holo_target.h"
#include "gstring/hologui/env_holo_system.h"
#include "gstring/cspacecraft.h"
#include "gstring/cgstring_globals.h"

#include "materialsystem/imaterialvar.h"
#include "view.h"
#include "view_scene.h"
#include "collisionutils.h"
#include "sourcevr/isourcevirtualreality.h"

extern void GetHoloTargetTypeColor( IHoloTarget::TargetType type, Vector &color, float &alpha );
extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

namespace
{
	const float g_flPanelRadius = 13.0f;
	const Vector g_vecPanelPosition = Vector( -7, 0, -4 );
}

CHoloShipAim::CHoloShipAim( ISpacecraftData *pSpacecraftData ) :
	m_pSpacecraftData( pSpacecraftData )
{
	CMatRenderContextPtr pRenderContext( materials );
	m_pMeshLargeReticule = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshReticule = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshTarget = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshPanel = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_COLOR | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );

	m_iAttachmentGUI = pSpacecraftData->GetEntity()->LookupAttachment( "gui" );

	CreateArc( m_pMeshLargeReticule, 32, 4.5f, 0.0f, 0.1f, M_PI_F * 0.3f, M_PI_F * 0.7f );
	CreateRecticule( m_pMeshReticule, 0.3f, 0.05f, 0.3f * 0.707f, DEG2RAD( 45.0f ) );
	CreateRecticule( m_pMeshTarget, 1.0f, 0.05f, 0.5f, 0.0f );
	CreateAimPanel( m_pMeshPanel, 10, 20, QAngle( -50, -80, 0 ), QAngle( 5, 80, 0 ), g_flPanelRadius, 0.1f );
}

CHoloShipAim::~CHoloShipAim()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DestroyStaticMesh( m_pMeshLargeReticule );
	pRenderContext->DestroyStaticMesh( m_pMeshReticule );
	pRenderContext->DestroyStaticMesh( m_pMeshTarget );
	pRenderContext->DestroyStaticMesh( m_pMeshPanel );
}

void CHoloShipAim::Draw( IMatRenderContext *pRenderContext )
{
	//matrix3x4_t matrixTemp;

	//GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	//GetAlphaVar()->SetFloatValue( 0.5f );
	//pRenderContext->Bind( GetMaterial() );

	//pRenderContext->PushMatrix();
	//MatrixBuildRotationAboutAxis( Vector( 0, 1, 0 ), 90.0f, matrixTemp );
	//pRenderContext->MultMatrixLocal( matrixTemp );

	//m_pMeshLargeReticule->Draw();

	//MatrixBuildRotationAboutAxis( Vector( 1, 0, 0 ), 180.0f, matrixTemp );
	//pRenderContext->MultMatrixLocal( matrixTemp );

	//m_pMeshLargeReticule->Draw();

	//pRenderContext->PopMatrix();

	DrawTargets( pRenderContext );
}

void FormatForViewSpace( const Vector &forward, const Vector &right, const Vector &up, const Vector &viewOrigin,
	Vector &vOrigin, float flWorldSpaceScale, bool bInverse )
{
	if ( UseVR() )
	{
		return;
	}

	// Presumably, SetUpView has been called so we know our FOV and render origin.
	const CViewSetup *pViewSetup = view->GetPlayerViewSetup();
	
	float worldx = tan( pViewSetup->fov * M_PI/360.0 );
	float viewx = tan( pViewSetup->fovViewmodel * M_PI/360.0 );

	// aspect ratio cancels out, so only need one factor
	// the difference between the screen coordinates of the 2 systems is the ratio
	// of the coefficients of the projection matrices (tan (fov/2) is that coefficient)
	float factorX = worldx / viewx;

	float factorY = factorX;

	// Get the coordinates in the viewer's space.
	// The factor has to be affected by the world space scale too.
	Vector tmp = vOrigin - viewOrigin * flWorldSpaceScale;
	Vector vTransformed( right.Dot( tmp ), up.Dot( tmp ), forward.Dot( tmp ) );

	// Now squash X and Y.
	if ( bInverse )
	{
		if ( factorX != 0 && factorY != 0 )
		{
			vTransformed.x /= factorX;
			vTransformed.y /= factorY;
		}
		else
		{
			vTransformed.x = 0.0f;
			vTransformed.y = 0.0f;
		}
	}
	else
	{
		vTransformed.x *= factorX;
		vTransformed.y *= factorY;
	}

	// Transform back to world space.
	Vector vOut = (right * vTransformed.x) + (up * vTransformed.y) + (forward * vTransformed.z);
	vOrigin = viewOrigin + vOut;
}

extern int ScreenTransform( const Vector& point, Vector& screen );

void CHoloShipAim::DrawTargets( IMatRenderContext *pRenderContext )
{
	pRenderContext->Bind( GetMaterial() );

	// Find direction vectors
	//pRenderContext->GetMatrix( MATERIAL_VIEW, &viewMatrix );
	const matrix3x4_t &viewMatrix = CurrentHoloViewMatrix();

	Vector f, r, u;
	MatrixGetColumn( viewMatrix, 0, f );
	MatrixGetColumn( viewMatrix, 1, r );
	MatrixGetColumn( viewMatrix, 2, u );

	f *= 16.0f;
	r *= 16.0f;
	u *= 16.0f;

	Vector forward( -f.z, -r.z, -u.z );
	Vector up( f.y, r.y, u.y );
	Vector right( f.x, r.x, u.x );

	float flWorldSpaceScale = g_pGstringGlobals ? g_pGstringGlobals->GetWorldScale() : 1.0f;


	pRenderContext->ClearBuffers( false, false, true );
	pRenderContext->SetStencilEnable( true );
	pRenderContext->SetStencilWriteMask( 1 );
	pRenderContext->SetStencilTestMask( 1 );
	pRenderContext->SetStencilReferenceValue( 1 );
	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_NEVER );
	pRenderContext->SetStencilFailOperation( STENCILOPERATION_REPLACE );
	pRenderContext->SetStencilPassOperation( STENCILOPERATION_KEEP );
	pRenderContext->SetStencilZFailOperation( STENCILOPERATION_KEEP );

	pRenderContext->PushMatrix();

	matrix3x4_t temp2;
	SetIdentityMatrix( temp2 );
	MatrixSetTranslation( g_vecPanelPosition, temp2 );
	pRenderContext->MultMatrixLocal( temp2 );
	m_pMeshPanel->Draw();

	pRenderContext->PopMatrix();

	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_EQUAL );
	pRenderContext->SetStencilWriteMask( 0 );



	pRenderContext->PushMatrix();

	matrix3x4_t guiAttachment;
	QAngle eyeAngles;
	Vector eyePosition;
	m_pSpacecraftData->GetEntity()->GetAttachment( m_iAttachmentGUI, eyePosition, eyeAngles );
	AngleMatrix( eyeAngles, vec3_origin, guiAttachment );

	const Vector &holoEyePos = CurrentHoloViewOrigin();
	const CUtlVector< IHoloTarget* > &targets = GetHoloTargets();
	FOR_EACH_VEC( targets, i )
	{
		const IHoloTarget *pTarget = targets[ i ];
		Vector position = pTarget->GetEntity()->GetAbsOrigin() - eyePosition;

		Vector vecGUISpace;
		VectorITransform( position, guiAttachment, vecGUISpace );

		FormatForViewSpace( forward, right, up, holoEyePos, vecGUISpace, flWorldSpaceScale, true );
		Vector sphereRay = vecGUISpace - holoEyePos;

		float pT1, pT2;
		if ( IntersectInfiniteRayWithSphere( holoEyePos, sphereRay, g_vecPanelPosition, g_flPanelRadius, &pT1, &pT2 ) && pT2 > 0.0f )
		{
			vecGUISpace = sphereRay * pT2 + holoEyePos;

			Vector color;
			float flAlphaScale;
			GetHoloTargetTypeColor( pTarget->GetType(), color, flAlphaScale );

			GetColorVar()->SetVecValue( XYZ( color ) );
			GetAlphaVar()->SetFloatValue( 1.0f );

			matrix3x4_t temp, mat;
			SetIdentityMatrix( temp );
			MatrixSetTranslation( vecGUISpace, temp );

			Vector normal = ( vecGUISpace - g_vecPanelPosition ).Normalized();
			QAngle an;
			VectorAngles( normal, an );
			matrix3x4_t rr;
			AngleMatrix( an, rr );
			ConcatTransforms( temp, rr, mat );

			pRenderContext->LoadMatrix( mat );
			m_pMeshTarget->Draw();
		}
	}
	pRenderContext->PopMatrix();

	GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar()->SetFloatValue( 0.5f );
	pRenderContext->Bind( GetMaterial() );
	// Draw projected center reticule
	pRenderContext->PushMatrix();
	if ( UseVR() )
	{
		//const CViewSetup *pMonoSetup = view->GetPlayerViewSetup();

		Vector guiForward, guiRight, guiUp;
		//AngleVectors( pMonoSetup->angles, &guiForward, &guiRight, &guiUp );
		//Vector start = eyePosition + guiForward * flWorldSpaceScale * 5.0f;
		//Vector delta = start - pMonoSetup->origin;
		//delta.NormalizeInPlace();
		//Vector end = start + delta * MAX_TRACE_LENGTH;

		trace_t tr;
		//UTIL_TraceLine( start, end, MASK_SHOT, m_pSpacecraftData->GetEntity(), COLLISION_GROUP_NONE, &tr );

		// Trace to actual shoot position
		AngleVectors( eyeAngles, &guiForward );
		Vector start = CurrentViewOrigin();
		Vector end = start + guiForward * MAX_TRACE_LENGTH;
		UTIL_TraceLine( start, end, MASK_SHOT, m_pSpacecraftData->GetEntity(), COLLISION_GROUP_NONE, &tr );

		//DebugDrawLine( CurrentViewOrigin(), tr.endpos, 255, 0, 0, true, -1 );
		//DebugDrawLine( tr.endpos + Vector(0,0,100), tr.endpos, 0, 255, 0, true, -1 );

		Vector vecGUISpace;
		VectorITransform( tr.endpos - eyePosition, guiAttachment, vecGUISpace );
		FormatForViewSpace( forward, right, up, holoEyePos, vecGUISpace, flWorldSpaceScale, true );

		Vector sphereRay = vecGUISpace - holoEyePos;
		float pT1, pT2;
		if ( IntersectInfiniteRayWithSphere( holoEyePos, sphereRay, g_vecPanelPosition, g_flPanelRadius, &pT1, &pT2 ) && pT2 > 0.0f )
		{
			Vector vecSpherePosition = sphereRay * pT2 + holoEyePos;

			const float flMaxDistance = 10000.0f;
			float length = vecGUISpace.NormalizeInPlace();
			if ( length > flMaxDistance )
			{
				length = flMaxDistance;
			}
			vecGUISpace *= length;

			matrix3x4_t mat;
			SetIdentityMatrix( mat );
			MatrixSetTranslation( vecGUISpace, mat );
			MatrixScaleBy( ( vecGUISpace - holoEyePos ).Length() * 0.05f, mat );


			Vector normal = ( vecSpherePosition - g_vecPanelPosition ).Normalized();
			QAngle an;
			VectorAngles( normal, an );
			matrix3x4_t rr, mat2;
			AngleMatrix( an, rr );
			ConcatTransforms( mat, rr, mat2 );


			pRenderContext->LoadMatrix( mat );
			pRenderContext->LoadMatrix( mat2 );
			m_pMeshReticule->Draw();

			// sides
			matrix3x4_t matrixTemp;
			MatrixBuildRotationAboutAxis( Vector( 0, 1, 0 ), 90.0f, matrixTemp );
			pRenderContext->MultMatrixLocal( matrixTemp );

			m_pMeshLargeReticule->Draw();

			MatrixBuildRotationAboutAxis( Vector( 1, 0, 0 ), 180.0f, matrixTemp );
			pRenderContext->MultMatrixLocal( matrixTemp );

			m_pMeshLargeReticule->Draw();
		}
	}
	else // Draw simple reticule
	{
		// center
		matrix3x4_t matrixTemp;
		SetIdentityMatrix( matrixTemp );
		MatrixSetTranslation( Vector( 5, 0, 0 ), matrixTemp );

		pRenderContext->MultMatrixLocal( matrixTemp );
		m_pMeshReticule->Draw();

		// sides
		MatrixBuildRotationAboutAxis( Vector( 0, 1, 0 ), 90.0f, matrixTemp );
		pRenderContext->MultMatrixLocal( matrixTemp );

		m_pMeshLargeReticule->Draw();

		MatrixBuildRotationAboutAxis( Vector( 1, 0, 0 ), 180.0f, matrixTemp );
		pRenderContext->MultMatrixLocal( matrixTemp );

		m_pMeshLargeReticule->Draw();

	}
	pRenderContext->PopMatrix();

	pRenderContext->SetStencilEnable( false );

	// Draw the panel
	pRenderContext->Bind( GetMaterial( MATERIALTYPE_VERTEXCOLOR ) );
	GetColorVar( MATERIALTYPE_VERTEXCOLOR )->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar( MATERIALTYPE_VERTEXCOLOR )->SetFloatValue( 0.1f );

	pRenderContext->PushMatrix();

	matrix3x4_t temp;
	SetIdentityMatrix( temp );
	MatrixSetTranslation( g_vecPanelPosition, temp );
	pRenderContext->MultMatrixLocal( temp );

	m_pMeshPanel->Draw();

	pRenderContext->PopMatrix();
}

