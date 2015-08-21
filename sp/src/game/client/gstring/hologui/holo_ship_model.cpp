
#include "cbase.h"
#include "holo_ship_model.h"
#include "holo_utilities.h"
#include "cspacecraft.h"
#include "gstring/cgstring_globals.h"
#include "gstring/hologui/env_holo_system.h"

#include "model_types.h"
#include "materialsystem/imaterialvar.h"

#define DAMAGE_EFFECT_DURATION 0.5f

CHoloShipModel::CHoloShipModel( vgui::Panel *pParent, ISpacecraftData *pSpacecraftData ) :
	BaseClass( pParent, "ship" ),
	m_pSpacecraftData( pSpacecraftData ),
	m_iHull( -1 ),
	m_flDamageTimer( 0.0f ),
	m_iShield( -1 ),
	m_flShieldTimer( 0.0f ),
	m_nSequenceCopy( 0 )
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
		color = Lerp( GetHoloAlpha(), vec3_origin, color );
		render->SetColorModulation( color.Base() );
		//render->SetBlend( 0.4f );

		static unsigned int iTokenEyePos = 0;
		IMaterialVar *pVarEyePos = m_MaterialHoloModel->FindVarFast( "$EYEPOS", &iTokenEyePos );
		pVarEyePos->SetVecValue( XYZ( CurrentHoloViewOrigin() ) );

		modelrender->ForcedMaterialOverride( m_MaterialHoloModel );

		const float flTargetScale = 0.012f * GetHoloScale();
		if ( m_hModel->GetModelScale() != flTargetScale )
		{
			m_hModel->SetModelScale( flTargetScale );
		}
		m_hModel->DrawModel( STUDIO_RENDER | STUDIO_TRANSPARENCY );
		modelrender->ForcedMaterialOverride( NULL );

		// Draw glow
		matrix3x4_t viewMatrixInv = CurrentHoloViewMatrixInverted();
		MatrixSetTranslation( Vector( 0, 8, -6.5f ), viewMatrixInv );
		pRenderContext->MultMatrixLocal( viewMatrixInv );

		GetColorVar( MATERIALTYPE_GLOW )->SetVecValue( color.Base(), 3 );
		SetHoloAlpha( 0.04f, MATERIALTYPE_GLOW );

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
		pAnimating->InitializeAsClientEntity( NULL, RENDER_GROUP_COUNT );
		pAnimating->RemoveFromLeafSystem();
		//cl_entitylist->RemoveEntity( pAnimating->GetRefEHandle() );
		pAnimating->CollisionProp()->DestroyPartitionHandle();
		pAnimating->SetModelPointer( pDesiredMdl );
		pAnimating->SetModelScale( 0.012f );

		m_hModel = pAnimating;
	}

	if ( m_hModel )
	{
		if ( m_hModel->GetRenderHandle() != INVALID_CLIENT_RENDER_HANDLE )
		{
			m_hModel->RemoveFromLeafSystem();
		}
		const Vector vecOrigin = Vector( 0, 8, -6.5f );

		if ( m_nSequenceCopy <= 0 )
		{
			m_hModel->SetAbsOrigin( vecOrigin );
			m_hModel->SetAbsAngles( m_pSpacecraftData->GetAngularImpulse() * 0.1f );
		}
		else
		{
			m_hModel->SetAbsOrigin( vecOrigin * GetHoloScale() + GetHoloOffset() );
			m_hModel->SetAbsAngles( GetHoloAngle() );

			switch ( m_nSequenceCopy )
			{
			case 1:
				{
					C_BaseAnimating *pAnimating = assert_cast<C_BaseAnimating*>(m_pSpacecraftData->GetEntity());
					m_hModel->SetSequence( pAnimating->GetSequence() );
					m_hModel->SetCycle( pAnimating->GetCycle() );
				}
				break;

			case 2:
				if ( m_hModel->GetSequence() != 0 )
				{
					m_hModel->SetSequence( 0 );
				}
				break;
			}
		}
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

