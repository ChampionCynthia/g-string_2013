#include "cbase.h"
#include "kbutton.h"
#include "gstring_in_main.h"
#include "c_gstring_player.h"
#include "gstring/cspacecraft.h"
#include "iviewrender.h"
#include "viewrender.h"
#include "view.h"
#include "view_scene.h"
#include "in_buttons.h"

#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar gstring_spacecraft_mouse_roll_speed( "gstring_spacecraft_mouse_roll_speed", "100.0" );
static ConVar gstring_spacecraft_mouse_drift_speed( "gstring_spacecraft_mouse_drift_speed", "0.15" );
static ConVar gstring_spacecraft_move_roll_speed( "gstring_spacecraft_move_roll_speed", "100.0" );
static ConVar gstring_spacecraft_autoaim_maxscreendist( "gstring_spacecraft_autoaim_maxscreendist", "30" );
static ConVar gstring_spacecraft_autoaim_maxworlddist( "gstring_spacecraft_autoaim_maxworlddist", "4096" );
static ConVar gstring_spacecraft_mouse_mode( "gstring_spacecraft_mouse_mode", "0", FCVAR_ARCHIVE );

extern kbutton_t in_attack;
extern kbutton_t in_duck;
extern kbutton_t in_alt1;
extern kbutton_t in_use;
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
	, m_flAutoAimUpdateTick( 0.0f )
	, m_flLockFraction( 0.0f )
{
}

CGstringInput::~CGstringInput()
{
}

bool CGstringInput::IsUsingDefaultCrosshair() const
{
	return GetCrosshairMode() == CROSSHAIRMODE_DEFAULT;
}

bool CGstringInput::IsUsingGamepadCrosshair() const
{
	return GetCrosshairMode() == CROSSHAIRMODE_GAMEPAD;
}

bool CGstringInput::IsUsingFreeCrosshair() const
{
	return GetCrosshairMode() == CROSSHAIRMODE_FREE;
}

void CGstringInput::GetNormalizedMousePosition( Vector2D &vecMousePosition ) const
{
	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );
	const float halfVw = vw * 0.5f;
	const float halfVh = vh * 0.5f;
	vecMousePosition.Init( ( m_MousePosition.x - halfVw ) / halfVw, ( m_MousePosition.y - halfVh ) / -halfVh );
}

void CGstringInput::GetCrosshairPosition( int &x, int &y, float &angle ) const
{
	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

	x = m_MousePosition.x;
	y = m_MousePosition.y;

	const float flMaxCrosshairAngle = 45.0f;
	angle = RemapValClamped( y, 0, vh, -flMaxCrosshairAngle, flMaxCrosshairAngle );
	angle *= RemapValClamped( x, 0, vw, -1.0f, 1.0f );
}

const CUtlVector< EHANDLE > &CGstringInput::GetPotentialAutoAimTargets() const
{
	return m_PotentialAutoAimTargets;
}

CBaseEntity *CGstringInput::GetAutoAimTarget() const
{
	return m_AutoAimTarget.Get();
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
	if ( pPlayer && pPlayer->IsInInteraction() )
	{
		InteractionMouseMove( cmd );
		return;
	}

	if ( pPlayer == NULL || !pPlayer->IsInSpacecraft() )
	{
		BaseClass::MouseMove( cmd );
		return;
	}

	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

	const CrosshairMode_e crosshairMode = GetCrosshairMode();
	if ( crosshairMode != CROSSHAIRMODE_FREE )
	{
		QAngle viewangles;
		engine->GetViewAngles( viewangles );
		viewangles.z = 0.0f;
		engine->SetViewAngles( viewangles );

		in_duck.state = 0;
		in_use.state = 0;
		in_alt1.state = 0;
	}

	if ( crosshairMode == CROSSHAIRMODE_DEFAULT )
	{
		m_MousePosition.x = vw / 2.0f;
		m_MousePosition.y = vh / 2.0f;

		PerformSpacecraftAutoAim( cmd );
		BaseClass::MouseMove( cmd );
		return;
	}

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
	const QAngle originalViewangles( viewangles );

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

		if ( crosshairMode == CROSSHAIRMODE_GAMEPAD )
		{
			float mouse_x, mouse_y;
			// Filter, etc. the delta values and place into mouse_x and mouse_y
			GetMouseDelta( mx, my, &mouse_x, &mouse_y );

			// Apply scaling factor
			ScaleMouse( &mouse_x, &mouse_y );

			m_MousePosition.x += mouse_x * 0.2f;
			m_MousePosition.y += mouse_y * 0.2f;

			const Vector2D vecCenter( vw / 2.0f, vh / 2.0f );
			Vector2D vecDelta( vecCenter - m_MousePosition );

			const float flDistance = vecDelta.Length();
			const float flMinDistance = ScreenHeight() * 0.05f;
			const float flMaxDistance = ScreenHeight() * 0.125f;
			if ( flDistance > flMinDistance )
			{
				Vector2D vecOverSteer( vec2_origin );
				if ( flDistance > flMaxDistance )
				{
					const Vector2D vecDirection( ( vecDelta / flDistance ) * ( flDistance - flMaxDistance ) );
					m_MousePosition += vecDirection;
					vecOverSteer -= vecDirection * 2.5f;
				}

				vecDelta *= RemapValClamped( flDistance, flMinDistance, flMaxDistance, 0.0f, -5.0f ) * gpGlobals->frametime;
				ApplyMouse( viewangles, cmd, vecDelta.x + vecOverSteer.x, vecDelta.y + vecOverSteer.y );
			}
		}
		else
		{
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

			const float flRollSpeed = gstring_spacecraft_move_roll_speed.GetFloat();
			const bool bRollLeft = ( in_alt1.state & 3 ) != 0;
			const bool bRollRight = ( in_use.state & 3 ) != 0;
			if ( bRollLeft || bRollRight )
			{
				if ( bRollLeft )
				{
					viewangles.z -= flRollSpeed * gpGlobals->frametime;
				}

				if ( bRollRight )
				{
					viewangles.z += flRollSpeed * gpGlobals->frametime;
				}
			}
			else if ( ( in_duck.state & 3 ) != 0 )
			{
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
		}

		// Re-center the mouse.
		ResetMouse();
	}
	else
	{
		m_bIsUsingCustomCrosshair = false;
	}

	m_MousePosition.x = clamp( m_MousePosition.x, 0, vw );
	m_MousePosition.y = clamp( m_MousePosition.y, 0, vh );

	PerformSpacecraftAutoAim( cmd );

	engine->SetViewAngles( viewangles );
}

CGstringInput::CrosshairMode_e CGstringInput::GetCrosshairMode() const
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();
	const bool bInSpaceShip = pPlayer != NULL && pPlayer->IsInSpacecraft();
	return bInSpaceShip ? (CrosshairMode_e)gstring_spacecraft_mouse_mode.GetInt() : CROSSHAIRMODE_DEFAULT;
}

void CGstringInput::InteractionMouseMove( CUserCmd *cmd )
{
	float	mouse_x, mouse_y;
	float	mx, my;

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
		GetMouseDelta( mx, my, &mouse_x, &mouse_y );

		// Apply scaling factor
		ScaleMouse( &mouse_x, &mouse_y );

		// Re-center the mouse.
		ResetMouse();

		C_GstringPlayer *pPlayer = LocalGstringPlayer();

		Assert( pPlayer );

		pPlayer->PerformInteractionMouseMove( mouse_x * 0.022f, mouse_y * 0.022f );
	}
}

void CGstringInput::PerformSpacecraftAutoAim( CUserCmd *cmd )
{
	C_GstringPlayer *pPlayer = LocalGstringPlayer();
	Assert( pPlayer != NULL );

	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

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
	CSpacecraft *pSpacecraft = pPlayer->GetSpacecraft();
	CTraceFilterSkipTwoEntities filter( pPlayer, pSpacecraft, COLLISION_GROUP_NONE );

	const Vector vecEnd = vecViewOrigin + vecPickingRay * MAX_TRACE_LENGTH;
	const Vector vecHull( 2, 2, 2 );
	UTIL_TraceHull( vecViewOrigin, vecEnd, -vecHull, vecHull, MASK_SOLID, &filter, &tr );

	cmd->worldShootPosition = tr.endpos;
	C_BaseEntity *pAutoAimTarget = NULL;

	if ( m_flAutoAimUpdateTick < 0.0f )
	{
		m_flAutoAimUpdateTick = 0.05f;
		m_PotentialAutoAimTargets.RemoveAll();

		const float flMaxWorldDistanceSqr = gstring_spacecraft_autoaim_maxworlddist.GetFloat() *
			gstring_spacecraft_autoaim_maxworlddist.GetFloat();
		float flBestScreenDistSqr = gstring_spacecraft_autoaim_maxscreendist.GetFloat() *
			gstring_spacecraft_autoaim_maxscreendist.GetFloat() * ( vh / 640.0f );
		for ( C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
		{
			if ( !pEnt || !pEnt->IsVisible() ||
				pEnt->IsPlayer() || pEnt->GetOwnerEntity() == pPlayer )
				continue;

			CSpacecraft *pSpacecraft = dynamic_cast< CSpacecraft* >( pEnt );
			if ( pSpacecraft != NULL )
			{
				const Vector &vecCenter = pSpacecraft->WorldSpaceCenter();
				const float flWorldDistanceSqr = ( vecCenter - vecViewOrigin ).LengthSqr();

				if ( flWorldDistanceSqr > flMaxWorldDistanceSqr )
				{
					continue;
				}

				m_PotentialAutoAimTargets.AddToTail( pSpacecraft );

				Vector vecScreen( vec3_origin );
				if ( !ScreenTransform( vecCenter, vecScreen ) )
				{
					vecScreen = vecScreen * Vector( 0.5f, -0.5f, 0 ) + Vector( 0.5f, 0.5f, 0 );
					vecScreen.x *= vw;
					vecScreen.y *= vh;
				}

				const float flScreenDistSqr = ( Vector2D( vecScreen.x, vecScreen.y ) - m_MousePosition ).LengthSqr();
				if ( flScreenDistSqr > flBestScreenDistSqr )
				{
					continue;
				}

				pAutoAimTarget = pSpacecraft;
				flBestScreenDistSqr = flScreenDistSqr;
			}
		}

		m_AutoAimTarget.Set( pAutoAimTarget );
	}
	else
	{
		m_flAutoAimUpdateTick -= gpGlobals->frametime;
	}

	if ( pAutoAimTarget == NULL )
	{
		pAutoAimTarget = m_AutoAimTarget.Get();
	}

	if ( pAutoAimTarget != NULL )
	{
		cmd->autoAimTarget = pAutoAimTarget->entindex();
	}
	else
	{
		cmd->autoAimTarget = 0;
	}
}
