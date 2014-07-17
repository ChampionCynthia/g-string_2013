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

extern kbutton_t in_attack;
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
	, m_flAutoAimUpdateTick( 0.0f )
	, m_flLockFraction( 0.0f )
{
}

CGstringInput::~CGstringInput()
{
}

void CGstringInput::GetNormalizedMousePosition( Vector2D &vecMousePosition )
{
	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );
	const float halfVw = vw * 0.5f;
	const float halfVh = vh * 0.5f;
	vecMousePosition.Init( ( m_MousePosition.x - halfVw ) / halfVw, ( m_MousePosition.y - halfVh ) / -halfVh );
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

		// Filter, etc. the delta values and place into mouse_x and mouse_y
		//GetMouseDelta( mx, my, &mouse_x, &mouse_y );

		// Apply scaling factor
		//ScaleMouse( &mouse_x, &mouse_y );

		// controls
		m_MousePosition.x += mx;
		m_MousePosition.y += my;

		//Vector2D vecMouseCenter( vw * 0.5f, vh * 0.5f );
		//Vector2D vecDelta = m_MousePosition - vecMouseCenter;
		//if ( vecDelta.LengthSqr() > Square( vh * 0.5f ) )
		//{
		//	vecDelta.NormalizeInPlace();
		//	vecDelta *= vh * 0.5f;
		//	m_MousePosition = vecMouseCenter + vecDelta;
		//}

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
	const Vector vecHull( 10, 10, 10 );
	UTIL_TraceHull( vecViewOrigin, vecEnd, -vecHull, vecHull, MASK_SOLID, &filter, &tr );
	//UTIL_TraceLine( vecViewOrigin, vecEnd, MASK_SOLID, &filter, &tr );

	cmd->worldShootPosition = tr.endpos;

	// gstring_spacecraft_autoaim_maxscreendist

	C_BaseEntity *pAutoAimTarget = NULL;

	if ( m_flAutoAimUpdateTick < 0.0f )
	{
		m_flAutoAimUpdateTick = 0.05f;
		m_PotentialAutoAimTargets.RemoveAll();

		const float flMaxWorldDistanceSqr = gstring_spacecraft_autoaim_maxworlddist.GetFloat() *
			gstring_spacecraft_autoaim_maxworlddist.GetFloat();
		float flBestWorldDistanceSqr = flMaxWorldDistanceSqr;
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

				if ( flWorldDistanceSqr > flBestWorldDistanceSqr )
				{
					continue;
				}

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
				flBestWorldDistanceSqr = flWorldDistanceSqr;
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

	bool bShouldLock = pAutoAimTarget != NULL && ( in_attack.state & 3 ) != 0;
	const float flBorderRange = 10.0f * ( vh / 480.0f );
	const float flDistanceToBorderX = ( vw * 0.5f ) - abs( m_MousePosition.x - vw * 0.5f );
	const float flDistanceToBorderY = ( vh * 0.5f ) - abs( m_MousePosition.y - vh * 0.5f );
	if ( MIN( flDistanceToBorderX, flDistanceToBorderY ) < flBorderRange )
	{
		bShouldLock = false;
	}
	//m_flLockFraction = Approach( bShouldLock ? 1.0f : 0.0f, m_flLockFraction, gpGlobals->frametime * 2.0f );

	//if ( m_flLockFraction < 1.0f )
	{
		//if ( m_flLockFraction > 0.0f )
		//{
		//	Quaternion qViewangles, qOriginalViewangles, qTemp;
		//	AngleQuaternion( originalViewangles, qOriginalViewangles );
		//	AngleQuaternion( viewangles, qViewangles );
		//	QuaternionSlerp( qViewangles, qOriginalViewangles, m_flLockFraction, qTemp );
		//	QuaternionAngles( qTemp, viewangles );
		//}

		// Store out the new viewangles.
		engine->SetViewAngles( viewangles );
	}
}
