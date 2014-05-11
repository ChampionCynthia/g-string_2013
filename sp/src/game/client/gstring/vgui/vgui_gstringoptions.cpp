
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "gstring/gstring_cvars.h"
#include "gstring/vgui/vgui_gstringoptions.h"
#include "ienginevgui.h"

#include "vgui_controls/Panel.h"

#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/PropertyPage.h"

using namespace vgui;

extern ConVar gstring_firstpersonbody_enable;
extern ConVar gstring_firstpersonbody_shadow_enable;
extern ConVar gstring_volumetrics_enabled;

CVGUIGstringOptions::CVGUIGstringOptions( VPANEL parent, const char *pName ) : BaseClass( NULL, pName )
{
	SetParent( parent );

	Activate();

	m_pPropertySheet = new PropertySheet( this, "property_sheet" );

	PropertyPage *pPagePostProcessing = new PropertyPage( m_pPropertySheet, "" );
	PropertyPage *pPageGame = new PropertyPage( m_pPropertySheet, "" );

	LoadControlSettings( "resource/gstring_options.res" );

	m_pPropertySheet->AddPage( pPagePostProcessing, "#pp_postprocessing_title" );
	m_pPropertySheet->AddPage( pPageGame, "#option_game_title" );

	m_pCheck_HurtFX = new CheckButton( pPagePostProcessing, "check_bars", "" );
	m_pCheck_Vignette = new CheckButton( pPagePostProcessing, "check_vignette", "" );
	m_pCheck_GodRays = new CheckButton( pPagePostProcessing, "check_godrays", "" );
	m_pCheck_WaterEffects = new CheckButton( pPagePostProcessing, "check_screenwater", "" );
	m_pCheck_LensFlare = new CheckButton( pPagePostProcessing, "check_lensflare", "" );
	m_pCBox_BloomFlare = new ComboBox( pPagePostProcessing, "check_bloomflare", 3, false );
	m_pCBox_BloomFlare->AddItem( "#pp_bloom_flare_never", NULL );
	m_pCBox_BloomFlare->AddItem( "#pp_bloom_flare_map_based", NULL );
	m_pCBox_BloomFlare->AddItem( "#pp_bloom_flare_always", NULL );
	m_pCheck_DreamBlur = new CheckButton( pPagePostProcessing, "check_dreamblur", "" );
	m_pCheck_ScreenBlur = new CheckButton( pPagePostProcessing, "check_screenblur", "" );

#define CREATE_VGUI_SLIDER( var, name, minRange, maxRange, ticks ) var = new Slider( pPagePostProcessing, name ); \
	var->SetRange( minRange, maxRange ); \
	var->SetNumTicks( ticks ); \
	var->AddActionSignalTarget( this )

#define CREATE_VGUI_CHECKBOX( var, name, page ) var = new CheckButton( page, name, "" ); \
	var->AddActionSignalTarget( this )

	CREATE_VGUI_SLIDER( m_pSlider_CinematicBars_Size, "slider_bars", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_MotionBlur_Strength, "slider_mblur", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_BloomFlare_Strength, "slider_bflare", 1, 10, 9 );
	CREATE_VGUI_SLIDER( m_pSlider_ExplosionBlur_Strength, "slider_expblur", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_Desaturation_Strength, "slider_desat", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_FilmGrain_Strength, "slider_filmgrain", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_Chromatic_Strength, "slider_chromatic", 0, 10, 10 );

	m_pLabel_Value_CinematicBars = new Label( pPagePostProcessing, "label_bars", "" );
	m_pLabel_Value_MotionBlur = new Label( pPagePostProcessing, "label_mblur", "" );
	m_pLabel_Value_BloomFlare = new Label( pPagePostProcessing, "label_bflare", "" );
	m_pLabel_Value_ExplosionBlur = new Label( pPagePostProcessing, "label_expblur", "" );
	m_pLabel_Value_Desaturation = new Label( pPagePostProcessing, "label_desat", "" );
	m_pLabel_Value_FilmGrain = new Label( pPagePostProcessing, "label_filmgrain", "" );
	m_pLabel_Value_Chromatic = new Label( pPagePostProcessing, "label_chromatic", "" );

	pPagePostProcessing->LoadControlSettings( "resource/gstring_options_page_postprocessing.res" );

	CREATE_VGUI_CHECKBOX( m_pCheck_FirstPersonBody, "check_first_person_body", pPageGame );
	CREATE_VGUI_CHECKBOX( m_pCheck_FirstPersonShadow, "check_first_person_shadow", pPageGame );
	CREATE_VGUI_CHECKBOX( m_pCheck_LightVolumetrics, "check_volumetrics", pPageGame );

	pPageGame->LoadControlSettings( "resource/gstring_options_page_game.res" );

	DoModal();

	SetDeleteSelfOnClose( true );
	SetVisible( true );
	SetSizeable(false);
	SetMoveable(true);

	SetTitle( "#pp_title", false );

	OnSliderMoved( NULL );
}

CVGUIGstringOptions::~CVGUIGstringOptions()
{
}

void CVGUIGstringOptions::OnCommand( const char *cmd )
{
	if ( !Q_stricmp( cmd, "save" ) )
	{
#define CVAR_CHECK_INTEGER( x, y ) ( x.SetValue( ( y->IsSelected() ? 1 : 0 ) ) )
#define CVAR_SLIDER_FLOAT( x, y, ratio ) ( x.SetValue( (float)(y->GetValue()/(float)ratio ) ) )

		CVAR_CHECK_INTEGER( cvar_gstring_drawhurtfx, m_pCheck_HurtFX );
		CVAR_CHECK_INTEGER( cvar_gstring_drawvignette, m_pCheck_Vignette );
		CVAR_CHECK_INTEGER( cvar_gstring_drawgodrays, m_pCheck_GodRays );
		CVAR_CHECK_INTEGER( cvar_gstring_drawscreenblur, m_pCheck_ScreenBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawdreamblur, m_pCheck_DreamBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawlensflare, m_pCheck_LensFlare );
		CVAR_CHECK_INTEGER( cvar_gstring_drawwatereffects, m_pCheck_WaterEffects );
		CVAR_CHECK_INTEGER( gstring_firstpersonbody_enable, m_pCheck_FirstPersonBody );
		CVAR_CHECK_INTEGER( gstring_firstpersonbody_shadow_enable, m_pCheck_FirstPersonShadow );
		CVAR_CHECK_INTEGER( gstring_volumetrics_enabled, m_pCheck_LightVolumetrics );

		CVAR_SLIDER_FLOAT( cvar_gstring_explosionfx_strength, m_pSlider_ExplosionBlur_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_bars_scale, m_pSlider_CinematicBars_Size, 50 );
		CVAR_SLIDER_FLOAT( cvar_gstring_motionblur_scale, m_pSlider_MotionBlur_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_bloomflare_strength, m_pSlider_BloomFlare_Strength, 2 );
		CVAR_SLIDER_FLOAT( cvar_gstring_desaturation_strength, m_pSlider_Desaturation_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_filmgrain_strength, m_pSlider_FilmGrain_Strength, 50 );
		CVAR_SLIDER_FLOAT( cvar_gstring_chromatic_aberration, m_pSlider_Chromatic_Strength, 1000 );

		cvar_gstring_drawbloomflare.SetValue( m_pCBox_BloomFlare->GetActiveItem() );

		engine->ClientCmd( "host_writeconfig" );

		CloseModal();
	}
	else if ( !Q_stricmp( cmd, "defaults" ) )
	{
		cvar_gstring_drawhurtfx.Revert();
		cvar_gstring_drawvignette.Revert();
		cvar_gstring_drawgodrays.Revert();
		cvar_gstring_drawscreenblur.Revert();
		cvar_gstring_drawdreamblur.Revert();
		cvar_gstring_drawlensflare.Revert();
		cvar_gstring_drawwatereffects.Revert();

		cvar_gstring_drawbloomflare.Revert();

		cvar_gstring_explosionfx_strength.Revert();
		cvar_gstring_bars_scale.Revert();
		cvar_gstring_motionblur_scale.Revert();
		cvar_gstring_bloomflare_strength.Revert();
		cvar_gstring_desaturation_strength.Revert();
		cvar_gstring_filmgrain_strength.Revert();
		cvar_gstring_chromatic_aberration.Revert();

		ReadValues();
	}
	else
		BaseClass::OnCommand( cmd );
}

void CVGUIGstringOptions::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	ReadValues();
}

void CVGUIGstringOptions::ReadValues()
{
#define CVAR_CHECK_SELECTED( x, y ) ( y->SetSelected( x.GetBool() ) )
#define CVAR_SLIDER_INTEGER( x, y, ratio ) ( y->SetValue( x.GetFloat() * ratio ) )

	CVAR_CHECK_SELECTED( cvar_gstring_drawhurtfx, m_pCheck_HurtFX );
	CVAR_CHECK_SELECTED( cvar_gstring_drawvignette, m_pCheck_Vignette );
	CVAR_CHECK_SELECTED( cvar_gstring_drawgodrays, m_pCheck_GodRays );
	CVAR_CHECK_SELECTED( cvar_gstring_drawscreenblur, m_pCheck_ScreenBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawdreamblur, m_pCheck_DreamBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawlensflare, m_pCheck_LensFlare );
	CVAR_CHECK_SELECTED( cvar_gstring_drawwatereffects, m_pCheck_WaterEffects );
	CVAR_CHECK_SELECTED( gstring_firstpersonbody_enable, m_pCheck_FirstPersonBody );
	m_pCheck_FirstPersonShadow->SetEnabled( gstring_firstpersonbody_enable.GetBool() );
	CVAR_CHECK_SELECTED( gstring_firstpersonbody_shadow_enable, m_pCheck_FirstPersonShadow );
	CVAR_CHECK_SELECTED( gstring_volumetrics_enabled, m_pCheck_LightVolumetrics );

	m_pCBox_BloomFlare->ActivateItem( clamp( cvar_gstring_drawbloomflare.GetInt(),
		0, m_pCBox_BloomFlare->GetItemCount() ) );

	CVAR_SLIDER_INTEGER( cvar_gstring_explosionfx_strength, m_pSlider_ExplosionBlur_Strength, 11 );
	CVAR_SLIDER_INTEGER( cvar_gstring_bars_scale, m_pSlider_CinematicBars_Size, 51 );
	CVAR_SLIDER_INTEGER( cvar_gstring_motionblur_scale, m_pSlider_MotionBlur_Strength, 11 );
	CVAR_SLIDER_INTEGER( cvar_gstring_bloomflare_strength, m_pSlider_BloomFlare_Strength, 2 );
	CVAR_SLIDER_INTEGER( cvar_gstring_desaturation_strength, m_pSlider_Desaturation_Strength, 10 );
	CVAR_SLIDER_INTEGER( cvar_gstring_filmgrain_strength, m_pSlider_FilmGrain_Strength, 51 );
	CVAR_SLIDER_INTEGER( cvar_gstring_chromatic_aberration, m_pSlider_Chromatic_Strength, 1100 );
}

void CVGUIGstringOptions::PerformLayout()
{
	BaseClass::PerformLayout();

	MoveToCenterOfScreen();
}

void CVGUIGstringOptions::OnCheckButtonChecked( Panel *panel )
{
	if ( panel == m_pCheck_FirstPersonBody )
	{
		m_pCheck_FirstPersonShadow->SetEnabled( m_pCheck_FirstPersonBody->IsSelected() );
	}
}

void CVGUIGstringOptions::OnSliderMoved( KeyValues *pKV )
{
	m_pLabel_Value_CinematicBars->SetText( VarArgs( "%.1f", m_pSlider_CinematicBars_Size->GetValue() / 10.0f ) );
	m_pLabel_Value_MotionBlur->SetText( VarArgs( "%.1f", m_pSlider_MotionBlur_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_BloomFlare->SetText( VarArgs( "%.1f", m_pSlider_BloomFlare_Strength->GetValue() / 2.0f ) );
	m_pLabel_Value_ExplosionBlur->SetText( VarArgs( "%.1f", m_pSlider_ExplosionBlur_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_Desaturation->SetText( VarArgs( "%.1f", m_pSlider_Desaturation_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_FilmGrain->SetText( VarArgs( "%.1f", m_pSlider_FilmGrain_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_Chromatic->SetText( VarArgs( "%.1f", m_pSlider_Chromatic_Strength->GetValue() / 10.0f ) );
}

CON_COMMAND( vgui_showGstringOptions, "" )
{
	vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	new CVGUIGstringOptions( GameUIRoot, "GstringOptions" );
}
