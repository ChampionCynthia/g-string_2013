#ifndef C_CLIENTPARTIALRAGDOLL_H
#define C_CLIENTPARTIALRAGDOLL_H

#include "c_baseanimating.h"
#include "bone_setup.h"

class C_ClientPartialRagdoll : public C_ClientRagdoll
{
	DECLARE_CLASS( C_ClientPartialRagdoll, C_ClientRagdoll );
public:

	C_ClientPartialRagdoll();

	virtual CStudioHdr *OnNewModel();

	virtual bool InitAsClientRagdoll( const matrix3x4_t *pDeltaBones0, const matrix3x4_t *pDeltaBones1,
		const matrix3x4_t *pCurrentBonePosition, float boneDt, bool bFixedConstraints = false, ragdollparams_partial_t *pPartialParams = NULL );

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q,
		const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );

	void SetShrinkingEnabled( bool bEnable );

private:

	void BuildPartial( ragdollparams_partial_t &params );

	bool m_bShrinking;
	bool m_bIsPartial;

	CBoneBitList m_normalBones;
	CBoneBitList m_trunkBones;
	//uint64 m_iNormalBones[ 2 ];
	//uint64 m_iTrunkBones[ 2 ];
};


#endif