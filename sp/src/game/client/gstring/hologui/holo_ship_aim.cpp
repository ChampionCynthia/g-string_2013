
#include "cbase.h"
#include "holo_ship_aim.h"
#include "holo_ship_radar.h"
#include "holo_utilities.h"
#include "gstring/hologui/point_holo_target.h"
#include "gstring/hologui/env_holo_system.h"
#include "gstring/cspacecraft.h"
#include "gstring/cgstring_globals.h"
#include "gstring/gstring_in_main.h"

#include "materialsystem/imaterialvar.h"
#include "view.h"
#include "view_scene.h"
#include "collisionutils.h"
#include "sourcevr/isourcevirtualreality.h"
#include "c_user_message_register.h"

extern void GetHoloTargetTypeColor( IHoloTarget::TargetType type, Vector &color, float &alpha );

namespace
{
	const float g_flPanelRadius = 13.0f;
	const Vector g_vecPanelPosition = Vector( -7, 0, -4 );
}

DECLARE_HOLO_MESSAGE( SpacecraftDamage, CHoloShipAim );

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
	m_pMeshTargetThick = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshTargetArrows = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshPanel = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_COLOR | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshDamagePanel = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_COLOR | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshDamagePanelInner = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_COLOR | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshDamagePanelOuter = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_COLOR | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );
	m_pMeshDamagePanelDecor = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_COLOR | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_MODEL, GetMaterial() );

	m_iAttachmentGUI = pSpacecraftData->GetEntity()->LookupAttachment( "gui" );

	CreateArc( m_pMeshLargeReticule, 32, 4.5f, 0.0f, 0.1f, M_PI_F * 0.3f, M_PI_F * 0.7f );
	CreateRecticule( m_pMeshReticule, 0.3f, 0.05f, 0.3f * 0.707f, DEG2RAD( 45.0f ) );
	CreateRecticule( m_pMeshTarget, 1.0f, 0.05f, 0.5f, 0.0f );
	CreateRecticule( m_pMeshTargetThick, 1.04f, 0.13f, 0.5f, 0.0f );
	CreateTargetArrows( m_pMeshTargetArrows, 45.0f, 8.0f, 0.25f, 0.8f, 0.12f );
	CreateAimPanel( m_pMeshPanel, 10, 20, QAngle( -50, -80, 0 ), QAngle( 5, 80, 0 ), g_flPanelRadius, 0.1f );
	CreateDamageIndicator( m_pMeshDamagePanel, 16, 4.5f, 1.2f, M_PI_F * -0.3f, M_PI_F * 0.3f, 0.0f, 1.0f );
	CreateRoundDamageIndicator( m_pMeshDamagePanelInner, 32, 0.001f, 1.2f );
	CreateRoundDamageIndicator( m_pMeshDamagePanelOuter, 32, 4.5f, -1.2f );
	CreateAimPanelDecor( m_pMeshDamagePanelDecor, 10, 20, QAngle( -50, -80, 0 ), QAngle( 5, 80, 0 ), g_flPanelRadius, 0.1f );

	ADD_HOLO_MESSAGE( SpacecraftDamage );
}

CHoloShipAim::~CHoloShipAim()
{
	REMOVE_HOLO_MESSAGE( SpacecraftDamage );

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->DestroyStaticMesh( m_pMeshLargeReticule );
	pRenderContext->DestroyStaticMesh( m_pMeshReticule );
	pRenderContext->DestroyStaticMesh( m_pMeshTarget );
	pRenderContext->DestroyStaticMesh( m_pMeshTargetThick );
	pRenderContext->DestroyStaticMesh( m_pMeshTargetArrows );
	pRenderContext->DestroyStaticMesh( m_pMeshPanel );
	pRenderContext->DestroyStaticMesh( m_pMeshDamagePanel );
	pRenderContext->DestroyStaticMesh( m_pMeshDamagePanelInner );
	pRenderContext->DestroyStaticMesh( m_pMeshDamagePanelOuter );
	pRenderContext->DestroyStaticMesh( m_pMeshDamagePanelDecor );
}

void CHoloShipAim::MsgFunc_SpacecraftDamage( bf_read &msg )
{
	Vector vecFrom;
	msg.ReadBitVec3Coord( vecFrom );

	Vector vecDelta = vecFrom - m_pSpacecraftData->GetEntity()->GetAbsOrigin();
	Vector vecFwd, vecRight, vecUp;
	AngleVectors( m_pSpacecraftData->GetEntity()->GetAbsAngles(), &vecFwd, &vecRight, &vecUp );

	vecDelta.NormalizeInPlace();
	const float flDotForward = DotProduct( vecDelta, vecFwd );
	vecDelta -= vecFwd * flDotForward;

	if ( vecDelta.LengthSqr() > 0.0001f )
	{
		vecDelta.NormalizeInPlace();

		const float flForwardThreshold = 0.92f;
		DamagePanel dmg;
		dmg.m_flAngle = RAD2DEG( atan2( DotProduct( vecDelta, vecUp ), -DotProduct( vecDelta, vecRight ) ) );
		dmg.m_flAlpha = 1.0f;
		dmg.m_iType = flDotForward > flForwardThreshold ? 2 : flDotForward < -flForwardThreshold ? 1 : 0;
		m_DamagePanels.AddToTail( dmg );
	}
}

void CHoloShipAim::Think( float frametime )
{
	const CUtlVector< IHoloTarget* > &targets = GetHoloTargets();
	if ( m_KnownTargetEntities.Count() != targets.Count() ||
		Q_memcmp( m_KnownTargetEntities.Base(), targets.Base(), sizeof( IHoloTarget* ) * m_KnownTargetEntities.Count() ) != 0 )
	{
		m_KnownTargetEntities.RemoveAll();
		m_KnownTargetEntities.AddVectorToTail( targets );

		CUtlVector< IHoloTarget* > newTargets;
		newTargets.AddVectorToTail( targets );

		FOR_EACH_VEC( m_Targets, oldTargetIndex )
		{
			bool bFound = false;
			FOR_EACH_VEC( newTargets, newTargetIndex )
			{
				// Target stays
				if ( newTargets[ newTargetIndex ] == m_Targets[ oldTargetIndex ].m_Entity )
				{
					newTargets.Remove( newTargetIndex );
					--newTargetIndex;
					bFound = true;
					break;
				}
			}

			// Target is removed
			if ( !bFound )
			{
				m_Targets.Remove( oldTargetIndex );
				--oldTargetIndex;
			}
		}

		// Add new targets
		FOR_EACH_VEC( newTargets, newTargetIndex )
		{
			Target target;
			target.m_Entity = newTargets[ newTargetIndex ];
			target.m_flBlinkTimer = 0.5f;
			m_Targets.AddToTail( target );
		}
	}

	const C_BaseEntity *pAutoAimTarget = GetGstringInput()->GetAutoAimTargetEntity();
	FOR_EACH_VEC( m_Targets, oldTargetIndex )
	{
		Target &target = m_Targets[ oldTargetIndex ];
		if ( target.m_flBlinkTimer > 0.0f )
		{
			target.m_flBlinkTimer -= frametime;
			target.m_flBlinkTimer = MAX( 0.0f, target.m_flBlinkTimer );
		}

		const float flDesiredFocus = pAutoAimTarget == target.m_Entity->GetEntity() ? 1.0f : 0.0f;
		if ( flDesiredFocus != target.m_flFocusTimer )
		{
			target.m_flFocusTimer = Approach( flDesiredFocus, target.m_flFocusTimer, frametime * 10.0f );
		}
	}

	FOR_EACH_VEC( m_DamagePanels, i )
	{
		DamagePanel &dmg = m_DamagePanels[ i ];
		dmg.m_flAlpha = Approach( 0.0f, dmg.m_flAlpha, frametime );
		if ( dmg.m_flAlpha <= 0.0f )
		{
			m_DamagePanels.Remove( i );
			--i;
		}
	}
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

static void FormatForViewSpace( const Vector &forward, const Vector &right, const Vector &up, const Vector &viewOrigin,
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

void CHoloShipAim::DrawTargets( IMatRenderContext *pRenderContext )
{
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

	pRenderContext->PushMatrix();

	matrix3x4_t temp2;
	SetIdentityMatrix( temp2 );
	MatrixSetTranslation( g_vecPanelPosition, temp2 );
	pRenderContext->MultMatrixLocal( temp2 );

	// Draw deco
	pRenderContext->Bind( GetMaterial( MATERIALTYPE_VERTEXCOLOR ) );
	GetColorVar( MATERIALTYPE_VERTEXCOLOR )->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar( MATERIALTYPE_VERTEXCOLOR )->SetFloatValue( 0.3f );
	m_pMeshDamagePanelDecor->Draw();

	pRenderContext->Bind( GetMaterial() );

	pRenderContext->ClearBuffers( false, false, true );
	pRenderContext->SetStencilEnable( true );
	pRenderContext->SetStencilWriteMask( 1 );
	pRenderContext->SetStencilTestMask( 1 );
	pRenderContext->SetStencilReferenceValue( 1 );
	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_NEVER );
	pRenderContext->SetStencilFailOperation( STENCILOPERATION_REPLACE );
	pRenderContext->SetStencilPassOperation( STENCILOPERATION_KEEP );
	pRenderContext->SetStencilZFailOperation( STENCILOPERATION_KEEP );

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

	const C_BaseEntity *pAutoAimTarget = GetGstringInput()->GetAutoAimTargetEntity();

	const Vector &holoEyePos = CurrentHoloViewOrigin();
	FOR_EACH_VEC( m_Targets, i )
	{
		const Target &target = m_Targets[ i ];
		const IHoloTarget *pTarget = target.m_Entity;
		if ( !pTarget || !pTarget->IsActive() )
		{
			continue;
		}

		//if (i != 1) {
		//	continue;
		//}

		Vector position = pTarget->GetEntity()->GetAbsOrigin() - eyePosition;

		if ( position.LengthSqr() > HOLO_TARGET_MAX_DISTANCE * HOLO_TARGET_MAX_DISTANCE )
		{
			continue;
		}

		Vector vecGUISpace;
		VectorITransform( position, guiAttachment, vecGUISpace );

		FormatForViewSpace( forward, right, up, holoEyePos, vecGUISpace, flWorldSpaceScale, true );
		Vector sphereRay = vecGUISpace - holoEyePos;

		float pT1, pT2;
		if ( IntersectInfiniteRayWithSphere( holoEyePos, sphereRay, g_vecPanelPosition, g_flPanelRadius, &pT1, &pT2 ) && pT2 > 0.0f )
		{
			vecGUISpace = sphereRay * pT2 + holoEyePos;

			//engine->Con_NPrintf(10, "%f %f %f", XYZ(vecGUISpace));
			//Vector sphereDelta = vecGUISpace - g_vecPanelPosition;
			//float dotFwd = DotProduct(sphereDelta.Normalized(), Vector(0, 1, 0));
			//float dotUp = DotProduct(sphereDelta.Normalized(), Vector(0, 0, 1));
			//engine->Con_NPrintf(11, "fwd %f up %f", dotFwd , dotUp);

			Vector color;
			float flAlphaScale;
			GetHoloTargetTypeColor( pTarget->GetType(), color, flAlphaScale );

			float flAlpha = 1.0f;
			if ( target.m_flBlinkTimer > 0.0f )
			{
				flAlpha = abs( cos( target.m_flBlinkTimer * M_PI_F * 3.0f ) );
				flAlpha *= flAlpha;
			}

			GetColorVar()->SetVecValue( XYZ( color ) );
			GetAlphaVar()->SetFloatValue( flAlpha );

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

			if ( pAutoAimTarget != pTarget->GetEntity() )
			{
				m_pMeshTarget->Draw();
			}
			else
			{
				m_pMeshTargetThick->Draw();
			}

			if ( target.m_flFocusTimer > 0.0f && pTarget->GetType() == IHoloTarget::ENEMY )
			{
				SetIdentityMatrix( rr );
				MatrixScaleBy( 1.0f + 0.2f * target.m_flFocusTimer, rr );
				pRenderContext->MultMatrixLocal( rr );

				GetAlphaVar()->SetFloatValue( flAlpha * target.m_flFocusTimer );
				m_pMeshTargetArrows->Draw();
			}
		}
	}
	pRenderContext->PopMatrix();

	// Draw projected center reticule
	pRenderContext->PushMatrix();
	if ( UseVR() )
	{
		//const CViewSetup *pMonoSetup = view->GetPlayerViewSetup();

		Vector guiForward/*, guiRight, guiUp*/;
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

			const float flDistanceToSphere = ( vecSpherePosition - holoEyePos ).Length();
			const float flDistanceScale = flDistanceToSphere > 0.0001f ? 1.0f / flDistanceToSphere * 23.0f : 1.0f;

			matrix3x4_t mat;
			SetIdentityMatrix( mat );
			MatrixSetTranslation( vecGUISpace, mat );
			MatrixScaleBy( ( vecGUISpace - holoEyePos ).Length() * 0.05f * flDistanceScale, mat );

			Vector normal = ( vecSpherePosition - g_vecPanelPosition ).Normalized();
			QAngle an;
			VectorAngles( normal, an );
			matrix3x4_t rr, mat2;
			AngleMatrix( an, rr );
			ConcatTransforms( mat, rr, mat2 );

			pRenderContext->LoadMatrix( mat2 );
			DrawReticule( pRenderContext );
		}
	}
	else // Draw simple reticule
	{
		// center
		matrix3x4_t matrixTemp;
		SetIdentityMatrix( matrixTemp );
		MatrixSetTranslation( Vector( 5, 0, 0 ), matrixTemp );

		pRenderContext->MultMatrixLocal( matrixTemp );
		DrawReticule( pRenderContext );
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

void CHoloShipAim::DrawReticule( IMatRenderContext *pRenderContext )
{
	matrix3x4_t matrixTemp;

	//GetColorVar( MATERIALTYPE_VERTEXCOLOR )->SetVecValue( 0.9f, 0.2f, 0.05f );
	//GetAlphaVar( MATERIALTYPE_VERTEXCOLOR )->SetFloatValue( 0.2f );

	//IMesh *pMeshGlow = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_VERTEXCOLOR ) );
	//CreateRoundDamageIndicator( pMeshGlow, 32, 4.5f, -1.2f, false );
	//pMeshGlow->Draw();
	// damage indicator
	if ( m_DamagePanels.Count() > 0 )
	{
		GetColorVar( MATERIALTYPE_VERTEXCOLOR )->SetVecValue( 0.9f, 0.2f, 0.05f );
		pRenderContext->Bind( GetMaterial( MATERIALTYPE_VERTEXCOLOR ) );
		FOR_EACH_VEC( m_DamagePanels, i )
		{
			const DamagePanel &panel = m_DamagePanels[ i ];
			float flAlpha = panel.m_flAlpha;
			if ( panel.m_iType != 0 )
			{
				flAlpha *= 0.3f;
			}
			GetAlphaVar( MATERIALTYPE_VERTEXCOLOR )->SetFloatValue( flAlpha );
			pRenderContext->PushMatrix();
			MatrixBuildRotationAboutAxis( Vector( 1, 0, 0 ), panel.m_flAngle, matrixTemp );
			pRenderContext->MultMatrixLocal( matrixTemp );

			switch ( panel.m_iType )
			{
			case 0:
				m_pMeshDamagePanel->Draw();
				break;

			case 1:
				m_pMeshDamagePanelOuter->Draw();
				break;

			case 2:
				m_pMeshDamagePanelInner->Draw();
			}

			pRenderContext->PopMatrix();
		}
	}

	GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar()->SetFloatValue( 0.5f );
	pRenderContext->Bind( GetMaterial() );

	// center
	m_pMeshReticule->Draw();

	// sides
	MatrixBuildRotationAboutAxis( Vector( 0, 1, 0 ), 90.0f, matrixTemp );
	pRenderContext->MultMatrixLocal( matrixTemp );

	m_pMeshLargeReticule->Draw();

	MatrixBuildRotationAboutAxis( Vector( 1, 0, 0 ), 180.0f, matrixTemp );
	pRenderContext->MultMatrixLocal( matrixTemp );

	m_pMeshLargeReticule->Draw();
}

