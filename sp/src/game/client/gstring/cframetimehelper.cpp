
#include "cbase.h"
#include "gstring/cframetimehelper.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISystem.h"
#include "sourcevr/isourcevirtualreality.h"
#include "ienginevgui.h"
#include "engine/IEngineSound.h"

static CFrameTimeHelper g_FrameTimeHelper;

CFrameTimeHelper::CFrameTimeHelper() : CAutoGameSystemPerFrame( "AutoFrameTimeAccess" )
{
	m_flFrameTime = 0.0f;
	m_flFrameTimeLast = 0.0f;
}

void CFrameTimeHelper::Update( float frametime )
{
	double curframetime = vgui::system()->GetCurrentTime();
	m_flFrameTime = min( 1.0f, curframetime - m_flFrameTimeLast );
	m_flFrameTimeLast = curframetime;

	extern int rainSound;
	const bool bIsPlayingRainSound = rainSound >= 0;
	if (bIsPlayingRainSound &&
		engine->IsConnected() &&
		!enginevgui->IsGameUIVisible())
	{
		if (enginesound->IsSoundStillPlaying(rainSound))
		{
			enginesound->StopSoundByGuid(rainSound);
		}
		rainSound = -1;
	}
	
	extern int scannerEngineSound;
	const bool bIsPlayingScannerEngineSound = scannerEngineSound >= 0;
	if (bIsPlayingScannerEngineSound &&
		engine->IsConnected() &&
		!enginevgui->IsGameUIVisible())
	{
		if (enginesound->IsSoundStillPlaying(scannerEngineSound))
		{
			enginesound->StopSoundByGuid(scannerEngineSound);
		}
		scannerEngineSound = -1;
	}
	
	extern int menuWindSound;
	const bool bIsPlayingMenuWindSound = menuWindSound >= 0;
	if (bIsPlayingMenuWindSound &&
		engine->IsConnected() &&
		!enginevgui->IsGameUIVisible())
	{
		if (enginesound->IsSoundStillPlaying(menuWindSound))
		{
			enginesound->StopSoundByGuid(menuWindSound);
		}
		menuWindSound = -1;
	}
}

void CFrameTimeHelper::LevelInitPostEntity()
{
	if ( UseVR() )
	{
		engine->ClientCmd( "vr_reset_home_pos" );
	}
}

double CFrameTimeHelper::GetFrameTime()
{
	return g_FrameTimeHelper.m_flFrameTime;
}

double CFrameTimeHelper::GetCurrentTime()
{
	return vgui::system()->GetCurrentTime();
}

void CFrameTimeHelper::RandomStart()
{
	int count = fmod( GetCurrentTime() * 10000.0, 1.0 ) * 10;
	for ( int i = 0; i < count; i++ )
		RandomInt( 0, 1 );
}
