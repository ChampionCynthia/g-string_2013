#include "cbase.h"
#include "kbutton.h"
#include "gstring_in_main.h"
#include "c_gstring_player.h"
#include "iviewrender.h"
#include "viewrender.h"
#include "view.h"
#include "in_buttons.h"

#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar gstring_spacecraft_mouse_roll_speed( "gstring_spacecraft_mouse_roll_speed", "100.0", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_mouse_drift_speed( "gstring_spacecraft_mouse_drift_speed", "0.1", FCVAR_REPLICATED );
static ConVar gstring_spacecraft_move_roll_speed( "gstring_spacecraft_move_roll_speed", "100.0", FCVAR_REPLICATED );

extern kbutton_t in_duck;
extern kbutton_t in_moveleft;
extern kbutton_t in_moveright;

static CGstringInput g_Input;
IInput *input = ( IInput* )&g_Input;

CGstringInput *GetGstringInput()
{
	return &g_Input;
}

CGstringInput::CGstringInput()
	: m_MousePosition( vec2_origin )
	, m_bIsUsingCustomCrosshair( false )
{
}

CGstringInput::~CGstringInput()
{
}

void CGstringInput::GetCrosshairPosition( int &x, int &y, float &angle )
{
	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

	x = m_MousePosition.x;
	y = m_MousePosition.y;

	const float flMaxCrosshairAngle = 45.0f;
	angle = RemapValClamped( y, 0, vh, -flMaxCrosshairAngle, flMaxCrosshairAngle );
	angle *= RemapValClamped( x, 0, vw, -1.0f, 1.0f );
}

void CGstringInput::ClampAngles( QAngle &viewangles )
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();

	if ( pPlayer == NULL || !pPlayer->IsInSpacecraft() )
	{
		BaseClass::ClampAngles( viewangles );
	}
}

void CGstringInput::MouseMove( CUserCmd *cmd )
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();

	if ( pPlayer == NULL || !pPlayer->IsInSpacecraft() )
	{
		m_bIsUsingCustomCrosshair = false;
		BaseClass::MouseMove( cmd );
		return;
	}

	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

	if ( !m_bIsUsingCustomCrosshair )
	{
		m_MousePosition.x = vw / 2.0f;
		m_MousePosition.y = vh / 2.0f;
		m_bIsUsingCustomCrosshair = true;
	}

	//float	mouse_x, mouse_y;
	float	mx, my;
	QAngle	viewangles;

	// Get view angles from engine
	engine->GetViewAngles( viewangles );

	// Validate mouse speed/acceleration settings
	CheckMouseAcclerationVars();

	// Don't drift pitch at all while mouselooking.
	view->StopPitchDrift();

	//jjb - this disables normal mouse control if the user is trying to 
	//      move the camera, or if the mouse cursor is visible 
	if ( !m_fCameraInterceptingMouse && 
		 !vgui::surface()->IsCursorVisible() )
	{
		// Sample mouse one more time
		AccumulateMouse();

		// Latch accumulated mouse movements and reset accumulators
		GetAccumulatedMouseDeltasAndResetAccumulators( &mx, &my );

		// Filter, etc. the delta values and place into mouse_x and mouse_y
		//GetMouseDelta( mx, my, &mouse_x, &mouse_y );

		// Apply scaling factor
		//ScaleMouse( &mouse_x, &mouse_y );

		// controls
		m_MousePosition.x += mx;
		m_MousePosition.y += my;

		const float flRollScale = gstring_spacecraft_mouse_roll_speed.GetFloat();
		const float flDriftScale = gstring_spacecraft_mouse_drift_speed.GetFloat();

		const float flHeightScale = 480.0f / vh;
		const float flWidthScale = 640.0f / vw;
		Vector2D vecRotationOffset( RemapValClamped( m_MousePosition.x, 0, vw, -vw, vw ) * flWidthScale,
			RemapValClamped( m_MousePosition.y, 0, vh, -vh, vh ) * flHeightScale );

		float flRoll = RemapValClamped( m_MousePosition.y, 0, vh, -1.0f, 1.0f );
		flRoll *= RemapValClamped( m_MousePosition.x, 0, vw, -1.0f, 1.0f );
		viewangles.z += flRoll * gpGlobals->frametime * flRollScale;

		if ( ( in_duck.state & 3 ) != 0 )
		{
			const float flRollSpeed = gstring_spacecraft_move_roll_speed.GetFloat();
			if ( ( in_moveleft.state & 3 ) != 0 )
			{
				viewangles.z -= flRollSpeed * gpGlobals->frametime;
			}

			if ( ( in_moveright.state & 3 ) != 0 )
			{
				viewangles.z += flRollSpeed * gpGlobals->frametime;
			}
		}

		Vector vecRight, vecUp;
		AngleVectors( viewangles, NULL, &vecRight, &vecUp );
		matrix3x4_t matRotation, matTransform, matTemp;
		AngleMatrix( viewangles, matTransform );
		MatrixBuildRotationAboutAxis( vecRight, vecRotationOffset.y * -gpGlobals->frametime * flDriftScale,
			matRotation );
		ConcatTransforms( matRotation, matTransform, matTemp );
		MatrixBuildRotationAboutAxis( vecUp, vecRotationOffset.x * -gpGlobals->frametime * flDriftScale,
			matRotation );
		ConcatTransforms( matRotation, matTemp, matTransform );

		MatrixAngles( matTransform, viewangles );

		// Re-center the mouse.
		ResetMouse();
	}
	else
	{
		m_bIsUsingCustomCrosshair = false;
	}

	m_MousePosition.x = clamp( m_MousePosition.x, 0, vw );
	m_MousePosition.y = clamp( m_MousePosition.y, 0, vh );

	// Store out the new viewangles.
	engine->SetViewAngles( viewangles );

	Vector vecPickingRay;
	const Vector vecViewOrigin( MainViewOrigin() );
	extern void ScreenToWorld( int mousex, int mousey, float fov,
					const Vector& vecRenderOrigin,
					const QAngle& vecRenderAngles,
					Vector& vecPickingRay );

	float ratio = engine->GetScreenAspectRatio();
	ratio = ( 1.0f / ratio ) * ( 4.0f / 3.0f );
	float flFov = ScaleFOVByWidthRatio( view->GetViewSetup()->fov, ratio );
	ScreenToWorld( m_MousePosition.x, m_MousePosition.y, flFov,
		vecViewOrigin, MainViewAngles(), vecPickingRay );

	trace_t tr;
	const float flMinimumTraceDistance = 128.0f;
	CSpacecraft *pSpacecraft = pPlayer->GetSpacecraft();
	CTraceFilterSkipTwoEntities filter( pPlayer, pSpacecraft, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecViewOrigin, vecViewOrigin + vecPickingRay * MAX_TRACE_LENGTH, MASK_SOLID, &filter, &tr );

	Vector vecSpacecraftOrigin = pSpacecraft->GetAbsOrigin();
	QAngle angSpacecraft = pSpacecraft->GetAbsAngles();
	Vector vecSpacecraftForward;
	AngleVectors( angSpacecraft, &vecSpacecraftForward );
	const float flSpacecraftDot = DotProduct( tr.endpos - vecSpacecraftOrigin, vecSpacecraftForward );
	if ( flSpacecraftDot < flMinimumTraceDistance )
	{
		tr.endpos += vecSpacecraftForward * ( flMinimumTraceDistance - flSpacecraftDot );
	}

	cmd->worldShootPosition = tr.endpos;
}
