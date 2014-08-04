#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "hud_fireball.h"

#include "iclientmode.h"

#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include "vutil.h"

#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudFireball );
DECLARE_HUD_MESSAGE( CHudFireball, Fireball );

CHudFireball::CHudFireball( const char *pElementName ) :
	CHudElement( pElementName ),
	BaseClass( NULL, "HudFireball" ),
	m_iTexBar( 0 ),
	m_vecBarAngleDirection( 0.0f, -1.0f ),
	m_bShowingBar( false ),
	m_flBarAnimationFraction( 0.0f ),
	m_flBarAnimationSpeed( 1.0f ),
	m_iFireballElementCount( 1 )
{
	*m_wszLabelText = 0;

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

CHudFireball::~CHudFireball()
{
}

void CHudFireball::MsgFunc_Fireball( bf_read &msg )
{
	const float flDuration = msg.ReadFloat();
	if ( flDuration > FLT_EPSILON )
	{
		if ( !m_bShowingBar )
		{
			m_bShowingBar = true;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "fireball_show" );
		}

		m_flBarAnimationFraction = 0.0f;
		m_flBarAnimationSpeed = 1.0f / flDuration;

		m_iFireballElementCount = 1 + floor( MAX( 0.0f, 10.0f - flDuration ) / 2.0f );
	}
}

void CHudFireball::Init()
{
	HOOK_HUD_MESSAGE( CHudFireball, Fireball );
	Reset();

	const wchar_t *pwszText = g_pVGuiLocalize->Find( "#gstring_fireball_label" );
	if ( pwszText != NULL )
	{
		Q_wcsncpy( m_wszLabelText, pwszText, sizeof( m_wszLabelText ) );
	}
}

void CHudFireball::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "fireball_reset" );
	m_bShowingBar = false;
	m_flBarAnimationFraction = 1.0f;
}

void CHudFireball::ApplySchemeSettings( IScheme *pScheme )
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

void CHudFireball::OnThink()
{
	if ( m_flBarAnimationFraction < 1.0f )
	{
		m_flBarAnimationFraction += gpGlobals->frametime * m_flBarAnimationSpeed;

		if ( m_flBarAnimationFraction > 1.0f )
		{
			m_flBarAnimationFraction = 1.0f;
			m_bShowingBar = false;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "fireball_hide" );
		}
	}
}

void CHudFireball::Paint()
{
	PaintLabel();
	PaintBar();
}

void CHudFireball::PaintLabel()
{
	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( GetFgColor() );
	surface()->DrawSetTextPos( m_flTextXPos, m_flTextYPos );
	surface()->DrawUnicodeString( m_wszLabelText );
}

void CHudFireball::PaintBar()
{
	surface()->DrawSetColor( m_BarBorderColor );

	const float flXInset = m_flBarHeight / m_vecBarAngleDirection.y * m_vecBarAngleDirection.x;
	const float flYInset = m_flBarY;
	float flBarOutlineX = m_flBarX + flXInset;

	surface()->DrawLine( flBarOutlineX, flYInset, m_flBarX, flYInset + m_flBarHeight );
	for ( int iBarComponent = 0; iBarComponent < m_iFireballElementCount; iBarComponent++ )
	{
		const float flWidth = ( iBarComponent > 0 ) ? m_flBarWidthElement : m_flBarWidthBase;
		surface()->DrawLine( flBarOutlineX, flYInset, flBarOutlineX + flWidth, flYInset );
		surface()->DrawLine( flBarOutlineX - flXInset, flYInset + m_flBarHeight,
			flBarOutlineX + flWidth - flXInset, flYInset + m_flBarHeight );

		flBarOutlineX += flWidth;

		surface()->DrawLine( flBarOutlineX, flYInset, flBarOutlineX - flXInset, flYInset + m_flBarHeight );
	}

	const float flAnimatedSize = ( m_flBarWidthBase + m_flBarWidthElement * ( m_iFireballElementCount - 1 ) ) * m_flBarAnimationFraction;
	if ( flAnimatedSize > 1.0f )
	{
		const float flUVY1 = m_flBarHeight / 64.0f;
		Vertex_t barVertices[ 4 ] =
		{
			Vertex_t( Vector2D( m_flBarX + flXInset, flYInset ), vec2_origin ),
			Vertex_t( Vector2D( m_flBarX + flXInset + flAnimatedSize, flYInset ), vec2_origin ),
			Vertex_t( Vector2D( m_flBarX + flAnimatedSize, flYInset + m_flBarHeight ), Vector2D( 0.0f, flUVY1 ) ),
			Vertex_t( Vector2D( m_flBarX, flYInset + m_flBarHeight ), Vector2D( 0.0f, flUVY1 ) )
		};

		surface()->DrawSetTexture( m_iTexBar );
		surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
		surface()->DrawTexturedPolygon( 4, barVertices );
	}
}
