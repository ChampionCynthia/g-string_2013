//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "basehlcombatweapon_shared.h"
#include "c_basehlcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

STUB_WEAPON_CLASS( cycler_weapon, WeaponCycler, C_BaseCombatWeapon );

STUB_WEAPON_CLASS( weapon_binoculars, WeaponBinoculars, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_bugbait, WeaponBugBait, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_flaregun, Flaregun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_annabelle, WeaponAnnabelle, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_gauss, WeaponGaussGun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_cubemap, WeaponCubemap, C_BaseCombatWeapon );
STUB_WEAPON_CLASS( weapon_alyxgun, WeaponAlyxGun, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_citizenpackage, WeaponCitizenPackage, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_citizensuitcase, WeaponCitizenSuitcase, C_WeaponCitizenPackage );

#ifndef HL2MP
//STUB_WEAPON_CLASS( weapon_ar2, WeaponAR2, C_HLMachineGun );
STUB_WEAPON_CLASS( weapon_frag, WeaponFrag, C_BaseHLCombatWeapon );
//STUB_WEAPON_CLASS( weapon_rpg, WeaponRPG, C_BaseHLCombatWeapon );
//STUB_WEAPON_CLASS( weapon_pistol, WeaponPistol, C_BaseHLCombatWeapon );
//STUB_WEAPON_CLASS( weapon_shotgun, WeaponShotgun, C_BaseHLCombatWeapon );
//STUB_WEAPON_CLASS( weapon_smg1, WeaponSMG1, C_HLSelectFireMachineGun );
//STUB_WEAPON_CLASS( weapon_357, Weapon357, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_crossbow, WeaponCrossbow, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_slam, Weapon_SLAM, C_BaseHLCombatWeapon );
//STUB_WEAPON_CLASS( weapon_crowbar, WeaponCrowbar, C_BaseHLBludgeonWeapon );
#ifdef HL2_EPISODIC
STUB_WEAPON_CLASS( weapon_hopwire, WeaponHopwire, C_BaseHLCombatWeapon );
//STUB_WEAPON_CLASS( weapon_proto1, WeaponProto1, C_BaseHLCombatWeapon );
#endif
#ifdef HL2_LOSTCOAST
STUB_WEAPON_CLASS( weapon_oldmanharpoon, WeaponOldManHarpoon, C_WeaponCitizenPackage );
#endif
#endif

// GSTRINGMIGRATION

#define STUB_WEAPON_CLASS_ACTTABLE( entityName, className, baseClassName, actTable )	\
	class C_##className : public baseClassName					\
	{																\
		DECLARE_CLASS( C_##className, baseClassName );							\
	public:															\
		DECLARE_PREDICTABLE();										\
		DECLARE_CLIENTCLASS();										\
		acttable_t *ActivityList( void );\
	int ActivityListCount( void ); \
		C_##className() {};											\
	private:														\
		C_##className( const C_##className & );						\
	};																\
	STUB_WEAPON_CLASS_IMPLEMENT( entityName, C_##className );		\
	IMPLEMENT_CLIENTCLASS_DT( C_##className, DT_##className, C##className )	\
	END_RECV_TABLE()	\
acttable_t *C_##className::ActivityList( void ) { return actTable; } \
	int C_##className::ActivityListCount( void ) { return ARRAYSIZE(actTable); }


acttable_t tableAR2[] = {
	{ ACT_IDLE,				ACT_IDLE_AR2,	false },
	{ ACT_RUN,				ACT_RUN_AR2,	false },
	{ ACT_RUN_CROUCH,		ACT_RUN_CROUCH_AIM_RIFLE,	false },
	{ ACT_GESTURE_RANGE_ATTACK1,		ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_GESTURE_RELOAD,				ACT_GESTURE_RELOAD_AR2,	false },
};
STUB_WEAPON_CLASS_ACTTABLE( weapon_ar2, WeaponAR2, C_HLMachineGun, tableAR2 );

acttable_t tablePistol[] = {
	{ ACT_IDLE,				ACT_IDLE_ANGRY_PISTOL,	false },
	{ ACT_RUN,				ACT_RUN_AIM_PISTOL,	false },
	{ ACT_RUN_CROUCH,		ACT_RUN_CROUCH_AIM_RIFLE,	false },
	{ ACT_GESTURE_RANGE_ATTACK1,		ACT_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_GESTURE_RELOAD,				ACT_GESTURE_RELOAD_SMG1,	false },
};
STUB_WEAPON_CLASS_ACTTABLE( weapon_pistol, WeaponPistol, C_BaseHLCombatWeapon, tablePistol );

acttable_t tableSmg1[] = {
	{ ACT_IDLE,				ACT_IDLE_SMG1,	false },
	{ ACT_RUN,				ACT_RUN_AIM_RIFLE,	false },
	{ ACT_RUN_CROUCH,		ACT_RUN_CROUCH_AIM_RIFLE,	false },
	{ ACT_GESTURE_RANGE_ATTACK1,		ACT_GESTURE_RANGE_ATTACK_SMG1,	false },
	{ ACT_GESTURE_RELOAD,				ACT_GESTURE_RELOAD_SMG1,	false },
};
STUB_WEAPON_CLASS_ACTTABLE( weapon_smg1, WeaponSMG1, C_HLSelectFireMachineGun, tableSmg1 );

STUB_WEAPON_CLASS_ACTTABLE( weapon_357, Weapon357, C_BaseHLCombatWeapon, tablePistol );

acttable_t tableRPG[] = {
	{ ACT_IDLE,				ACT_IDLE_ANGRY_RPG,	false },
	{ ACT_RUN,				ACT_RUN_RPG,	false },
	{ ACT_RUN_CROUCH,		ACT_RUN_CROUCH_RPG,	false },
	{ ACT_COVER_LOW,		ACT_COVER_LOW_RPG,	false },
	{ ACT_GESTURE_RANGE_ATTACK1,		ACT_GESTURE_RANGE_ATTACK_RPG,	false },
	{ ACT_GESTURE_RELOAD,				ACT_GESTURE_RELOAD_AR2,	false },
};
STUB_WEAPON_CLASS_ACTTABLE( weapon_rpg, WeaponRPG, C_BaseHLCombatWeapon, tableRPG );

acttable_t tableShotgun[] = {
	{ ACT_IDLE,				ACT_IDLE_SHOTGUN_AGITATED,	false },
	{ ACT_RUN,				ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_CROUCH,		ACT_RUN_CROUCH_AIM_RIFLE,	false },
	{ ACT_GESTURE_RANGE_ATTACK1,		ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_GESTURE_RELOAD,				ACT_GESTURE_RELOAD_AR2,	false },
};
STUB_WEAPON_CLASS_ACTTABLE( weapon_shotgun, WeaponShotgun, C_BaseHLCombatWeapon, tableShotgun );

acttable_t tableCrowbar[] = {
	{ ACT_IDLE,				ACT_IDLE_ANGRY,	false },
	{ ACT_GESTURE_RANGE_ATTACK1,		ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
};
STUB_WEAPON_CLASS_ACTTABLE( weapon_crowbar, WeaponCrowbar, C_BaseHLBludgeonWeapon, tableCrowbar );

// END GSTRINGMIGRATION
