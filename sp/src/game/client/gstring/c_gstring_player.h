#ifndef C_GSTRING_PLAYER_H
#define C_GSTRING_PLAYER_H

#include "c_basehlplayer.h"
#include "c_firstpersonbody.h"

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

protected:

private:
	void UpdateBodyModel();

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