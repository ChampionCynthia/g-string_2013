
#include "cbase.h"
#include "holo_ship_aim.h"
#include "holo_utilities.h"
#include "gstring/hologui/point_holo_target.h"
#include "gstring/cspacecraft.h"
#include "gstring/cgstring_globals.h"

#include "materialsystem/imaterialvar.h"
#include "view.h"
#include "view_scene.h"
#include "collisionutils.h"

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
	CreateRecticule( m_pMeshReticule, 0.5f, 0.05f, 0.5f * 0.707f, DEG2RAD( 45.0f ) );
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
	matrix3x4_t matrixTemp;

	GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar()->SetFloatValue( 0.5f );
	pRenderContext->Bind( GetMaterial() );

	pRenderContext->PushMatrix();
	MatrixBuildRotationAboutAxis( Vector( 0, 1, 0 ), 90.0f, matrixTemp );
	pRenderContext->MultMatrixLocal( matrixTemp );

	m_pMeshLargeReticule->Draw();

	MatrixBuildRotationAboutAxis( Vector( 1, 0, 0 ), 180.0f, matrixTemp );
	pRenderContext->MultMatrixLocal( matrixTemp );

	m_pMeshLargeReticule->Draw();

	pRenderContext->PopMatrix();

	DrawTargets( pRenderContext );

	SetIdentityMatrix( matrixTemp );
	MatrixSetTranslation( Vector( 5, 0, 0 ), matrixTemp );
	pRenderContext->MultMatrixLocal( matrixTemp );

	GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar()->SetFloatValue( 0.5f );
	pRenderContext->Bind( GetMaterial() );
	m_pMeshReticule->Draw();
}



void FormatForViewSpace( const Vector &forward, const Vector &right, const Vector &up, const Vector &viewOrigin,
	Vector &vOrigin, float flWorldSpaceScale, bool bInverse )
{
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
	//matrix3x4_t matToDevice, matToDeviceInv;
	//MatrixSetTranslation( vec3_origin, matToDevice );
	//MatrixSetColumn( Vector( 0, -1, 0 ), 0, matToDevice );
	//MatrixSetColumn( Vector( 0, 0, 1 ), 1, matToDevice );
	//MatrixSetColumn( Vector( -1, 0, 0 ), 2, matToDevice );
	//MatrixInvert( matToDevice, matToDeviceInv );


	// Find direction vectors



	matrix3x4_t viewMatrix, viewMatrixInv;
	pRenderContext->GetMatrix( MATERIAL_VIEW, &viewMatrix );
	MatrixInvert( viewMatrix, viewMatrixInv );
	//viewMatrixInv = viewMatrix;
	MatrixScaleBy( 16, viewMatrixInv );

	Vector f, r, u;
	MatrixGetColumn( viewMatrix, 0, f );
	MatrixGetColumn( viewMatrix, 1, r );
	MatrixGetColumn( viewMatrix, 2, u );
	f *= 16.0f;
	r *= 16.0f;
	u *= 16.0f;
	//engine->Con_NPrintf( 32, "%f %f %f", XYZ(f) );
	//engine->Con_NPrintf( 33, "%f %f %f", XYZ(r) );
	//engine->Con_NPrintf( 34, "%f %f %f", XYZ(u) );

	Vector forward( -f.z, -r.z, -u.z );
	Vector up( f.y, r.y, u.y );
	Vector right( f.x, r.x, u.x );

	engine->Con_NPrintf( 42, "%f %f %f", XYZ(forward) );
	engine->Con_NPrintf( 43, "%f %f %f", XYZ(right) );
	engine->Con_NPrintf( 44, "%f %f %f", XYZ(up) );


	//viewMatrixInv = viewMatrix;
	//MatrixSetTranslation( vec3_origin, viewMatrixInv );
	//MatrixScaleBy( 16.0f, viewMatrixInv );

	//matrix3x4_t worldView, worldViewI;
	//ConcatTransforms( viewMatrix, matToDeviceInv, worldView );
	//MatrixScaleBy( 16, worldView);
	//MatrixSetTranslation( vec3_origin, worldView );
	//MatrixInvert(worldView, worldViewI);

	Vector eyePos;
	MatrixGetTranslation( viewMatrix, eyePos );
			engine->Con_NPrintf(60, "eye: %f %f %f", XYZ(eyePos));
	eyePos.Init( eyePos.z, eyePos.x, -eyePos.y );
	//eyePosOffset.Init();
	Vector tt  = eyePos;
	//VectorRotate(tt, viewMatrixR, eyePos);

	float flWorldSpaceScale = g_pGstringGlobals ? g_pGstringGlobals->GetWorldScale() : 1.0f;
	eyePos /= flWorldSpaceScale;
	//engine->Con_NPrintf(22, "eyePos: %f %f %f", XYZ(eyePos));
	Vector eyePosOffset = eyePos.x * forward + eyePos.y * right + eyePos.z * up;

	//eyePos *= -1;

	pRenderContext->PushMatrix();

	matrix3x4_t guiAttachment, guiAttachmentRotation;
	QAngle eyeAngles;
	Vector eyePosition;

	Quaternion modelOrientation;
	AngleQuaternion( MainViewAngles(), modelOrientation );

	Quaternion viewOrientation;
	VectorAngles( forward, up, eyeAngles );
	QAngle viewAngles = eyeAngles;
	AngleQuaternion( eyeAngles, viewOrientation );
	QuaternionInvert( viewOrientation, viewOrientation );

	Quaternion qt;
	QuaternionMult(modelOrientation, viewOrientation, qt);
	QAngle angg;
	QuaternionAngles(qt, angg);


	VectorAngles( forward, up, eyeAngles );
	AngleQuaternion( eyeAngles, viewOrientation );

	//Vector vecHelper;
	m_pSpacecraftData->GetEntity()->GetAttachment( m_iAttachmentGUI, eyePosition, eyeAngles );
	//m_pSpacecraftData->GetEntity()->GetAttachment( m_pSpacecraftData->GetEntity()->LookupAttachment( "eyes" ), vecHelper );
	//vecHelper -= eyePosition;
	//vecHelper *= 10.0f;
	//vecHelper = Vector( 27 * (15.0f / 16.0f), 0, 0 );
	//vecHelper = forward * vecHelper.x + right * vecHelper.y + up * vecHelper.z;
	AngleMatrix( eyeAngles, vec3_origin, guiAttachment );




	//MatrixScaleBy( 16.0f, guiAttachment );
	matrix3x4_t localViewMatrix, localViewMatrixInv;
	AngleMatrix( viewAngles * flWorldSpaceScale, localViewMatrix );
	//MatrixScaleBy( flWorldSpaceScale, localViewMatrix );
	MatrixInvert( localViewMatrix, localViewMatrixInv );

	//matrix3x4_t abc = guiAttachment;
	//ConcatTransforms( abc, localViewMatrixInv, guiAttachment );

	matrix3x4_t rot;
	//MatrixBuildRotationAboutAxis( Vector( 0, 1, 0 ), 0.0f, rot );
	MatrixBuildRotationAboutAxis( Vector( 0, 1, 0 ), 0.0f, rot );

	const CUtlVector< IHoloTarget* > &targets = GetHoloTargets();
	FOR_EACH_VEC( targets, i )
	{
		const IHoloTarget *pTarget = targets[ i ];
		Vector position = pTarget->GetEntity()->GetAbsOrigin() - eyePosition; // - eyePosOffset;
		//position /= 16.0f;

		Vector world = pTarget->GetEntity()->GetAbsOrigin();
		Vector screen;
		if ( ScreenTransform( world, screen ) == 0 )
		{
			engine->Con_NPrintf(10 + i, "screen: %f %f", screen.x, screen.y);
		}


		//Vector tt;
		//VectorTransform( position, guiAttachmentRotation, tt );
		//position = tt;

		//FormatViewModelAttachment( position, true );

		//VectorITransform( position, guiAttachmentRotation, tt );
		//position = tt;

		Vector vecGUISpace;
		//VectorTransform( pTarget->GetEntity()->GetAbsOrigin(), worldView, position );
		//VectorITransform( position, worldView, vecGUISpace );
		//position = vecGUISpace;
		VectorITransform( position, guiAttachment, vecGUISpace );
		//vecGUISpace *= 20.0f;
		
		//matrix3x4_t camera, cameraOffset, eyes, eyesInv;
		//QAngle eyeAngles2;
		//Vector eyePosition2;
		//m_pSpacecraftData->GetEntity()->GetAttachment( m_pSpacecraftData->GetEntity()->LookupAttachment("eyes"), eyePosition2, eyeAngles2 );
		//AngleMatrix( eyeAngles2, eyePosition2, camera );

		//AngleMatrix( CurrentViewAngles(), CurrentViewOrigin(), eyes );
		//MatrixInvert( camera, eyesInv );
		//ConcatTransforms( eyesInv, eyes, cameraOffset );
		////MatrixScaleBy(16.0f, cameraOffset);
		//tt = vecGUISpace;
		//VectorIRotate(tt, cameraOffset, vecGUISpace);

		//VectorRotate(eyePos, eyeAngles, tt);
		//vecGUISpace += tt;
		//vecGUISpace -= eyePosOffset;

		//vecGUISpace -= eyePos * 15.0f/16.0f;
		//vecGUISpace += eyePos;

		//engine->Con_NPrintf(60 + i, "pre format: %f %f %f", XYZ(vecGUISpace));
		//vecGUISpace -= eyePosOffset;
		Vector vecGUISpaceOriginal(vecGUISpace);
		Vector sphereRayOriginal = vecGUISpaceOriginal - eyePos;

		extern Vector _eye;
		FormatForViewSpace( forward, right, up, _eye, vecGUISpace, flWorldSpaceScale, true );
		//FormatForViewSpace( forward, right, up, eyePos * flWorldSpaceScale, vecGUISpace, flWorldSpaceScale, true );
		Vector sphereRay = vecGUISpace - _eye;

		//sphereRay = forward;
		//sphereRay = Vector(1,0,0);
		float pT1, pT2;
		//eyePos.Init(-27,0 ,0);
		if ( IntersectInfiniteRayWithSphere( _eye, sphereRay, g_vecPanelPosition, g_flPanelRadius, &pT1, &pT2 ) && pT2 > 0.0f )
		{
			//FormatForViewSpace( forward, right, up, eyePos, sphereRay, flWorldSpaceScale, false );

			vecGUISpace = sphereRayOriginal * pT2 + eyePos;

			vecGUISpace = sphereRay * pT2 + _eye;

			//FormatForViewSpace( forward, right, up, eyePos, vecGUISpace, 1, false );

			//vecGUISpace += eyePosOffset;
			//vecGUISpace -= eyePosOffset / 16.0f;
			//vecGUISpace += eyePos;
			//vecGUISpace = eyePos + sphereRay * pT2;

			Vector color;
			float flAlphaScale;
			GetHoloTargetTypeColor( pTarget->GetType(), color, flAlphaScale );

			GetColorVar()->SetVecValue( XYZ( color ) );
			GetAlphaVar()->SetFloatValue( 1.0f );

			matrix3x4_t mat = viewMatrixInv;
			SetIdentityMatrix( mat );
			MatrixSetTranslation( vecGUISpace, mat );
			matrix3x4_t mat2;
			ConcatTransforms( mat, rot, mat2 );

			Vector normal = ( vecGUISpace - g_vecPanelPosition ).Normalized();
			//normal.Init( normal.x, normal.z, -normal.y );
			QAngle an;
			VectorAngles( normal, an );
			matrix3x4_t rr;
			AngleMatrix( an, rr );
			ConcatTransforms( mat2, rr, mat );


			//const float flScale = ( eyePos - vecGUISpace ).Length() * 0.3f;
			//MatrixScaleBy( flScale / 16.0f, mat2 );
			//MatrixScaleBy( 2.0f, mat2 );

			pRenderContext->LoadMatrix( mat2 );
			pRenderContext->LoadMatrix( mat );
			m_pMeshTarget->Draw();
		}
	}

	pRenderContext->PopMatrix();

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

