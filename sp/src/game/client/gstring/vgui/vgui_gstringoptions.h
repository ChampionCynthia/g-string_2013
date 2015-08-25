#ifndef VGUI_GSTRING_OPTIONS_H
#define VGUI_GSTRING_OPTIONS_H


#include "cbase.h"
#include "vgui_controls/Controls.h"

#include "vgui_controls/Frame.h"
#include "vgui_controls/PropertySheet.h"

class CColorPickerButton;
class CVGUIGstringOptions : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE( CVGUIGstringOptions, vgui::Frame );

	CVGUIGstringOptions( vgui::VPANEL parent, const char *pName );
	~CVGUIGstringOptions();

	void OnCommand( const char *cmd );

	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", pKV );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", pKV );
	MESSAGE_FUNC_PARAMS( OnPicked, "ColorPickerPicked", pKV );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void PerformLayout();

private:
	void ReadValues( bool bUpdatePreset );
	void UpdateLabels();
	void ApplyPreset( int index );
	int FindCurrentPreset();
	void OnPresetModified();

	vgui::PropertySheet		*m_pPropertySheet;

	// Post-processing
	vgui::ComboBox		*m_pCBox_Preset;

	vgui::CheckButton	*m_pCheck_HurtFX;
	vgui::CheckButton	*m_pCheck_GodRays;
	vgui::CheckButton	*m_pCheck_WaterEffects;
	vgui::CheckButton	*m_pCheck_Vignette;
	vgui::CheckButton	*m_pCheck_LensFlare;
	vgui::CheckButton	*m_pCheck_DreamBlur;
	vgui::CheckButton	*m_pCheck_ScreenBlur;

	vgui::Slider		*m_pSlider_CinematicBars_Size;
	vgui::Slider		*m_pSlider_MotionBlur_Strength;
	vgui::Slider		*m_pSlider_BloomFlare_Strength;
	vgui::Slider		*m_pSlider_ExplosionBlur_Strength;
	vgui::Slider		*m_pSlider_Desaturation_Strength;
	vgui::Slider		*m_pSlider_FilmGrain_Strength;
	vgui::Slider		*m_pSlider_Bend_Strength;
	vgui::Slider		*m_pSlider_Chromatic_Strength;

	vgui::Label			*m_pLabel_Value_CinematicBars;
	vgui::Label			*m_pLabel_Value_MotionBlur;
	vgui::Label			*m_pLabel_Value_BloomFlare;
	vgui::Label			*m_pLabel_Value_ExplosionBlur;
	vgui::Label			*m_pLabel_Value_Desaturation;
	vgui::Label			*m_pLabel_Value_FilmGrain;
	vgui::Label			*m_pLabel_Value_Bend;
	vgui::Label			*m_pLabel_Value_Chromatic;

	CColorPickerButton	*m_pHUDColorPicker;
	Color m_colHUD;

	// Game
	vgui::CheckButton	*m_pCheck_FirstPersonBody;
	//vgui::CheckButton	*m_pCheck_FirstPersonShadow;
	vgui::CheckButton	*m_pCheck_LightVolumetrics;
};


#endif
