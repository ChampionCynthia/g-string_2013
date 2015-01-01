#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "hud_spacecrafthealth.h"

#include "iclientmode.h"

#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include "vutil.h"

#include "c_gstring_player.h"
#include "cspacecraft.h"

#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudSpacecraftHealth );
//DECLARE_HUD_MESSAGE( CHudSpacecraftHealth, Fireball );

CHudSpacecraftHealth::CHudSpacecraftHealth( const char *pElementName ) :
	CHudElement( pElementName ),
	BaseClass( NULL, "HudSpacecraftHealth" ),
	m_iTexBar( 0 ),
	m_vecBarAngleDirection( 0.0f, -1.0f ),
	m_flBarAnimationFraction( 0.0f ),
	m_flBarBlinkTimer( 0.0f )
{
	*m_wszLabelText = 0;

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_SPACECRAFT );
}

CHudSpacecraftHealth::~CHudSpacecraftHealth()
{
}

bool CHudSpacecraftHealth::ShouldDraw()
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();
	return CHudElement::ShouldDraw() && pPlayer && !pPlayer->IsInSpacecraftFirstperson() && pPlayer->IsAlive();
}

//void CHudSpacecraftHealth::MsgFunc_Fireball( bf_read &msg )
//{
//	const float flDuration = msg.ReadFloat();
//	if ( flDuration > FLT_EPSILON )
//	{
//		if ( !m_bShowingBar )
//		{
//			m_bShowingBar = true;
//			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "fireball_show" );
//		}
//
//		m_flBarAnimationFraction = 0.0f;
//		m_flBarAnimationSpeed = 1.0f / flDuration;
//
//		m_iFireballElementCount = 1 + floor( MAX( 0.0f, 10.0f - flDuration ) / 2.0f );
//	}
//}

void CHudSpacecraftHealth::Init()
{
	//HOOK_HUD_MESSAGE( CHudSpacecraftHealth, Fireball );
	Reset();

	const wchar_t *pwszText = g_pVGuiLocalize->Find( "#gstring_spacecrafthealth_label" );
	if ( pwszText != NULL )
	{
		Q_wcsncpy( m_wszLabelText, pwszText, sizeof( m_wszLabelText ) );
	}
}

void CHudSpacecraftHealth::Reset()
{
	m_flBarAnimationFraction = 0.0f;
}

void CHudSpacecraftHealth::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	const float flAngleRad = DEG2RAD( m_flBarAngle );
	m_vecBarAngleDirection.Init( sin( flAngleRad ), cos( flAngleRad ) );

	const int iScreenHeight = ScreenHeight();
	if ( iScreenHeight > 900 )
	{
		SetupVGUITex( "vgui/fireball_bar_900", m_iTexBar );
	}
	else if ( iScreenHeight > 700 )
	{
		SetupVGUITex( "vgui/fireball_bar_700", m_iTexBar );
	}
	else
	{
		SetupVGUITex( "vgui/fireball_bar", m_iTexBar );
	}
}

void CHudSpacecraftHealth::OnThink()
{
	float flFraction = 0.0f;
	C_GstringPlayer *pLocal = LocalGstringPlayer();
	const CSpacecraft *pSpacecraft = pLocal ? pLocal->GetSpacecraft() : NULL;
	if ( pSpacecraft != NULL && pSpacecraft->GetMaxHealth() > 0 )
	{
		flFraction = pSpacecraft->GetHealth() / float( pSpacecraft->GetMaxHealth() );
	}

	const float flLastAnimationFraction = m_flBarAnimationFraction;
	if ( abs( flFraction - m_flBarAnimationFraction ) <= 0.001f )
	{
		m_flBarAnimationFraction = flFraction;
	}
	else
	{
		m_flBarAnimationFraction += ( flFraction - m_flBarAnimationFraction ) * MIN( 1.0f, gpGlobals->frametime * 18.0f );
	}

	if ( flLastAnimationFraction < 1.0f && m_flBarAnimationFraction >= 1.0f )
	{
		m_flBarBlinkTimer = M_PI_F * 2.0f;
	}

	if ( m_flBarBlinkTimer > 0.0f )
	{
		m_flBarBlinkTimer -= gpGlobals->frametime * 6.0f;
		if ( m_flBarBlinkTimer < 0.0f )
		{
			m_flBarBlinkTimer = 0.0f;
		}
	}
}

void CHudSpacecraftHealth::Paint()
{
	PaintLabel();
	PaintBar();
}

void CHudSpacecraftHealth::PaintLabel()
{
	int iTextW, iTextT;
	surface()->GetTextSize( m_hTextFont, m_wszLabelText, iTextW, iTextT );

	const float flBlinkFraction = abs( sin( m_flBarBlinkTimer ) );
	Color color = Lerp( flBlinkFraction, GetFgColor(), Color( 255, 255, 255, 0 ) );

	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( color );
	surface()->DrawSetTextPos( ( GetWide() - iTextW ) * 0.5f, m_flTextYPos );
	surface()->DrawUnicodeString( m_wszLabelText );
}

void CHudSpacecraftHealth::PaintBar()
{
	const float flBlinkFraction = abs( sin( m_flBarBlinkTimer ) );
	Color color = Lerp( flBlinkFraction, m_BarBorderColor, Color( 255, 255, 255, 0 ) );
	surface()->DrawSetColor( color );

	float flXInset = m_flBarHeight / m_vecBarAngleDirection.y * m_vecBarAngleDirection.x;
	float flYInset = m_flBarY;
	const float flBoxInset = 2.0f;

	surface()->DrawLine( m_flBarX, flYInset,
		m_flBarX + flXInset, flYInset + m_flBarHeight );
	surface()->DrawLine( m_flBarX + m_flBarWidthBase - 1, flYInset,
		m_flBarX + m_flBarWidthBase - flXInset, flYInset + m_flBarHeight );

	surface()->DrawLine( m_flBarX, flYInset,
		m_flBarX + m_flBarWidthBase - 1, flYInset );
	surface()->DrawLine( m_flBarX + flXInset, flYInset + m_flBarHeight,
		m_flBarX + m_flBarWidthBase - flXInset - 1, flYInset + m_flBarHeight );

#if 0
	if ( m_flBarAnimationFraction > 0.0f )
	{
		const float flAnimatedSize = m_flBarWidthBase * m_flBarAnimationFraction;
		const float flCenterWidth = m_flBarWidthBase - flXInset * 2.0f;
		int iNumElements = flAnimatedSize > 0.0f ? 1 : 0;
		if ( flAnimatedSize > flXInset )
		{
			iNumElements++;
		}

		if ( flAnimatedSize > flCenterWidth + flXInset )
		{
			iNumElements++;
		}

		const float flUVY1 = m_flBarHeight / 64.0f;
		if ( iNumElements > 0 )
		{
			surface()->DrawSetTexture( m_iTexBar );
			surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );

			const float flElementFraction = MIN( 1.0f, flAnimatedSize / flXInset );
			Vertex_t barVertices[ 3 ] =
			{
				Vertex_t( Vector2D( m_flBarX, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( m_flBarX + flXInset * flElementFraction, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( m_flBarX + flXInset * flElementFraction, flYInset + m_flBarHeight * flElementFraction ),
					Vector2D( 0.0f, flUVY1 * flElementFraction ) ),
			};

			surface()->DrawTexturedPolygon( 3, barVertices );
		}

		if ( iNumElements > 1 )
		{
			const float flElementFraction = MIN( 1.0f, ( flAnimatedSize - flXInset ) / flCenterWidth );
			Vertex_t barVertices[ 4 ] =
			{
				Vertex_t( Vector2D( m_flBarX + flXInset, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( m_flBarX + flXInset + flCenterWidth * flElementFraction, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( m_flBarX + flXInset + flCenterWidth * flElementFraction, flYInset + m_flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
				Vertex_t( Vector2D( m_flBarX + flXInset, flYInset + m_flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
			};

			surface()->DrawTexturedPolygon( 4, barVertices );
		}

		if ( iNumElements > 2 )
		{
			const float flElementFraction = MIN( 1.0f, ( flAnimatedSize - flXInset - flCenterWidth ) / flXInset );
			Vertex_t barVertices[ 3 ] =
			{
				Vertex_t( Vector2D( m_flBarX + flXInset + flCenterWidth, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( m_flBarX + flXInset + flCenterWidth + flXInset * flElementFraction, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( m_flBarX + flXInset + flCenterWidth, flYInset + m_flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
			};

			surface()->DrawTexturedPolygon( 3, barVertices );
		}
	}
#else
	if ( m_flBarAnimationFraction > 0.0f )
	{
		const float flWidthHalf = m_flBarWidthBase * 0.5f;
		const float flAnimatedSize = flWidthHalf * m_flBarAnimationFraction;
		const float flCenterWidth = flWidthHalf - flXInset - flBoxInset;
		int iNumElements = flAnimatedSize > 0.0f ? 1 : 0;

		if ( flAnimatedSize > flCenterWidth )
		{
			iNumElements++;
		}

		flXInset += flBoxInset;
		flYInset += flBoxInset + 1;

		const float flUVY1 = ( m_flBarHeight - flBoxInset * 2.0f - 1.0f ) / 64.0f;
		const float flBarHeight =  m_flBarHeight - flBoxInset * 2.0f - 1.0f;
		if ( iNumElements > 0 )
		{
			surface()->DrawSetTexture( m_iTexBar );
			surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );

			const float flElementFraction = MIN( 1.0f, flAnimatedSize / flCenterWidth );
			const float flX0 = flWidthHalf - flCenterWidth * flElementFraction;
			const float flX1 = flWidthHalf + flCenterWidth * flElementFraction;
			Vertex_t barVertices[ 4 ] =
			{
				Vertex_t( Vector2D( flX0, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( flX1, flYInset ), vec2_origin ),
				Vertex_t( Vector2D( flX1, flYInset + flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
				Vertex_t( Vector2D( flX0, flYInset + flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
			};

			surface()->DrawTexturedPolygon( 4, barVertices );
		}

		if ( iNumElements > 1 )
		{
			{
				const float flElementFraction = 1.0f - MIN( 1.0f, ( flAnimatedSize - flCenterWidth + flBoxInset * 2.0f ) / flXInset );
				if ( flElementFraction >= 1.0f )
				{
					Vertex_t barVertices[ 3 ] =
					{
						Vertex_t( Vector2D( m_flBarX + flBoxInset * 2.0f + flXInset, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( m_flBarX + flXInset, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( m_flBarX + flXInset, flYInset + flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
					};
					surface()->DrawTexturedPolygon( 3, barVertices );
				}
				else
				{
					Vertex_t barVertices[ 4 ] =
					{
						Vertex_t( Vector2D( m_flBarX + flBoxInset * 2.0f + flXInset * flElementFraction, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( m_flBarX + flXInset, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( m_flBarX + flXInset, flYInset + flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
						Vertex_t( Vector2D( m_flBarX + flBoxInset * 2.0f + flXInset * flElementFraction, flYInset + flBarHeight * flElementFraction ),
							Vector2D( 0.0f, flUVY1 * flElementFraction ) ),
					};
					surface()->DrawTexturedPolygon( 4, barVertices );
				}
			}

			{
				const float flX0 = m_flBarX + m_flBarWidthBase - flXInset;
				const float flElementFraction = MIN( 1.0f, ( flAnimatedSize - flCenterWidth ) / flXInset );
				if ( flElementFraction >= 1.0f )
				{
					Vertex_t barVertices[ 3 ] =
					{
						Vertex_t( Vector2D( flX0, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( flX0 + flXInset - flBoxInset * 2.0f, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( flX0, flYInset + flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
					};
					surface()->DrawTexturedPolygon( 3, barVertices );
				}
				else
				{
					Vertex_t barVertices[ 4 ] =
					{
						Vertex_t( Vector2D( flX0, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( flX0 + ( flXInset - flBoxInset * 2.0f ) * flElementFraction, flYInset ), vec2_origin ),
						Vertex_t( Vector2D( flX0 + ( flXInset - flBoxInset * 2.0f ) * flElementFraction, flYInset + flBarHeight * ( 1.0f - flElementFraction ) ),
							Vector2D( 0.0f, flUVY1 * ( 1.0f - flElementFraction ) ) ),
						Vertex_t( Vector2D( flX0, flYInset + flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
					};
					surface()->DrawTexturedPolygon( 4, barVertices );
				}
			}
		}
	}
#endif

}
