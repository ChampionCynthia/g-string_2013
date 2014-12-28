#ifndef C_CLIENTPARTIALRAGDOLL_H
#define C_CLIENTPARTIALRAGDOLL_H

#include "cbase.h"

#include "c_baseanimating.h"
#include "bone_setup.h"

void DispatchGibParticle( C_BaseAnimating *pEntity, const char *pszBone, bool bExplosion, int iBloodType );

struct PartialRagdollGore_t
{
	IMesh *m_pMesh;
	int m_iBone;
};

class C_ClientPartialRagdoll : public C_ClientRagdoll
{
	DECLARE_CLASS( C_ClientPartialRagdoll, C_ClientRagdoll );
	DECLARE_DATADESC();
public:

	C_ClientPartialRagdoll( bool bRestoring = true );
	~C_ClientPartialRagdoll();

	void SetShrinkingEnabled( bool bEnable );
	void SetRecursiveGibData( const char *pszParentName, const char *pszGoreName, const char *pszGoreMaterialName );

	void OnRestore();

	virtual void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	virtual bool TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	virtual bool InitAsClientRagdoll( const matrix3x4_t *pDeltaBones0, const matrix3x4_t *pDeltaBones1,
		const matrix3x4_t *pCurrentBonePosition, float boneDt, bool bFixedConstraints = false, ragdollparams_partial_t *pPartialParams = NULL );
	virtual void Touch( C_BaseEntity *pOther );

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q,
		const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual int DrawModel( int flags );

	virtual int BloodColor() { return m_iBloodColor; }
	virtual void SetBloodColor( int iBloodColor ) { m_iBloodColor = iBloodColor; }

	void RebuildGore();
	void DestroyGore();

private:
	void BuildPartial( ragdollparams_partial_t &params );
	IMesh *CreateGoreMeshForBone( const CStudioHdr *pHdr, int iBone, bool bRoot );
	bool ShouldCreateBloodParticles();

	bool m_bShrinking;
	bool m_bIsPartial;
	float m_flTouchDecalDelay;
	static float m_flLastParticleTime;
	static int m_iParticleCount;

	int m_iBloodColor;

	CBoneBitList m_normalBones;
	CBoneBitList m_trunkBones;
	CBoneBitList m_jointBones;

	string_t m_strRecursiveParent;
	string_t m_strRecursiveGoreName;
	string_t m_strRecursiveGoreMaterialName;
	int m_iBranchRootBone;

	int m_iBoneFlagStorage[ 12 ];

	CUtlVector< PartialRagdollGore_t > m_Gore;
	CMaterialReference m_GoreMaterial;
};


#endif