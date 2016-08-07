#ifndef ENV_POST_PROCESSING_H
#define ENV_POST_PROCESSING_H


class CEnv_PostProcessingCtrl : public CBaseEntity
{
	DECLARE_CLASS(CEnv_PostProcessingCtrl, CBaseEntity);
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:

	CEnv_PostProcessingCtrl();
	~CEnv_PostProcessingCtrl();

	bool IsGodraysEnabled();
	bool IsBarsEnabled();
	bool IsBloomflareEnabled();
	bool IsDoFEnabled();

	float GetScreenBlurStrength(){
		return m_flIntensity_ScreenBlur;
	};
	float GetDreamStrength(){
		return m_flIntensity_Dream;
	};

	float GetDoFRadius() const { return m_flDofRadius; }
	float GetDoFFocalDistance() const { return m_flDofFocalDistance; }
	float GetDoFFocalLength() const { return m_flDofFocalLength; }
	float GetDoFAutoFocusInterp() const { return m_flDofAutoFocusInterp; }

#ifdef GAME_DLL
	virtual void Spawn();

	virtual int ObjectCaps( void ){
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();

	void SetGodraysEnabled( bool bEnabled );
	void SetBarsEnabled( bool bEnabled );
	void SetBloomflareEnabled( bool bEnabled );
	void SetDoFEnabled( bool bEnabled );

	void InputColorGodrays( inputdata_t &inputdata );
	void InputIntensityGodrays( inputdata_t &inputdata );

	void InputEnableGodrays( inputdata_t &inputdata );
	void InputDisableGodrays( inputdata_t &inputdata );
	void InputEnableBars( inputdata_t &inputdata );
	void InputDisableBars( inputdata_t &inputdata );
	void InputEnableBloomflare( inputdata_t &inputdata );
	void InputDisableBloomflare( inputdata_t &inputdata );
	void InputEnableDoF( inputdata_t &inputdata );
	void InputDisableDoF( inputdata_t &inputdata );

	void InputStartTransition( inputdata_t &inputdata );
#else
	virtual void OnDataChanged(DataUpdateType_t t);

#endif

private:

	CNetworkVar(bool, m_bEnable_Godrays);
	CNetworkVar(bool, m_bEnable_Bars);
	CNetworkVar(bool, m_bEnable_Bloomflare);
	CNetworkVar(bool, m_bEnable_DoF);

	CNetworkVector(m_vecColor_Godrays);
	CNetworkVar(float, m_flIntensity_Godrays);

	CNetworkVar(float, m_flIntensity_ScreenBlur);
	CNetworkVar(float, m_flIntensity_Dream);

	CNetworkVar(float, m_flDofRadius);
	CNetworkVar(float, m_flDofFocalLength);
	CNetworkVar(float, m_flDofFocalDistance);
	CNetworkVar(float, m_flDofAutoFocusInterp);

#ifdef GAME_DLL
	float m_flTransition_Time;
	float m_flScreenBlur_Goal;
	float m_flDream_Goal;

	void TransitionThink();

	float m_flCurTransition_StartTime;
	float m_flCurTransition_Duration;

	float m_flCurTransition_ScreenBlurOld;
	float m_flCurTransition_ScreenBlurGoal;
	float m_flCurTransition_DreamOld;
	float m_flCurTransition_DreamGoal;

	float m_flDofRadius_Goal;
	float m_flCurTransition_DofRadiusOld;
	float m_flCurTransition_DofRadiusGoal;
	float m_flDofFocalLength_Goal;
	float m_flCurTransition_DofFocalLengthOld;
	float m_flCurTransition_DofFocalLengthGoal;
	float m_flDofFocalDistance_Goal;
	float m_flCurTransition_DofFocalDistanceOld;
	float m_flCurTransition_DofFocalDistanceGoal;
	float m_flDofAutoFocusInterp_Goal;
	float m_flCurTransition_DofAutoFocusInterpOld;
	float m_flCurTransition_DofAutoFocusInterpGoal;
#endif
};

extern CEnv_PostProcessingCtrl *g_pPPCtrl;

#endif