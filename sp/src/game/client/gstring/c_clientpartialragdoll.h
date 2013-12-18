#ifndef C_CLIENTPARTIALRAGDOLL_H
#define C_CLIENTPARTIALRAGDOLL_H

#include "c_baseanimating.h"
#include "bone_setup.h"

void DispatchGibParticle( C_BaseAnimating *pEntity, const char *pszBone, bool bExplosion, int iBloodType );

class C_ClientPartialRagdoll : public C_ClientRagdoll
{
	DECLARE_CLASS( C_ClientPartialRagdoll, C_ClientRagdoll );
	DECLARE_DATADESC();
public:

	C_ClientPartialRagdoll( bool bRestoring = true );

	void SetShrinkingEnabled( bool bEnable );

	void SetRecursiveGibData( const char *pszParentName );

	void OnRestore();

	virtual void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	virtual bool TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	virtual bool InitAsClientRagdoll( const matrix3x4_t *pDeltaBones0, const matrix3x4_t *pDeltaBones1,
		const matrix3x4_t *pCurrentBonePosition, float boneDt, bool bFixedConstraints = false, ragdollparams_partial_t *pPartialParams = NULL );

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q,
		const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual int BloodColor() { return m_iBloodColor; }
	virtual void SetBloodColor( int iBloodColor ) { m_iBloodColor = iBloodColor; }

private:

	void BuildPartial( ragdollparams_partial_t &params );

	bool m_bShrinking;
	bool m_bIsPartial;

	int m_iBloodColor;

	CBoneBitList m_normalBones;
	CBoneBitList m_trunkBones;
	CBoneBitList m_jointBones;

	string_t m_strRecursiveParent;
	int m_iBranchRootBone;

	int m_iBoneFlagStorage[ 12 ];
};


#endif