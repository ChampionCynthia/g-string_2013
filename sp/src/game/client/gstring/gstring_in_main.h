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

	const CUtlVector< EHANDLE > &GetPotentialAutoAimTargets() const;
	CBaseEntity *GetAutoAimTarget() const;

protected:
	virtual void ClampAngles( QAngle &viewangles );
	virtual void MouseMove( CUserCmd *cmd );

private:
	Vector2D m_MousePosition;
	bool m_bIsUsingCustomCrosshair;

	float m_flAutoAimUpdateTick;
	float m_flLockFraction;
	CUtlVector< EHANDLE > m_PotentialAutoAimTargets;
	EHANDLE m_AutoAimTarget;
};

extern CGstringInput *GetGstringInput();

#endif