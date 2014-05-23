#ifndef HUD_SPACE_AUTOAIM_H
#define HUD_SPACE_AUTOAIM_H

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/Panel.h"

class CHudSpaceAutoAim : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudSpaceAutoAim, vgui::Panel );

public:
	CHudSpaceAutoAim( const char *pElementName );
	~CHudSpaceAutoAim();

	void Init();
	void PostDLLInit();
	void LevelInit();
	void Reset();

	virtual bool ShouldDraw();

	virtual CHud::HUDRENDERSTAGE_t	GetRenderStage()
	{
		return CHud::HUDRENDERSTAGE_DEFAULT_HUD;
	};

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();
	virtual void Paint();
	virtual void PaintBackground() {}

private:
	float m_flTargetFraction[ MAX_EDICTS ];
	uint8 m_iTargetUpdateBits[ MAX_EDICTS ];
	uint8 m_iTargetParity;

	bool m_bWasPainting;
};

#endif
