
#include "cbase.h"
#include "gstring/vgui/vgui_gstringmain.h"
#include "vgui_controls/Label.h"


CVGUIMenuLabel::CVGUIMenuLabel( vgui::Panel *parent, const char *pszText ) : BaseClass( parent, "", pszText )
{
}


void CVGUIMenuLabel::Paint()
{
	Paint3DAdvanceDepth();

	BaseClass::Paint();
}
