
#include "cbase.h"
#include "bone_setup.h"


int BoneParentDepth( CStudioHdr *pHdr, int iBone, int iPotentialParent )
{
	if ( iBone < 0
		|| iPotentialParent < 0 )
		return -1;

	if ( iBone == iPotentialParent )
		return 0;

	int iParent = pHdr->pBone( iBone )->parent;

	if ( iParent == iPotentialParent )
		return 1;

	if ( iParent < 0 )
		return -1;

	int iRet = BoneParentDepth( pHdr, iParent, iPotentialParent );

	return ( iRet < 0 ) ? -1 : 1 + iRet;
}

int BoneParentDepth( CStudioHdr *pHdr, const char *pszBone, const char *pszPotentialParent )
{
	Assert( pHdr );

	int iBone = Studio_BoneIndexByName( pHdr, pszBone );
	int iParent = Studio_BoneIndexByName( pHdr, pszPotentialParent );

	return BoneParentDepth( pHdr, iBone, iParent );
}

bool BoneHasParent( CStudioHdr *pHdr, int iBone, int iPotentialParent )
{
	return BoneParentDepth( pHdr, iBone, iPotentialParent ) > 0;
}

bool BoneHasParent( CStudioHdr *pHdr, const char *pszBone, const char *pszPotentialParent )
{
	return BoneParentDepth( pHdr, pszBone, pszPotentialParent ) > 0;
}