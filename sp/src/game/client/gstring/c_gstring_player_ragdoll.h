#ifndef C_GSTRING_PLAYER_RAGDOLL_H
#define C_GSTRING_PLAYER_RAGDOLL_H

#include "c_baseanimating.h"

class C_GStringPlayerRagdoll : public C_ClientRagdoll
{
	DECLARE_CLASS( C_GStringPlayerRagdoll, C_ClientRagdoll );
public:

	C_GStringPlayerRagdoll();

	virtual CStudioHdr *OnNewModel();

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q,
		const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );

private:
	int m_iBoneHead;
};

#endif