
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

struct Preset_t
{
	bool checks[7];
	float val[8];
};

// m_pCheck_HurtFX, m_pCheck_GodRays, m_pCheck_WaterEffects, m_pCheck_Vignette, m_pCheck_LensFlare, m_pCheck_DreamBlur, m_pCheck_ScreenBlur
// m_pSlider_CinematicBars_Size, m_pSlider_MotionBlur_Strength, m_pSlider_BloomFlare_Strength, m_pSlider_ExplosionBlur_Strength,
//		m_pSlider_Desaturation_Strength, m_pSlider_FilmGrain_Strength, m_pSlider_Bend_Strength, m_pSlider_Chromatic_Strength
static Preset_t presets[] =
{
	// Subtle
	{ true, true, true, true, true, true, false, 0.0f, 0.3f, 0.7f, 0.3f, 0.0f, 0.1f, 0.2f, 0.2f },
	// Vibrant
	{ true, true, true, true, true, true, true, 0.0f, 0.7f, 1.0f, 0.5f, 0.0f, 0.2f, 0.4f, 0.8f },
	// film noir
	{ true, true, true, true, true, true, true, 0.0f, 0.8f, 1.0f, 0.5f, 0.5f, 0.3f, 0.0f, 0.9f },
	// 70 mm
	{ true, true, true, true, true, true, true, 1.0f, 0.2f, 0.8f, 0.1f, 0.1f, 0.2f, 0.7f, 0.6f },
	// bw
	{ true, true, true, true, true, true, true, 0.0f, 0.3f, 0.7f, 0.2f, 1.0f, 0.2f, 0.0f, 0.4f },
	// none
	{ false, false, false, false, false, false, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
};

static float scales[] = {
	5.0f,
	1.0f,
	0.2f,
	1.0f,
	1.0f,
	5.0f,
	1.0f,
	100.0f
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
	m_pCBox_Preset = new ComboBox( pPagePostProcessing, "combo_preset", 10, false );
	m_pCBox_Preset->AddItem( "#pp_preset_subtle", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_vibrant", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_filmnoir", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_70mm", NULL );
	m_pCBox_Preset->AddItem( "#pp_preset_bw", NULL );
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

	m_pHUDColorPicker = new CColorPickerButton( pPageGame, "hud_color_picker_button", this );

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
#define CVAR_CHECK_INTEGER( x, y ) ( x.SetValue( ( y->IsSelected() ? 1 : int(0) ) ) )
#define CVAR_SLIDER_FLOAT( x, y, ratio ) ( x.SetValue( (float)(y->GetValue()/(float)ratio ) ) )

		CVAR_CHECK_INTEGER( cvar_gstring_drawhurtfx, m_pCheck_HurtFX );
		CVAR_CHECK_INTEGER( cvar_gstring_drawvignette, m_pCheck_Vignette );
		CVAR_CHECK_INTEGER( cvar_gstring_drawgodrays, m_pCheck_GodRays );
		CVAR_CHECK_INTEGER( cvar_gstring_drawscreenblur, m_pCheck_ScreenBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawdreamblur, m_pCheck_DreamBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawlensflare, m_pCheck_LensFlare );
		CVAR_CHECK_INTEGER( cvar_gstring_drawwatereffects, m_pCheck_WaterEffects );
		CVAR_CHECK_INTEGER( gstring_firstpersonbody_enable, m_pCheck_FirstPersonBody );
		//CVAR_CHECK_INTEGER( gstring_firstpersonbody_shadow_enable, m_pCheck_FirstPersonShadow );
		CVAR_CHECK_INTEGER( gstring_volumetrics_enabled, m_pCheck_LightVolumetrics );

		CVAR_SLIDER_FLOAT( cvar_gstring_explosionfx_strength, m_pSlider_ExplosionBlur_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_bars_scale, m_pSlider_CinematicBars_Size, 50 );
		CVAR_SLIDER_FLOAT( cvar_gstring_motionblur_scale, m_pSlider_MotionBlur_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_bloomflare_strength, m_pSlider_BloomFlare_Strength, 2 );
		CVAR_SLIDER_FLOAT( cvar_gstring_desaturation_strength, m_pSlider_Desaturation_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_filmgrain_strength, m_pSlider_FilmGrain_Strength, 50 );
		CVAR_SLIDER_FLOAT( cvar_gstring_bend_strength, m_pSlider_Bend_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_chromatic_aberration, m_pSlider_Chromatic_Strength, 1000 );

		//cvar_gstring_drawbloomflare.SetValue( m_pCBox_BloomFlare->GetActiveItem() );
		gstring_hud_color.SetValue( VarArgs( "%i %i %i 255", m_colHUD.r(), m_colHUD.g(), m_colHUD.b() ) );

		engine->ClientCmd( "host_writeconfig" );
		engine->ClientCmd( "hud_reloadscheme" );

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

		cvar_gstring_explosionfx_strength.Revert();
		cvar_gstring_bars_scale.Revert();
		cvar_gstring_motionblur_scale.Revert();
		cvar_gstring_bloomflare_strength.Revert();
		cvar_gstring_desaturation_strength.Revert();
		cvar_gstring_filmgrain_strength.Revert();
		cvar_gstring_bend_strength.Revert();
		cvar_gstring_chromatic_aberration.Revert();

		gstring_hud_color.Revert();

		ReadValues( true );
		UpdateLabels();
	}
	else
		BaseClass::OnCommand( cmd );
}

void CVGUIGstringOptions::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	ReadValues( true );
	UpdateLabels();
}

void CVGUIGstringOptions::ReadValues( bool bUpdatePreset )
{
#define CVAR_CHECK_SELECTED( x, y ) ( y->SetSelected( x.GetBool(), false ) )
#define CVAR_SLIDER_INTEGER( x, y, ratio ) ( y->SetValue( x.GetFloat() * ratio, false ) )

	CVAR_CHECK_SELECTED( cvar_gstring_drawhurtfx, m_pCheck_HurtFX );
	CVAR_CHECK_SELECTED( cvar_gstring_drawvignette, m_pCheck_Vignette );
	CVAR_CHECK_SELECTED( cvar_gstring_drawgodrays, m_pCheck_GodRays );
	CVAR_CHECK_SELECTED( cvar_gstring_drawscreenblur, m_pCheck_ScreenBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawdreamblur, m_pCheck_DreamBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawlensflare, m_pCheck_LensFlare );
	CVAR_CHECK_SELECTED( cvar_gstring_drawwatereffects, m_pCheck_WaterEffects );
	CVAR_CHECK_SELECTED( gstring_firstpersonbody_enable, m_pCheck_FirstPersonBody );
	//m_pCheck_FirstPersonShadow->SetEnabled( gstring_firstpersonbody_enable.GetBool() );
	//CVAR_CHECK_SELECTED( gstring_firstpersonbody_shadow_enable, m_pCheck_FirstPersonShadow );
	CVAR_CHECK_SELECTED( gstring_volumetrics_enabled, m_pCheck_LightVolumetrics );

	//m_pCBox_BloomFlare->ActivateItem( clamp( cvar_gstring_drawbloomflare.GetInt(),
	//	0, m_pCBox_BloomFlare->GetItemCount() ) );

	CVAR_SLIDER_INTEGER( cvar_gstring_explosionfx_strength, m_pSlider_ExplosionBlur_Strength, 11 );
	CVAR_SLIDER_INTEGER( cvar_gstring_bars_scale, m_pSlider_CinematicBars_Size, 51 );
	CVAR_SLIDER_INTEGER( cvar_gstring_motionblur_scale, m_pSlider_MotionBlur_Strength, 11 );
	CVAR_SLIDER_INTEGER( cvar_gstring_bloomflare_strength, m_pSlider_BloomFlare_Strength, 2 );
	CVAR_SLIDER_INTEGER( cvar_gstring_desaturation_strength, m_pSlider_Desaturation_Strength, 10 );
	CVAR_SLIDER_INTEGER( cvar_gstring_filmgrain_strength, m_pSlider_FilmGrain_Strength, 51 );
	CVAR_SLIDER_INTEGER( cvar_gstring_bend_strength, m_pSlider_Bend_Strength, 11 );
	CVAR_SLIDER_INTEGER( cvar_gstring_chromatic_aberration, m_pSlider_Chromatic_Strength, 1100 );

	UTIL_StringToColor( m_colHUD, gstring_hud_color.GetString() );
	m_pHUDColorPicker->SetColor( m_colHUD );

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
	OnPresetModified();
	//if ( panel == m_pCheck_FirstPersonBody )
	//{
	//	m_pCheck_FirstPersonShadow->SetEnabled( m_pCheck_FirstPersonBody->IsSelected() );
	//}
}

void CVGUIGstringOptions::UpdateLabels()
{
	m_pLabel_Value_CinematicBars->SetText( VarArgs( "%.1f", m_pSlider_CinematicBars_Size->GetValue() / 10.0f ) );
	m_pLabel_Value_MotionBlur->SetText( VarArgs( "%.1f", m_pSlider_MotionBlur_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_BloomFlare->SetText( VarArgs( "%.1f", m_pSlider_BloomFlare_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_ExplosionBlur->SetText( VarArgs( "%.1f", m_pSlider_ExplosionBlur_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_Desaturation->SetText( VarArgs( "%.1f", m_pSlider_Desaturation_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_FilmGrain->SetText( VarArgs( "%.1f", m_pSlider_FilmGrain_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_Bend->SetText( VarArgs( "%.1f", m_pSlider_Bend_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_Chromatic->SetText( VarArgs( "%.1f", m_pSlider_Chromatic_Strength->GetValue() / 10.0f ) );
}

void CVGUIGstringOptions::OnSliderMoved( KeyValues *pKV )
{
	OnPresetModified();
	UpdateLabels();
}

void CVGUIGstringOptions::OnTextChanged( KeyValues *pKV )
{
	ApplyPreset( m_pCBox_Preset->GetActiveItem() );
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

	ConVar *pVarChecks[] =
	{
		&cvar_gstring_drawhurtfx,
		&cvar_gstring_drawgodrays,
		&cvar_gstring_drawwatereffects,
		&cvar_gstring_drawvignette,
		&cvar_gstring_drawlensflare,
		&cvar_gstring_drawdreamblur,
		&cvar_gstring_drawscreenblur
	};

	ConVar *pVarValues[] =
	{
		&cvar_gstring_bars_scale,
		&cvar_gstring_motionblur_scale,
		&cvar_gstring_bloomflare_strength,
		&cvar_gstring_explosionfx_strength,
		&cvar_gstring_desaturation_strength,
		&cvar_gstring_filmgrain_strength,
		&cvar_gstring_bend_strength,
		&cvar_gstring_chromatic_aberration
	};

	const Preset_t &p = presets[ index ];
	for ( int c = 0; c < ARRAYSIZE( pVarChecks ); ++c )
	{
		pVarChecks[ c ]->SetValue( p.checks[ c ] ? 1 : int( 0 ) );
	}
	for ( int v = 0; v < ARRAYSIZE( pVarValues ); ++v )
	{
		pVarValues[ v ]->SetValue( p.val[ v ] / scales[ v ] );
	}

	ReadValues( false );
	UpdateLabels();
}

int CVGUIGstringOptions::FindCurrentPreset()
{
	ConVar *pVarChecks[] =
	{
		&cvar_gstring_drawhurtfx,
		&cvar_gstring_drawgodrays,
		&cvar_gstring_drawwatereffects,
		&cvar_gstring_drawvignette,
		&cvar_gstring_drawlensflare,
		&cvar_gstring_drawdreamblur,
		&cvar_gstring_drawscreenblur
	};

	ConVar *pVarValues[] =
	{
		&cvar_gstring_bars_scale,
		&cvar_gstring_motionblur_scale,
		&cvar_gstring_bloomflare_strength,
		&cvar_gstring_explosionfx_strength,
		&cvar_gstring_desaturation_strength,
		&cvar_gstring_filmgrain_strength,
		&cvar_gstring_bend_strength,
		&cvar_gstring_chromatic_aberration
	};

	for ( int i = 0; i < ARRAYSIZE( presets ); ++i )
	{
		const Preset_t &p = presets[ i ];
		bool bWrong = false;
		for ( int c = 0; c < ARRAYSIZE( pVarChecks ); ++c )
		{
			if ( pVarChecks[ c ]->GetBool() != p.checks[ c ] )
			{
				bWrong = true;
			}
		}
		for ( int v = 0; v < ARRAYSIZE( pVarValues ); ++v )
		{
			if ( !CloseEnough( pVarValues[ v ]->GetFloat() * scales[ v ], p.val[ v ] ) )
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

CON_COMMAND( vgui_showGstringOptions, "" )
{
	vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	new CVGUIGstringOptions( GameUIRoot, "GstringOptions" );
}
