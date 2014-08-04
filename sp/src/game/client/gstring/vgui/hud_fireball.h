#ifndef HUD_FIREBALL_H
#define HUD_FIREBALL_H

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"

class CHudFireball : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudFireball, vgui::Panel );

public:
	CHudFireball( const char *pElementName );
	~CHudFireball();

	void MsgFunc_Fireball( bf_read &msg );

	virtual void Init();
	virtual void Reset();

	//virtual void OnSizeChanged( int newWide, int newTall );
	//virtual bool ShouldDraw();

	//virtual CHud::HUDRENDERSTAGE_t GetRenderStage()
	//{
	//	return CHud::HUDRENDERSTAGE_DEFAULT_HUD;
	//};

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();
	virtual void Paint();

private:
	void PaintLabel();
	void PaintBar();

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );

	CPanelAnimationVarAliasType( float, m_flTextXPos, "text_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flTextYPos, "text_ypos", "20", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBarX, "bar_x", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarY, "bar_y", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarHeight, "bar_height", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarWidthBase, "bar_width_base", "96", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarWidthElement, "bar_width_element", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarAngle, "bar_angle", "0", "proportional_float" );
	CPanelAnimationVar( Color, m_BarBorderColor, "bar_border_color", "133 133 133 255" );

	wchar_t m_wszLabelText[ 32 ];
	int m_iTexBar;
	Vector2D m_vecBarAngleDirection;
	bool m_bShowingBar;
	int m_iFireballElementCount;
	float m_flBarAnimationFraction;
	float m_flBarAnimationSpeed;
};

#endif
