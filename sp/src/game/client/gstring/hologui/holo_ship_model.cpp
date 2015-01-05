
#include "cbase.h"
#include "holo_ship_model.h"
#include "holo_utilities.h"
#include "cspacecraft.h"
#include "gstring/cgstring_globals.h"
#include "gstring/hologui/env_holo_system.h"

#include "model_types.h"
#include "materialsystem/imaterialvar.h"

#define DAMAGE_EFFECT_DURATION 0.5f

CHoloShipModel::CHoloShipModel( ISpacecraftData *pSpacecraftData ) :
	m_pSpacecraftData( pSpacecraftData ),
	m_iHull( -1 ),
	m_flDamageTimer( 0.0f ),
	m_iShield( -1 ),
	m_flShieldTimer( 0.0f )
{
	m_MaterialHoloModel.Init( materials->FindMaterial( "engine/hologui_model", TEXTURE_GROUP_MODEL ) );
}

CHoloShipModel::~CHoloShipModel()
{
	if ( m_hModel )
	{
		m_hModel->Release();
		m_hModel = NULL;
	}
	m_MaterialHoloModel.Shutdown();
}

void CHoloShipModel::Draw( IMatRenderContext *pRenderContext )
{
	if ( m_hModel )
	{
		//m_hModel->InvalidateBoneCache();
		//m_hModel->SetupBones( NULL, -1, -1, gpGlobals->curtime );
		float warningAmount = ( m_pSpacecraftData->GetHull() / (float)m_pSpacecraftData->GetMaxHull() ) > 0.25f ? 0.0f :
			abs( sin( gpGlobals->curtime * 5.0f ) );
		Vector color( Vector( HOLO_COLOR_DEFAULT ) );

		if ( m_flShieldTimer > 0.0f )
		{
			float shieldEffect = m_flShieldTimer / DAMAGE_EFFECT_DURATION;
			m_flShieldTimer -= gpGlobals->frametime;
			color = Lerp( shieldEffect, color, Vector( HOLO_COLOR_HIGHLIGHT ) );
		}

		if ( m_flDamageTimer > 0.0f )
		{
			float damageEffect = m_flDamageTimer / DAMAGE_EFFECT_DURATION;
			warningAmount = MAX( warningAmount, damageEffect );
			m_flDamageTimer -= gpGlobals->frametime;
		}

		color = Lerp( warningAmount, color, Vector( HOLO_COLOR_WARNING ) );
		render->SetColorModulation( color.Base() );
		//render->SetBlend( 0.4f );

		static unsigned int iTokenEyePos = 0;
		IMaterialVar *pVarEyePos = m_MaterialHoloModel->FindVarFast( "$EYEPOS", &iTokenEyePos );
		pVarEyePos->SetVecValue( XYZ( CurrentHoloViewOrigin() ) );

		modelrender->ForcedMaterialOverride( m_MaterialHoloModel );
		m_hModel->DrawModel( STUDIO_RENDER | STUDIO_TRANSPARENCY );
		modelrender->ForcedMaterialOverride( NULL );

		// Draw glow
		matrix3x4_t viewMatrixInv = CurrentHoloViewMatrixInverted();
		MatrixSetTranslation( Vector( 0, 8, -6.5f ), viewMatrixInv );
		pRenderContext->MultMatrixLocal( viewMatrixInv );

		GetColorVar( MATERIALTYPE_GLOW )->SetVecValue( color.Base(), 3 );
		GetAlphaVar( MATERIALTYPE_GLOW )->SetFloatValue( 0.04f );

		const float flScale = 8.0f;
		IMesh *pMeshGlow = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_GLOW ) );
		CreateTexturedRect( pMeshGlow, -flScale, -flScale, flScale * 2, flScale * 2 );
		pMeshGlow->Draw();
	}
}

void CHoloShipModel::Think( float frametime )
{
	const model_t *pMdl = m_hModel ? m_hModel->GetModel() : NULL;
	const model_t *pDesiredMdl = m_pSpacecraftData->GetEntity()->GetModel();

	if ( pMdl != pDesiredMdl )
	{
		if ( m_hModel )
		{
			m_hModel->Release();
		}

		C_BaseAnimating *pAnimating = new C_BaseAnimating();
		pAnimating->InitializeAsClientEntity( NULL, RENDER_GROUP_OPAQUE_ENTITY );
		pAnimating->SetModelPointer( pDesiredMdl );
		pAnimating->SetModelScale( 0.012f );

		m_hModel = pAnimating;
	}

	if ( m_hModel )
	{
		m_hModel->SetAbsOrigin( Vector( 0, 8, -6.5f ) );
		m_hModel->SetAbsAngles( m_pSpacecraftData->GetAngularImpulse() * 0.1f );
	}

	int iHull = m_pSpacecraftData->GetHull();
	if ( m_iHull != iHull )
	{
		if ( m_iHull > iHull )
		{
			m_flDamageTimer = DAMAGE_EFFECT_DURATION;
		}
		m_iHull = iHull;
	}

	int iShield = m_pSpacecraftData->GetShield();
	if ( m_iShield != iShield )
	{
		if ( m_iShield > iShield )
		{
			m_flShieldTimer = DAMAGE_EFFECT_DURATION;
		}
		m_iShield = iShield;
	}
}

