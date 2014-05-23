
#include "cbase.h"
#include "hud_space_autoaim.h"
#include "gstring/vgui/vutil.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "view_scene.h"

#include "../c_gstring_player.h"
#include "../gstring_postprocess.h"
#include "gstring/gstring_cvars.h"
#include "../gstring_in_main.h"


using namespace vgui;

DECLARE_HUDELEMENT( CHudSpaceAutoAim );

CHudSpaceAutoAim::CHudSpaceAutoAim( const char *pElementName )
	: CHudElement( pElementName )
	, BaseClass( NULL, pElementName )
	, m_bWasPainting( false )
	, m_iTargetParity( 0 )
{
	Plat_FastMemset( m_flTargetFraction, 0, sizeof( m_flTargetFraction ) );
	Plat_FastMemset( m_iTargetUpdateBits, 0, sizeof( m_iTargetUpdateBits ) );

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

CHudSpaceAutoAim::~CHudSpaceAutoAim()
{
}

void CHudSpaceAutoAim::Init()
{
}

void CHudSpaceAutoAim::PostDLLInit()
{
}

void CHudSpaceAutoAim::LevelInit()
{
	Reset();
}

void CHudSpaceAutoAim::Reset()
{
	m_bWasPainting = false;
}

bool CHudSpaceAutoAim::ShouldDraw()
{
	const C_GstringPlayer *pPlayer = LocalGstringPlayer();
	return CHudElement::ShouldDraw() && pPlayer && pPlayer->IsInSpacecraft();
}

void CHudSpaceAutoAim::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	int w, t;
	GetHudSize( w, t );
	SetSize( w, t );
}

void CHudSpaceAutoAim::OnThink()
{
	const CGstringInput *pInput = GetGstringInput();
	const CUtlVector< EHANDLE > &PotentialAutoAimTargets = pInput->GetPotentialAutoAimTargets();
	CBaseEntity *pAutoAimTarget = pInput->GetAutoAimTarget();
	FOR_EACH_VEC( PotentialAutoAimTargets, i )
	{
		CBaseEntity *pPotentialTarget = PotentialAutoAimTargets[ i ].Get();
		if ( pPotentialTarget )
		{
			const int iTargetEntIndex = pPotentialTarget->entindex();
			if ( m_iTargetUpdateBits[ iTargetEntIndex ] != m_iTargetParity )
			{
				m_iTargetUpdateBits[ iTargetEntIndex ] = m_iTargetParity;
				m_flTargetFraction[ iTargetEntIndex ] = 0.0f;
			}

			const bool bIsAutoAimTarget = pAutoAimTarget == pPotentialTarget;
			m_flTargetFraction[ iTargetEntIndex ] = Approach( bIsAutoAimTarget ? 1.0f : 0.0f,
				m_flTargetFraction[ iTargetEntIndex ], gpGlobals->frametime * 6.0f );
			m_iTargetUpdateBits[ iTargetEntIndex ]++;
		}
	}
	m_iTargetParity++;
}

void CHudSpaceAutoAim::Paint()
{
	const int w = GetWide();
	const int h = GetTall();

	const CGstringInput *pInput = GetGstringInput();
	const CUtlVector< EHANDLE > &PotentialAutoAimTargets = pInput->GetPotentialAutoAimTargets();
	FOR_EACH_VEC( PotentialAutoAimTargets, i )
	{
		CBaseEntity *pPotentialTarget = PotentialAutoAimTargets[ i ].Get();

		if ( pPotentialTarget )
		{
			const int iTargetEntIndex = pPotentialTarget->entindex();
			const Vector &vecCenter = pPotentialTarget->WorldSpaceCenter();
			Vector vecScreen( vec3_origin );
			if ( !ScreenTransform( vecCenter, vecScreen ) )
			{
				vecScreen = vecScreen * Vector( 0.5f, -0.5f, 0 ) + Vector( 0.5f, 0.5f, 0 );
				vecScreen.x *= w;
				vecScreen.y *= h;

				const float flFraction = m_flTargetFraction[ iTargetEntIndex ];
				const float flSize = Lerp( flFraction, 3.0f, 20.0f );
				const float flSegments = Lerp( flFraction, 12.0f, 32.0f );
				const float flAlpha = Lerp( flFraction, 32.0f, 255.0f );
				const int iSize = scheme()->GetProportionalScaledValue( flSize );

				vecScreen.x -= 1.0f;

				surface()->DrawSetColor( Color( 255, 0, 0, 32 * flFraction ) );
				surface()->DrawOutlinedCircle( vecScreen.x, vecScreen.y, iSize, flSegments );

				vecScreen.x += 2.0f;

				surface()->DrawSetColor( Color( 0, 0, 255, 32 * flFraction ) );
				surface()->DrawOutlinedCircle( vecScreen.x, vecScreen.y, iSize, flSegments );

				vecScreen.x -= 1.0f;

				surface()->DrawSetColor( 255, 255, 255, flAlpha );
				surface()->DrawOutlinedCircle( vecScreen.x, vecScreen.y, iSize, flSegments );

				if ( flFraction > 0.0f )
				{
					const float flSize = Lerp( flFraction, 42.0f, 24.0f );
					const float flSegments = Lerp( flFraction, 32.0f, 48.0f );
					const float flAlpha = Lerp( flFraction, 0.0f, 64.0f );
					const int iSize = scheme()->GetProportionalScaledValue( flSize );

					vecScreen.x -= 1.0f;

					surface()->DrawSetColor( Color( 255, 0, 0, 32 * flFraction ) );
					surface()->DrawOutlinedCircle( vecScreen.x, vecScreen.y, iSize, flSegments );

					vecScreen.x += 2.0f;

					surface()->DrawSetColor( Color( 0, 0, 255, 32 * flFraction ) );
					surface()->DrawOutlinedCircle( vecScreen.x, vecScreen.y, iSize, flSegments );

					vecScreen.x -= 1.0f;

					surface()->DrawSetColor( 255, 255, 255, flAlpha );
					surface()->DrawOutlinedCircle( vecScreen.x, vecScreen.y, iSize, flSegments );
				}
			}
		}
	}
}
