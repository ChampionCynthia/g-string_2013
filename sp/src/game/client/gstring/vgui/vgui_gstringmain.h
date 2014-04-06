#ifndef VGUI_GSTRING_MAIN_H
#define VGUI_GSTRING_MAIN_H


#include "cbase.h"
#include "vgui_controls/Controls.h"

#include "vgui_controls/Frame.h"

#include "gstring/vgui/vgui_menulabel.h"
#include "gstring/vgui/vgui_menubutton.h"
#include "gstring/vgui/vgui_menulayer.h"
#include "gstring/vgui/vgui_menuembedded.h"
#include "gstring/vgui/vutil.h"
#include "gstring/vgui/vparticlecontainer.h"

#include "gstring/cframetimehelper.h"

class CVGUIGstringMain : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CVGUIGstringMain, vgui::Panel );

	~CVGUIGstringMain();

	static void Create();

	void Paint3DDepthReset();
	void Paint3DDepthAdvance();

	void ReloadLayout();

protected:
	CVGUIGstringMain( vgui::VPANEL parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();

	virtual void OnThink();
	virtual void Paint();

private:

	void InitializeLayout();
	void UpdateLayoutVisibility();

	void LoadBackground00( Vector &vOrigin, QAngle &vAng, float &vFOV );

	CUtlVector< CVGUIMenuLayer* > m_hLayer_Top;
	CUtlVector< CVGUIMenuLayer* > m_hLayer_Background;

	//CVGUIMenu3DWrapper *m_pWrappedMenu;
	CVGUIMenuEmbedded *m_pWrappedMenu;

	float m_flNextDepth;

	int m_ilastSize;
};

extern CVGUIGstringMain *GetGstringMain();


// Wondering what this is? ISurface::draw stuff always renders depth in 2006, which has been fixed in newer branches.
// This hack avoids zfighting.
inline void Paint3DAdvanceDepth()
{
	GetGstringMain()->Paint3DDepthAdvance();
	//materials->ClearBuffers( false, true );
}

#endif
