#ifndef GSTRING_CVARS_H
#define GSTRING_CVARS_H

#ifdef GAME_DLL
extern ConVar cvar_gstring_enable_entities;
#else
extern ConVar cvar_gstring_enable_postprocessing;
extern ConVar cvar_gstring_enable_hud;

extern ConVar cvar_gstring_drawdetailpropsfirst;
//extern ConVar cvar_gstring_drawbars;
//extern ConVar cvar_gstring_drawfilmgrain;
extern ConVar cvar_gstring_drawvignette;
extern ConVar cvar_gstring_drawgodrays;
//extern ConVar cvar_gstring_drawexplosionblur;
extern ConVar cvar_gstring_drawmotionblur;
extern ConVar cvar_gstring_drawscreenblur;
extern ConVar cvar_gstring_drawdreamblur;
//extern ConVar cvar_gstring_drawbloomflare;
extern ConVar cvar_gstring_drawlensflare;
extern ConVar cvar_gstring_drawwatereffects;
extern ConVar cvar_gstring_drawcinemaoverlay;
extern ConVar cvar_gstring_drawdof;

extern ConVar cvar_gstring_bars_scale;
extern ConVar cvar_gstring_explosionfx_strength;
extern ConVar cvar_gstring_motionblur_scale;
extern ConVar cvar_gstring_bloomflare_strength;
extern ConVar cvar_gstring_desaturation_strength;
extern ConVar cvar_gstring_filmgrain_strength;
extern ConVar cvar_gstring_bend_strength;
extern ConVar cvar_gstring_vignette_strength;
extern ConVar cvar_gstring_vignette_range_min;
extern ConVar cvar_gstring_vignette_range_max;

extern ConVar cvar_gstring_debug_vguiparticles;
extern ConVar cvar_gstring_nightvision_minlighting;

extern ConVar cvar_gstring_drawhurtfx;
extern ConVar cvar_gstring_chromatic_aberration;

extern ConVar gstring_spacecraft_firstperson;
extern ConVar gstring_hud_color;

extern ConVar gstring_dof_autofocus;
extern ConVar gstring_dof_focaldistance;
extern ConVar gstring_dof_focallength;
extern ConVar gstring_dof_aperture;
extern ConVar gstring_dof_smooth_speed;
extern ConVar gstring_dof_max_distance;
extern ConVar gstring_dof_radius;
extern ConVar gstring_dof_override;

#endif



#endif