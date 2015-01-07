#ifndef GSTRING_IN_MAIN_H
#define GSTRING_IN_MAIN_H

#include "input.h"
#include "gstring/gstring_player_shared_forward.h"

class IHoloTarget;

class CGstringInput : public CInput
{
	typedef CInput BaseClass;
public:
	CGstringInput();
	virtual ~CGstringInput();

	bool IsUsingDefaultCrosshair() const;
	bool IsUsingGamepadCrosshair() const;
	bool IsUsingFreeCrosshair() const;

	void GetNormalizedMousePosition( Vector2D &vecMousePosition ) const;
	void GetCrosshairPosition( int &x, int &y, float &angle ) const;

	//const CUtlVector< EHANDLE > &GetPotentialAutoAimTargets() const;
	const IHoloTarget *GetAutoAimTarget() const { return m_AutoAimTargetEntity.Get() != NULL ? m_AutoAimTarget : NULL; }
	const CBaseEntity *GetAutoAimTargetEntity() const { return m_AutoAimTargetEntity.Get(); }

protected:
	virtual void ClampAngles( QAngle &viewangles );
	virtual void MouseMove( CUserCmd *cmd );
	virtual void JoyStickMove( float frametime, CUserCmd *cmd );

private:
	enum CrosshairMode_e
	{
		CROSSHAIRMODE_DEFAULT = 0,
		CROSSHAIRMODE_GAMEPAD,
		CROSSHAIRMODE_FREE
	};

	CrosshairMode_e GetCrosshairMode() const;

	virtual void InteractionMouseMove( CUserCmd *cmd );
	virtual void PerformSpacecraftAutoAim( CUserCmd *cmd );

	Vector2D m_MousePosition;
	bool m_bIsUsingCustomCrosshair;

	float m_flAutoAimUpdateTick;
	float m_flLockFraction;
	//CUtlVector< EHANDLE > m_PotentialAutoAimTargets;
	EHANDLE m_AutoAimTargetEntity;
	const IHoloTarget *m_AutoAimTarget;
	bool m_bAutoAimTargetIsEnemy;
};

extern CGstringInput *GetGstringInput();

#endif