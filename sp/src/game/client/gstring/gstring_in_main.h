#ifndef GSTRING_IN_MAIN_H
#define GSTRING_IN_MAIN_H

#include "input.h"
#include "gstring/gstring_player_shared_forward.h"

class CGstringInput : public CInput
{
	typedef CInput BaseClass;
public:
	CGstringInput();
	virtual ~CGstringInput();

	void GetCrosshairPosition( int &x, int &y, float &angle );

protected:
	virtual void ClampAngles( QAngle &viewangles );
	virtual void MouseMove( CUserCmd *cmd );

private:
	Vector2D m_MousePosition;
	bool m_bIsUsingCustomCrosshair;
};

extern CGstringInput *GetGstringInput();

#endif