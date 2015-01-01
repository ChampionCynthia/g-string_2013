
#include "cbase.h"
#include "holo_ship_health_text.h"
#include "gstring/cspacecraft.h"

#include "vgui_controls/Label.h"
#include "vgui/ISurface.h"

using namespace vgui;

CHoloShipHealthText::CHoloShipHealthText( ISpacecraftData *pSpacecraftData ) :
	m_pSpacecraftData( pSpacecraftData ),
	m_iShield( -1 ),
	m_iHull( -1 )
{
	m_pLabelShieldText = new Label( this, "", "#holo_gui_shield" );
	m_pLabelHullText = new Label( this, "", "#holo_gui_hull" );
	m_pLabelShieldValue = new Label( this, "", "" );
	m_pLabelHullValue = new Label( this, "", "" );

	m_flScale = 0.02f;

	SetAngles( QAngle( 0, 200, 0 ) );
	//SetOrigin( Vector( -4.5f, 14, -8.5f ) );
	SetOrigin( Vector( -4.5f, 14.5f, -7.0f ) );
}

void CHoloShipHealthText::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelShieldText->SetFgColor( Color( HOLO_COLOR255_HIGHLIGHT, 255 ) );
	m_pLabelHullText->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelShieldValue->SetFgColor( Color( HOLO_COLOR255_HIGHLIGHT, 255 ) );
	m_pLabelHullValue->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );

	const int iFontTall = surface()->GetFontTall( m_FontLarge );
	m_pLabelShieldText->SetFont( m_FontLarge );
	m_pLabelHullText->SetFont( m_FontLarge );
	m_pLabelShieldValue->SetFont( m_FontLarge );
	m_pLabelHullValue->SetFont( m_FontLarge );

	m_pLabelShieldText->SizeToContents();
	m_pLabelHullText->SizeToContents();

	m_pLabelShieldText->SetPos( 5, 0 );
	m_pLabelHullText->SetPos( 5, iFontTall );
	m_pLabelShieldValue->SetContentAlignment( Label::a_east );
	m_pLabelHullValue->SetContentAlignment( Label::a_east );
	m_pLabelHullValue->SetPos( 0, iFontTall );

	SetBounds( 0, 0, iFontTall * 6, iFontTall * 2 );
}

void CHoloShipHealthText::Think( float frametime )
{
	const int iShield = m_pSpacecraftData->GetShield();
	if ( iShield != m_iShield )
	{
		m_iShield = iShield;
		MakeReadyForUse();

		int shieldPercentage = int(iShield / (float)m_pSpacecraftData->GetMaxShield() * 100.0f);
		m_pLabelShieldValue->SetText( VarArgs( "%i", shieldPercentage ) );
		m_pLabelShieldValue->MakeReadyForUse();
		m_pLabelShieldValue->SetSize( GetWide(), GetTall() / 2 );
	}

	const int iHull = m_pSpacecraftData->GetHull();
	if ( iHull != m_iHull )
	{
		m_iHull = iHull;
		MakeReadyForUse();

		int hullPercentage = int(iHull / (float)m_pSpacecraftData->GetMaxHull() * 100.0f);
		m_pLabelHullValue->SetText( VarArgs( "%i", hullPercentage ) );
		m_pLabelHullValue->MakeReadyForUse();
		m_pLabelHullValue->SetSize( GetWide(), GetTall() / 2 );
	}
}

