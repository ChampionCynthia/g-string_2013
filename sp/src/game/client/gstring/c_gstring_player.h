#ifndef C_GSTRING_PLAYER_H
#define C_GSTRING_PLAYER_H

#include "c_basehlplayer.h"
#include "c_firstpersonbody.h"
#include "gstring/cspacecraft.h"

class C_MuzzleflashEffect;
class C_BobModel;

class C_GstringPlayer : public C_BaseHLPlayer
{
	DECLARE_CLASS( C_GstringPlayer, C_BaseHLPlayer );
	DECLARE_CLIENTCLASS();

public:
	C_GstringPlayer();
	~C_GstringPlayer();

	bool IsNightvisionActive() const;
	float GetNightvisionFraction() const;

	virtual void OnDataChanged( DataUpdateType_t updateType );

	virtual void ClientThink();

	virtual void OverrideView( CViewSetup *pSetup );
	virtual const Vector &GetViewOffset() const;
	virtual int DrawModel( int flags );

	virtual void ProcessMuzzleFlashEvent();
	virtual void UpdateFlashlight();

	virtual bool IsRenderingFlashlight() const;
	virtual void GetFlashlightPosition( Vector &vecPos ) const;
	virtual void GetFlashlightForward( Vector &vecForward ) const;
	virtual float GetFlashlightDot() const;

	virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	virtual void UpdateStepSoundOverride( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );

	surfacedata_t* GetGroundSurface();

	virtual void GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt );
	C_ClientRagdoll *CreateRagdollCopyInstance();

	static ShadowHandle_t GetFlashlightHandle();
	static bool ShouldFirstpersonModelCastShadow();

	C_FirstpersonBody *GetBodyModel() { return m_pBodyModel; }

	bool IsInSpacecraft() const;
	CSpacecraft *GetSpacecraft();

	bool IsInInteraction() const;

	virtual bool Weapon_CanSwitchTo( C_BaseCombatWeapon *pWeapon );
	virtual bool IsOverridingViewmodel();
	virtual int DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags ) { return 0; };

protected:

private:
	void UpdateBodyModel();
	void UpdateCustomStepSound();
	void GetSpacecraftCamera( Vector &origin, QAngle &angles, float &flFov );
	void UpdateInteraction();
	void GetInteractionCamera( Vector &origin, QAngle &angles );

private:
	CNetworkVar( unsigned char, m_nReloadParity );
	unsigned char m_nOldReloadParity;

	CNetworkVar( bool, m_bNightvisionActive );

	float m_flNightvisionFraction;

	float m_flMuzzleFlashTime;
	float m_flMuzzleFlashDuration;
	float m_flMuzzleFlashRoll;
	C_MuzzleflashEffect *m_pMuzzleFlashEffect;

	bool m_bFlashlightVisible;
	Vector m_vecFlashlightPosition;
	Vector m_vecFlashlightForward;
	float m_flFlashlightDot;

	C_BobModel *m_pBobViewModel;
	float m_flBobModelAmount;
	QAngle m_angLastBobAngle;
	float m_flLandBobTime;
	float m_flLandBobDynamicScale;
	float m_flLastFallVelocity;
	bool m_bBobWasInAir;

	CNetworkVar( bool, m_bHasUseEntity );

	C_FirstpersonBody *m_pBodyModel;
	float m_flBodyYawLast;
	Vector m_vecBodyOffset;
	bool m_bBodyWasMoving;
	bool m_bBodyWasInAir;
	bool m_bBodyWasHidden;
	bool m_bBodyPlayingLandAnim;
	int m_iBodyNextAttackLayer;
	float m_flBodyStepSoundHack;

	CNetworkHandle( CSpacecraft, m_hSpacecraft );
	bool m_bSpacecraftDeath;
	QAngle m_angSpacecraftDeathAngle;
	Vector m_vecSpacecraftDeathOrigin;
	Vector m_vecSpacecraftDeathVelocity;

	CNetworkHandle( C_BaseAnimating, m_hInteractionBody );
	float m_flInteractionBodyTransitionBlend;
	Vector m_vecInteractionViewOrigin;
	QAngle m_angInteractionViewAngles;
};

inline C_GstringPlayer *ToGstringPlayer( C_BaseEntity *pPlayer )
{
	return assert_cast< C_GstringPlayer* >( pPlayer );
}

inline C_GstringPlayer *LocalGstringPlayer()
{
	return ToGstringPlayer( C_BasePlayer::GetLocalPlayer() );
}

#endif