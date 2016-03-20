
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

#include "matsys_controls/colorpickerpanel.h"

using namespace vgui;

extern ConVar gstring_firstpersonbody_enable;
//extern ConVar gstring_firstpersonbody_shadow_enable;
extern ConVar gstring_volumetrics_enabled;


static PostProcessingState_t presets[] =
{
	// Subtle
	{ true, true, true, true, true, true, false, true, 0.0f, 0.3f, 0.7f, 0.3f, 0.2f, 0.1f, 0.2f, 0.2f },
	// Vibrant
	{ true, true, true, true, true, true, true, true, 0.0f, 0.7f, 1.0f, 0.5f, 0.0f, 0.2f, 0.6f, 0.8f },
	// film noir
	{ true, true, true, true, true, true, true, true, 0.0f, 0.8f, 1.0f, 0.5f, 1.0f, 0.3f, 0.2f, 0.9f },
	// film noir red
	{ true, true, true, true, true, true, true, true, 0.0f, 0.8f, 1.0f, 0.5f, 0.5f, 0.3f, 0.2f, 0.9f },
	// bw
	{ false, false, false, false, false, false, false, false, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
	// bw red
	//{ false, false, false, false, false, false, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f },
	// 70 mm
	{ true, true, true, true, true, true, true, true, 1.0f, 0.2f, 0.8f, 0.1f, 0.1f, 0.2f, 0.7f, 0.6f },
	// none
	{ false, false, false, false, false, false, false, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
};

static float scales[ PP_VALS ] = {
	5.0f,
	1.0f,
	0.2f,
	1.0f,
	1.0f,
	5.0f,
	1.0f,
	100.0f
};

static Color HUDColors[ HUDCOLOR_VALS ] = {
	Color( 255, 229, 153, 255 ),
	Color( 255, 128, 0, 255 ),
	Color( 255, 255, 255, 255 ),
	Color( 164, 164, 164, 255 ),
};

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

#define CREATE_VGUI_SLIDER( var, name, minRange, maxRange, ticks ) var = new Slider( pPagePostProcessing, name ); \
	var->SetRange( minRange, maxRange ); \
	var->SetNumTicks( ticks ); \
	var->AddActionSignalTarget( this )

#define CREATE_VGUI_CHECKBOX( var, name, page ) var = new CheckButton( page, name, "" ); \
	var->AddActionSignalTarget( this )

	CREATE_VGUI_CHECKBOX( m_pCheck_HurtFX, "check_damageeffects", pPagePostProcessing );
	CREATE_VGUI_CHECKBOX( m_pCheck_Vignette, "check_vignette", pPagePostProcessing );
	CREATE_VGUI_CHECKBOX( m_pCheck_GodRays, "check_godrays", pPagePostProcessing );
	CREATE_VGUI_CHECKBOX( m_pCheck_WaterEffects, "check_screenwater", pPagePostProcessing );
	CREATE_VGUI_CHECKBOX( m_pCheck_LensFlare, "check_lensflare", pPagePostProcessing );
	CREATE_VGUI_CHECKBOX( m_pCheck_DreamBlur, "check_dreamblur", pPagePostProcessing );
	CREATE_VGUI_CHECKBOX( m_pCheck_ScreenBlur, "check_screenblur", pPagePostProcessing );
	CREATE_VGUI_CHECKBOX( m_pCheck_CinemaOverlay, "check_cinemaoverlay", pPagePostProcessing );
	m_pCBox_Preset = new ComboBox( pPagePostProcessing, "combo_preset", 10, false );
	m_pCBox_Preset->AddItem( "#pp_preset_subtle", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_vibrant", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_filmnoir", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_filmnoir_red", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_bw", NULL );
	//m_pCBox_Preset->AddItem( "#pp_preset_bw_red", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_70mm", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_none", NULL );
	m_pCBox_Preset->AddActionSignalTarget( this );

	CREATE_VGUI_SLIDER( m_pSlider_CinematicBars_Size, "slider_bars", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_MotionBlur_Strength, "slider_mblur", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_BloomFlare_Strength, "slider_bflare", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_ExplosionBlur_Strength, "slider_expblur", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_Desaturation_Strength, "slider_desat", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_FilmGrain_Strength, "slider_filmgrain", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_Bend_Strength, "slider_bend", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_Chromatic_Strength, "slider_chromatic", 0, 10, 10 );

	m_pLabel_Value_CinematicBars = new Label( pPagePostProcessing, "label_bars", "" );
	m_pLabel_Value_MotionBlur = new Label( pPagePostProcessing, "label_mblur", "" );
	m_pLabel_Value_BloomFlare = new Label( pPagePostProcessing, "label_bflare", "" );
	m_pLabel_Value_ExplosionBlur = new Label( pPagePostProcessing, "label_expblur", "" );
	m_pLabel_Value_Desaturation = new Label( pPagePostProcessing, "label_desat", "" );
	m_pLabel_Value_FilmGrain = new Label( pPagePostProcessing, "label_filmgrain", "" );
	m_pLabel_Value_Bend = new Label( pPagePostProcessing, "label_bend", "" );
	m_pLabel_Value_Chromatic = new Label( pPagePostProcessing, "label_chromatic", "" );

	pPagePostProcessing->LoadControlSettings( "resource/gstring_options_page_postprocessing.res" );

	CREATE_VGUI_CHECKBOX( m_pCheck_FirstPersonBody, "check_first_person_body", pPageGame );
	//CREATE_VGUI_CHECKBOX( m_pCheck_FirstPersonShadow, "check_first_person_shadow", pPageGame );
	CREATE_VGUI_CHECKBOX( m_pCheck_LightVolumetrics, "check_volumetrics", pPageGame );

	m_pCBox_HUDColorPreset = new ComboBox( pPageGame, "cbox_color_preset", 10, false );
	m_pCBox_HUDColorPreset->AddItem( "#options_game_hud_color_preset_default", NULL );
	m_pCBox_HUDColorPreset->AddItem( "#options_game_hud_color_preset_orange", NULL );
	m_pCBox_HUDColorPreset->AddItem( "#options_game_hud_color_preset_white", NULL );
	m_pCBox_HUDColorPreset->AddItem( "#options_game_hud_color_preset_dark", NULL );
	m_pCBox_HUDColorPreset->AddActionSignalTarget( this );
	m_pHUDColorPicker = new CColorPickerButton( pPageGame, "hud_color_picker_button", this );

	pPageGame->LoadControlSettings( "resource/gstring_options_page_game.res" );

	DoModal();

	SetDeleteSelfOnClose( true );
	SetVisible( true );
	SetSizeable(false);
	SetMoveable(true);

	SetTitle( "#pp_title", false );

	m_pVarChecks[ 0 ] = &cvar_gstring_drawhurtfx;
	m_pVarChecks[ 1 ] = &cvar_gstring_drawgodrays;
	m_pVarChecks[ 2 ] = &cvar_gstring_drawwatereffects;
	m_pVarChecks[ 3 ] = &cvar_gstring_drawvignette;
	m_pVarChecks[ 4 ] = &cvar_gstring_drawlensflare;
	m_pVarChecks[ 5 ] = &cvar_gstring_drawdreamblur;
	m_pVarChecks[ 6 ] = &cvar_gstring_drawscreenblur;
	m_pVarChecks[ 7 ] = &cvar_gstring_drawcinemaoverlay;
	m_pVarValues[ 0 ] = &cvar_gstring_bars_scale;
	m_pVarValues[ 1 ] = &cvar_gstring_motionblur_scale;
	m_pVarValues[ 2 ] = &cvar_gstring_bloomflare_strength;
	m_pVarValues[ 3 ] = &cvar_gstring_explosionfx_strength;
	m_pVarValues[ 4 ] = &cvar_gstring_desaturation_strength;
	m_pVarValues[ 5 ] = &cvar_gstring_filmgrain_strength;
	m_pVarValues[ 6 ] = &cvar_gstring_bend_strength;
	m_pVarValues[ 7 ] = &cvar_gstring_chromatic_aberration;

	CvarToState();

	OnSliderMoved( NULL );
}

CVGUIGstringOptions::~CVGUIGstringOptions()
{
}

void CVGUIGstringOptions::OnCommand( const char *cmd )
{
	if ( !Q_stricmp( cmd, "save" ) )
	{
#define CVAR_CHECK_INTEGER( x, y ) ( x.SetValue( ( y->IsSelected() ? 1 : int(0) ) ) )
//#define CVAR_SLIDER_FLOAT( x, y, ratio ) ( x.SetValue( (float)(y->GetValue()/(float)ratio ) ) )
//
//		CVAR_CHECK_INTEGER( cvar_gstring_drawhurtfx, m_pCheck_HurtFX );
//		CVAR_CHECK_INTEGER( cvar_gstring_drawvignette, m_pCheck_Vignette );
//		CVAR_CHECK_INTEGER( cvar_gstring_drawgodrays, m_pCheck_GodRays );
//		CVAR_CHECK_INTEGER( cvar_gstring_drawscreenblur, m_pCheck_ScreenBlur );
//		CVAR_CHECK_INTEGER( cvar_gstring_drawdreamblur, m_pCheck_DreamBlur );
//		CVAR_CHECK_INTEGER( cvar_gstring_drawlensflare, m_pCheck_LensFlare );
//		CVAR_CHECK_INTEGER( cvar_gstring_drawwatereffects, m_pCheck_WaterEffects );
//
//		CVAR_SLIDER_FLOAT( cvar_gstring_explosionfx_strength, m_pSlider_ExplosionBlur_Strength, 10 );
//		CVAR_SLIDER_FLOAT( cvar_gstring_bars_scale, m_pSlider_CinematicBars_Size, 50 );
//		CVAR_SLIDER_FLOAT( cvar_gstring_motionblur_scale, m_pSlider_MotionBlur_Strength, 10 );
//		CVAR_SLIDER_FLOAT( cvar_gstring_bloomflare_strength, m_pSlider_BloomFlare_Strength, 2 );
//		CVAR_SLIDER_FLOAT( cvar_gstring_desaturation_strength, m_pSlider_Desaturation_Strength, 10 );
//		CVAR_SLIDER_FLOAT( cvar_gstring_filmgrain_strength, m_pSlider_FilmGrain_Strength, 50 );
//		CVAR_SLIDER_FLOAT( cvar_gstring_bend_strength, m_pSlider_Bend_Strength, 10 );
//		CVAR_SLIDER_FLOAT( cvar_gstring_chromatic_aberration, m_pSlider_Chromatic_Strength, 1000 );
		CVAR_CHECK_INTEGER( gstring_firstpersonbody_enable, m_pCheck_FirstPersonBody );
		CVAR_CHECK_INTEGER( gstring_volumetrics_enabled, m_pCheck_LightVolumetrics );
		StateToCvar();

		gstring_hud_color.SetValue( VarArgs( "%i %i %i 255", m_colHUD.r(), m_colHUD.g(), m_colHUD.b() ) );

		engine->ClientCmd( "host_writeconfig" );
		engine->ClientCmd( "hud_reloadscheme" );

		CloseModal();
	}
	else if ( !Q_stricmp( cmd, "defaults" ) )
	{
		m_pCBox_Preset->ActivateItem( 0 );

		gstring_hud_color.Revert();

		ReadValues( true );
		UpdateLabels();
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

void CVGUIGstringOptions::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	ReadValues( true );
	UpdateLabels();
}

void CVGUIGstringOptions::ReadValues( bool bUpdatePreset )
{
#define CVAR_CHECK_SELECTED( x, y ) ( y->SetSelectedNoMessage( x.GetBool() ) )
//#define CVAR_SLIDER_INTEGER( x, y, ratio ) ( y->SetValue( x.GetFloat() * ratio, false ) )

	CVAR_CHECK_SELECTED( gstring_firstpersonbody_enable, m_pCheck_FirstPersonBody );
	CVAR_CHECK_SELECTED( gstring_volumetrics_enabled, m_pCheck_LightVolumetrics );

#define CVAR_STATE_CHECK_SELECTED( i, y ) ( y->SetSelectedNoMessage( m_state.checks[ i ] ) )
#define CVAR_STATE_SLIDER_INTEGER( i, y ) ( y->SetValue( m_state.val[ i ] * 10.1f, false ) )
	
	CVAR_STATE_CHECK_SELECTED( 0, m_pCheck_HurtFX );
	CVAR_STATE_CHECK_SELECTED( 1, m_pCheck_GodRays );
	CVAR_STATE_CHECK_SELECTED( 2, m_pCheck_WaterEffects );
	CVAR_STATE_CHECK_SELECTED( 3, m_pCheck_Vignette );
	CVAR_STATE_CHECK_SELECTED( 4, m_pCheck_LensFlare );
	CVAR_STATE_CHECK_SELECTED( 5, m_pCheck_DreamBlur );
	CVAR_STATE_CHECK_SELECTED( 6, m_pCheck_ScreenBlur );
	CVAR_STATE_CHECK_SELECTED( 7, m_pCheck_CinemaOverlay );
	CVAR_STATE_SLIDER_INTEGER( 0, m_pSlider_CinematicBars_Size );
	CVAR_STATE_SLIDER_INTEGER( 1, m_pSlider_MotionBlur_Strength );
	CVAR_STATE_SLIDER_INTEGER( 2, m_pSlider_BloomFlare_Strength );
	CVAR_STATE_SLIDER_INTEGER( 3, m_pSlider_ExplosionBlur_Strength );
	CVAR_STATE_SLIDER_INTEGER( 4, m_pSlider_Desaturation_Strength );
	CVAR_STATE_SLIDER_INTEGER( 5, m_pSlider_FilmGrain_Strength );
	CVAR_STATE_SLIDER_INTEGER( 6, m_pSlider_Bend_Strength );
	CVAR_STATE_SLIDER_INTEGER( 7, m_pSlider_Chromatic_Strength );

	UTIL_StringToColor( m_colHUD, gstring_hud_color.GetString() );
	m_pHUDColorPicker->SetColor( m_colHUD );

	for ( int i = 0; i < HUDCOLOR_VALS; ++i )
	{
		const Color &col = HUDColors[ i ];
		if ( col == m_colHUD )
		{
			m_pCBox_HUDColorPreset->ActivateItem( i );
			break;
		}
	}

	if ( bUpdatePreset )
	{
		int presetIndex = FindCurrentPreset();
		if ( presetIndex >= 0 )
		{
			m_pCBox_Preset->ActivateItem( presetIndex );
		}
	}
}

void CVGUIGstringOptions::PerformLayout()
{
	BaseClass::PerformLayout();

	MoveToCenterOfScreen();
}

void CVGUIGstringOptions::OnCheckButtonChecked( Panel *panel )
{
	CheckButton *pCheckButton = dynamic_cast< CheckButton* >( panel );
	if ( pCheckButton != NULL )
	{
		bool value = pCheckButton->IsSelected();
		CheckButton *checkButtons[ PP_CHECKS ] = {
			m_pCheck_HurtFX,
			m_pCheck_GodRays,
			m_pCheck_WaterEffects,
			m_pCheck_Vignette,
			m_pCheck_LensFlare,
			m_pCheck_DreamBlur,
			m_pCheck_ScreenBlur,
			m_pCheck_CinemaOverlay
		};
		for ( int i = 0; i < PP_CHECKS; ++i )
		{
			if ( checkButtons[ i ] == panel )
			{
				m_state.checks[ i ] = value;
			}
		}
	}

	OnPresetModified();
}

void CVGUIGstringOptions::UpdateLabels()
{
	m_pLabel_Value_CinematicBars->SetText( VarArgs( "%.1f", m_state.val[ 0 ] ) );
	m_pLabel_Value_MotionBlur->SetText( VarArgs( "%.1f", m_state.val[ 1 ] ) );
	m_pLabel_Value_BloomFlare->SetText( VarArgs( "%.1f", m_state.val[ 2 ] ) );
	m_pLabel_Value_ExplosionBlur->SetText( VarArgs( "%.1f", m_state.val[ 3 ] ) );
	m_pLabel_Value_Desaturation->SetText( VarArgs( "%.1f", m_state.val[ 4 ] ) );
	m_pLabel_Value_FilmGrain->SetText( VarArgs( "%.1f", m_state.val[ 5 ] ) );
	m_pLabel_Value_Bend->SetText( VarArgs( "%.1f", m_state.val[ 6 ] ) );
	m_pLabel_Value_Chromatic->SetText( VarArgs( "%.1f", m_state.val[ 7 ] ) );
}

void CVGUIGstringOptions::OnSliderMoved( Panel *panel )
{
	Slider *pSlider = dynamic_cast< Slider* >( panel );
	if ( pSlider != NULL )
	{
		int value = pSlider->GetValue();

		Slider *sliders[ PP_VALS ] = {
			m_pSlider_CinematicBars_Size,
			m_pSlider_MotionBlur_Strength,
			m_pSlider_BloomFlare_Strength,
			m_pSlider_ExplosionBlur_Strength,
			m_pSlider_Desaturation_Strength,
			m_pSlider_FilmGrain_Strength,
			m_pSlider_Bend_Strength,
			m_pSlider_Chromatic_Strength
		};
		for ( int i = 0; i < PP_VALS; ++i )
		{
			if ( sliders[ i ] == panel )
			{
				m_state.val[ i ] = value * 0.101f;
			}
		}
	}

	OnPresetModified();
	UpdateLabels();
}

void CVGUIGstringOptions::OnTextChanged( KeyValues *pKV )
{
	Panel *p = (Panel*)pKV->GetPtr( "panel" );
	if ( p == m_pCBox_Preset )
	{
		ApplyPreset( m_pCBox_Preset->GetActiveItem() );
	}
	else if ( p == m_pCBox_HUDColorPreset )
	{
		const Color &col = HUDColors[ m_pCBox_HUDColorPreset->GetActiveItem() ];
		m_colHUD = col;
		m_pHUDColorPicker->SetColor( col );
	}
}

void CVGUIGstringOptions::OnPicked( KeyValues *pKV )
{
	m_colHUD = pKV->GetColor( "color" );
}

void CVGUIGstringOptions::ApplyPreset( int index )
{
	if ( index < 0 || index >= ARRAYSIZE( presets ) )
	{
		return;
	}

	const PostProcessingState_t &p = presets[ index ];
	for ( int c = 0; c < PP_CHECKS; ++c )
	{
		m_state.checks[ c ] = p.checks[ c ];
	}
	for ( int v = 0; v < PP_VALS; ++v )
	{
		m_state.val[ v ] = p.val[ v ];
	}

	ReadValues( false );
	UpdateLabels();
}

int CVGUIGstringOptions::FindCurrentPreset()
{
	for ( int i = 0; i < ARRAYSIZE( presets ); ++i )
	{
		const PostProcessingState_t &p = presets[ i ];
		bool bWrong = false;
		for ( int c = 0; c < PP_CHECKS; ++c )
		{
			if ( m_state.checks[ c ] != p.checks[ c ] )
			{
				bWrong = true;
			}
		}
		for ( int v = 0; v < PP_VALS; ++v )
		{
			if ( !CloseEnough( m_state.val[ v ], p.val[ v ] ) )
			{
				bWrong = true;
			}
		}
		if ( !bWrong )
		{
			return i;
		}
	}
	return -1;
}

void CVGUIGstringOptions::OnPresetModified()
{
	m_pCBox_Preset->SetText( "#pp_preset_custom" );
}

void CVGUIGstringOptions::CvarToState()
{
	for ( int i = 0; i < PP_CHECKS; ++i )
	{
		m_state.checks[ i ] = m_pVarChecks[ i ]->GetBool();
	}

	for ( int i = 0; i < PP_VALS; ++i )
	{
		m_state.val[ i ] = m_pVarValues[ i ]->GetFloat() * scales[ i ];
	}
}

void CVGUIGstringOptions::StateToCvar()
{
	for ( int i = 0; i < PP_CHECKS; ++i )
	{
		m_pVarChecks[ i ]->SetValue( m_state.checks[ i ] ? 1 : int( 0 ) );
	}

	for ( int i = 0; i < PP_VALS; ++i )
	{
		m_pVarValues[ i ]->SetValue( m_state.val[ i ] / scales[ i ] );
	}
}

CON_COMMAND( vgui_showGstringOptions, "" )
{
	vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	new CVGUIGstringOptions( GameUIRoot, "GstringOptions" );
}
