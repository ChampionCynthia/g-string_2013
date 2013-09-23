#ifndef GSTRING_CVARS_H
#define GSTRING_CVARS_H

#ifdef GAME_DLL
extern ConVar cvar_gstring_enable_entities;
#else
extern ConVar cvar_gstring_enable_postprocessing;
extern ConVar cvar_gstring_enable_hud;

extern ConVar cvar_gstring_drawbars;
extern ConVar cvar_gstring_drawfilmgrain;
extern ConVar cvar_gstring_drawvignette;
extern ConVar cvar_gstring_drawgodrays;
extern ConVar cvar_gstring_drawexplosionblur;
extern ConVar cvar_gstring_drawmotionblur;
extern ConVar cvar_gstring_drawscreenblur;
extern ConVar cvar_gstring_drawdreamblur;
extern ConVar cvar_gstring_drawbloomflare;
extern ConVar cvar_gstring_drawlensflare;
extern ConVar cvar_gstring_drawwatereffects;

extern ConVar cvar_gstring_bars_scale;
extern ConVar cvar_gstring_explosionfx_strength;
extern ConVar cvar_gstring_motionblur_scale;
extern ConVar cvar_gstring_bloomflare_strength;
extern ConVar cvar_gstring_desaturation_strength;
extern ConVar cvar_gstring_filmgrain_strength;
extern ConVar cvar_gstring_vignette_strength;
extern ConVar cvar_gstring_vignette_range_min;
extern ConVar cvar_gstring_vignette_range_max;

extern ConVar cvar_gstring_debug_vguiparticles;
extern ConVar cvar_gstring_nightvision_minlighting;
#endif



#endif